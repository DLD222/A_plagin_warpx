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
#include "Runtime/IParticleThermalizer.H"
#include "Runtime/IDiagnosticsService.H"
#include "Runtime/ILegacyRuntime.H"
#include "Runtime/IReducedDiagsService.H"
#include "Runtime/NullDiagnosticsService.H"
#include "Runtime/RuntimeData.H"

#include <stdexcept>
#include <string>

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
    ParticleThermalizerFactory particle_thermalizer = nullptr;
    ParticleSystemFactory particle_system = nullptr;
    //! Set when two plugins register the same kind of service (finding F-09).
    std::string conflict;
};

/** Assign one factory slot, recording (not silently accepting) a second, different registration.
 *
 * The registry has a single slot per service kind, so two plugins of the same kind cannot
 * coexist.  Before this, the last registration simply won and which one that was depended on
 * static-initialisation and link order - the same input deck could take a different code path
 * depending on how the binary was linked.  A conflict is now recorded here and reported by the
 * create_*() entry point, which runs at a point where aborting is safe.
 */
template <class F>
void assign_factory (F& slot, F factory, char const* what, std::string& conflict)
{
    if (slot != nullptr && slot != factory && conflict.empty()) {
        conflict = std::string("two plugins registered a ") + what
                 + " factory. The service registry holds one slot per kind, so which one takes "
                   "effect would depend on static-initialisation and link order (CODE_REVIEW "
                   "F-09); make the build link only one of them, or give the second one a "
                   "distinct service kind.";
    }
    slot = factory;
}

void require_no_conflict (std::string const& conflict)
{
    if (!conflict.empty()) { throw std::runtime_error(conflict); }
}

Registry& registry ()
{
    static Registry r{};
    return r;
}
} // namespace

void
register_diagnostics_factory (DiagnosticsServiceFactory factory)
{
    assign_factory(registry().diagnostics, factory, "diagnostics", registry().conflict);
}

void
register_reduced_diags_factory (ReducedDiagsServiceFactory factory)
{
    assign_factory(registry().reduced_diags, factory, "reduced-diagnostics", registry().conflict);
}

void
register_legacy_runtime_factories (LegacyRuntimeDataFactory data_factory,
                                  LegacyHostServicesFactory host_factory)
{
    assign_factory(registry().legacy_data, data_factory, "legacy-runtime-data", registry().conflict);
    assign_factory(registry().legacy_host, host_factory, "legacy-host-services", registry().conflict);
}

void
register_collision_handler_factory (CollisionHandlerFactory factory)
{
    assign_factory(registry().collision, factory, "collision-handler", registry().conflict);
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
    require_no_conflict(registry().conflict);
    if (registry().collision == nullptr) { return nullptr; }
    return registry().collision(particles, data, config, host);
}

void
register_electrostatic_solver_factory (ElectrostaticSolverFactory factory)
{
    assign_factory(registry().electrostatic, factory, "electrostatic-solver", registry().conflict);
}

bool
has_electrostatic_solver_factory ()
{
    return registry().electrostatic != nullptr;
}

std::unique_ptr<IElectrostaticSolver>
create_electrostatic_solver (int algorithm, int max_level, const RuntimeContext* context)
{
    require_no_conflict(registry().conflict);
    if (registry().electrostatic == nullptr) { return nullptr; }
    return registry().electrostatic(algorithm, max_level, context);
}

void
register_particle_thermalizer_factory (ParticleThermalizerFactory factory)
{
    assign_factory(registry().particle_thermalizer, factory, "particle-thermalizer", registry().conflict);
}

bool
has_particle_thermalizer_factory ()
{
    return registry().particle_thermalizer != nullptr;
}

void
register_particle_system_factory (ParticleSystemFactory factory)
{
    assign_factory(registry().particle_system, factory, "particle-system", registry().conflict);
}

std::unique_ptr<IParticleSystem>
create_particle_system (void* owner)
{
    require_no_conflict(registry().conflict);
    if (registry().particle_system == nullptr) { return nullptr; }
    return registry().particle_system(owner);
}

std::unique_ptr<IParticleThermalizer>
create_particle_thermalizer ()
{
    require_no_conflict(registry().conflict);
    if (registry().particle_thermalizer == nullptr) { return nullptr; }
    return registry().particle_thermalizer();
}

void
register_finite_difference_solver_factory (FiniteDifferenceSolverFactory factory)
{
    assign_factory(registry().finite_difference, factory, "finite-difference-solver", registry().conflict);
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
    require_no_conflict(registry().conflict);
    if (registry().finite_difference == nullptr) { return nullptr; }
    return registry().finite_difference(algo, cell_size, grid_type, context);
}

void
register_macroscopic_properties_factory (MacroscopicPropertiesFactory factory)
{
    assign_factory(registry().macroscopic, factory, "macroscopic-properties", registry().conflict);
}

bool
has_macroscopic_properties_factory ()
{
    return registry().macroscopic != nullptr;
}

std::unique_ptr<IMacroscopicProperties>
create_macroscopic_properties ()
{
    require_no_conflict(registry().conflict);
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
    require_no_conflict(registry().conflict);
    if (registry().diagnostics == nullptr) {
        return std::make_unique<NullDiagnosticsService>();
    }
    return registry().diagnostics(ctx);
}

std::unique_ptr<IReducedDiagsService>
create_reduced_diags_service (const RuntimeContext* ctx)
{
    require_no_conflict(registry().conflict);
    if (registry().reduced_diags == nullptr) {
        return std::make_unique<NullReducedDiagsService>();
    }
    return registry().reduced_diags(ctx);
}

//! Looked up by the diagnostics plugin on the legacy (no runtime) path.
std::unique_ptr<RuntimeData>
create_legacy_runtime_data ()
{
    require_no_conflict(registry().conflict);
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
    require_no_conflict(registry().conflict);
    if (registry().legacy_host == nullptr) {
        throw std::runtime_error(
            "Runtime: no legacy host-services provider is registered. Link the "
            "PicEm plugin, or pass a RuntimeContext instead of using the legacy path.");
    }
    return registry().legacy_host();
}

} // namespace warpx::runtime
