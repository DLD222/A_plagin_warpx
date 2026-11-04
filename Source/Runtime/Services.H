/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_RUNTIME_SERVICES_H_
#define WARPX_RUNTIME_SERVICES_H_

#include <AMReX_MultiFab.H>
#include <AMReX_Parser.H>
#include <AMReX_Random.H>

#include <string>

namespace warpx::runtime
{

/**
 * \brief Host-side services exposed to plugins.
 *
 * This is the A1.3 interface only: no implementation is wired yet. The
 * implementation will wrap the existing WarpX/ablastr helpers (FillBoundary,
 * SumBoundary, reductions, parser, RNG, temporary allocation), so plugins never
 * call WarpX::GetInstance() directly.
 *
 * Calls are coarse-grained (per level/patch), never per cell.
 */
class Services
{
public:
    virtual ~Services () = default;

    virtual void FillBoundary (amrex::MultiFab& mf, int ng) = 0;
    virtual void SumBoundary (amrex::MultiFab& mf, int scomp, int ncomp) = 0;
    virtual amrex::Real ReduceSum (const amrex::MultiFab& mf, int comp) = 0;

    virtual amrex::Parser MakeParser (const std::string& expr) = 0;
    virtual amrex::RandomEngine& Rng () = 0;

    virtual void Log (const std::string& message) = 0;
    virtual void Abort (const std::string& message) = 0;
};

} // namespace warpx::runtime

#endif // WARPX_RUNTIME_SERVICES_H_
