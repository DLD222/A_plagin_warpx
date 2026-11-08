/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * The collision plugin's entry point: it builds the concrete handler from the
 * generic inputs the core provides, so the kernel never names it.
 */
#include "Collision/CollisionHandler.H"
#include "Collision/CollisionSpeciesTable.H"

#include "Runtime/HostServices.H"
#include "Runtime/ICollisionHandler.H"
#include "Runtime/IParticleContainerAccess.H"
#include "Runtime/IParticleServices.H"
#include "Runtime/RuntimeConfig.H"
#include "Runtime/RuntimeData.H"
#include "Runtime/ServiceFactory.H"

#include <memory>
#include <string>

//! Linker anchor: keeps this registration object in the aggregate archive.
extern "C" void warpx_collision_anchor (void) {}

namespace warpx::runtime
{
namespace
{
std::unique_ptr<ICollisionHandler>
make_collision_handler (const IParticleContainerAccess& particles,
                        const RuntimeData& data, const RuntimeConfig& config,
                        HostServices* host)
{
    CollisionSpeciesTable::NueiFunction calculate_nuei{};
    if (host != nullptr) {
        calculate_nuei = [host] (const std::string& name) {
            host->particleServices().calculateNuei(name);
        };
    }
    CollisionSpeciesTable species(particles, calculate_nuei);
    return std::make_unique<CollisionHandler>(species, data, config, host);
}

// Static registration: the core looks the factory up.
const bool collision_factory_registered = [] {
    register_collision_handler_factory(&make_collision_handler);
    return true;
}();
} // namespace
} // namespace warpx::runtime
