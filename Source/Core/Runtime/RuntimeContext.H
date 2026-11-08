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

class DataBus;
class HostServices;
class StateRegistry;
class RuntimeData;
struct RuntimeConfigSnapshot;

/**
 * \brief Non-owning view of the runtime state handed to kernel/plugin code.
 *
 * Lifetime: owned by Runtime; the pointers stay valid until Runtime is
 * destroyed. WarpX stores a raw pointer (nullptr on the legacy/Python path) and
 * falls back to its static state when it is null.
 */
struct RuntimeContext
{
    RuntimeContext () = default;
    RuntimeContext (const RuntimeConfig* config_, const TimeState* time_,
                    const FrameState* frame_, RuntimeData* data_, DataBus* bus_,
                    HostServices* host_, std::function<void()> refresh_time_,
                    const RuntimeConfigSnapshot* config_snapshot_ = nullptr,
                    StateRegistry* state_ = nullptr)
        : config(config_), time(time_), frame(frame_), data(data_), bus(bus_),
          host(host_), refresh_time(std::move(refresh_time_)),
          config_snapshot(config_snapshot_), state(state_) {}

    RuntimeContext (const RuntimeContext&) = delete;
    RuntimeContext& operator= (const RuntimeContext&) = delete;
    RuntimeContext (RuntimeContext&&) = default;
    RuntimeContext& operator= (RuntimeContext&&) = default;

    const RuntimeConfig* config = nullptr;
    const TimeState* time = nullptr;
    const FrameState* frame = nullptr;
    RuntimeData* data = nullptr;
    DataBus* bus = nullptr;
    HostServices* host = nullptr;
    const RuntimeConfigSnapshot* config_snapshot = nullptr;
    //! Plugin-declared state the core owns (fields today; S3).
    StateRegistry* state = nullptr;

    //! Transitional live-clock hook: WarpX calls this at step boundaries and
    //! after InitData so TimeState is fresh when diagnostics run.
    std::function<void()> refresh_time{};
};

} // namespace warpx::runtime

#endif // WARPX_RUNTIME_RUNTIMECONTEXT_H_
