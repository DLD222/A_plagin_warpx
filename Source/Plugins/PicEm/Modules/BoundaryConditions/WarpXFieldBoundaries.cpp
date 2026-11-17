#include "WarpX.H"
#include "BoundaryConditions/PEC_Insulator.H"
#include "BoundaryConditions/PML.H"
#include "Runtime/IFiniteDifferenceSolver.H"
#include "HybridPIC/HybridPICModel/HybridPICModel.H"
#include "WarpX_PEC.H"

#include <AMReX.H>
#include <cstdlib>
#include <string>
#include <AMReX_Geometry.H>
#include <AMReX_IntVect.H>
#include <AMReX_REAL.H>
#include <AMReX_Vector.H>
#include <AMReX_Print.H>

#include <algorithm>
#include <array>
#include <memory>

using namespace amrex;
using namespace amrex::literals;
using warpx::fields::FieldType;

namespace
{
    /** Returns true if any field boundary is set to FieldBoundaryType FT, else returns false.*/
    template <FieldBoundaryType FT>
    [[nodiscard]]
    bool isAnyBoundary (const amrex::Array<FieldBoundaryType,AMREX_SPACEDIM>& field_boundary_lo,
        const amrex::Array<FieldBoundaryType,AMREX_SPACEDIM>& field_boundary_hi)
    {
        const auto isFT = [](const auto& b){
            return b == FT;};
        return std::any_of(field_boundary_lo.begin(), field_boundary_lo.end(), isFT) ||
               std::any_of(field_boundary_hi.begin(), field_boundary_hi.end(), isFT);
    }

    /** Returns true if any particle boundary is set to ParticleBoundaryType PT, else returns false.*/
    template <ParticleBoundaryType PT>
    [[nodiscard]]
    bool isAnyBoundary (const amrex::Array<ParticleBoundaryType,AMREX_SPACEDIM>& particle_boundary_lo,
        const amrex::Array<ParticleBoundaryType,AMREX_SPACEDIM>& particle_boundary_hi)
    {
        const auto isPT = [](const auto& b){
            return b == PT;};
        return std::any_of(particle_boundary_lo.begin(), particle_boundary_lo.end(), isPT) ||
               std::any_of(particle_boundary_hi.begin(), particle_boundary_hi.end(), isPT);
    }

}

