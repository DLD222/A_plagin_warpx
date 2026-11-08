/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * S3b acceptance: a named scalar is declared by a plugin, owned by the core,
 * and reachable through the bus under the same declared-access rule as
 * fields.  Scalars are also ordering edges: a writer runs before a reader in
 * the same stage.
 */
#include "Runtime/HandlerSpec.H"
#include "Runtime/Runtime.H"
#include "Runtime/ScalarRegistry.H"
#include "Runtime/StagePoint.H"

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

template <typename Fn>
void check_throws (Fn&& fn, const std::string& what)
{
    bool threw = false;
    try {
        fn();
    } catch (const std::runtime_error&) {
        threw = true;
    }
    check(threw, what);
}

class NamedHandler final : public warpx::runtime::IHandler
{
public:
    explicit NamedHandler (warpx::runtime::HandlerSpec spec) : m_spec(std::move(spec)) {}
    [[nodiscard]] warpx::runtime::HandlerSpec spec () const override { return m_spec; }
    void apply (warpx::runtime::StageContext&) override {}

private:
    warpx::runtime::HandlerSpec m_spec;
};

} // namespace

int main (int argc, char* argv[])
{
    if (argc < 2) {
        std::cerr << "usage: test_scalar_state <inputs_file>\n";
        return 2;
    }

    using namespace warpx::runtime;

    char* rt_argv[3] = {argv[0], argv[1], nullptr};
    Runtime runtime(2, rt_argv);

    const std::string scalar_name = "plugin_state.temperature";
    ScalarSpec scalar;
    scalar.name = scalar_name;
    scalar.value = 300.0;
    scalar.unit = "K";
    runtime.state().request_scalar(scalar);

    check(runtime.state().scalars().has(scalar_name),
          "scalar request is recorded in the core registry");
    check(runtime.state().scalars().get(scalar_name) == 300.0,
          "scalar starts at the declared value");

    runtime.initialize();

    DataBus& bus = runtime.bus();

    // --- kernel path: unrestricted ---------------------------------------
    check(bus.scalar(scalar_name) == 300.0, "kernel path can read the scalar");

    // --- declared writer --------------------------------------------------
    const HandlerSpec writer{.name = "scalar_writer",
                             .point = {Stage::AfterPush, 0},
                             .priority = 0,
                             .writes_scalars = {scalar_name}};
    bus.set_active_handler(&writer);
    bus.set_scalar(scalar_name, 512.0);
    check(bus.scalar(scalar_name) == 512.0, "declared writer can read and write");
    check_throws([&] { (void) bus.scalar("plugin_state.undeclared"); },
                 "undeclared scalar read is rejected");
    bus.clear_active_handler();
    check(runtime.state().scalars().get(scalar_name) == 512.0,
          "write reached the core-owned registry");

    // --- declared reader cannot write ------------------------------------
    const HandlerSpec reader{.name = "scalar_reader",
                             .point = {Stage::AfterPush, 0},
                             .priority = 0,
                             .reads_scalars = {scalar_name}};
    bus.set_active_handler(&reader);
    check(bus.scalar(scalar_name) == 512.0, "declared reader can read");
    check_throws([&] { bus.set_scalar(scalar_name, 1.0); },
                 "reader cannot write a scalar it did not declare as written");
    bus.clear_active_handler();

    // --- ordering: scalar writer before scalar reader --------------------
    PluginHost host;
    host.add(std::make_shared<NamedHandler>(HandlerSpec{
        .name = "scalar_reader_2", .point = {Stage::FieldSolve, 0}, .priority = 0,
        .reads_scalars = {scalar_name}}));
    host.add(std::make_shared<NamedHandler>(HandlerSpec{
        .name = "scalar_writer_2", .point = {Stage::FieldSolve, 0}, .priority = 0,
        .writes_scalars = {scalar_name}}));
    const auto ordered = host.ordered_specs();
    check(ordered.size() == 2, "two scalar handlers registered");
    check(ordered[0].name == "scalar_writer_2",
          "scalar writer is ordered before the scalar reader");

    // --- two writers of the same scalar on one stage are rejected --------
    bool conflict_rejected = false;
    try {
        PluginHost conflict;
        conflict.add(std::make_shared<NamedHandler>(HandlerSpec{
            .name = "w1", .point = {Stage::Push, 0}, .priority = 0,
            .writes_scalars = {scalar_name}}));
        conflict.add(std::make_shared<NamedHandler>(HandlerSpec{
            .name = "w2", .point = {Stage::Push, 0}, .priority = 0,
            .writes_scalars = {scalar_name}}));
    } catch (const std::runtime_error&) {
        conflict_rejected = true;
    }
    check(conflict_rejected, "two writers of one scalar on a stage are rejected");

    runtime.finalize();

    if (failures != 0) {
        std::cerr << failures << " check(s) failed\n";
        return 1;
    }
    return 0;
}
