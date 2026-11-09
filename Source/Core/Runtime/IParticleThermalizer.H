/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_RUNTIME_IPARTICLETHERMALIZER_H_
#define WARPX_RUNTIME_IPARTICLETHERMALIZER_H_

#include "Runtime/ParticleContext.H"

#include <memory>

namespace warpx::runtime
{

/**
 * \brief Particle thermalizer plugin contract (A7.4).
 *
 * The thermalizer is a particle operation: it reads a parameter block from the
 * inputs and resamples the momentum of particles that enter a region.  Core
 * only names the operation; it does not know which implementation performs it,
 * nor does it name any concrete particle container.
 *
 * The kernel obtains the implementation through the factory below.  A build
 * without the plugin returns `nullptr`, and the kernel skips the operation
 * (with an actionable error if the input actually requested it).
 */
class IParticleThermalizer
{
public:
    virtual ~IParticleThermalizer () = default;

    //! True when the inputs enabled the thermalizer.
    [[nodiscard]] virtual bool defined () const = 0;

    //! Apply the thermalizer to the species reached through `particles`.
    virtual void apply (ParticleContext const& particles) const = 0;
};

/** \brief Factory provided by the particle plugin and looked up by the core. */
using ParticleThermalizerFactory = std::unique_ptr<IParticleThermalizer> (*) ();

void register_particle_thermalizer_factory (ParticleThermalizerFactory factory);

//! Null when no particle plugin is linked.
[[nodiscard]] std::unique_ptr<IParticleThermalizer> create_particle_thermalizer ();

[[nodiscard]] bool has_particle_thermalizer_factory ();

} // namespace warpx::runtime

#endif // WARPX_RUNTIME_IPARTICLETHERMALIZER_H_