void WarpX::ApplyEfieldBoundary(const int lev, PatchType patch_type, amrex::Real time)
{
    using ablastr::fields::Direction;

#if defined(AMREX_USE_EB)
    // ---- PEC on embedded surfaces, staircase form ----
    // Cells that the surface covers are inside the conductor, so the electric field
    // there is zero; with a staircase mask this also sets the tangential component on
    // the surface to zero, which is the PEC condition `E_tangential = 0`.
    // (The normal component of B is handled separately in `ApplyBfieldBoundary`, where
    //  it is made *continuous* rather than zero.)
    // NOTE: do not read MultiCutFab arrays (e.g. `getBndryNormal().const_array(mfi)`)
    // here — their data does not outlive the statement and ASAN flags it as
    // heap-use-after-free.  The staircase form needs no boundary normal.
    // WARPX_PEC_EB_MASK=0 disables the staircase E=0 masking, for diagnosing whether a
    // solver (e.g. hybrid) is disturbed by the discontinuity it introduces.
    static bool const pec_eb_mask = (std::getenv("WARPX_PEC_EB_MASK") == nullptr
                                     || std::string(std::getenv("WARPX_PEC_EB_MASK")) != "0");
    if (EB::enabled() && pec_eb_mask) {
        auto const& eb_flags = fieldEBFactory(lev).getMultiEBCellFlagFab();
        using warpx::fields::FieldType;
        auto const efield = m_fields.get_alldirs(
            (patch_type == PatchType::fine) ? FieldType::Efield_fp : FieldType::Efield_cp, lev);
        for (amrex::MFIter mfi(eb_flags); mfi.isValid(); ++mfi) {
            auto const& fab = eb_flags[mfi];
            if (fab.getType() == amrex::FabType::regular) { continue; }
            auto const& flag = fab.const_array();
            amrex::Box const& tile = mfi.validbox();
            for (int idim = 0; idim < 3; ++idim) {
                if (efield[idim] == nullptr) { continue; }
                amrex::Array4<amrex::Real> const E = efield[idim]->array(mfi);
                amrex::ParallelFor(tile, [=] AMREX_GPU_DEVICE (int i, int j, int k) {
                    if (flag(i, j, k).isCovered()) { E(i, j, k) = 0.0_rt; }
                });
            }
        }
    }
#endif

    if (::isAnyBoundary<FieldBoundaryType::PEC>(field_boundary_lo, field_boundary_hi)) {
        if (patch_type == PatchType::fine) {
            PEC::ApplyPECtoEfield(
                    m_fields.get_alldirs(FieldType::Efield_fp, lev),
                    field_boundary_lo, field_boundary_hi, FieldBoundaryType::PEC,
                    get_ng_fieldgather(), Geom(lev),
                    lev, patch_type, ref_ratio);
            if (::isAnyBoundary<FieldBoundaryType::PML>(field_boundary_lo, field_boundary_hi)) {
                // apply pec on split E-fields in PML region
                const bool split_pml_field = true;
                PEC::ApplyPECtoEfield(
                    m_fields.get_alldirs(FieldType::pml_E_fp, lev),
                    field_boundary_lo, field_boundary_hi, FieldBoundaryType::PEC,
                    get_ng_fieldgather(), Geom(lev),
                    lev, patch_type, ref_ratio,
                    split_pml_field);
            }
        } else {
            PEC::ApplyPECtoEfield(
                    m_fields.get_alldirs(FieldType::Efield_cp, lev),
                    field_boundary_lo, field_boundary_hi, FieldBoundaryType::PEC,
                    get_ng_fieldgather(), Geom(lev),
                    lev, patch_type, ref_ratio);
            if (::isAnyBoundary<FieldBoundaryType::PML>(field_boundary_lo, field_boundary_hi)) {
                // apply pec on split E-fields in PML region
                const bool split_pml_field = true;
                PEC::ApplyPECtoEfield(
                    m_fields.get_alldirs(FieldType::pml_E_cp, lev),
                    field_boundary_lo, field_boundary_hi, FieldBoundaryType::PEC,
                    get_ng_fieldgather(), Geom(lev),
                    lev, patch_type, ref_ratio,
                    split_pml_field);
            }
        }
    }

    if (::isAnyBoundary<FieldBoundaryType::PMC>(field_boundary_lo, field_boundary_hi)) {
        if (patch_type == PatchType::fine) {
            PEC::ApplyPECtoBfield(
                    m_fields.get_alldirs(FieldType::Efield_fp, lev),
                    field_boundary_lo, field_boundary_hi, FieldBoundaryType::PMC,
                    get_ng_fieldgather(), Geom(lev),
                    lev, patch_type, ref_ratio);
            if (::isAnyBoundary<FieldBoundaryType::PML>(field_boundary_lo, field_boundary_hi)) {
                // apply pec on split E-fields in PML region
                const bool split_pml_field = true;
                PEC::ApplyPECtoBfield(
                    m_fields.get_alldirs(FieldType::pml_E_fp, lev),
                    field_boundary_lo, field_boundary_hi, FieldBoundaryType::PMC,
                    get_ng_fieldgather(), Geom(lev),
                    lev, patch_type, ref_ratio,
                    split_pml_field);
            }
        } else {
            PEC::ApplyPECtoBfield(
                    m_fields.get_alldirs(FieldType::Efield_cp, lev),
                    field_boundary_lo, field_boundary_hi, FieldBoundaryType::PMC,
                    get_ng_fieldgather(), Geom(lev),
                    lev, patch_type, ref_ratio);
            if (::isAnyBoundary<FieldBoundaryType::PML>(field_boundary_lo, field_boundary_hi)) {
                // apply pec on split E-fields in PML region
                const bool split_pml_field = true;
                PEC::ApplyPECtoBfield(
                    m_fields.get_alldirs(FieldType::pml_E_cp, lev),
                    field_boundary_lo, field_boundary_hi, FieldBoundaryType::PMC,
                    get_ng_fieldgather(), Geom(lev),
                    lev, patch_type, ref_ratio,
                    split_pml_field);
            }
        }
    }

    if (::isAnyBoundary<FieldBoundaryType::PEC_Insulator>(field_boundary_lo, field_boundary_hi)) {
        if (patch_type == PatchType::fine) {
            pec_insulator_boundary->ApplyPEC_InsulatortoEfield(
                    m_fields.get_alldirs(FieldType::Efield_fp, lev),
                    field_boundary_lo, field_boundary_hi,
                    get_ng_fieldgather(), Geom(lev),
                    lev, patch_type, ref_ratio, time);
            if (::isAnyBoundary<FieldBoundaryType::PML>(field_boundary_lo, field_boundary_hi)) {
                // apply on split E-fields in PML region
                const bool split_pml_field = true;
                pec_insulator_boundary->ApplyPEC_InsulatortoEfield(
                    m_fields.get_alldirs(FieldType::pml_E_fp, lev),
                    field_boundary_lo, field_boundary_hi,
                    get_ng_fieldgather(), Geom(lev),
                    lev, patch_type, ref_ratio, time,
                    split_pml_field);
            }
        } else {
            pec_insulator_boundary->ApplyPEC_InsulatortoEfield(
                m_fields.get_alldirs(FieldType::Efield_cp, lev),
                field_boundary_lo, field_boundary_hi,
                get_ng_fieldgather(), Geom(lev),
                lev, patch_type, ref_ratio, time);
            if (::isAnyBoundary<FieldBoundaryType::PML>(field_boundary_lo, field_boundary_hi)) {
                // apply on split E-fields in PML region
                const bool split_pml_field = true;
                pec_insulator_boundary->ApplyPEC_InsulatortoEfield(
                    m_fields.get_alldirs(FieldType::pml_E_cp, lev),
                    field_boundary_lo, field_boundary_hi,
                    get_ng_fieldgather(), Geom(lev),
                    lev, patch_type, ref_ratio, time,
                    split_pml_field);
            }
        }
    }

#if defined(WARPX_DIM_RZ) || defined(WARPX_DIM_RCYLINDER) || defined(WARPX_DIM_RSPHERE)
    if (patch_type == PatchType::fine) {
        ApplyFieldBoundaryOnAxis(m_fields.get(FieldType::Efield_fp, Direction{0}, lev),
                                 m_fields.get(FieldType::Efield_fp, Direction{1}, lev),
                                 m_fields.get(FieldType::Efield_fp, Direction{2}, lev), lev);
    } else {
        ApplyFieldBoundaryOnAxis(m_fields.get(FieldType::Efield_cp, Direction{0}, lev),
                                 m_fields.get(FieldType::Efield_cp, Direction{1}, lev),
                                 m_fields.get(FieldType::Efield_cp, Direction{2}, lev), lev);
    }
#endif
}

