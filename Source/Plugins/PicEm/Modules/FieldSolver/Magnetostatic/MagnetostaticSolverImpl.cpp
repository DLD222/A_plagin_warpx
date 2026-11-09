/* Copyright 2022 S. Eric Clark, LLNL
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause-LBNL
 */
#include "MagnetostaticSolverImpl.H"

#include "FieldSolver/Magnetostatic/MagnetostaticSolver.H"
#include "Fields.H"
#include "EmbeddedBoundary/Enabled.H"
#include "Utils/WarpXAlgorithmSelection.H"
#include "Utils/WarpXConst.H"
#include "Parallelization/WarpXComm_K.H"
#include "Utils/TextMsg.H"

#include <ablastr/fields/MultiFabRegister.H>
#include <ablastr/profiler/ProfilerWrapper.H>
#include <ablastr/utils/Communication.H>
#include <ablastr/fields/VectorPoissonSolver.H>

#include <AMReX_Array.H>
#include <AMReX_Array4.H>
#include <AMReX_BLassert.H>
#include <AMReX_Box.H>
#include <AMReX_Config.H>
#include <AMReX_FArrayBox.H>
#include <AMReX_GpuControl.H>
#include <AMReX_GpuLaunch.H>
#include <AMReX_IndexType.H>
#include <AMReX_IntVect.H>
#include <AMReX_LO_BCTYPES.H>
#include <AMReX_MFIter.H>
#include <AMReX_MLMG.H>
#include <AMReX_MultiFab.H>
#include <AMReX_REAL.H>
#include <AMReX_SPACE.H>
#include <AMReX_Vector.H>
#ifdef AMREX_USE_EB
#   include <AMReX_EBFabFactory.H>
#endif

#include <optional>

using namespace amrex;

void
MagnetostaticSolverImpl::computeField (warpx::runtime::MagnetostaticConfig const& config)
{
    using ablastr::fields::Direction;
    using warpx::fields::FieldType;

    ABLASTR_PROFILE("MagnetostaticSolverImpl::computeField");

    // Store the boundary conditions for the field solver if they haven't been
    // stored yet
    if (!m_bc.bcs_set) {
        m_boundary_lo = config.field_boundary_lo;
        m_boundary_hi = config.field_boundary_hi;
        m_bc.defineVectorPotentialBCs(*config.geom, m_boundary_lo, m_boundary_hi);
    }

#ifdef WARPX_DIM_RZ
    WARPX_ALWAYS_ASSERT_WITH_MESSAGE(config.n_rz_azimuthal_modes == 1,
                                     "Error: RZ magnetostatic only implemented for a single mode");
#endif

    // reset current_fp before depositing current density for this step
    for (int lev = 0; lev <= config.max_level; lev++) {
        for (int dim=0; dim < 3; dim++) {
            config.fields->get(FieldType::current_fp, Direction{dim}, lev)->setVal(0.);
        }
    }

    // Deposit current density (source of Poisson solver)
    if (config.deposit_current) { config.deposit_current(*config.fields, config.dt); }

    if (config.finalize_current) { config.finalize_current(*config.fields, config.max_level); }

    // set the boundary and current density potentials
    setVectorPotentialBC(config,
        config.fields->get_mr_levels_alldirs(FieldType::vector_potential_fp_nodal,
                                            config.finest_level));

    // Compute the vector potential A, by solving the Poisson equation
    computeVectorPotential(config,
        config.fields->get_mr_levels_alldirs(FieldType::current_fp, config.finest_level),
        config.fields->get_mr_levels_alldirs(FieldType::vector_potential_fp_nodal,
                                             config.finest_level));
}

