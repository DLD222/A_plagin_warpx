/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_RUNTIME_RUNTIMEDATA_H_
#define WARPX_RUNTIME_RUNTIMEDATA_H_

#include "Utils/WarpXAlgorithmSelection.H"

#include <ablastr/fields/MultiFabRegister.H>

#include <AMReX_Array.H>
#include <AMReX_BoxArray.H>
#include <AMReX_DistributionMapping.H>
#include <AMReX_Geometry.H>
#include <AMReX_LayoutData.H>
#include <AMReX_REAL.H>

class MultiParticleContainer;
class MultiFluidContainer;

namespace warpx::runtime
{

/**
 * \brief Narrow, read-mostly view of the kernel state used by diagnostics.
 *
 * This is the A1.4 replacement target for `WarpX::GetInstance()` in the
 * reduced diagnostics. It deliberately exposes data (fields, particles,
 * geometry, clock), not algorithms.
 */
class RuntimeData
{
public:
    virtual ~RuntimeData () = default;

    [[nodiscard]] virtual ablastr::fields::MultiFabRegister& fields () = 0;
    [[nodiscard]] virtual const ablastr::fields::MultiFabRegister& fields () const = 0;

    [[nodiscard]] virtual MultiParticleContainer& particles () = 0;
    [[nodiscard]] virtual MultiFluidContainer* fluids () = 0;

    [[nodiscard]] virtual int finestLevel () const = 0;
    [[nodiscard]] virtual int maxLevel () const = 0;

    [[nodiscard]] virtual const amrex::Geometry& geom (int lev) const = 0;
    [[nodiscard]] virtual const amrex::BoxArray& boxArray (int lev) const = 0;
    [[nodiscard]] virtual const amrex::DistributionMapping& distributionMap (int lev) const = 0;
    [[nodiscard]] virtual amrex::Array<amrex::Real, 3> cellSize (int lev) const = 0;

    [[nodiscard]] virtual amrex::Real t_new (int lev) const = 0;
    [[nodiscard]] virtual amrex::Real dt (int lev) const = 0;
    [[nodiscard]] virtual int istep (int lev) const = 0;
    [[nodiscard]] virtual EvolveScheme evolveScheme () const = 0;

    [[nodiscard]] virtual amrex::LayoutData<amrex::Real>* costs (int lev) const = 0;
    [[nodiscard]] virtual amrex::Real loadBalanceEfficiency (int lev) const = 0;
};

} // namespace warpx::runtime

#endif // WARPX_RUNTIME_RUNTIMEDATA_H_
