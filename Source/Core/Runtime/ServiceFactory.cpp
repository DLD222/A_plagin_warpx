/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "Runtime/ServiceFactory.H"

#include "Runtime/HostServices.H"
#include "Runtime/IDiagnosticsService.H"
#include "Runtime/ILegacyRuntime.H"
#include "Runtime/IReducedDiagsService.H"
#include "Runtime/NullDiagnosticsService.H"
#include "Runtime/RuntimeData.H"

#include <stdexcept>

namespace warpx::runtime
{
namespace
{
struct Registry
{
    DiagnosticsServiceFactory diagnostics = nullptr;
    ReducedDiagsServiceFactory reduced_diags = nullptr;
    LegacyRuntimeDataFactory legacy_data = nullptr;
    LegacyHostServicesFactory legacy_host = nullptr;
};

Registry& registry ()
{
    static Registry r{};
    return r;
}
} // namespace

void
register_diagnostics_factory (DiagnosticsServiceFactory factory)
{
    registry().diagnostics = factory;
}

void
register_reduced_diags_factory (ReducedDiagsServiceFactory factory)
{
    registry().reduced_diags = factory;
}

void
register_legacy_runtime_factories (LegacyRuntimeDataFactory data_factory,
                                  LegacyHostServicesFactory host_factory)
{
    registry().legacy_data = data_factory;
    registry().legacy_host = host_factory;
}

void
clear_service_factories ()
{
    registry() = Registry{};
}

bool
has_diagnostics_factory ()
{
    return registry().diagnostics != nullptr;
}

bool
has_legacy_runtime_factories ()
{
    return registry().legacy_data != nullptr && registry().legacy_host != nullptr;
}

//! Looked up by the kernel; a build without the diagnostics plugin gets a no-op.
std::unique_ptr<IDiagnosticsService>
create_diagnostics_service (const RuntimeContext* ctx)
{
    if (registry().diagnostics == nullptr) {
        return std::make_unique<NullDiagnosticsService>();
    }
    return registry().diagnostics(ctx);
}

std::unique_ptr<IReducedDiagsService>
create_reduced_diags_service (const RuntimeContext* ctx)
{
    if (registry().reduced_diags == nullptr) {
        return std::make_unique<NullReducedDiagsService>();
    }
    return registry().reduced_diags(ctx);
}

//! Looked up by the diagnostics plugin on the legacy (no runtime) path.
std::unique_ptr<RuntimeData>
create_legacy_runtime_data ()
{
    if (registry().legacy_data == nullptr) {
        throw std::runtime_error(
            "Runtime: no legacy runtime provider is registered. Link the PicEm "
            "plugin, or pass a RuntimeContext instead of using the legacy path.");
    }
    return registry().legacy_data();
}

std::unique_ptr<HostServices>
create_legacy_host_services ()
{
    if (registry().legacy_host == nullptr) {
        throw std::runtime_error(
            "Runtime: no legacy host-services provider is registered. Link the "
            "PicEm plugin, or pass a RuntimeContext instead of using the legacy path.");
    }
    return registry().legacy_host();
}

} // namespace warpx::runtime
