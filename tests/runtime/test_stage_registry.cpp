/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * S2: stages are registry entries, not a closed enum.  A plugin can add a
 * stage between the standard ones without touching the core, and a handler
 * declared at an unregistered stage is rejected instead of silently never
 * running.
 */
#include "Runtime/HandlerSpec.H"
#include "Runtime/PluginCApi.H"
#include "Runtime/StagePoint.H"
#include "Runtime/StageRegistry.H"

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

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

class NamedHandler final : public warpx::runtime::IHandler
{
public:
    NamedHandler (std::string name, int stage, int sub = 0)
        : m_name(std::move(name)), m_stage(stage), m_sub(sub) {}

    [[nodiscard]] warpx::runtime::HandlerSpec spec () const override
    {
        return warpx::runtime::HandlerSpec{
            .name = m_name, .point = {m_stage, m_sub}, .priority = 0};
    }
    void apply (warpx::runtime::StageContext&) override {}

private:
    std::string m_name;
    int m_stage;
    int m_sub;
};

} // namespace

int main ()
{
    using namespace warpx::runtime;

    auto& registry = StageRegistry::instance();
    registry.reset();

    // --- standard stages -------------------------------------------------
    check(registry.find("push") == Stage::Push, "push ordinal");
    check(registry.find("field_solve") == Stage::FieldSolve, "field_solve ordinal");
    check(registry.find("diagnostics") == Stage::Diagnostics, "diagnostics ordinal");
    check(registry.name_of(Stage::AfterPush) == "after_push", "after_push name");
    check(registry.all().size() == 14, "fourteen standard stages");
    check(Stage::is_standard(Stage::Collisions), "standard stage predicate");
    check(!Stage::is_standard(Stage::Push + 1), "non-standard ordinal predicate");

    // --- insertion between standard stages -------------------------------
    const int chemistry = registry.register_stage("chemistry", "after_push",
                                                  StagePosition::After);
    check(chemistry > Stage::AfterPush && chemistry < Stage::BeforeCollisions,
          "chemistry inserted between after_push and before_collisions");
    check(registry.register_stage("chemistry", "after_push", StagePosition::After) == chemistry,
          "stage registration is idempotent by name");
    check(registry.name_of(chemistry) == "chemistry", "custom stage name");

    const int pre_chem = registry.register_stage("pre_chem", "chemistry",
                                                 StagePosition::Before);
    check(pre_chem < chemistry && pre_chem > Stage::AfterPush,
          "second registration lands in the remaining gap");

    bool threw = false;
    try {
        registry.register_stage("orphan", "no_such_stage");
    } catch (const std::runtime_error&) {
        threw = true;
    }
    check(threw, "unknown anchor is rejected");

    threw = false;
    try {
        registry.register_stage_at("chemistry", Stage::Push);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    check(threw, "re-registering a name at another ordinal is rejected");

    // --- ordering of handlers across standard and custom stages ----------
    PluginHost host;
    host.add(std::make_shared<NamedHandler>("solve", Stage::FieldSolve));
    host.add(std::make_shared<NamedHandler>("chem", chemistry));
    const auto ordered = host.ordered_specs();
    check(ordered.size() == 2, "two handlers registered");
    check(ordered[0].name == "chem", "custom stage sorts before field_solve");
    check(ordered[1].name == "solve", "field_solve handler is second");

    // --- unregistered stage is rejected ---------------------------------
    threw = false;
    try {
        host.add(std::make_shared<NamedHandler>("bogus", 12345));
    } catch (const std::runtime_error&) {
        threw = true;
    }
    check(threw, "handler at an unregistered stage is rejected");
    check(host.size() == 2, "rejected handler was not stored");

    // --- C ABI ordinals stay in sync ------------------------------------
    check(WARPX_STAGE_PUSH == Stage::Push, "C ABI push ordinal");
    check(WARPX_STAGE_FIELD_SOLVE == Stage::FieldSolve, "C ABI field_solve ordinal");
    check(WARPX_STAGE_FINALIZE == Stage::Finalize, "C ABI finalize ordinal");

    registry.reset();
    check(registry.find("chemistry") < 0, "reset drops custom stages");

    if (failures != 0) {
        std::cerr << failures << " check(s) failed\n";
        return 1;
    }
    return 0;
}
