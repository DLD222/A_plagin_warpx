/* Copyright 2022 S. Eric Clark, LLNL
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause-LBNL
 */
#include "WarpX.H"

#include "Fields.H"
#include "EmbeddedBoundary/Enabled.H"
#include "Parallelization/GuardCellManager.H"
#include "Particles/WarpXParticleContainer.H"
#include "Python/callbacks.H"
#include "Utils/WarpXAlgorithmSelection.H"
#include "Utils/WarpXConst.H"
#include "Utils/TextMsg.H"
#include "Utils/WarpXUtil.H"
#include "Parallelization/WarpXComm_K.H"

#include <ablastr/fields/MultiFabRegister.H>
#include <ablastr/profiler/ProfilerWrapper.H>
#include <ablastr/utils/Communication.H>
#include <ablastr/warn_manager/WarnManager.H>
#include <ablastr/fields/VectorPoissonSolver.H>

#include <AMReX_Array.H>
#include <AMReX_Array4.H>
#include <AMReX_BLassert.H>
#include <AMReX_Box.H>
#include <AMReX_BoxArray.H>
#include <AMReX_Config.H>
#include <AMReX_DistributionMapping.H>
#include <AMReX_FArrayBox.H>
#include <AMReX_FabArray.H>
#include <AMReX_Geometry.H>
#include <AMReX_GpuControl.H>
#include <AMReX_GpuLaunch.H>
#include <AMReX_GpuQualifiers.H>
#include <AMReX_IndexType.H>
#include <AMReX_IntVect.H>
#include <AMReX_LO_BCTYPES.H>
#include <AMReX_MFIter.H>
#include <AMReX_MLMG.H>
#include <AMReX_MultiFab.H>
#include <AMReX_REAL.H>
#include <AMReX_SPACE.H>
#include <AMReX_Vector.H>
#include <AMReX_MFInterp_C.H>
#ifdef AMREX_USE_EB
#   include <AMReX_EBFabFactory.H>
#endif

#include <array>
#include <memory>
#include <string>

using namespace amrex;

void
WarpX::ComputeMagnetostaticField()
{
    ABLASTR_PROFILE("WarpX::ComputeMagnetostaticField");
    // Fields have been reset in Electrostatic solver for this time step, these fields
    // are added into the B fields after electrostatic solve

    WARPX_ALWAYS_ASSERT_WITH_MESSAGE(this->max_level == 0,
        "Magnetostatic solver not implemented with mesh refinement.");

    if (!m_magnetostatic_solver) {
        m_magnetostatic_solver = warpx::runtime::create_magnetostatic_solver();
    }
    WARPX_ALWAYS_ASSERT_WITH_MESSAGE(m_magnetostatic_solver != nullptr,
        "this input asks for the magnetostatic solver, but no field-solver plugin "
        "is linked: build with WarpX_PLUGIN_FIELDSOLVER=ON.");

    warpx::runtime::MagnetostaticConfig config;
    config.fields = &m_fields;
    config.geom = &Geom(0);
    config.level_geom = &geom;
    config.level_dmap = &dmap;
    config.level_grids = &grids;
    config.ref_ratio = &ref_ratio;
    config.t_new = gett_new(0);
    config.max_level = max_level;
    config.finest_level = finest_level;
    config.dt = dt[0];
    config.n_rz_azimuthal_modes = n_rz_azimuthal_modes;
    config.field_boundary_lo = field_boundary_lo;
    config.field_boundary_hi = field_boundary_hi;
    config.field_centering_nox = field_centering_nox;
    config.field_centering_noy = field_centering_noy;
    config.field_centering_noz = field_centering_noz;
    config.field_centering_coeffs_x = device_field_centering_stencil_coeffs_x.data();
    config.field_centering_coeffs_y = device_field_centering_stencil_coeffs_y.data();
    config.field_centering_coeffs_z = device_field_centering_stencil_coeffs_z.data();
    config.do_single_precision_comms = do_single_precision_comms;
    config.required_precision = magnetostatic_solver_required_precision;
    config.absolute_tolerance = magnetostatic_solver_absolute_tolerance;
    config.max_iters = magnetostatic_solver_max_iters;
    config.verbosity = magnetostatic_solver_verbosity;
#ifdef AMREX_USE_EB
    config.eb_factory = [this] (int lev) -> amrex::FabFactory<amrex::FArrayBox> const& {
        return fieldEBFactory(lev);
    };
#endif

    // The three operations that are still kernel work travel as callbacks.
    config.deposit_current = [this] (ablastr::fields::MultiFabRegister& fields,
                                     amrex::Real step_dt) {
        for (int ispecies = 0; ispecies < particleSystem().nSpecies(); ispecies++) {
            warpx::runtime::IParticleContainer& species = particleSystem().GetParticleContainer(ispecies);
            if (!species.doNotDeposit()) {
                species.DepositCurrent(
                    fields.get_mr_levels_alldirs(warpx::fields::FieldType::current_fp, finest_level),
                    step_dt, 0.);
            }
        }
    };
    config.finalize_current = [this] (ablastr::fields::MultiFabRegister& fields, int max_lev) {
#if defined(WARPX_DIM_RZ) || defined(WARPX_DIM_RCYLINDER) || defined(WARPX_DIM_RSPHERE)
        for (int lev = 0; lev <= max_lev; lev++) {
            ApplyInverseVolumeScalingToCurrentDensity(
                fields.get(warpx::fields::FieldType::current_fp, ablastr::fields::Direction{0}, lev),
                fields.get(warpx::fields::FieldType::current_fp, ablastr::fields::Direction{1}, lev),
                fields.get(warpx::fields::FieldType::current_fp, ablastr::fields::Direction{2}, lev),
                lev);
        }
#else
        amrex::ignore_unused(fields, max_lev);
#endif
        SyncCurrent("current_fp");
        FinalizeJ();
    };

    WARPX_ALWAYS_ASSERT_WITH_MESSAGE(!IsPythonCallbackInstalled("poissonsolver"),
        "Python Level Poisson Solve not supported for Magnetostatic implementation.");

    m_magnetostatic_solver->computeField(config);
}

