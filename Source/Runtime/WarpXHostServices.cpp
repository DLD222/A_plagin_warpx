/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "Runtime/WarpXHostServices.H"

#include "BoundaryConditions/PML.H"
#include "FieldSolver/FiniteDifferenceSolver/HybridPICModel/HybridPICModel.H"
#include "Diagnostics/MultiDiagnostics.H"
#include "Diagnostics/ReducedDiags/MultiReducedDiags.H"
#include "Diagnostics/Diagnostics.H"
#include "Particles/MultiParticleContainer.H"
#include "WarpX.H"

#include <ostream>

namespace warpx::runtime
{

void
WarpXHostServices::prepareFieldsForDiagnostics ()
{
    const amrex::IntVect ngEB = m_warpx.getngEB();
    m_warpx.FillBoundaryE(ngEB);
    m_warpx.FillBoundaryB(ngEB);
    m_warpx.UpdateAuxiliaryData();
    m_warpx.FillBoundaryAux(m_warpx.getngUpdateAux());
}

void
WarpXHostServices::computeDivB (amrex::MultiFab& divB, int dcomp,
                                ablastr::fields::VectorField const& B,
                                const std::array<amrex::Real, 3>& dx)
{
    WarpX::ComputeDivB(divB, dcomp, B, dx);
}

void
WarpXHostServices::computeDivE (amrex::MultiFab& divE, int lev)
{
    m_warpx.ComputeDivE(divE, lev);
}

amrex::Real WarpXHostServices::movingWindowX () const { return m_warpx.getmoving_window_x(); }
bool WarpXHostServices::movingWindowActive (int step) const { return WarpX::moving_window_active(step); }
int WarpXHostServices::numCellsShifted (amrex::Real distance, amrex::Real dx) const { return WarpX::NumCellsShifted(distance, dx); }
bool WarpXHostServices::doMovingWindow () const { return m_warpx.getdo_moving_window(); }
amrex::Real WarpXHostServices::timeOfLastGalileanShift () const { return m_warpx.time_of_last_gal_shift; }
bool WarpXHostServices::doPML () const { return m_warpx.DoPML(); }

void
WarpXHostServices::checkpointPML (int lev, const std::string& prefix,
                                  ablastr::fields::MultiFabRegister& fields)
{
    if (m_warpx.GetPML(lev)) {
        m_warpx.GetPML(lev)->CheckPoint(fields, prefix);
    }
#if (defined WARPX_DIM_RZ) && (defined WARPX_USE_FFT)
    if (m_warpx.GetPML_RZ(lev)) {
        m_warpx.GetPML_RZ(lev)->CheckPoint(fields, prefix + "_rz");
    }
#endif
}

amrex::AmrMesh* WarpXHostServices::amrMesh () { return &m_warpx; }
std::vector<bool> WarpXHostServices::pmlDirections () const { return m_warpx.getPMLdirections(); }
std::string WarpXHostServices::authors () const { return m_warpx.GetAuthors(); }
void WarpXHostServices::writeParticleHeader (std::ostream& os) { m_warpx.GetPartContainer().WriteHeader(os); }
void WarpXHostServices::writeReducedDiagCheckpoint (const std::string& dir) { m_warpx.reduced_diags->WriteCheckpointData(dir); }
amrex::Vector<int> WarpXHostServices::nsubsteps () const { return m_warpx.getnsubsteps(); }
amrex::Vector<amrex::Real> WarpXHostServices::tNew () const { return m_warpx.gett_new(); }
amrex::Vector<amrex::Real> WarpXHostServices::tOld () const { return m_warpx.gett_old(); }
amrex::Vector<amrex::Real> WarpXHostServices::dt () const { return m_warpx.getdt(); }
int WarpXHostServices::totalDiags () const { return m_warpx.GetMultiDiags().GetTotalDiags(); }
std::string WarpXHostServices::diagName (int i) const { return m_warpx.GetMultiDiags().GetDiag(i).GetName(); }
bool WarpXHostServices::isDiagBackTransformed (int i) const { return m_warpx.GetMultiDiags().diagstypes(i) == DiagTypes::BackTransformed; }
int WarpXHostServices::diagNumBuffers (int i) const { return m_warpx.GetMultiDiags().GetDiag(i).getnumbuffers(); }
amrex::Real WarpXHostServices::diagTlab (int i, int buffer) const { return m_warpx.GetMultiDiags().GetDiag(i).gettlab(buffer); }
int WarpXHostServices::diagBufferKIndexHi (int i, int buffer) const { return m_warpx.GetMultiDiags().GetDiag(i).get_buffer_k_index_hi(buffer); }
amrex::Real WarpXHostServices::diagSnapshotDomainLo (int i, int buffer, int dim) const { return m_warpx.GetMultiDiags().GetDiag(i).get_snapshot_domain_lo(buffer, dim); }
amrex::Real WarpXHostServices::diagSnapshotDomainHi (int i, int buffer, int dim) const { return m_warpx.GetMultiDiags().GetDiag(i).get_snapshot_domain_hi(buffer, dim); }
int WarpXHostServices::diagFlushCounter (int i, int buffer) const { return m_warpx.GetMultiDiags().GetDiag(i).get_flush_counter(buffer); }
int WarpXHostServices::diagLastValidZslice (int i, int buffer) const { return m_warpx.GetMultiDiags().GetDiag(i).get_last_valid_Zslice(buffer); }
int WarpXHostServices::diagSnapshotFullFlag (int i, int buffer) const { return m_warpx.GetMultiDiags().GetDiag(i).get_snapshot_full_flag(buffer); }
bool WarpXHostServices::hasHybridPICModel () const
{
    return m_warpx.get_pointer_HybridPICModel() != nullptr;
}

int WarpXHostServices::hybridPicSubsteps () const
{
    auto const* hybrid = m_warpx.get_pointer_HybridPICModel();
    return hybrid ? hybrid->m_substeps : -1;
}

} // namespace warpx::runtime
