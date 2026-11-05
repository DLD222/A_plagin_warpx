/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_RUNTIME_WARPXRUNTIMEDATA_H_
#define WARPX_RUNTIME_WARPXRUNTIMEDATA_H_

#include "Runtime/RuntimeData.H"

class WarpX;

namespace warpx::runtime
{

/** RuntimeData adapter over a WarpX instance (transitional, not for plugins). */
class WarpXRuntimeData final : public RuntimeData
{
public:
    explicit WarpXRuntimeData (WarpX& warpx) : m_warpx(warpx) {}

    [[nodiscard]] ablastr::fields::MultiFabRegister& fields () override;
    [[nodiscard]] const ablastr::fields::MultiFabRegister& fields () const override;
    [[nodiscard]] amrex::AmrCore& amrCore () const override;
    [[nodiscard]] MultiParticleContainer& particles () override;
    [[nodiscard]] const MultiParticleContainer& particles () const override;
    [[nodiscard]] MultiFluidContainer* fluids () override;
    [[nodiscard]] int finestLevel () const override;
    [[nodiscard]] int maxLevel () const override;
    [[nodiscard]] const amrex::Geometry& geom (int lev) const override;
    [[nodiscard]] const amrex::Vector<amrex::Geometry>& geomAll () const override;
    [[nodiscard]] const amrex::BoxArray& boxArray (int lev) const override;
    [[nodiscard]] const amrex::Vector<amrex::BoxArray>& boxArrayAll () const override;
    [[nodiscard]] const amrex::DistributionMapping& distributionMap (int lev) const override;
    [[nodiscard]] const amrex::Vector<amrex::DistributionMapping>& distributionMapAll () const override;
    [[nodiscard]] amrex::Array<amrex::Real, 3> cellSize (int lev) const override;
    [[nodiscard]] amrex::XDim3 lowerCorner (const amrex::Box& box, int lev) const override;
    [[nodiscard]] amrex::XDim3 upperCorner (const amrex::Box& box, int lev) const override;
    [[nodiscard]] amrex::RealBox realBox (const amrex::Box& box, int lev) const override;
    [[nodiscard]] amrex::XDim3 invCellSize (int lev) const override;
    [[nodiscard]] amrex::IntVect ngEB () const override;
    [[nodiscard]] amrex::IntVect ngDeposRho () const override;
    [[nodiscard]] amrex::IntVect ngDeposJ () const override;
    [[nodiscard]] bool useVerboncoeurAxisCorrection () const override;
    [[nodiscard]] const amrex::Vector<amrex::Real>& vGalilean () const override;
    [[nodiscard]] const amrex::EB2::IndexSpace* ebIndexSpace (int lev) const override;
    [[nodiscard]] const amrex::Vector<std::unique_ptr<NCIGodfreyFilter>>&
    nciGodfreyFilterExEyBz () const override;
    [[nodiscard]] const amrex::Vector<std::unique_ptr<NCIGodfreyFilter>>&
    nciGodfreyFilterBxByEz () const override;
    [[nodiscard]] amrex::IntVect blockingFactor (int lev) const override;
    [[nodiscard]] amrex::IntVect numProcs () const override;
    [[nodiscard]] amrex::IntVect maxGridSize (int lev) const override;
    [[nodiscard]] amrex::IntVect refRatio (int lev) const override;
    [[nodiscard]] const amrex::Vector<amrex::IntVect>& refRatioAll () const override;
    [[nodiscard]] const amrex::Vector<amrex::IntVect>& refRatioVector () const override;
    [[nodiscard]] const amrex::Array<amrex::Real, 3>& galileanShift () const override;
    [[nodiscard]] amrex::Real t_new (int lev) const override;
    [[nodiscard]] amrex::Real dt (int lev) const override;
    [[nodiscard]] int istep (int lev) const override;
    [[nodiscard]] amrex::Vector<int> istepVector () const override;
    [[nodiscard]] EvolveScheme evolveScheme () const override;
    [[nodiscard]] amrex::LayoutData<amrex::Real>* costs (int lev) const override;
    [[nodiscard]] amrex::Real loadBalanceEfficiency (int lev) const override;
    [[nodiscard]] ablastr::utils::text::IntervalsParser loadBalanceIntervals () const override;
    [[nodiscard]] int maxStep () const override;
    [[nodiscard]] amrex::Real stopTime () const override;
    [[nodiscard]] bool isSynchronized () const override;
    void computeCostsHeuristic (
        amrex::Vector<std::unique_ptr<amrex::LayoutData<amrex::Real>>>& costs) const override;
    [[nodiscard]] const amrex::EBFArrayBoxFactory& fieldEBFactory (int lev) const override;

private:
    WarpX& m_warpx;
};

} // namespace warpx::runtime

#endif // WARPX_RUNTIME_WARPXRUNTIMEDATA_H_
