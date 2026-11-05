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
#include "FieldSolver/ElectrostaticSolvers/ElectrostaticSolver.H"
#include "Fluids/MultiFluidContainer.H"
#include "Parallelization/WarpXSumGuardCells.H"
#include "Particles/MultiParticleContainer.H"
#include "WarpX.H"

#include <memory>
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


void
WarpXHostServices::depositCurrent (int lev, amrex::Real dt)
{
    using warpx::fields::FieldType;
    using ablastr::fields::Direction;

    ablastr::fields::MultiLevelVectorField current_fp_temp {
        m_warpx.m_fields.get_alldirs(FieldType::current_fp, lev)
    };

    auto& mypc = m_warpx.GetPartContainer();
    mypc.DepositCurrent(current_fp_temp, dt, 0.0);

    for (int idim = 0; idim < 3; ++idim) {
        auto& J = *current_fp_temp[0][idim];
        WarpXSumGuardCells(J, m_warpx.Geom(lev).periodicity(), J.nGrowVect(), 0, J.nComp());
    }

    m_warpx.FinalizeJOnLevel(lev,
        current_fp_temp[0][0], current_fp_temp[0][1], current_fp_temp[0][2],
        PatchType::fine);
}

std::unique_ptr<amrex::MultiFab>
WarpXHostServices::computeRho (int lev, int species_index, bool apply_rz_psatd_filter)
{
    std::unique_ptr<amrex::MultiFab> rho;
    if (species_index == -1) {
        auto& mypc = m_warpx.GetPartContainer();
        rho = mypc.GetChargeDensityOnLevel(lev, true, /*finalize*/false);
        if (m_warpx.DoFluidSpecies()) {
            auto& myfl = m_warpx.GetFluidContainer();
            myfl.DepositCharge(m_warpx.m_fields, *rho, lev);
        }
    } else {
        auto& mypc = m_warpx.GetPartContainer().GetParticleContainer(species_index);
        rho = mypc.GetChargeDensityOnLevel(lev, true, /*finalize*/false);
    }

    m_warpx.ApplyFilterandSumBoundaryRho(lev, lev, *rho, 0, rho->nComp());
    m_warpx.FinalizeRhoOnLevel(lev, rho.get(), PatchType::fine);

#if (defined WARPX_DIM_RZ) && (defined WARPX_USE_FFT)
    if (WarpX::electromagnetic_solver_id == ElectromagneticSolverAlgo::PSATD &&
        WarpX::use_kspace_filter && apply_rz_psatd_filter) {
        auto& solver = m_warpx.get_spectral_solver_fp(lev);
        const SpectralFieldIndex& Idx = solver.m_spectral_index;
        solver.ForwardTransform(lev, *rho, Idx.rho_new);
        solver.ApplyFilter(lev, Idx.rho_new);
        solver.BackwardTransform(lev, *rho, Idx.rho_new);
    }
#else
    amrex::ignore_unused(apply_rz_psatd_filter);
#endif
    return rho;
}


std::unique_ptr<amrex::MultiFab>
WarpXHostServices::computePhi (int lev)
{
    if (m_warpx.m_fields.has(warpx::fields::FieldType::phi_fp, lev)) {
        const auto* src = m_warpx.m_fields.get(warpx::fields::FieldType::phi_fp, lev);
        auto phi = std::make_unique<amrex::MultiFab>(
            src->boxArray(), src->DistributionMap(), src->nComp(), src->nGrowVect());
        amrex::MultiFab::Copy(*phi, *src, 0, 0, src->nComp(), src->nGrowVect());
        return phi;
    }

    const int num_levels = 1;
    amrex::Vector<std::unique_ptr<amrex::MultiFab>> rho_vec(num_levels);
    amrex::Vector<std::unique_ptr<amrex::MultiFab>> phi_vec(num_levels);

    amrex::BoxArray ba = m_warpx.boxArray(lev);
    ba.surroundingNodes();
    rho_vec[0] = std::make_unique<amrex::MultiFab>(ba, m_warpx.DistributionMap(lev), 1, 1);
    rho_vec[0]->setVal(0.);
    m_warpx.ComputeDivE(*rho_vec[0], lev);
    rho_vec[0]->mult(ablastr::constant::SI::epsilon_0);

    phi_vec[0] = std::make_unique<amrex::MultiFab>(ba, m_warpx.DistributionMap(lev), 1, 1);
    phi_vec[0]->setVal(0.);

    auto& es_solver = m_warpx.GetElectrostaticSolver();
    es_solver.m_poisson_boundary_handler->DefinePhiBCs(m_warpx.Geom(0));
    const std::array<amrex::Real, 3> beta = {0.0};
    es_solver.computePhi(
        amrex::GetVecOfPtrs(rho_vec), amrex::GetVecOfPtrs(phi_vec),
        beta, es_solver.m_mlmg_options, /*is_igf_2d_slices*/ false);

    return std::move(phi_vec[0]);
}

void
WarpXHostServices::applyFilterMF (ablastr::fields::MultiLevelVectorField const& field,
                                  int lev, int idir)
{
    m_warpx.ApplyFilterMF(field, lev, idir);
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
