/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_RUNTIME_SERVICEFACTORY_H_
#define WARPX_RUNTIME_SERVICEFACTORY_H_

#include <memory>

namespace warpx::runtime
{

class HostServices;
class IDiagnosticsService;
class IReducedDiagsService;
class RuntimeContext;
class RuntimeData;

/**
 * \brief Factories that plugins provide to the core, looked up by the core.
 *
 * Same pattern as the kernel factory: a plugin *registers* its implementation
 * at static-initialization time, the core looks it up when it needs it.  This
 * is what lets a plugin be omitted from a build without the remaining plugins
 * failing to link against each other.
 */
using DiagnosticsServiceFactory = std::unique_ptr<IDiagnosticsService> (*) (const RuntimeContext*);
using ReducedDiagsServiceFactory = std::unique_ptr<IReducedDiagsService> (*) (const RuntimeContext*);
using LegacyRuntimeDataFactory = std::unique_ptr<RuntimeData> (*) ();
using LegacyHostServicesFactory = std::unique_ptr<HostServices> (*) ();

void register_diagnostics_factory (DiagnosticsServiceFactory factory);
void register_reduced_diags_factory (ReducedDiagsServiceFactory factory);
void register_legacy_runtime_factories (LegacyRuntimeDataFactory data_factory,
                                        LegacyHostServicesFactory host_factory);

//! Drop every registration (tests only).
void clear_service_factories ();

[[nodiscard]] bool has_diagnostics_factory ();
[[nodiscard]] bool has_legacy_runtime_factories ();

} // namespace warpx::runtime

#endif // WARPX_RUNTIME_SERVICEFACTORY_H_
