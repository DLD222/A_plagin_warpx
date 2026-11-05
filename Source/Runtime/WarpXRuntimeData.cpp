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
const amrex::BoxArray& WarpXRuntimeData::boxArray (int lev) const { return m_warpx.boxArray(lev); }
const amrex::DistributionMapping& WarpXRuntimeData::distributionMap (int lev) const { return m_warpx.DistributionMap(lev); }
amrex::Array<amrex::Real, 3> WarpXRuntimeData::cellSize (int lev) const { return WarpX::CellSize(lev); }
amrex::XDim3 WarpXRuntimeData::lowerCorner (const amrex::Box& box, int lev) const { return WarpX::LowerCorner(box, lev, amrex::Real(0.0)); }
amrex::XDim3 WarpXRuntimeData::invCellSize (int lev) const { return WarpX::InvCellSize(lev); }
amrex::IntVect WarpXRuntimeData::ngEB () const { return m_warpx.getngEB(); }
amrex::Real WarpXRuntimeData::t_new (int lev) const { return m_warpx.gett_new(lev); }
amrex::Real WarpXRuntimeData::dt (int lev) const { return m_warpx.getdt(lev); }
int WarpXRuntimeData::istep (int lev) const { return m_warpx.getistep(lev); }
EvolveScheme WarpXRuntimeData::evolveScheme () const { return m_warpx.evolve_scheme; }
amrex::LayoutData<amrex::Real>* WarpXRuntimeData::costs (int lev) const { return WarpX::getCosts(lev); }
amrex::Real WarpXRuntimeData::loadBalanceEfficiency (int lev) const { return m_warpx.getLoadBalanceEfficiency(lev); }

} // namespace warpx::runtime
