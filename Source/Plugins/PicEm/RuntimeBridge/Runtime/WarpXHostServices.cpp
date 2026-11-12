#include "Particles/ParticleBoundaryBuffer.H"
/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "Runtime/WarpXHostServices.H"

#include "BoundaryConditions/PML.H"
#include "HybridPIC/HybridPICModel/HybridPICModel.H"
#include "Fluids/MultiFluidContainer.H"
#include "Parallelization/WarpXSumGuardCells.H"
#include "WarpX.H"

#ifdef WARPX_USE_FFT
#   ifdef WARPX_DIM_RZ
// The RZ PSATD rho filtering and the RZ PML checkpoint need the complete types;
// WarpX.H only carries forward declarations for them.
#       include "FieldSolver/PSATD/SpectralSolverRZ.H"
#       include "BoundaryConditions/PML_RZ.H"
#   endif
#endif

#ifdef WARPX_QED
#include "Particles/ElementaryProcess/VirtualPhotonCreation.H"
#endif

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

    auto& mypc = m_warpx.particleSystem();
    mypc.DepositCurrent(current_fp_temp, dt, 0.0);

    for (int idim = 0; idim < 3; ++idim) {
        auto& J = *current_fp_temp[0][idim];
        WarpXSumGuardCells(J, m_warpx.Geom(lev).periodicity(), J.nGrowVect(), 0, J.nComp());
    }

    m_warpx.FinalizeJOnLevel(lev,
        current_fp_temp[0][0], current_fp_temp[0][1], current_fp_temp[0][2],
        PatchType::fine);
}

void
WarpXHostServices::depositCharge (ablastr::fields::MultiLevelScalarField const& rho,
                                 amrex::Real relative_time)
{
    m_warpx.particleSystem().DepositCharge(rho, relative_time);
}

