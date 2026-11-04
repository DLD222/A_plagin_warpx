/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_RUNTIME_FRAMESTATE_H_
#define WARPX_RUNTIME_FRAMESTATE_H_

#include <AMReX_REAL.H>
#include <AMReX_Vector.H>

namespace warpx::runtime
{

/** Read-only snapshot of frame/moving-window state (A1 replacement target). */
struct FrameState
{
    amrex::Real gamma_boost = 1.0;
    amrex::Real beta_boost = 0.0;
    amrex::Vector<int> boost_direction{};
    bool do_moving_window = false;
    int moving_window_dir = -1;
    amrex::Real moving_window_v = 0.0;
    int start_moving_window_step = -1;
    int end_moving_window_step = -1;
};

} // namespace warpx::runtime

#endif // WARPX_RUNTIME_FRAMESTATE_H_
