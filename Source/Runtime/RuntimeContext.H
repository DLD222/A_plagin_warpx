/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_RUNTIME_RUNTIMECONTEXT_H_
#define WARPX_RUNTIME_RUNTIMECONTEXT_H_

#include "Runtime/FrameState.H"
#include "Runtime/RuntimeConfig.H"
#include "Runtime/TimeState.H"

#include <functional>

namespace warpx::runtime
{

class Services;

/**
 * \brief Non-owning view of the runtime state handed to kernel/plugin code.
 *
 * Lifetime: owned by Runtime; the pointers stay valid until Runtime is
 * destroyed. WarpX stores a raw pointer (nullptr on the legacy/Python path) and
 * falls back to its static state when it is null.
 */
struct RuntimeContext
{
    const RuntimeConfig* config = nullptr;
    const TimeState* time = nullptr;
    const FrameState* frame = nullptr;
    Services* services = nullptr;

    //! Transitional live-clock hook: WarpX calls this at step boundaries and
    //! after InitData so TimeState is fresh when diagnostics run.
    std::function<void()> refresh_time{};
};

} // namespace warpx::runtime

#endif // WARPX_RUNTIME_RUNTIMECONTEXT_H_