void WarpX::ApplyBfieldBoundary (const int lev, PatchType patch_type, SubcyclingHalf subcycling_half, amrex::Real time)
{
    using ablastr::fields::Direction;

#if defined(AMREX_USE_EB)
    // ---- Ideal-conductor surface: keep B_normal CONTINUOUS across it ----
    // A perfect conductor does not expel the normal field: flux is frozen inside it, so
    // B_n must match across the surface while B_t may jump (surface current).  With a
    // staircase representation the "normal" is an axis direction, so the rule is: for a
    // covered cell, take B in the component whose direction points at the neighbouring
    // fluid cell and copy that fluid cell's value; leave the other components frozen.
    // (Zeroing B instead would violate B_n continuity and is wrong.)
    if (EB::enabled()) {
        auto const& eb_factory = fieldEBFactory(lev);
        auto const& eb_flags = eb_factory.getMultiEBCellFlagFab();
        using warpx::fields::FieldType;
        auto const bfield = m_fields.get_alldirs(
            (patch_type == PatchType::fine) ? FieldType::Bfield_fp : FieldType::Bfield_cp, lev);
        if (bfield[0] != nullptr && bfield[1] != nullptr && bfield[2] != nullptr) {
            for (amrex::MFIter mfi(eb_flags); mfi.isValid(); ++mfi) {
                auto const& fab = eb_flags[mfi];
                if (fab.getType() == amrex::FabType::regular) { continue; }
                auto const& flag = fab.const_array();
                amrex::Box const& tile = mfi.validbox();
                auto const bx_a = bfield[0]->array(mfi);
                auto const by_a = bfield[1]->array(mfi);
                auto const bz_a = bfield[2]->array(mfi);
                amrex::ParallelFor(tile, [=] AMREX_GPU_DEVICE (int i, int j, int k) {
                    if (!flag(i, j, k).isCovered()) { return; }
                    // Staircase: look for the first fluid neighbour, and copy the component
                    // that is normal to that face.
                    if (!flag(i + 1, j, k).isCovered()) { bx_a(i, j, k) = bx_a(i + 1, j, k); }
                    else if (!flag(i - 1, j, k).isCovered()) { bx_a(i, j, k) = bx_a(i - 1, j, k); }
#if AMREX_SPACEDIM == 2
                    else if (!flag(i, j, k + 1).isCovered()) { bz_a(i, j, k) = bz_a(i, j, k + 1); }
                    else if (!flag(i, j, k - 1).isCovered()) { bz_a(i, j, k) = bz_a(i, j, k - 1); }
#else
                    else if (!flag(i, j + 1, k).isCovered()) { by_a(i, j, k) = by_a(i, j + 1, k); }
                    else if (!flag(i, j - 1, k).isCovered()) { by_a(i, j, k) = by_a(i, j - 1, k); }
                    else if (!flag(i, j, k + 1).isCovered()) { bz_a(i, j, k) = bz_a(i, j, k + 1); }
                    else if (!flag(i, j, k - 1).isCovered()) { bz_a(i, j, k) = bz_a(i, j, k - 1); }
#endif
                });
            }
        }
    }
#endif

    if (::isAnyBoundary<FieldBoundaryType::PEC>(field_boundary_lo, field_boundary_hi)) {
        if (patch_type == PatchType::fine) {
            PEC::ApplyPECtoBfield(
                m_fields.get_alldirs(FieldType::Bfield_fp, lev),
                field_boundary_lo, field_boundary_hi, FieldBoundaryType::PEC,
                get_ng_fieldgather(), Geom(lev),
                lev, patch_type, ref_ratio);
        } else {
            PEC::ApplyPECtoBfield(
                m_fields.get_alldirs(FieldType::Bfield_cp, lev),
                field_boundary_lo, field_boundary_hi, FieldBoundaryType::PEC,
                get_ng_fieldgather(), Geom(lev),
                lev, patch_type, ref_ratio);
        }
    }

    if (::isAnyBoundary<FieldBoundaryType::PMC>(field_boundary_lo, field_boundary_hi)) {
        if (patch_type == PatchType::fine) {
            PEC::ApplyPECtoEfield(
                m_fields.get_alldirs(FieldType::Bfield_fp, lev),
                field_boundary_lo, field_boundary_hi, FieldBoundaryType::PMC,
                get_ng_fieldgather(), Geom(lev),
                lev, patch_type, ref_ratio);
        } else {
            PEC::ApplyPECtoEfield(
                m_fields.get_alldirs(FieldType::Bfield_cp, lev),
                field_boundary_lo, field_boundary_hi, FieldBoundaryType::PMC,
                get_ng_fieldgather(), Geom(lev),
                lev, patch_type, ref_ratio);
        }
    }

    if (::isAnyBoundary<FieldBoundaryType::PEC_Insulator>(field_boundary_lo, field_boundary_hi)) {
        if (patch_type == PatchType::fine) {
            pec_insulator_boundary->ApplyPEC_InsulatortoBfield(
                m_fields.get_alldirs(FieldType::Bfield_fp, lev),
                field_boundary_lo, field_boundary_hi,
                get_ng_fieldgather(), Geom(lev),
                lev, patch_type, ref_ratio, time);
        } else {
            pec_insulator_boundary->ApplyPEC_InsulatortoBfield(
                m_fields.get_alldirs(FieldType::Bfield_cp, lev),
                field_boundary_lo, field_boundary_hi,
                get_ng_fieldgather(), Geom(lev),
                lev, patch_type, ref_ratio, time);
        }
    }

    // Silver-Mueller boundaries are only applied on the first half-push of B
    // This is because the formula used for Silver-Mueller assumes that
    // E and B are staggered in time, which is only true after the first half-push
    if (lev == 0) {
        if (subcycling_half == SubcyclingHalf::FirstHalf) {
            if(::isAnyBoundary<FieldBoundaryType::Absorbing_Silver_Mueller>(field_boundary_lo, field_boundary_hi)){
                auto Efield_fp = m_fields.get_mr_levels_alldirs(FieldType::Efield_fp, max_level);
                auto Bfield_fp = m_fields.get_mr_levels_alldirs(FieldType::Bfield_fp, max_level);
                m_fdtd_solver_fp[0]->ApplySilverMuellerBoundary(
                Efield_fp[lev], Bfield_fp[lev],
                Geom(lev).Domain(), dt[lev],
                field_boundary_lo, field_boundary_hi);
            }
        }
    }

#if defined(WARPX_DIM_RZ) || defined(WARPX_DIM_RCYLINDER) || defined(WARPX_DIM_RSPHERE)
    if (patch_type == PatchType::fine) {
        ApplyFieldBoundaryOnAxis(m_fields.get(FieldType::Bfield_fp,Direction{0},lev),
                                 m_fields.get(FieldType::Bfield_fp,Direction{1},lev),
                                 m_fields.get(FieldType::Bfield_fp,Direction{2},lev), lev);
    } else {
        ApplyFieldBoundaryOnAxis(m_fields.get(FieldType::Bfield_cp,Direction{0},lev),
                                 m_fields.get(FieldType::Bfield_cp,Direction{1},lev),
                                 m_fields.get(FieldType::Bfield_cp,Direction{2},lev), lev);
    }
#endif
}

