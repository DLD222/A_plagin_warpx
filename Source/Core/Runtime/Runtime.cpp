/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "Runtime/Runtime.H"

#include "Runtime/DataBus.H"
#include "Runtime/IPicKernel.H"
#include "Runtime/KernelFactory.H"
#include "Runtime/PluginLoader.H"
#include "Runtime/RuntimeConfig.H"
#include "Runtime/RuntimeConfigSnapshot.H"
#include "Runtime/RuntimeContext.H"


#include <cstdlib>
#include <stdexcept>

namespace warpx::runtime
{

Runtime::Runtime (int argc, char* argv[])
{
    initialize_runtime_libraries(argc, argv);
    m_external_libraries_initialized = true;

    m_kernel = create_pic_kernel(argc, argv);
    m_bus = std::make_unique<DataBus>(m_kernel->fields(), m_kernel->schema(),
                                     &m_state.scalars());
    sync_config();
    m_plugin_host = std::make_unique<PluginHost>();

    const bool disable_handlers = std::getenv("WARPX_DISABLE_PLUGIN_HANDLERS") != nullptr;
    m_kernel->set_plugin_host(m_plugin_host.get());
    m_kernel->register_plugins(*m_plugin_host, disable_handlers);

    sync_time();
    m_context = RuntimeContext(&m_config, &m_time, &m_frame,
                               &m_kernel->data(), m_bus.get(), &m_kernel->host(),
                               [this] { sync_time(); }, &m_config_snapshot, &m_state);
    m_kernel->set_runtime_context(&m_context);

    if (const char* plugin_path = std::getenv("WARPX_PLUGIN_PATH")) {
        PluginLoader::load(plugin_path, *m_plugin_host);
    }
}

Runtime::~Runtime ()
{
    finalize();
    if (m_external_libraries_initialized) {
        finalize_runtime_libraries();
        m_external_libraries_initialized = false;
    }
}

void
Runtime::initialize ()
{
    if (m_finalized) {
        throw std::runtime_error("Runtime::initialize() called after finalize()");
    }
    if (m_initialized) { return; }

    // S4: plugins may be added between construction and initialize(), so the
    // schema phase and the manifest check happen here, before any physics.
    SchemaContext schema_ctx{&m_state};
    m_plugin_host->run_schema(schema_ctx);
    m_plugin_host->validate_plugins();
    m_plugin_host->validate_capabilities(m_kernel->active_features());

    m_kernel->initialize();
    sync_config();
    m_kernel->allocate_collision_data();
    m_kernel->register_species(*m_bus);
    if (m_bus != nullptr) {
        StageContext ctx{&m_context, m_bus.get(), &m_kernel->host(), &m_kernel->data(),
                         &m_config_snapshot};
        m_plugin_host->run_init(ctx);
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
    m_kernel->evolve(numsteps);
    sync_state();
}

void
Runtime::finalize ()
{
    if (m_finalized) { return; }
    if (m_plugin_host != nullptr && m_bus != nullptr && m_kernel != nullptr) {
        StageContext ctx{&m_context, m_bus.get(), &m_kernel->host(), &m_kernel->data(),
                         &m_config_snapshot};
        m_plugin_host->run_finalize(ctx);
    }
    m_kernel->clear();
    m_bus.reset();
    m_plugin_host.reset();
    m_kernel.reset();
    m_finalized = true;
}

bool Runtime::verbose () const { return m_initialized && !m_finalized && m_kernel->verbose(); }
const RuntimeConfig& Runtime::config () const { return m_config; }
const RuntimeConfigSnapshot& Runtime::config_snapshot () const { return m_config_snapshot; }
const TimeState& Runtime::time () const { return m_time; }
const FrameState& Runtime::frame () const { return m_frame; }
const RuntimeContext& Runtime::context () const { return m_context; }
ablastr::fields::MultiFabRegister& Runtime::fields () { return m_kernel->fields(); }
const ablastr::fields::MultiFabRegister& Runtime::fields () const { return m_kernel->fields(); }
DataBus& Runtime::bus () { return *m_bus; }
const DataBus& Runtime::bus () const { return *m_bus; }
HostServices& Runtime::host () { return m_kernel->host(); }
const HostServices& Runtime::host () const { return m_kernel->host(); }
PluginHost& Runtime::plugins () { return *m_plugin_host; }
const PluginHost& Runtime::plugins () const { return *m_plugin_host; }

void Runtime::sync_config ()
{
    m_config = RuntimeConfig::FromLegacy();
    m_config_snapshot = m_kernel->config_snapshot();
}
void Runtime::sync_time ()
{
    m_time = m_kernel->time_state(m_initialized);
    m_frame = m_kernel->frame_state();
}
void Runtime::sync_state () { sync_config(); sync_time(); }

} // namespace warpx::runtime
