#include "PlasmaBoundary.H"

#include <AMReX_EBFabFactory.H>
#include <AMReX_MFIter.H>
#include <AMReX_ParallelDescriptor.H>

using warpx::runtime::IBoundaryConditions;
using warpx::runtime::IEBGeometry;

namespace warpx::boundary {

PlasmaBoundary::PlasmaBoundary (IEBGeometry const& eb_geometry) : m_eb(eb_geometry) {}

void PlasmaBoundary::ApplyPecToEfield (ablastr::fields::MultiFabRegister& /*fields*/, int lev,
                                       ablastr::utils::enums::PatchType /*patch_type*/,
                                       amrex::Real /*time*/)
{
    m_last_cut_cells = 0;
    if (!m_eb.hasEB(lev)) { return; }
    // The actual kernel needs the field register entries for Efield_fp/Efield_cp; the
    // next slice wires `fields.get_alldirs(...)` in.  What is validated here is the
    // geometry path only: how many cut cells the core reports for this level.
    auto const& flags = m_eb.factory(lev).getMultiEBCellFlagFab();
    long local = 0;
    for (amrex::MFIter mfi(flags); mfi.isValid(); ++mfi) {
        auto const& fab = flags[mfi];
        if (fab.getType() != amrex::FabType::singlevalued) { continue; }
        amrex::Box const& box = mfi.validbox();
        local += static_cast<long>(box.numPts());
    }
    amrex::ParallelDescriptor::ReduceLongSum(local);
    m_last_cut_cells = local;
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