void WarpX::ApplyRhofieldBoundary (const int lev, MultiFab* rho,
                                   PatchType patch_type, int scomp, int ncomp)
{
    if (ncomp < 0) { ncomp = rho->nComp() - scomp; }
    AMREX_ALWAYS_ASSERT(scomp >= 0 && ncomp > 0 && scomp + ncomp <= rho->nComp());

    // View only the requested components without copying field data.
    // Component n of this view refers to component scomp+n of rho;
    // boundary updates through the view modify the original rho.
    // Subcycling finalizes the two coarse time components at different stages.
    MultiFab rho_components(*rho, amrex::make_alias, scomp, ncomp);

    if (::isAnyBoundary<ParticleBoundaryType::Reflecting>(particle_boundary_lo, particle_boundary_hi) ||
        ::isAnyBoundary<ParticleBoundaryType::Thermal>(particle_boundary_lo, particle_boundary_hi) ||
        ::isAnyBoundary<FieldBoundaryType::PEC>(field_boundary_lo, field_boundary_hi) ||
        ::isAnyBoundary<FieldBoundaryType::PEC_Insulator>(field_boundary_lo, field_boundary_hi) ||
        ::isAnyBoundary<FieldBoundaryType::PMC>(field_boundary_lo, field_boundary_hi))
    {
        PEC::ApplyReflectiveBoundarytoRhofield(&rho_components,
            field_boundary_lo, field_boundary_hi,
            particle_boundary_lo, particle_boundary_hi,
            Geom(lev), lev, patch_type, ref_ratio);
    }

    if (::isAnyBoundary<FieldBoundaryType::PEC_Insulator>(field_boundary_lo, field_boundary_hi)) {
        pec_insulator_boundary->ZeroParallelScalarInConductor(&rho_components,
            field_boundary_lo, field_boundary_hi,
            Geom(lev), lev, patch_type, ref_ratio);
    }
}

