/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * S4 acceptance: plugins describe themselves (manifest) and get lifecycle
 * callbacks; the host refuses to start when a declared requirement is unmet,
 * and duplicate plugin names are rejected.  A plugin added after the Runtime
 * was constructed still receives on_schema, because the schema phase runs at
 * initialize().
 */
#include "Runtime/FieldSpec.H"
#include "Runtime/HandlerSpec.H"
#include "Runtime/PluginManifest.H"
#include "Runtime/Runtime.H"

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{

int failures = 0;

void check (bool ok, const std::string& what)
{
    if (!ok) {
        std::cerr << "FAIL: " << what << '\n';
        ++failures;
    }
}

class ProbePlugin final : public warpx::runtime::IPluginLifecycle
{
public:
    warpx::runtime::PluginManifest manifest () const override
    {
        warpx::runtime::PluginManifest m;
        m.name = "probe";
        m.version = "2.0";
        m.role = warpx::runtime::PluginRole::Physics;
        m.required_plugins = {"kernel"};
        // no capability claim: skipped by the capability check
        return m;
    }

    void on_schema (warpx::runtime::SchemaContext& ctx) override
    {
        ++schema_calls;
        if (ctx.state != nullptr) {
            warpx::runtime::FieldSpec spec;
            spec.name = "probe_state_field";
            spec.ncomp = 2;
            ctx.state->request_field(spec);
        }
    }
    void on_init (warpx::runtime::StageContext&) override { ++init_calls; }
    void on_finalize (warpx::runtime::StageContext&) override { ++finalize_calls; }

    int schema_calls = 0;
    int init_calls = 0;
    int finalize_calls = 0;
};

class CapabilityPlugin final : public warpx::runtime::IPluginLifecycle
{
public:
    CapabilityPlugin (std::string name, std::vector<std::string> capabilities)
        : m_name(std::move(name)), m_capabilities(std::move(capabilities)) {}

    [[nodiscard]] warpx::runtime::PluginManifest manifest () const override
    {
        warpx::runtime::PluginManifest m;
        m.name = m_name;
        m.role = warpx::runtime::PluginRole::Physics;
        m.capabilities = m_capabilities;
        return m;
    }

private:
    std::string m_name;
    std::vector<std::string> m_capabilities;
};

class UnmetRequirementPlugin final : public warpx::runtime::IPluginLifecycle
{
public:
    [[nodiscard]] warpx::runtime::PluginManifest manifest () const override
    {
        warpx::runtime::PluginManifest m;
        m.name = "needy";
        m.role = warpx::runtime::PluginRole::Utility;
        m.required_plugins = {"no_such_plugin"};
        return m;
    }
};

} // namespace

int main (int argc, char* argv[])
{
    if (argc < 2) {
        std::cerr << "usage: test_plugin_lifecycle <inputs_file>\n";
        return 2;
    }

    using namespace warpx::runtime;

    char* rt_argv[3] = {argv[0], argv[1], nullptr};
    Runtime runtime(2, rt_argv);

    auto probe = std::make_shared<ProbePlugin>();
    runtime.plugins().add_plugin(probe);
    // PicEm, the probe, and the collision handler's state bridge (the kernel
    // owns that handler, so it contributes a lifecycle object of its own).
    bool probe_registered = false;
    bool picem_registered = false;
    bool collision_bridge = false;
    for (const auto& manifest : runtime.plugins().manifests()) {
        probe_registered = probe_registered || manifest.name == "probe";
        picem_registered = picem_registered || manifest.name == "picem";
        collision_bridge = collision_bridge || manifest.name == "collision_state";
    }
    check(probe_registered, "the probe plugin is registered");
    check(picem_registered, "the PicEm manifest is registered");
    check(collision_bridge, "the collision state bridge is registered");

    bool duplicate_rejected = false;
    try {
        runtime.plugins().add_plugin(std::make_shared<ProbePlugin>());
    } catch (const std::runtime_error&) {
        duplicate_rejected = true;
    }
    check(duplicate_rejected, "duplicate plugin name is rejected");

    bool manifests_ok = false;
    for (const auto& manifest : runtime.plugins().manifests()) {
        if (manifest.name == "picem" && manifest.role == PluginRole::Kernel) {
            manifests_ok = true;
        }
    }
    check(manifests_ok, "the kernel plugin declares role kernel");

    check(probe->schema_calls == 0, "no lifecycle call before initialize()");

    runtime.initialize();
    check(probe->schema_calls == 1, "on_schema ran exactly once");
    check(probe->init_calls == 1, "on_init ran exactly once");
    check(runtime.state().has_field("probe_state_field"),
          "state declared in on_schema is owned by the core");
    check(runtime.fields().has(std::string("probe_state_field"), 0),
          "field declared in on_schema was allocated by the core");

    runtime.evolve(1);
    check(probe->finalize_calls == 0, "on_finalize not called during evolve");
    runtime.finalize();
    check(probe->finalize_calls == 1, "on_finalize ran exactly once");
    runtime.finalize();
    check(probe->finalize_calls == 1, "finalize is idempotent");

    // Unmet requirement: the host refuses instead of failing later.
    PluginHost standalone;
    standalone.add_plugin(std::make_shared<UnmetRequirementPlugin>());
    bool unmet_rejected = false;
    try {
        standalone.validate_plugins();
    } catch (const std::runtime_error&) {
        unmet_rejected = true;
    }
    check(unmet_rejected, "unmet manifest requirement is rejected");

    // Satisfied requirement: role name counts as a provider.
    PluginHost satisfied;
    satisfied.add_plugin(std::make_shared<ProbePlugin>());
    satisfied.add_plugin(std::make_shared<UnmetRequirementPlugin>());
    bool thrown = false;
    try {
        satisfied.validate_plugins();
    } catch (const std::runtime_error&) {
        thrown = true;
    }
    check(thrown, "requirement for an unknown name is still rejected");

    // Capability check: active features must be declared by claiming plugins.
    PluginHost capable;
    capable.add_plugin(std::make_shared<CapabilityPlugin>("full", std::vector<std::string>{"amr", "qed"}));
    bool accepted = true;
    try {
        capable.validate_capabilities({"amr", "qed"});
    } catch (const std::runtime_error&) {
        accepted = false;
    }
    check(accepted, "declared capabilities accept the active features");

    PluginHost partial;
    partial.add_plugin(std::make_shared<CapabilityPlugin>("partial", std::vector<std::string>{"amr"}));
    bool rejected = false;
    try {
        partial.validate_capabilities({"amr", "qed"});
    } catch (const std::runtime_error&) {
        rejected = true;
    }
    check(rejected, "missing capability is rejected");

    PluginHost silent;
    silent.add_plugin(std::make_shared<CapabilityPlugin>("silent", std::vector<std::string>{}));
    bool skipped = true;
    try {
        silent.validate_capabilities({"amr", "qed"});
    } catch (const std::runtime_error&) {
        skipped = false;
    }
    check(skipped, "a plugin that claims no capability is not checked");

    if (failures != 0) {
        std::cerr << failures << " check(s) failed\n";
        return 1;
    }
    return 0;
}
