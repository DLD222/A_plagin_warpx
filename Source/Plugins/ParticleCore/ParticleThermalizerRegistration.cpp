/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * The particle-core plugin's entry point: it registers the particle units it
 * implements with the core, so the kernel never names a concrete unit.
 */
#include "ParticleThermalizer.H"

#include "Runtime/IParticleThermalizer.H"

#include <memory>

//! Linker anchor: keeps this registration object in the aggregate archive.
extern "C" void warpx_particlecore_anchor (void) {}

namespace warpx::runtime
{
namespace
{
std::unique_ptr<IParticleThermalizer>
make_particle_thermalizer ()
{
    return std::make_unique<ParticleThermalizer>();
}

// Static registration: the core looks the factory up.
const bool particle_thermalizer_factory_registered = [] {
    register_particle_thermalizer_factory(&make_particle_thermalizer);
    return true;
}();
} // namespace
} // namespace warpx::runtime