void WarpX::ApplyJfieldBoundary (const int lev, amrex::MultiFab* Jx,
                                 amrex::MultiFab* Jy, amrex::MultiFab* Jz,
                                 PatchType patch_type)
{
    BL_PROFILE("WarpX::ApplyJfieldBoundary()");
    if (::isAnyBoundary<ParticleBoundaryType::Reflecting>(particle_boundary_lo, particle_boundary_hi) ||
        ::isAnyBoundary<ParticleBoundaryType::Thermal>(particle_boundary_lo, particle_boundary_hi) ||
        ::isAnyBoundary<FieldBoundaryType::PEC>(field_boundary_lo, field_boundary_hi) ||
        ::isAnyBoundary<FieldBoundaryType::PEC_Insulator>(field_boundary_lo, field_boundary_hi) ||
        ::isAnyBoundary<FieldBoundaryType::PMC>(field_boundary_lo, field_boundary_hi))
    {
        PEC::ApplyReflectiveBoundarytoJfield(Jx, Jy, Jz,
            field_boundary_lo, field_boundary_hi,
            particle_boundary_lo, particle_boundary_hi,
            Geom(lev), lev, patch_type, ref_ratio);
    }

    if (::isAnyBoundary<FieldBoundaryType::PEC_Insulator>(field_boundary_lo, field_boundary_hi)) {
        pec_insulator_boundary->ZeroParallelFieldInConductor({Jx, Jy, Jz},
            field_boundary_lo, field_boundary_hi,
            get_ng_fieldgather(), Geom(lev),
            lev, patch_type, ref_ratio);
    }
}

