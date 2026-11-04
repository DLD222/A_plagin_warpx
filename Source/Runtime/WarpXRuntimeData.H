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

    //! Legacy fallback (WarpX singleton); used when no RuntimeContext is installed.
    static WarpXRuntimeData& Legacy ();

    [[nodiscard]] ablastr::fields::MultiFabRegister& fields () override;
    [[nodiscard]] const ablastr::fields::MultiFabRegister& fields () const override;
    [[nodiscard]] MultiParticleContainer& particles () override;
    [[nodiscard]] MultiFluidContainer* fluids () override;
    [[nodiscard]] int finestLevel () const override;
    [[nodiscard]] int maxLevel () const override;
    [[nodiscard]] const amrex::Geometry& geom (int lev) const override;
    [[nodiscard]] const amrex::BoxArray& boxArray (int lev) const override;
    [[nodiscard]] const amrex::DistributionMapping& distributionMap (int lev) const override;
    [[nodiscard]] amrex::Array<amrex::Real, 3> cellSize (int lev) const override;
    [[nodiscard]] amrex::Real t_new (int lev) const override;
    [[nodiscard]] amrex::Real dt (int lev) const override;
    [[nodiscard]] int istep (int lev) const override;
    [[nodiscard]] EvolveScheme evolveScheme () const override;
    [[nodiscard]] amrex::LayoutData<amrex::Real>* costs (int lev) const override;
    [[nodiscard]] amrex::Real loadBalanceEfficiency (int lev) const override;

private:
    WarpX& m_warpx;
};

} // namespace warpx::runtime

#endif // WARPX_RUNTIME_WARPXRUNTIMEDATA_H_
