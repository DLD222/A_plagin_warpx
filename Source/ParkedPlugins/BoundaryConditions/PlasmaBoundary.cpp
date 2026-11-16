#include "PlasmaBoundary.H"

#include <Fields.H>
#include <AMReX_EBFabFactory.H>
#include <AMReX_MFIter.H>
#include <AMReX_ParallelDescriptor.H>

using warpx::runtime::IBoundaryConditions;
using warpx::runtime::IEBGeometry;

namespace warpx::boundary {

PlasmaBoundary::PlasmaBoundary (IEBGeometry const& eb_geometry) : m_eb(eb_geometry) {}

void PlasmaBoundary::ApplyPecToEfield (ablastr::fields::MultiFabRegister& fields, int lev,
                                       ablastr::utils::enums::PatchType patch_type,
                                       amrex::Real /*time*/)
{
    m_last_cut_cells = 0;
    if (!m_eb.hasEB(lev)) { return; }

    using ablastr::fields::Direction;
    using warpx::fields::FieldType;
    using PatchType = ablastr::utils::enums::PatchType;

    // Fine and coarse patches live in different field entries.
    auto const& field_type = (patch_type == PatchType::fine) ? FieldType::Efield_fp
                                                            : FieldType::Efield_cp;
    if (!fields.has_vector(field_type, lev)) { return; }
    auto const efield = fields.get_alldirs(field_type, lev);

    auto const& flags = m_eb.factory(lev).getMultiEBCellFlagFab();

    long local_cut = 0;
    for (amrex::MFIter mfi(flags); mfi.isValid(); ++mfi) {
        auto const& fab = flags[mfi];
        // Only cells cut by the surface need the condition: regular cells are all
        // fluid, covered cells are already excluded from the E update by the mask.
        if (fab.getType() != amrex::FabType::singlevalued) { continue; }
        local_cut += static_cast<long>(mfi.validbox().numPts());

        auto const& flag = fab.const_array();
        for (int idim = 0; idim < 3; ++idim) {
            if (efield[idim] == nullptr) { continue; }
            amrex::Array4<amrex::Real> const& E = efield[idim]->array(mfi);
            // The E component lives on a staggered (nodal) grid in its own direction,
            // so iterate over that component's own index space.
            amrex::Box const& tile = mfi.growntilebox(efield[idim]->nGrowVect());
            amrex::ParallelFor(tile, [=] AMREX_GPU_DEVICE (int i, int j, int k) {
                if (!flag(i, j, k).isRegular()) { E(i, j, k) = 0.0; }
            });
        }
    }
    amrex::ParallelDescriptor::ReduceLongSum(local_cut);
    m_last_cut_cells = local_cut;
}

void PlasmaBoundary::ApplyPecToBfield (ablastr::fields::MultiFabRegister&, int, 
                                       ablastr::utils::enums::PatchType, amrex::Real) {}

void PlasmaBoundary::FillFacePermittivity (ablastr::fields::MultiFabRegister&, int,
                                           ablastr::utils::enums::PatchType) {}

void PlasmaBoundary::ApplyDielectricJump (ablastr::fields::MultiFabRegister&, int,
                                          ablastr::utils::enums::PatchType, amrex::Real) {}

void PlasmaBoundary::ApplyWallToFluidVelocity (amrex::MultiFab&, int, int, int) {}
void PlasmaBoundary::ApplyWallToHeatFlux (amrex::MultiFab&, int, int, int, amrex::Real) {}
void PlasmaBoundary::ApplyWallToStressTensor (amrex::MultiFab&, int, int, int) {}

} // namespace warpx::boundary
