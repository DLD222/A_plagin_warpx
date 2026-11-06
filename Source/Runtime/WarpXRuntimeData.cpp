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
#include "FieldSolver/FiniteDifferenceSolver/MacroscopicProperties/MacroscopicProperties.H"

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
    // method reaches WarpX via w() on demand, so there
    // is nothing here to dangle when WarpX singleton is torn down.
    thread_local std::unique_ptr<WarpXRuntimeData> legacy;
    if (!legacy) {
        legacy = std::make_unique<WarpXRuntimeData>();
    }
    // Refresh the non-owning pointer on every call: WarpX may have been
    // finalized and re-created since the previous call.
    legacy->m_warpx = &WarpX::GetInstance();
    return *legacy;
}

WarpX&
WarpXRuntimeData::w () const
{
    return m_warpx != nullptr ? *m_warpx : WarpX::GetInstance();
}

ablastr::fields::MultiFabRegister& WarpXRuntimeData::fields () { return w().GetMultiFabRegister(); }
const ablastr::fields::MultiFabRegister& WarpXRuntimeData::fields () const { return w().GetMultiFabRegister(); }
amrex::AmrCore& WarpXRuntimeData::amrCore () const { return w(); }
MultiParticleContainer& WarpXRuntimeData::particles () { return w().GetPartContainer(); }
const MultiParticleContainer& WarpXRuntimeData::particles () const { return w().GetPartContainer(); }
MultiFluidContainer* WarpXRuntimeData::fluids () { auto& warpx = w(); return warpx.DoFluidSpecies() ? &warpx.GetFluidContainer() : nullptr; }
int WarpXRuntimeData::finestLevel () const { return w().finestLevel(); }
int WarpXRuntimeData::maxLevel () const { return w().maxLevel(); }
const amrex::Geometry& WarpXRuntimeData::geom (int lev) const { return w().Geom(lev); }
const amrex::Vector<amrex::Geometry>& WarpXRuntimeData::geomAll () const { return w().Geom(); }
const amrex::BoxArray& WarpXRuntimeData::boxArray (int lev) const { return w().boxArray(lev); }
const amrex::Vector<amrex::BoxArray>& WarpXRuntimeData::boxArrayAll () const { return w().boxArray(); }
const amrex::DistributionMapping& WarpXRuntimeData::distributionMap (int lev) const { return w().DistributionMap(lev); }
const amrex::Vector<amrex::DistributionMapping>& WarpXRuntimeData::distributionMapAll () const { return w().DistributionMap(); }
amrex::Array<amrex::Real, 3> WarpXRuntimeData::cellSize (int lev) const { return WarpX::CellSize(lev); }
amrex::XDim3 WarpXRuntimeData::lowerCorner (const amrex::Box& box, int lev, amrex::Real delta) const { return WarpX::LowerCorner(box, lev, delta); }
amrex::XDim3 WarpXRuntimeData::upperCorner (const amrex::Box& box, int lev) const { return WarpX::UpperCorner(box, lev, amrex::Real(0.0)); }
amrex::RealBox WarpXRuntimeData::realBox (const amrex::Box& box, int lev) const { return WarpX::getRealBox(box, lev); }
amrex::XDim3 WarpXRuntimeData::invCellSize (int lev) const { return WarpX::InvCellSize(lev); }
amrex::IntVect WarpXRuntimeData::ngEB () const { return w().getngEB(); }
amrex::IntVect WarpXRuntimeData::ngDeposRho () const { return w().get_ng_depos_rho(); }
amrex::IntVect WarpXRuntimeData::ngDeposJ () const { return w().get_ng_depos_J(); }
bool WarpXRuntimeData::useVerboncoeurAxisCorrection () const { return w().UseVerboncoeurAxisCorrection(); }
const amrex::Vector<amrex::Real>& WarpXRuntimeData::vGalilean () const { return w().m_v_galilean; }
const amrex::Vector<amrex::Real>& WarpXRuntimeData::dtAll () const { return w().getdt(); }
const AcceleratorLattice& WarpXRuntimeData::acceleratorLattice (int lev) const { return w().get_accelerator_lattice(lev); }
HybridPICModel* WarpXRuntimeData::hybridPICModel () const { return w().get_pointer_HybridPICModel(); }
const FieldRegistry& WarpXRuntimeData::fieldRegistry () const { return w().fieldSchema(); }
const MaterialRegistry& WarpXRuntimeData::materials () const
{
    if (const auto* macroscopic = w().get_pointer_MacroscopicProperties()) {
        return macroscopic->materials();
    }
    static const MaterialRegistry empty;
    return empty;
}
const amrex::EB2::IndexSpace* WarpXRuntimeData::ebIndexSpace (int lev) const { return w().GetEBIndexSpace(lev); }
const amrex::Vector<std::unique_ptr<NCIGodfreyFilter>>& WarpXRuntimeData::nciGodfreyFilterExEyBz () const { return w().nci_godfrey_filter_exeybz; }
const amrex::Vector<std::unique_ptr<NCIGodfreyFilter>>& WarpXRuntimeData::nciGodfreyFilterBxByEz () const { return w().nci_godfrey_filter_bxbyez; }
amrex::IntVect WarpXRuntimeData::blockingFactor (int lev) const { return w().blockingFactor(lev); }
amrex::IntVect WarpXRuntimeData::numProcs () const { return w().get_numprocs(); }
amrex::IntVect WarpXRuntimeData::maxGridSize (int lev) const { return w().maxGridSize(lev); }
amrex::IntVect WarpXRuntimeData::refRatio (int lev) const { return w().refRatio(lev); }
const amrex::Vector<amrex::IntVect>& WarpXRuntimeData::refRatioAll () const { return w().refRatio(); }
const amrex::Vector<amrex::IntVect>& WarpXRuntimeData::refRatioVector () const { return w().refRatio(); }
const amrex::Array<amrex::Real, 3>& WarpXRuntimeData::galileanShift () const { return w().m_galilean_shift; }
amrex::Real WarpXRuntimeData::t_new (int lev) const { return w().gett_new(lev); }
amrex::Real WarpXRuntimeData::dt (int lev) const { return w().getdt(lev); }
int WarpXRuntimeData::istep (int lev) const { return w().getistep(lev); }
amrex::Vector<int> WarpXRuntimeData::istepVector () const { return w().getistep(); }
EvolveScheme WarpXRuntimeData::evolveScheme () const { return w().evolve_scheme; }
amrex::LayoutData<amrex::Real>* WarpXRuntimeData::costs (int lev) const { return WarpX::getCosts(lev); }
amrex::Real WarpXRuntimeData::loadBalanceEfficiency (int lev) const { return w().getLoadBalanceEfficiency(lev); }
ablastr::utils::text::IntervalsParser WarpXRuntimeData::loadBalanceIntervals () const { return w().get_load_balance_intervals(); }
int WarpXRuntimeData::maxStep () const { return w().maxStep(); }
amrex::Real WarpXRuntimeData::stopTime () const { return w().stopTime(); }
bool WarpXRuntimeData::isSynchronized () const { return w().getis_synchronized(); }
void WarpXRuntimeData::computeCostsHeuristic () const { w().ComputeCostsHeuristic(); }

const amrex::EBFArrayBoxFactory& WarpXRuntimeData::fieldEBFactory (int lev) const
{
#if defined(AMREX_USE_EB)
    return w().fieldEBFactory(lev);
#else
    amrex::ignore_unused(lev);
    throw std::runtime_error("WarpXRuntimeData::fieldEBFactory requires AMREX_USE_EB");
#endif
}

} // namespace warpx::runtime
