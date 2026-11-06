/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_RUNTIME_RUNTIME_H_
#define WARPX_RUNTIME_RUNTIME_H_

#include "Runtime/FrameState.H"

#include <ablastr/fields/MultiFabRegister.H>
#include "Runtime/RuntimeConfig.H"
#include "Runtime/RuntimeContext.H"
#include "Runtime/Services.H"
#include "Runtime/TimeState.H"

#include <memory>

class WarpX;

namespace warpx::runtime
{

class WarpXRuntimeData;
class DataBus;
class PluginHost;
class WarpXHostServices;

/**
 * \brief Process-level owner of the simulation lifetime.
 *
 * This is the first step of the plugin-runtime refactor (see
 * Docs/research_notes/最终的技术路线.md). It currently forwards to the legacy
 * WarpX singleton, so behaviour is unchanged. Later steps move configuration,
 * time state, services, and the plugin host into this class.
 */
class Runtime
{
public:
    Runtime (int argc, char* argv[]);
    ~Runtime ();

    Runtime (Runtime const &) = delete;
    Runtime& operator= (Runtime const &) = delete;
    Runtime (Runtime &&) = delete;
    Runtime& operator= (Runtime &&) = delete;

    /** Allocate state and run the package initialization (currently WarpX::InitData). */
    void initialize ();

    /** Advance the simulation. */
    void evolve (int numsteps = -1);

    /** Release simulation state. Idempotent. */
    void finalize ();

    [[nodiscard]] bool verbose () const;

    //! Read-only configuration snapshot (populated after initialize()).
    [[nodiscard]] const RuntimeConfig& config () const;

    //! Read-only clock/level snapshot.
    [[nodiscard]] const TimeState& time () const;

    //! Read-only frame/moving-window snapshot.
    [[nodiscard]] const FrameState& frame () const;

    //! Non-owning context view (config/time/frame/services) installed into WarpX.
    [[nodiscard]] const RuntimeContext& context () const;

    //! Live field registry (the authoritative storage; no DataBus copy).
    [[nodiscard]] ablastr::fields::MultiFabRegister& fields ();
    [[nodiscard]] const ablastr::fields::MultiFabRegister& fields () const;

    //! Thin schema/forwarding layer over fields(); no storage of its own.
    [[nodiscard]] DataBus& bus ();
    [[nodiscard]] const DataBus& bus () const;

    //! Kernel services (FillBoundary/moving window/PML/in-situ metadata).
    [[nodiscard]] HostServices& host ();
    [[nodiscard]] PluginHost& plugins ();
    [[nodiscard]] const HostServices& host () const;

private:
    bool m_initialized = false;
    bool m_finalized = false;
    RuntimeConfig m_config{};
    TimeState m_time{};
    FrameState m_frame{};
    RuntimeContext m_context{};
    std::unique_ptr<WarpX> m_warpx;
    std::unique_ptr<WarpXRuntimeData> m_data;
    std::unique_ptr<DataBus> m_bus;
    std::unique_ptr<WarpXHostServices> m_host;
    std::unique_ptr<PluginHost> m_plugin_host;

    void sync_config ();
    void sync_time ();
    void sync_state ();
};

} // namespace warpx::runtime

#endif // WARPX_RUNTIME_RUNTIME_H_
