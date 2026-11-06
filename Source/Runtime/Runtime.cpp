/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "Runtime/Runtime.H"

#include "Initialization/WarpXInit.H"
#include "Runtime/DataBus.H"
#include "Runtime/WarpXHostServices.H"
#include "Runtime/WarpXRuntimeData.H"
#include "Particles/Collision/CollisionHandler.H"
#include "Particles/Collision/CollisionSpeciesTable.H"
#include "Particles/MultiParticleContainer.H"
#include "Runtime/CollisionHandlerAdapter.H"
#include "Runtime/PicFieldSolveHandler.H"
#include "Runtime/PicFinalizeHandler.H"
#include "Runtime/PicPushHandler.H"
#include "WarpX.H"

#include <stdexcept>

namespace warpx::runtime
{

namespace
{

TimeState make_time (const WarpX& warpx)
{
    TimeState t;
    t.step = warpx.getistep(0);
    t.finest_level = warpx.finestLevel();
    t.time = warpx.gett_new(0);
    t.istep = warpx.getistep();
    t.nsubsteps = warpx.getnsubsteps();
    t.dt = warpx.getdt();
    t.t_new = warpx.gett_new();
    t.t_old = warpx.gett_old();
    return t;
}

} // namespace

Runtime::Runtime (int argc, char* argv[])
{
    warpx::initialization::initialize_external_libraries(argc, argv);

    // Construct WarpX (reads parameters) before InitData, so diagnostics and
    // other setup-time consumers can see a config snapshot.
    m_warpx = WarpX::Create();
    auto& warpx = *m_warpx;
    m_data = std::make_unique<WarpXRuntimeData>(warpx);
    m_bus = std::make_unique<DataBus>(warpx.GetMultiFabRegister(), warpx.fieldSchema());
    m_plugin_host = std::make_unique<PluginHost>();
    // A/B switch for regression: with handlers disabled the legacy direct
    // phase calls are used, which must be bitwise identical.
    const bool disable_handlers = std::getenv("WARPX_DISABLE_PLUGIN_HANDLERS") != nullptr;
    if (!disable_handlers) {
        m_plugin_host->add(std::make_shared<PicPushHandler>());
        m_plugin_host->add(std::make_shared<PicFinalizeHandler>());
        m_plugin_host->add(std::make_shared<PicFieldSolveHandler>());
    }
    warpx.SetPluginHost(m_plugin_host.get());
    m_host = std::make_unique<WarpXHostServices>(warpx);
    sync_config();
    sync_time();
    m_context = RuntimeContext{&m_config, &m_time, &m_frame, m_data.get(), m_bus.get(),
                               m_host.get(), [this] { sync_time(); }};
    warpx.SetRuntimeContext(&m_context);

    auto& mpc = warpx.GetPartContainer();
    auto* host = m_host.get();
    CollisionSpeciesTable species(mpc, [host](const std::string& name) {
        host->calculateNuei(name);
    });
    m_collision_handler = std::make_unique<CollisionHandler>(
        species, *m_data, m_config, host);
    warpx.SetCollisionHandler(m_collision_handler.get());
    if (!disable_handlers) {
        m_plugin_host->add(std::make_shared<CollisionHandlerAdapter>(m_collision_handler.get()));
    }
}

Runtime::~Runtime ()
{
    finalize();
    warpx::initialization::finalize_external_libraries();
}

void
Runtime::initialize ()
{
    if (m_finalized) {
        throw std::runtime_error("Runtime::initialize() called after finalize()");
    }
    m_warpx->InitData();
    m_collision_handler->AllocData();

    auto& mpc = m_warpx->GetPartContainer();
    for (const auto& name : mpc.GetSpeciesNames()) {
        auto& pc = mpc.GetParticleContainerFromName(name);
        warpx::runtime::SpeciesSpec spec;
        spec.name = name;
        spec.charge = pc.getCharge();
        spec.mass = pc.getMass();
        m_bus->register_species(spec, &pc);
    }

    m_initialized = true;
    sync_state();
}

void
Runtime::evolve (int numsteps)
{
    if (!m_initialized || m_finalized) {
        throw std::runtime_error("Runtime::evolve() called before initialize() or after finalize()");
    }
    m_warpx->Evolve(numsteps);
    sync_state();
}

void
Runtime::finalize ()
{
    if (m_finalized) { return; }
    // Clear the WarpX→RuntimeContext back-pointer before tearing anything
    // down, so no concurrent path can dereference a destroyed object.
    if (m_warpx) {
        m_warpx->SetRuntimeContext(nullptr);
    }
    m_host.reset();
    if (m_warpx != nullptr) {
        m_warpx->SetCollisionHandler(nullptr);
    }
    m_collision_handler.reset();
    m_bus.reset();
    m_plugin_host.reset();
    m_data.reset();
    m_warpx.reset();
    WarpX::ResetInstance();
    m_finalized = true;
}

bool
Runtime::verbose () const
{
    return m_initialized && !m_finalized && m_warpx->Verbose();
}

const RuntimeConfig&
Runtime::config () const
{
    return m_config;
}

const TimeState&
Runtime::time () const
{
    return m_time;
}

const FrameState&
Runtime::frame () const
{
    return m_frame;
}

const RuntimeContext&
Runtime::context () const
{
    return m_context;
}

ablastr::fields::MultiFabRegister&
Runtime::fields ()
{
    return m_warpx->GetMultiFabRegister();
}

const ablastr::fields::MultiFabRegister&
Runtime::fields () const
{
    return m_warpx->GetMultiFabRegister();
}

DataBus&
Runtime::bus ()
{
    return *m_bus;
}

const DataBus&
Runtime::bus () const
{
    return *m_bus;
}

PluginHost&
Runtime::plugins ()
{
    return *m_plugin_host;
}

HostServices&
Runtime::host ()
{
    return *m_host;
}

const HostServices&
Runtime::host () const
{
    return *m_host;
}

void
Runtime::sync_config ()
{
    m_config = RuntimeConfig::FromLegacy();
}

void
Runtime::sync_time ()
{
    m_time = make_time(*m_warpx);
    m_frame = FrameState::FromLegacy();
}

void
Runtime::sync_state ()
{
    sync_config();
    sync_time();
}

} // namespace warpx::runtime
