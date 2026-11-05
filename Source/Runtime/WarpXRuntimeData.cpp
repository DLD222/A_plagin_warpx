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


ablastr::fields::MultiFabRegister& WarpXRuntimeData::fields () { return m_warpx.GetMultiFabRegister(); }
const ablastr::fields::MultiFabRegister& WarpXRuntimeData::fields () const { return m_warpx.GetMultiFabRegister(); }
amrex::AmrCore& WarpXRuntimeData::amrCore () const { return m_warpx; }
MultiParticleContainer& WarpXRuntimeData::particles () { return m_warpx.GetPartContainer(); }
const MultiParticleContainer& WarpXRuntimeData::particles () const { return m_warpx.GetPartContainer(); }
MultiFluidContainer* WarpXRuntimeData::fluids () { return m_warpx.DoFluidSpecies() ? &m_warpx.GetFluidContainer() : nullptr; }
int WarpXRuntimeData::finestLevel () const { return m_warpx.finestLevel(); }
int WarpXRuntimeData::maxLevel () const { return m_warpx.maxLevel(); }
const amrex::Geometry& WarpXRuntimeData::geom (int lev) const { return m_warpx.Geom(lev); }
const amrex::Vector<amrex::Geometry>& WarpXRuntimeData::geomAll () const { return m_warpx.Geom(); }
const amrex::BoxArray& WarpXRuntimeData::boxArray (int lev) const { return m_warpx.boxArray(lev); }
const amrex::Vector<amrex::BoxArray>& WarpXRuntimeData::boxArrayAll () const { return m_warpx.boxArray(); }
const amrex::DistributionMapping& WarpXRuntimeData::distributionMap (int lev) const { return m_warpx.DistributionMap(lev); }
const amrex::Vector<amrex::DistributionMapping>& WarpXRuntimeData::distributionMapAll () const { return m_warpx.DistributionMap(); }
amrex::Array<amrex::Real, 3> WarpXRuntimeData::cellSize (int lev) const { return WarpX::CellSize(lev); }
amrex::XDim3 WarpXRuntimeData::lowerCorner (const amrex::Box& box, int lev) const { return WarpX::LowerCorner(box, lev, amrex::Real(0.0)); }
amrex::XDim3 WarpXRuntimeData::upperCorner (const amrex::Box& box, int lev) const { return WarpX::UpperCorner(box, lev, amrex::Real(0.0)); }
amrex::RealBox WarpXRuntimeData::realBox (const amrex::Box& box, int lev) const { return WarpX::getRealBox(box, lev); }
amrex::XDim3 WarpXRuntimeData::invCellSize (int lev) const { return WarpX::InvCellSize(lev); }
amrex::IntVect WarpXRuntimeData::ngEB () const { return m_warpx.getngEB(); }
amrex::IntVect WarpXRuntimeData::ngDeposRho () const { return m_warpx.get_ng_depos_rho(); }
amrex::IntVect WarpXRuntimeData::ngDeposJ () const { return m_warpx.get_ng_depos_J(); }
bool WarpXRuntimeData::useVerboncoeurAxisCorrection () const { return m_warpx.UseVerboncoeurAxisCorrection(); }
amrex::IntVect WarpXRuntimeData::blockingFactor (int lev) const { return m_warpx.blockingFactor(lev); }
amrex::IntVect WarpXRuntimeData::numProcs () const { return m_warpx.get_numprocs(); }
amrex::IntVect WarpXRuntimeData::maxGridSize (int lev) const { return m_warpx.maxGridSize(lev); }
amrex::IntVect WarpXRuntimeData::refRatio (int lev) const { return m_warpx.refRatio(lev); }
const amrex::Vector<amrex::IntVect>& WarpXRuntimeData::refRatioAll () const { return m_warpx.refRatio(); }
const amrex::Vector<amrex::IntVect>& WarpXRuntimeData::refRatioVector () const { return m_warpx.refRatio(); }
const amrex::Array<amrex::Real, 3>& WarpXRuntimeData::galileanShift () const { return m_warpx.m_galilean_shift; }
amrex::Real WarpXRuntimeData::t_new (int lev) const { return m_warpx.gett_new(lev); }
amrex::Real WarpXRuntimeData::dt (int lev) const { return m_warpx.getdt(lev); }
int WarpXRuntimeData::istep (int lev) const { return m_warpx.getistep(lev); }
amrex::Vector<int> WarpXRuntimeData::istepVector () const { return m_warpx.getistep(); }
EvolveScheme WarpXRuntimeData::evolveScheme () const { return m_warpx.evolve_scheme; }
amrex::LayoutData<amrex::Real>* WarpXRuntimeData::costs (int lev) const { return WarpX::getCosts(lev); }
amrex::Real WarpXRuntimeData::loadBalanceEfficiency (int lev) const { return m_warpx.getLoadBalanceEfficiency(lev); }
ablastr::utils::text::IntervalsParser WarpXRuntimeData::loadBalanceIntervals () const { return m_warpx.get_load_balance_intervals(); }
int WarpXRuntimeData::maxStep () const { return m_warpx.maxStep(); }
bool WarpXRuntimeData::isSynchronized () const { return m_warpx.getis_synchronized(); }
void WarpXRuntimeData::computeCostsHeuristic (
    amrex::Vector<std::unique_ptr<amrex::LayoutData<amrex::Real>>>& costs) const
{
    m_warpx.ComputeCostsHeuristic(costs);
}
const amrex::EBFArrayBoxFactory& WarpXRuntimeData::fieldEBFactory (int lev) const
{
#if defined(AMREX_USE_EB)
    return m_warpx.fieldEBFactory(lev);
#else
    amrex::ignore_unused(lev);
    throw std::runtime_error("WarpXRuntimeData::fieldEBFactory requires AMREX_USE_EB");
#endif
}

} // namespace warpx::runtime