#if defined(WARPX_DIM_RZ) || defined(WARPX_DIM_RCYLINDER) || defined(WARPX_DIM_RSPHERE)
// Applies the boundary conditions that are specific to the axis when in cylindrical or spherical
void
WarpX::ApplyFieldBoundaryOnAxis (amrex::MultiFab* Er, amrex::MultiFab* Et, amrex::MultiFab* Ez, int lev) const
{
    const amrex::IntVect ngE = get_ng_fieldgather();

    constexpr int NODE = amrex::IndexType::NODE;

#ifdef AMREX_USE_OMP
#pragma omp parallel if (amrex::Gpu::notInLaunchRegion())
#endif
    for ( amrex::MFIter mfi(*Er, amrex::TilingIfNotGPU()); mfi.isValid(); ++mfi )
    {

        amrex::Box const & tilebox = mfi.tilebox();

        // Lower corner of tile box physical domain
        // Note that this is done before the tilebox.grow so that
        // these do not include the guard cells.
        const amrex::XDim3 xyzmin = LowerCorner(tilebox, lev, 0._rt);
        const amrex::Real rmin = xyzmin.x;

        // Skip blocks that don't touch the axis
        if (rmin > 0._rt) { continue; }

        amrex::Array4<amrex::Real> const& Er_arr = Er->array(mfi);
        amrex::Array4<amrex::Real> const& Et_arr = Et->array(mfi);
        amrex::Array4<amrex::Real> const& Ez_arr = Ez->array(mfi);

        amrex::Box tbr = amrex::convert( tilebox, Er->ixType().toIntVect() );
        amrex::Box tbt = amrex::convert( tilebox, Et->ixType().toIntVect() );
        amrex::Box tbz = amrex::convert( tilebox, Ez->ixType().toIntVect() );

        // For ishift, 1 means cell centered, 0 means node centered
        int const ishift_r = (tbr.type(0) != NODE);
        int const ishift_t = (tbt.type(0) != NODE);
        int const ishift_z = (tbz.type(0) != NODE);

        // Set tileboxes to only include the axis guard cells
        // (including the corners in z).
        tbr.setRange(0, -ngE[0], ngE[0]);
        tbt.setRange(0, -ngE[0], ngE[0]);
        tbz.setRange(0, -ngE[0], ngE[0]);
#ifdef WARPX_DIM_RZ
        tbr.grow(1, ngE[1]);
        tbt.grow(1, ngE[1]);
        tbz.grow(1, ngE[1]);
#endif

        const int nmodes = n_rz_azimuthal_modes;

        amrex::ParallelFor(tbr, tbt, tbz,
        [=] AMREX_GPU_DEVICE (int i, int j, int /*k*/)
        {
            Er_arr(i,j,0,0) = -Er_arr(-i-ishift_r,j,0,0);

            for (int imode=1 ; imode < nmodes ; imode++) {
                Er_arr(i,j,0,2*imode-1) = std::pow(-1._rt, imode+1._rt)*Er_arr(-i-ishift_r,j,0,2*imode-1);
                Er_arr(i,j,0,2*imode) = std::pow(-1._rt, imode+1._rt)*Er_arr(-i-ishift_r,j,0,2*imode);
            }
        },
        [=] AMREX_GPU_DEVICE (int i, int j, int /*k*/)
        {
            Et_arr(i,j,0,0) = -Et_arr(-i-ishift_t,j,0,0);

            for (int imode=1 ; imode < nmodes ; imode++) {
                Et_arr(i,j,0,2*imode-1) = std::pow(-1._rt, imode+1._rt)*Et_arr(-i-ishift_t,j,0,2*imode-1);
                Et_arr(i,j,0,2*imode) = std::pow(-1._rt, imode+1._rt)*Et_arr(-i-ishift_t,j,0,2*imode);
            }
        },
        [=] AMREX_GPU_DEVICE (int i, int j, int /*k*/)
        {
#if defined(WARPX_DIM_RSPHERE)
            // Ephi is anti-symmetric
            Ez_arr(i,j,0,0) = -Ez_arr(-i-ishift_z,j,0,0);

#elif defined(WARPX_DIM_RZ) || defined(WARPX_DIM_RCYLINDER)
            Ez_arr(i,j,0,0) = Ez_arr(-i-ishift_z,j,0,0);

            for (int imode=1 ; imode < nmodes ; imode++) {
                Ez_arr(i,j,0,2*imode-1) = -std::pow(-1._rt, imode+1._rt)*Ez_arr(-i-ishift_z,j,0,2*imode-1);
                Ez_arr(i,j,0,2*imode) = -std::pow(-1._rt, imode+1._rt)*Ez_arr(-i-ishift_z,j,0,2*imode);
            }
#endif

        });
    }
}
#endif

void WarpX::ApplyElectronPressureBoundary (const int lev, PatchType patch_type)
{
    if (::isAnyBoundary<FieldBoundaryType::PEC>(field_boundary_lo, field_boundary_hi)) {
        if (patch_type == PatchType::fine) {
            ablastr::fields::ScalarField electron_pressure_fp = m_fields.get(FieldType::hybrid_electron_pressure_fp, lev);
            PEC::ApplyPECtoElectronPressure(
                electron_pressure_fp,
                field_boundary_lo, field_boundary_hi,
                Geom(lev), lev, patch_type, ref_ratio);
        } else {
            amrex::Abort(Utils::TextMsg::Err(
            "ApplyElectronPressureBoundary: Only one level implemented for hybrid solver."));
        }
    }
}
