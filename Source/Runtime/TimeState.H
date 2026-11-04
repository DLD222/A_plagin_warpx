/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_RUNTIME_TIMESTATE_H_
#define WARPX_RUNTIME_TIMESTATE_H_

#include <AMReX_REAL.H>
#include <AMReX_Vector.H>

namespace warpx::runtime
{

/** Read-only snapshot of the clock/level state (A1 replacement target). */
struct TimeState
{
    int step = 0;
    int finest_level = 0;
    amrex::Real time = 0.0;
    amrex::Vector<int> istep{};
    amrex::Vector<int> nsubsteps{};
    amrex::Vector<amrex::Real> dt{};
    amrex::Vector<amrex::Real> t_new{};
    amrex::Vector<amrex::Real> t_old{};
};

} // namespace warpx::runtime

#endif // WARPX_RUNTIME_TIMESTATE_H_
