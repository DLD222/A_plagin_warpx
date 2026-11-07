#include "PartPerGridFunctor.H"
#include "Diagnostics/DiagnosticsDependencies.H"
#include "Runtime/IParticleContainerAccess.H"
#include "Runtime/ISpeciesMetadata.H"

#include "Runtime/RuntimeData.H"

#include "Diagnostics/ComputeDiagFunctors/ComputeDiagFunctor.H"

#include <ablastr/coarsen/sample.H>

#include <AMReX_BLassert.H>
#include <AMReX_Config.H>
#include <AMReX_FArrayBox.H>
#include <AMReX_FabArray.H>
#include <AMReX_GpuControl.H>
#include <AMReX_INT.H>
#include <AMReX_IntVect.H>
#include <AMReX_MFIter.H>
#include <AMReX_MultiFab.H>
#include <AMReX_REAL.H>
#include <AMReX_Vector.H>

#include <memory>

PartPerGridFunctor::PartPerGridFunctor(const amrex::MultiFab * const mf_src, const int lev, const amrex::IntVect crse_ratio, const int ncomp)
    : ComputeDiagFunctor(ncomp, crse_ratio), m_lev(lev)
{
    // mf_src will not be used, let's make sure it's null.
    AMREX_ALWAYS_ASSERT(mf_src == nullptr);
    // Write only in one output component.
    AMREX_ALWAYS_ASSERT(ncomp == 1);
}

void
PartPerGridFunctor::operator()(amrex::MultiFab& mf_dst, const int dcomp, const int /*i_buffer*/) const
{
    
    const amrex::Vector<amrex::Long>& npart_in_grid = data().speciesMetadata().NumberOfParticlesInGrid(m_lev);
    // Guard cell is set to 1 for generality. However, for a cell-centered
    // output Multifab, mf_dst, the guard-cell data is not needed especially considering
    // the operations performend in the CoarsenAndInterpolate function.
    constexpr int ng = 1;
    // Temporary MultiFab containing number of particles per grid.
    // (stored as constant for all cells in each grid)
    amrex::MultiFab ppg_mf(data().boxArray(m_lev), data().distributionMap(m_lev), 1, ng);
#ifdef AMREX_USE_OMP
#pragma omp parallel
#endif
    for (amrex::MFIter mfi(ppg_mf); mfi.isValid(); ++mfi) {
        ppg_mf[mfi].setVal<amrex::RunOn::Device>(static_cast<amrex::Real>(npart_in_grid[mfi.index()]));
    }

    // Coarsen and interpolate from ppg_mf to the output diagnostic MultiFab, mf_dst.
    ablastr::coarsen::sample::Coarsen(mf_dst, ppg_mf, dcomp, 0, nComp(), 0, m_crse_ratio);
}
