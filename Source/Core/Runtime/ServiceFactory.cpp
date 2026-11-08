/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "Runtime/ServiceFactory.H"

#include "Runtime/HostServices.H"
#include "Runtime/ICollisionHandler.H"
#include "Runtime/IElectrostaticSolver.H"
#include "Runtime/IFiniteDifferenceSolver.H"
#include "Runtime/IMacroscopicProperties.H"
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
    CollisionHandlerFactory collision = nullptr;
    ElectrostaticSolverFactory electrostatic = nullptr;
    FiniteDifferenceSolverFactory finite_difference = nullptr;
    MacroscopicPropertiesFactory macroscopic = nullptr;
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
register_collision_handler_factory (CollisionHandlerFactory factory)
{
    registry().collision = factory;
}

bool
has_collision_handler_factory ()
{
    return registry().collision != nullptr;
}

std::unique_ptr<ICollisionHandler>
create_collision_handler (const IParticleContainerAccess& particles,
                          const RuntimeData& data, const RuntimeConfig& config,
                          HostServices* host)
{
    if (registry().collision == nullptr) { return nullptr; }
    return registry().collision(particles, data, config, host);
}

void
register_electrostatic_solver_factory (ElectrostaticSolverFactory factory)
{
    registry().electrostatic = factory;
}

bool
has_electrostatic_solver_factory ()
{
    return registry().electrostatic != nullptr;
}

std::unique_ptr<IElectrostaticSolver>
create_electrostatic_solver (int algorithm, int max_level, const RuntimeContext* context)
{
    if (registry().electrostatic == nullptr) { return nullptr; }
    return registry().electrostatic(algorithm, max_level, context);
}

void
register_finite_difference_solver_factory (FiniteDifferenceSolverFactory factory)
{
    registry().finite_difference = factory;
}

bool
has_finite_difference_solver_factory ()
{
    return registry().finite_difference != nullptr;
}

std::unique_ptr<IFiniteDifferenceSolver>
create_finite_difference_solver (ElectromagneticSolverAlgo algo,
                                 std::array<amrex::Real,3> cell_size,
                                 ablastr::utils::enums::GridType grid_type,
                                 const RuntimeContext* context)
{
    if (registry().finite_difference == nullptr) { return nullptr; }
    return registry().finite_difference(algo, cell_size, grid_type, context);
}

void
register_macroscopic_properties_factory (MacroscopicPropertiesFactory factory)
{
    registry().macroscopic = factory;
}

bool
has_macroscopic_properties_factory ()
{
    return registry().macroscopic != nullptr;
}

std::unique_ptr<IMacroscopicProperties>
create_macroscopic_properties ()
{
    if (registry().macroscopic == nullptr) { return nullptr; }
    return registry().macroscopic();
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

const RuntimeData&
legacy_runtime_data ()
{
    static const std::unique_ptr<RuntimeData> holder = create_legacy_runtime_data();
    return *holder;
}

HostServices&
legacy_host_services ()
{
    static const std::unique_ptr<HostServices> holder = create_legacy_host_services();
    return *holder;
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
