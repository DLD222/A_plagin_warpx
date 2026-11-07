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
#include "Runtime/CollisionPackage.H"
#include "Modules/PicEm/PicEmPackage.H"
#include "Runtime/DiagnosticsPackage.H"
#include "Runtime/PluginLoader.H"
#include "Utils/WarpXAlgorithmSelection.H"
#include "WarpX.H"

#include <stdexcept>

namespace warpx::runtime
{

namespace
{

std::string field_solver_module_name (const RuntimeConfig& config)
{
    if (config.electrostatic_solver_id() != ElectrostaticSolverAlgo::None) {
        return "field_solve.electrostatic";
    }
    switch (config.electromagnetic_solver_id()) {
        case ElectromagneticSolverAlgo::Yee:      return "field_solve.yee";
        case ElectromagneticSolverAlgo::CKC:      return "field_solve.ckc";
        case ElectromagneticSolverAlgo::PSATD:    return "field_solve.psatd";
        case ElectromagneticSolverAlgo::ECT:      return "field_solve.ect";
        case ElectromagneticSolverAlgo::HybridPIC:return "field_solve.hybrid_pic";
        case ElectromagneticSolverAlgo::None:     return "field_solve.none";
        default:                                  return "field_solve.unknown";
    }
}

TimeState make_time (const WarpX& warpx, bool valid)
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
    t.valid = valid;
    return t;
}

} // namespace

Runtime::Runtime (int argc, char* argv[])
{
    warpx::initialization::initialize_external_libraries(argc, argv);
    m_external_libraries_initialized = true;

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
        std::set<std::string> disabled;
        if (const char* names = std::getenv("WARPX_DISABLE_BUILTIN_MODULES")) {
            std::stringstream stream(names);
            std::string item;
            while (std::getline(stream, item, ',')) {
                if (!item.empty()) { disabled.insert(item); }
            }
        }
        PicEmPackage::register_handlers(*m_plugin_host,
                                        field_solver_module_name(m_config), disabled);
        DiagnosticsPackage::register_handlers(*m_plugin_host);
    }
    warpx.SetPluginHost(m_plugin_host.get());
    m_host = std::make_unique<WarpXHostServices>(warpx);
    sync_config();
    sync_time();
    m_context = RuntimeContext(&m_config, &m_time, &m_frame, m_data.get(), m_bus.get(),
                               m_host.get(), [this] { sync_time(); },
                               &m_config_snapshot);
    warpx.SetRuntimeContext(&m_context);

    auto& mpc = warpx.GetPartContainer();
    auto* host = m_host.get();
    CollisionSpeciesTable species(mpc, [host](const std::string& name) {
        host->particleServices().calculateNuei(name);
    });
    m_collision_handler = std::make_unique<CollisionHandler>(
        species, *m_data, m_config, host);
    warpx.SetCollisionHandler(m_collision_handler.get());
    if (!disable_handlers) {
        CollisionPackage::register_handler(*m_plugin_host, m_collision_handler.get());
    }

    if (const char* plugin_path = std::getenv("WARPX_PLUGIN_PATH")) {
        PluginLoader::load(plugin_path, *m_plugin_host);
    }
}

Runtime::~Runtime ()
{
    finalize();
    if (m_external_libraries_initialized) {
        warpx::initialization::finalize_external_libraries();
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

const RuntimeConfigSnapshot&
Runtime::config_snapshot () const
{
    return m_config_snapshot;
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
    m_config_snapshot = RuntimeConfigSnapshot::FromLegacy();
}

void
Runtime::sync_time ()
{
    m_time = make_time(*m_warpx, m_initialized);
    m_frame = FrameState::FromLegacy();
}

void
Runtime::sync_state ()
{
    sync_config();
    sync_time();
}

} // namespace warpx::runtime
