/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * S5 acceptance (first slice): while a handler runs, the bus only allows the
 * field/species accesses that handler declared.  A plugin that forgets to
 * declare a write fails at the access point with an actionable message,
 * instead of silently corrupting another plugin's state.
 */
#include "Runtime/DataBus.H"
#include "Runtime/FieldSpec.H"
#include "Runtime/HandlerSpec.H"
#include "Runtime/Runtime.H"
#include "Runtime/StagePoint.H"

#include <iostream>
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

} // namespace

int main (int argc, char* argv[])
{
    if (argc < 2) {
        std::cerr << "usage: test_bus_access <inputs_file>\n";
        return 2;
    }

    using namespace warpx::runtime;

    char* rt_argv[3] = {argv[0], argv[1], nullptr};
    Runtime runtime(2, rt_argv);

    FieldSpec spec;
    spec.name = "plugin_probe_field";
    spec.ncomp = 1;
    runtime.state().request_field(spec);
    runtime.initialize();

    DataBus& bus = runtime.bus();
    const std::string probe = "plugin_probe_field";

    // --- handler that declared the write ---------------------------------
    const HandlerSpec writer{
        .name = "writer", .point = {Stage::Push, 0}, .writes_fields = {probe}};
    bus.set_active_handler(&writer);
    check(bus.field(probe, 0) != nullptr, "declared write is allowed");
    check_throws([&] { (void) bus.field("some_undeclared_field", 0); },
                 "undeclared field access is rejected");
    bus.clear_active_handler();

    // --- handler that declared only a read -------------------------------
    const HandlerSpec reader{
        .name = "reader", .point = {Stage::Push, 0}, .reads_fields = {probe}};
    bus.set_active_handler(&reader);
    const DataBus& const_bus = bus;
    check(const_bus.field(probe, 0) != nullptr, "declared read is allowed");
    check_throws([&] { (void) bus.field(probe, 0); },
                 "write through a read-only declaration is rejected");
    bus.clear_active_handler();

    // --- no active handler: unrestricted (kernel / legacy path) ----------
    check(bus.field(probe, 0) != nullptr, "kernel path is unrestricted");

    // --- aggregate name declared, component/qualified key requested -------
    // "plugin_probe" must cover "plugin_probe_field" (prefix + separator),
    // the same rule the plugin host uses for ordering.
    const HandlerSpec aggregate{
        .name = "aggregate", .point = {Stage::Push, 0}, .writes_fields = {"plugin_probe"}};
    bus.set_active_handler(&aggregate);
    check(bus.field(probe, 0) != nullptr,
          "aggregate declaration covers a qualified key");
    bus.clear_active_handler();

    runtime.finalize();

    if (failures != 0) {
        std::cerr << failures << " check(s) failed\n";
        return 1;
    }
    return 0;
}
