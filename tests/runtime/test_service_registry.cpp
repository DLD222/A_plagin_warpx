/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * S4c acceptance: plugins provide their services through Core registries, so
 * the kernel and the diagnostics plugin no longer reference each other.
 * When a provider is missing the core degrades to a no-op service (diagnostics)
 * or fails with an actionable message (legacy runtime), never a link error.
 */
#include "Runtime/ICollisionHandler.H"
#include "Runtime/IDiagnosticsService.H"
#include "Runtime/HostServices.H"
#include "Runtime/ILegacyRuntime.H"
#include "Runtime/RuntimeData.H"
#include "Runtime/ServiceFactory.H"

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

} // namespace

int main ()
{
    using namespace warpx::runtime;

    check(has_diagnostics_factory(), "diagnostics plugin registered its factory");
    check(has_collision_handler_factory(), "collision plugin registered its factory");
    check(has_legacy_runtime_factories(), "kernel plugin registered the legacy factories");

    auto diagnostics = create_diagnostics_service(nullptr);
    check(diagnostics != nullptr, "diagnostics service can be created");
    check(diagnostics && diagnostics->GetTotalDiags() == 0,
          "service reports no diagnostics for a null context");

    // Simulate a build without the diagnostics plugin.
    clear_service_factories();
    check(!has_diagnostics_factory(), "factories can be cleared");
    check(!has_collision_handler_factory(), "collision factory can be cleared too");
    auto fallback = create_diagnostics_service(nullptr);
    check(fallback != nullptr, "missing diagnostics plugin yields a no-op service");
    check(fallback && fallback->GetTotalDiags() == 0, "no-op service reports nothing");

    bool legacy_throws = false;
    try {
        (void) create_legacy_runtime_data();
    } catch (const std::runtime_error&) {
        legacy_throws = true;
    }
    check(legacy_throws, "missing legacy provider fails with a message, not a crash");

    if (failures != 0) {
        std::cerr << failures << " check(s) failed\n";
        return 1;
    }
    return 0;
}