std::unique_ptr<amrex::MultiFab>
WarpXHostServices::computeRho (int lev, int species_index, bool apply_rz_psatd_filter)
{
    std::unique_ptr<amrex::MultiFab> rho;
    if (species_index == -1) {
        auto& mypc = m_warpx.particleSystem();
        rho = mypc.GetChargeDensityOnLevel(lev, true, /*finalize*/false);
        if (m_warpx.DoFluidSpecies()) {
            auto& myfl = m_warpx.GetFluidContainer();
            myfl.DepositCharge(m_warpx.m_fields, *rho, lev);
        }
    } else {
        auto& mypc = m_warpx.particleSystem().GetParticleContainer(species_index);
        rho = mypc.GetChargeDensityOnLevel(lev, true, /*finalize*/false);
    }

    m_warpx.ApplyFilterandSumBoundaryRho(lev, lev, *rho, 0, rho->nComp());
    m_warpx.FinalizeRhoOnLevel(lev, rho.get(), PatchType::fine);

#if (defined WARPX_DIM_RZ) && (defined WARPX_USE_FFT)
    if (WarpX::electromagnetic_solver_id == ElectromagneticSolverAlgo::PSATD &&
        WarpX::use_kspace_filter && apply_rz_psatd_filter) {
        // RZ k-space filtering is an RZ-solver capability, so the concrete
        // type is narrowed here; the rest of the kernel only sees the contract.
        auto& solver = dynamic_cast<SpectralSolverRZ&>(m_warpx.get_spectral_solver_fp(lev));
        const SpectralFieldIndex& Idx = solver.spectralIndex();
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
WarpXHostServices::computeRho (int lev, int species_index, bool apply_rz_psatd_filter,
                               int ncomp, const amrex::IntVect& nodal,
                               const amrex::IntVect& ngrow, bool allow_extra)
{
    auto rho = computeRho(lev, species_index, apply_rz_psatd_filter);
    WARPX_ALWAYS_ASSERT_WITH_MESSAGE(
        rho->nComp() == ncomp || (allow_extra && rho->nComp() > ncomp),
        "computeRho: requested " + std::to_string(ncomp) + " components but the "
        "deposited rho has " + std::to_string(rho->nComp()));
#ifndef WARPX_DIM_RZ
    WARPX_ALWAYS_ASSERT_WITH_MESSAGE(
        rho->ixType() == amrex::IndexType(nodal),
        "computeRho: requested nodal type does not match the deposited rho");
#else
    // RZ PSATD deposits rho cell-centred (MultiParticleContainer::GetZeroChargeDensity
    // skips surroundingNodes for RZ PSATD, same as upstream) while a diagnostic
    // interpolates it onto nodes in InterpolateMFForDiag.  That mismatch is
    // legitimate, so the layout is not validated here on RZ.
    amrex::ignore_unused(nodal);
#endif
    WARPX_ALWAYS_ASSERT_WITH_MESSAGE(
        rho->nGrowVect() == ngrow,
        "computeRho: requested ngrow does not match the deposited rho");
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
    es_solver.definePhiBCs(m_warpx.Geom(0));
    const std::array<amrex::Real, 3> beta = {0.0};
    es_solver.computePhi(
        amrex::GetVecOfPtrs(rho_vec), amrex::GetVecOfPtrs(phi_vec),
        beta, m_warpx.GetElectrostaticSolver().mlmgOptions(), /*is_igf_2d_slices*/ false);

    return std::move(phi_vec[0]);
}

void
WarpXHostServices::syncRho (
    ablastr::fields::MultiLevelScalarField const& rho_fp,
    ablastr::fields::MultiLevelScalarField const& rho_cp,
    ablastr::fields::MultiLevelScalarField const& rho_buffer)
{
    m_warpx.SyncRho(rho_fp, rho_cp, rho_buffer);
}

void
WarpXHostServices::finalizeRhoOnLevel (int lev, amrex::MultiFab* rho,
                                       ablastr::utils::enums::PatchType patch)
{
    m_warpx.FinalizeRhoOnLevel(lev, rho, patch);
}

void
WarpXHostServices::applyFilterandSumBoundaryRho (int lev_min, int lev_max,
                                                 amrex::MultiFab& rho, int scomp, int ncomp)
{
    m_warpx.ApplyFilterandSumBoundaryRho(lev_min, lev_max, rho, scomp, ncomp);
}

void
WarpXHostServices::applyFilterMF (ablastr::fields::MultiLevelVectorField const& field,
                                  int lev, int idir)
{
    m_warpx.ApplyFilterMF(field, lev, idir);
}

const amrex::Vector<std::unique_ptr<amrex::iMultiFab>>&
WarpXHostServices::ebReduceParticleShapeFlag () const
{
    return m_warpx.GetEBReduceParticleShapeFlag();
}

void
WarpXHostServices::syncCurrent (const std::string& current_fp_string)
{
    m_warpx.SyncCurrent(current_fp_string);
}

void
WarpXHostServices::applyInverseVolumeScalingToCurrentDensity (
    amrex::MultiFab* Jx, amrex::MultiFab* Jy, amrex::MultiFab* Jz, int lev)
{
#if defined(WARPX_DIM_RZ) || defined(WARPX_DIM_RCYLINDER) || defined(WARPX_DIM_RSPHERE)
    m_warpx.ApplyInverseVolumeScalingToCurrentDensity(Jx, Jy, Jz, lev);
#else
    amrex::ignore_unused(Jx, Jy, Jz, lev);
#endif
}

amrex::Real WarpXHostServices::movingWindowX () const { return m_warpx.getmoving_window_x(); }
bool WarpXHostServices::movingWindowActive (int step) const { return WarpX::moving_window_active(step); }
int WarpXHostServices::numCellsShifted (amrex::Real distance, amrex::Real dx) const { return WarpX::NumCellsShifted(distance, dx); }
bool WarpXHostServices::doMovingWindow () const { return m_warpx.getdo_moving_window(); }
void
WarpXHostServices::pushParticlesAndDeposit (amrex::Real cur_time, amrex::Real dt, int step)
{
    m_warpx.RunPicPush(cur_time, dt, step);
}

void
WarpXHostServices::finalizeDeposition ()
{
    m_warpx.RunPicFinalizeDeposition();
}

void
WarpXHostServices::computeDiagnostics (int step)
{
    m_warpx.FlushDiagnostics(step);
}

void
WarpXHostServices::calculateNuei (const std::string& species)
{
    m_warpx.particleSystem().CalculateNuei(species);
}

void
WarpXHostServices::transformMomentumToCurvilinear (bool forward)
{
    m_warpx.particleSystem().TransformMomentumToCurvilinear(forward);
}

void
WarpXHostServices::generateGlobalDebyeLength ()
{
    m_warpx.particleSystem().GenerateGlobalDebyeLength();
}

void
WarpXHostServices::generateVirtualPhotons ()
{
#if defined(WARPX_QED)
    collision::binarycollision::virtualphotons::GenerateVirtualPhotons(
        &m_warpx.particleSystem());
#endif
}

void
WarpXHostServices::fieldSolve (amrex::Real cur_time)
{
    m_warpx.RunPicFieldSolve(cur_time);
}

void WarpXHostServices::updateMaxStep (int new_max_step) { m_warpx.updateMaxStep(new_max_step); }
void WarpXHostServices::updateStopTime (amrex::Real new_stop_time) { m_warpx.updateStopTime(new_stop_time); }

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

std::vector<bool> WarpXHostServices::pmlDirections () const { return m_warpx.getPMLdirections(); }
std::string WarpXHostServices::authors () const { return m_warpx.GetAuthors(); }
void WarpXHostServices::writeParticleHeader (std::ostream& os) { m_warpx.particleSystem().WriteHeader(os); }
void WarpXHostServices::writeReducedDiagCheckpoint (const std::string& dir) { m_warpx.reduced_diags->WriteCheckpointData(dir); }
amrex::Vector<int> WarpXHostServices::nsubsteps () const { return m_warpx.getnsubsteps(); }
amrex::Vector<amrex::Real> WarpXHostServices::tNew () const { return m_warpx.gett_new(); }
amrex::Vector<amrex::Real> WarpXHostServices::tOld () const { return m_warpx.gett_old(); }
amrex::Vector<amrex::Real> WarpXHostServices::dt () const { return m_warpx.getdt(); }
int WarpXHostServices::totalDiags () const { return m_warpx.GetMultiDiags().GetTotalDiags(); }
std::string WarpXHostServices::diagName (int i) const { return m_warpx.GetMultiDiags().diagName(i); }
bool WarpXHostServices::isDiagBackTransformed (int i) const { return m_warpx.GetMultiDiags().isDiagBackTransformed(i); }
int WarpXHostServices::diagNumBuffers (int i) const { return m_warpx.GetMultiDiags().diagNumBuffers(i); }
amrex::Real WarpXHostServices::diagTlab (int i, int buffer) const { return m_warpx.GetMultiDiags().diagTlab(i, buffer); }
int WarpXHostServices::diagBufferKIndexHi (int i, int buffer) const { return m_warpx.GetMultiDiags().diagBufferKIndexHi(i, buffer); }
amrex::Real WarpXHostServices::diagSnapshotDomainLo (int i, int buffer, int dim) const { return m_warpx.GetMultiDiags().diagSnapshotDomainLo(i, buffer, dim); }
amrex::Real WarpXHostServices::diagSnapshotDomainHi (int i, int buffer, int dim) const { return m_warpx.GetMultiDiags().diagSnapshotDomainHi(i, buffer, dim); }
int WarpXHostServices::diagFlushCounter (int i, int buffer) const { return m_warpx.GetMultiDiags().diagFlushCounter(i, buffer); }
int WarpXHostServices::diagLastValidZslice (int i, int buffer) const { return m_warpx.GetMultiDiags().diagLastValidZslice(i, buffer); }
int WarpXHostServices::diagSnapshotFullFlag (int i, int buffer) const { return m_warpx.GetMultiDiags().diagSnapshotFullFlag(i, buffer); }
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

namespace warpx::runtime
{
int WarpXHostServices::getNumParticlesInContainer (const std::string& species, int boundary, bool local)
{
    return m_warpx.GetParticleBoundaryBuffer().getNumParticlesInContainer(species, boundary, local);
}
void WarpXHostServices::gatherParticlesFromDomainBoundaries (
    warpx::runtime::IParticleSystem& particles, amrex::Real cur_time)
{
    m_warpx.GetParticleBoundaryBuffer().gatherParticlesFromDomainBoundaries(particles, cur_time);
}

void WarpXHostServices::gatherParticlesFromEmbeddedBoundaries (
    warpx::runtime::IParticleSystem& particles,
    ablastr::fields::MultiLevelScalarField const& distance_to_eb, amrex::Real cur_time)
{
    m_warpx.GetParticleBoundaryBuffer().gatherParticlesFromEmbeddedBoundaries(
        particles, distance_to_eb, cur_time);
}

void WarpXHostServices::redistribute ()
{
    m_warpx.GetParticleBoundaryBuffer().redistribute();
}

std::string WarpXHostServices::boundaryName (int boundary)
{
    return m_warpx.GetParticleBoundaryBuffer().boundaryName(boundary);
}
void* WarpXHostServices::getParticleBufferPointer (const std::string& species, int boundary)
{
    return static_cast<void*>(m_warpx.GetParticleBoundaryBuffer().getParticleBufferPointer(species, boundary));
}
void WarpXHostServices::clearParticles (int boundary)
{
    m_warpx.GetParticleBoundaryBuffer().clearParticles(boundary);
}
}