void
MagnetostaticSolverImpl::computeVectorPotential (
    warpx::runtime::MagnetostaticConfig const& config,
    ablastr::fields::MultiLevelVectorField const& curr,
    ablastr::fields::MultiLevelVectorField const& A) const
{
    using ablastr::fields::Direction;
    using warpx::fields::FieldType;

    // create a vector to our fields, sorted by level
    amrex::Vector<amrex::Array<amrex::MultiFab*,3>> sorted_curr;
    amrex::Vector<amrex::Array<amrex::MultiFab*,3>> sorted_A;
    for (int lev = 0; lev <= config.finest_level; ++lev) {
        sorted_curr.emplace_back(amrex::Array<amrex::MultiFab*,3> ({curr[lev][0],
                                                                    curr[lev][1],
                                                                    curr[lev][2]}));
        sorted_A.emplace_back(amrex::Array<amrex::MultiFab*,3> ({A[lev][0],
                                                                 A[lev][1],
                                                                 A[lev][2]}));
    }

    const ablastr::fields::MultiLevelVectorField Bfield_fp = config.fields->get_mr_levels_alldirs(FieldType::Bfield_fp, config.finest_level);
    const std::optional<MagnetostaticSolver::EBCalcBfromVectorPotentialPerLevel> post_A_calculation(
    {
        Bfield_fp,
        config.fields->get_mr_levels_alldirs(FieldType::vector_potential_grad_buf_e_stag, config.finest_level),
        config.fields->get_mr_levels_alldirs(FieldType::vector_potential_grad_buf_b_stag, config.finest_level),
        config.field_centering_nox,
        config.field_centering_noy,
        config.field_centering_noz,
        config.field_centering_coeffs_x,
        config.field_centering_coeffs_y,
        config.field_centering_coeffs_z,
        config.do_single_precision_comms
    });

#if defined(AMREX_USE_EB)
    std::optional<amrex::Vector<amrex::EBFArrayBoxFactory const *> > eb_farray_box_factory;
    if (EB::enabled() && config.eb_factory) {
        amrex::Vector<amrex::EBFArrayBoxFactory const *> factories;
        for (int lev = 0; lev <= config.finest_level; ++lev) {
            factories.push_back(static_cast<amrex::EBFArrayBoxFactory const*>(&config.eb_factory(lev)));
        }
        eb_farray_box_factory = factories;
    }
#else
    const std::optional<amrex::Vector<amrex::FArrayBoxFactory const *> > eb_farray_box_factory;
#endif

    ablastr::fields::computeVectorPotential(
        sorted_curr,
        sorted_A,
        config.required_precision,
        config.absolute_tolerance,
        config.max_iters,
        config.verbosity,
        *config.level_geom,
        *config.level_dmap,
        *config.level_grids,
        m_bc,
        EB::enabled(),
        config.do_single_precision_comms,
        *config.ref_ratio,
        post_A_calculation,
        config.t_new,
        eb_farray_box_factory
    );

}

void
MagnetostaticSolverImpl::setVectorPotentialBC (
    warpx::runtime::MagnetostaticConfig const& config,
    ablastr::fields::MultiLevelVectorField const& A) const
{
    using ablastr::fields::Direction;

    // check if any dimension has non-periodic boundary conditions
    if (!m_bc.has_non_periodic) { return; }

    auto dirichlet_flag = m_bc.dirichlet_flag;

    // loop over all mesh refinement levels and set the boundary values
    for (int lev=0; lev <= config.max_level; lev++) {

        amrex::Box domain = (*config.level_geom)[lev].Domain();
        domain.surroundingNodes();
        for (int adim=0; adim < 3; adim++) {
#ifdef AMREX_USE_OMP
#pragma omp parallel if (amrex::Gpu::notInLaunchRegion())
#endif
            for ( MFIter mfi(*A[lev][Direction{adim}], TilingIfNotGPU()); mfi.isValid(); ++mfi ) {
                // Extract the vector potential
                auto A_arr = A[lev][Direction{adim}]->array(mfi);
                // Extract tileboxes for which to loop
                const Box& tb  = mfi.tilebox( A[lev][Direction{adim}]->ixType().toIntVect());

                // loop over dimensions
                for (int idim=0; idim<AMREX_SPACEDIM; idim++){
                    // check if neither boundaries in this dimension should be set
                    if (!(dirichlet_flag[adim][2*idim] || dirichlet_flag[adim][2*idim+1])) { continue; }

                    // a check can be added below to test if the boundary values
                    // are already correct, in which case the ParallelFor over the
                    // cells can be skipped

                    if (!domain.strictly_contains(tb)) {
                        amrex::ParallelFor( tb,
                            [=] AMREX_GPU_DEVICE (int i, int j, int k) {

                                IntVect iv(AMREX_D_DECL(i,j,k));

                                if (dirichlet_flag[adim][2*idim] && iv[idim] == domain.smallEnd(idim)) {
                                    A_arr(i,j,k) = 0.;
                                }

                                if (dirichlet_flag[adim][2*idim+1] && iv[idim] == domain.bigEnd(idim)) {
                                    A_arr(i,j,k) = 0.;
                                }
                            } // loop ijk
                        );
                    }
                } // idim
            } // MFIter
        }
    } // lev
}

void
MagnetostaticSolverImpl::defineVectorPotentialBCs (const amrex::Geometry& geom)
{
    // The boundary types come from the last configuration the kernel handed in;
    // asking for the boundary conditions again re-runs the same setup.
    m_bc.defineVectorPotentialBCs(geom, m_boundary_lo, m_boundary_hi);
}

