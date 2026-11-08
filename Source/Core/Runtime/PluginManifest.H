/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_RUNTIME_PLUGINMANIFEST_H_
#define WARPX_RUNTIME_PLUGINMANIFEST_H_

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace warpx::runtime
{

class StateRegistry;
struct StageContext;

/** What a plugin brings to the runtime. */
enum class PluginRole
{
    Kernel,
    Physics,
    FieldSolver,
    Particles,
    Diagnostics,
    IO,
    Boundary,
    Materials,
    Amr,
    TimeIntegrator,
    Utility
};

[[nodiscard]] inline const char* role_name (PluginRole role)
{
    switch (role) {
        case PluginRole::Kernel: return "kernel";
        case PluginRole::Physics: return "physics";
        case PluginRole::FieldSolver: return "field_solver";
        case PluginRole::Particles: return "particles";
        case PluginRole::Diagnostics: return "diagnostics";
        case PluginRole::IO: return "io";
        case PluginRole::Boundary: return "boundary";
        case PluginRole::Materials: return "materials";
        case PluginRole::Amr: return "amr";
        case PluginRole::TimeIntegrator: return "time_integrator";
        case PluginRole::Utility: return "utility";
    }
    return "unknown";
}

/**
 * \brief Self-description a plugin registers (S4).
 *
 * `required_plugins` names other plugins by manifest name; `capabilities` lists the
 * runtime features the plugin can cope with ("amr", "eb", "qed", ...).
 * The host refuses to start when a requirement is unmet, instead of failing
 * later at a random access.
 */
struct PluginManifest
{
    std::string name;
    std::string version;
    PluginRole role = PluginRole::Utility;
    std::vector<std::string> required_plugins;
    std::vector<std::string> capabilities;
};

/** Context of the schema phase: declare state, do not run physics. */
struct SchemaContext
{
    StateRegistry* state = nullptr;
};

/**
 * \brief Lifecycle of a plugin (S4).
 *
 * Handlers say *what runs each step*; this interface says *when a plugin is
 * brought up, torn down and kept consistent with the mesh*.  All stages are
 * optional; the core only calls the ones a plugin overrides.
 */
class IPluginLifecycle
{
public:
    virtual ~IPluginLifecycle () = default;

    [[nodiscard]] virtual PluginManifest manifest () const = 0;

    //! Declare state the core must own (fields, scalars, ...).
    virtual void on_schema (SchemaContext& /*ctx*/) {}
    //! Bring the plugin up. Called after the kernel has allocated levels.
    virtual void on_init (StageContext& /*ctx*/) {}
    //! A new mesh level exists (initial hierarchy or dynamic refinement).
    virtual void on_level_created (StageContext& /*ctx*/, int /*lev*/) {}
    //! An existing level was remade (regrid / load balance).
    virtual void on_regrid (StageContext& /*ctx*/) {}
    virtual void on_checkpoint (StageContext& /*ctx*/) {}
    virtual void on_restart (StageContext& /*ctx*/) {}
    //! Release resources; called once, after the last step.
    virtual void on_finalize (StageContext& /*ctx*/) {}
};

} // namespace warpx::runtime

#endif // WARPX_RUNTIME_PLUGINMANIFEST_H_
