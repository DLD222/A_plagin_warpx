/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "Runtime/FrameState.H"

#include "WarpX.H"

namespace warpx::runtime
{

FrameState
FrameState::FromLegacy ()
{
    FrameState f;
    f.gamma_boost = WarpX::gamma_boost;
    f.beta_boost = WarpX::beta_boost;
    f.boost_direction = WarpX::boost_direction;
    f.do_moving_window = (WarpX::do_moving_window != 0);
    f.moving_window_dir = WarpX::moving_window_dir;
    f.moving_window_v = WarpX::moving_window_v;
    f.start_moving_window_step = WarpX::start_moving_window_step;
    f.end_moving_window_step = WarpX::end_moving_window_step;
    return f;
}

} // namespace warpx::runtime