void MagnetostaticSolver::VectorPoissonBoundaryHandler::defineVectorPotentialBCs (
    const amrex::Geometry& geom,
    amrex::Array<FieldBoundaryType,AMREX_SPACEDIM> const& field_boundary_lo,
    amrex::Array<FieldBoundaryType,AMREX_SPACEDIM> const& field_boundary_hi)
{
    for (int adim = 0; adim < 3; adim++) {
#if defined(WARPX_DIM_RZ) || defined(WARPX_DIM_RCYLINDER) || defined(WARPX_DIM_RSPHERE)
        int dim_start = 0;
        if (geom.ProbLo(0) == 0){
#if defined(WARPX_DIM_RZ)
            // For the m=0 mode, regularity on the axis requires A_r = A_t = 0
            // there, which is also the boundary condition that goes with the
            // -1/r^2 term of the vector Laplacian (see setAlpha below). A_z has
            // no such term and keeps a Neumann axis.
            const bool axis_dirichlet = (adim < 2);
#else
            const bool axis_dirichlet = false;
#endif
            lobc[adim][0] = axis_dirichlet ? LinOpBCType::Dirichlet
                                           : LinOpBCType::Neumann;
            dirichlet_flag[adim][0] = axis_dirichlet;
            dim_start = 1;

            // handle the r_max boundary explicitly
            if (field_boundary_hi[0] == FieldBoundaryType::PEC) {
                if (adim == 0) {
                    hibc[adim][0] = LinOpBCType::Neumann;
                    dirichlet_flag[adim][1] = false;
                } else{
                    hibc[adim][0] = LinOpBCType::Dirichlet;
                    dirichlet_flag[adim][1] = true;
                }
            }
            else if (field_boundary_hi[0] == FieldBoundaryType::Neumann) {
                hibc[adim][0] = LinOpBCType::Neumann;
                dirichlet_flag[adim][1] = false;
            }
        }
#else
        amrex::ignore_unused(geom);
        const int dim_start = 0;
#endif
        bool ndotA = false;
        for (int idim=dim_start; idim<AMREX_SPACEDIM; idim++){
            ndotA = (adim == idim);

#if defined(WARPX_DIM_XZ) || defined(WARPX_DIM_RZ)
            if (idim == 1) { ndotA = (adim == 2); }
#endif

            if ( field_boundary_lo[idim] == FieldBoundaryType::Periodic
                && field_boundary_hi[idim] == FieldBoundaryType::Periodic ) {
                lobc[adim][idim] = LinOpBCType::Periodic;
                hibc[adim][idim] = LinOpBCType::Periodic;
                dirichlet_flag[adim][idim*2] = false;
                dirichlet_flag[adim][idim*2+1] = false;
            }
            else {
                has_non_periodic = true;
                if ( field_boundary_lo[idim] == FieldBoundaryType::PEC ) {
                    if (ndotA) {
                        lobc[adim][idim] = LinOpBCType::Neumann;
                        dirichlet_flag[adim][idim*2] = false;
                    } else {
                        lobc[adim][idim] = LinOpBCType::Dirichlet;
                        dirichlet_flag[adim][idim*2] = true;
                    }
                }

                else if ( field_boundary_lo[idim] == FieldBoundaryType::Neumann ) {
                    lobc[adim][idim] = LinOpBCType::Neumann;
                    dirichlet_flag[adim][idim*2] = false;
                }
                else {
                    WARPX_ABORT_WITH_MESSAGE(
                        "Field boundary conditions have to be either periodic, PEC or neumann "
                        "when using the magnetostatic solver,  but they are " + amrex::getEnumNameString(field_boundary_lo[idim])
                    );
                }

                if ( field_boundary_hi[idim] == FieldBoundaryType::PEC ) {
                    if (ndotA) {
                        hibc[adim][idim] = LinOpBCType::Neumann;
                        dirichlet_flag[adim][idim*2+1] = false;
                    } else {
                        hibc[adim][idim] = LinOpBCType::Dirichlet;
                        dirichlet_flag[adim][idim*2+1] = true;
                    }
                }
                else if ( field_boundary_hi[idim] == FieldBoundaryType::Neumann ) {
                    hibc[adim][idim] = LinOpBCType::Neumann;
                    dirichlet_flag[adim][idim*2+1] = false;
                }
                else {
                    WARPX_ABORT_WITH_MESSAGE(
                        "Field boundary conditions have to be either periodic, PEC or neumann "
                        "when using the magnetostatic solver,  but they are " + amrex::getEnumNameString(field_boundary_lo[idim])
                    );
                }
            }
        }
    }
    bcs_set = true;
}
