/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "Runtime/WarpXRuntimeData.H"

#include "Fluids/MultiFluidContainer.H"
#include "Particles/MultiParticleContainer.H"
#include "WarpX.H"

#include <AMReX_EBFabFactory.H>

#include <stdexcept>

namespace warpx::runtime
{

WarpXRuntimeData&
WarpXRuntimeData::Legacy ()
{
    // Use a thread-local unique_ptr so that the adapter outlives any
    // single call site, but is NOT a static that would be destroyed
    // alongside WarpX itself (avoiding the unordered static-destruction
    // order trap).  The adapter holds NO back-reference to WarpX; every
    // method reaches WarpX via WarpX::GetInstance() on demand, so there
    // is nothing here to dangle when WarpX singleton is torn down.
    thread_local std::unique_ptr<WarpXRuntimeData> legacy;
    if (!legacy) {
        legacy = std::make_unique<WarpXRuntimeData>();
    }
    return *legacy;
}

ablastr::fields::MultiFabRegister& WarpXRuntimeData::fields () { return WarpX::GetInstance().GetMultiFabRegister(); }
const ablastr::fields::MultiFabRegister& WarpXRuntimeData::fields () const { return WarpX::GetInstance().GetMultiFabRegister(); }
amrex::AmrCore& WarpXRuntimeData::amrCore () const { return WarpX::GetInstance(); }
MultiParticleContainer& WarpXRuntimeData::particles () { return WarpX::GetInstance().GetPartContainer(); }
const MultiParticleContainer& WarpXRuntimeData::particles () const { return WarpX::GetInstance().GetPartContainer(); }
MultiFluidContainer* WarpXRuntimeData::fluids () { auto& w = WarpX::GetInstance(); return w.DoFluidSpecies() ? &w.GetFluidContainer() : nullptr; }
int WarpXRuntimeData::finestLevel () const { return WarpX::GetInstance().finestLevel(); }
int WarpXRuntimeData::maxLevel () const { return WarpX::GetInstance().maxLevel(); }
const amrex::Geometry& WarpXRuntimeData::geom (int lev) const { return WarpX::GetInstance().Geom(lev); }
const amrex::Vector<amrex::Geometry>& WarpXRuntimeData::geomAll () const { return WarpX::GetInstance().Geom(); }
const amrex::BoxArray& WarpXRuntimeData::boxArray (int lev) const { return WarpX::GetInstance().boxArray(lev); }
const amrex::Vector<amrex::BoxArray>& WarpXRuntimeData::boxArrayAll () const { return WarpX::GetInstance().boxArray(); }
const amrex::DistributionMapping& WarpXRuntimeData::distributionMap (int lev) const { return WarpX::GetInstance().DistributionMap(lev); }
const amrex::Vector<amrex::DistributionMapping>& WarpXRuntimeData::distributionMapAll () const { return WarpX::GetInstance().DistributionMap(); }
amrex::Array<amrex::Real, 3> WarpXRuntimeData::cellSize (int lev) const { return WarpX::CellSize(lev); }
amrex::XDim3 WarpXRuntimeData::lowerCorner (const amrex::Box& box, int lev, amrex::Real delta) const { return WarpX::LowerCorner(box, lev, delta); }
amrex::XDim3 WarpXRuntimeData::upperCorner (const amrex::Box& box, int lev) const { return WarpX::UpperCorner(box, lev, amrex::Real(0.0)); }
amrex::RealBox WarpXRuntimeData::realBox (const amrex::Box& box, int lev) const { return WarpX::getRealBox(box, lev); }
amrex::XDim3 WarpXRuntimeData::invCellSize (int lev) const { return WarpX::InvCellSize(lev); }
amrex::IntVect WarpXRuntimeData::ngEB () const { return WarpX::GetInstance().getngEB(); }
amrex::IntVect WarpXRuntimeData::ngDeposRho () const { return WarpX::GetInstance().get_ng_depos_rho(); }
amrex::IntVect WarpXRuntimeData::ngDeposJ () const { return WarpX::GetInstance().get_ng_depos_J(); }
bool WarpXRuntimeData::useVerboncoeurAxisCorrection () const { return WarpX::GetInstance().UseVerboncoeurAxisCorrection(); }
const amrex::Vector<amrex::Real>& WarpXRuntimeData::vGalilean () const { return WarpX::GetInstance().m_v_galilean; }
const amrex::Vector<amrex::Real>& WarpXRuntimeData::dtAll () const { return WarpX::GetInstance().getdt(); }
const AcceleratorLattice& WarpXRuntimeData::acceleratorLattice (int lev) const { return WarpX::GetInstance().get_accelerator_lattice(lev); }
HybridPICModel* WarpXRuntimeData::hybridPICModel () const { return WarpX::GetInstance().get_pointer_HybridPICModel(); }
const amrex::EB2::IndexSpace* WarpXRuntimeData::ebIndexSpace (int lev) const { return WarpX::GetInstance().GetEBIndexSpace(lev); }
const amrex::Vector<std::unique_ptr<NCIGodfreyFilter>>& WarpXRuntimeData::nciGodfreyFilterExEyBz () const { return WarpX::GetInstance().nci_godfrey_filter_exeybz; }
const amrex::Vector<std::unique_ptr<NCIGodfreyFilter>>& WarpXRuntimeData::nciGodfreyFilterBxByEz () const { return WarpX::GetInstance().nci_godfrey_filter_bxbyez; }
amrex::IntVect WarpXRuntimeData::blockingFactor (int lev) const { return WarpX::GetInstance().blockingFactor(lev); }
amrex::IntVect WarpXRuntimeData::numProcs () const { return WarpX::GetInstance().get_numprocs(); }
amrex::IntVect WarpXRuntimeData::maxGridSize (int lev) const { return WarpX::GetInstance().maxGridSize(lev); }
amrex::IntVect WarpXRuntimeData::refRatio (int lev) const { return WarpX::GetInstance().refRatio(lev); }
const amrex::Vector<amrex::IntVect>& WarpXRuntimeData::refRatioAll () const { return WarpX::GetInstance().refRatio(); }
const amrex::Vector<amrex::IntVect>& WarpXRuntimeData::refRatioVector () const { return WarpX::GetInstance().refRatio(); }
const amrex::Array<amrex::Real, 3>& WarpXRuntimeData::galileanShift () const { return WarpX::GetInstance().m_galilean_shift; }
amrex::Real WarpXRuntimeData::t_new (int lev) const { return WarpX::GetInstance().gett_new(lev); }
amrex::Real WarpXRuntimeData::dt (int lev) const { return WarpX::GetInstance().getdt(lev); }
int WarpXRuntimeData::istep (int lev) const { return WarpX::GetInstance().getistep(lev); }
amrex::Vector<int> WarpXRuntimeData::istepVector () const { return WarpX::GetInstance().getistep(); }
EvolveScheme WarpXRuntimeData::evolveScheme () const { return WarpX::GetInstance().evolve_scheme; }
amrex::LayoutData<amrex::Real>* WarpXRuntimeData::costs (int lev) const { return WarpX::getCosts(lev); }
amrex::Real WarpXRuntimeData::loadBalanceEfficiency (int lev) const { return WarpX::GetInstance().getLoadBalanceEfficiency(lev); }
ablastr::utils::text::IntervalsParser WarpXRuntimeData::loadBalanceIntervals () const { return WarpX::GetInstance().get_load_balance_intervals(); }
int WarpXRuntimeData::maxStep () const { return WarpX::GetInstance().maxStep(); }
amrex::Real WarpXRuntimeData::stopTime () const { return WarpX::GetInstance().stopTime(); }
bool WarpXRuntimeData::isSynchronized () const { return WarpX::GetInstance().getis_synchronized(); }
void WarpXRuntimeData::computeCostsHeuristic () const { WarpX::GetInstance().ComputeCostsHeuristic(); }

const amrex::EBFArrayBoxFactory& WarpXRuntimeData::fieldEBFactory (int lev) const
{
#if defined(AMREX_USE_EB)
    return WarpX::GetInstance().fieldEBFactory(lev);
#else
    amrex::ignore_unused(lev);
    throw std::runtime_error("WarpXRuntimeData::fieldEBFactory requires AMREX_USE_EB");
#endif
}

} // namespace warpx::runtime
