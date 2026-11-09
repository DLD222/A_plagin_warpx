/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_RUNTIME_IPARTICLECAPABILITY_H_
#define WARPX_RUNTIME_IPARTICLECAPABILITY_H_

namespace warpx::runtime
{

class DataBus;

/**
 * \brief The particle capability a kernel may offer.
 *
 * Kept apart from the kernel contract so that a kernel without particles (a
 * fluid or ten-moment kernel, for instance) does not carry particle methods at
 * all: it returns nullptr from ISimulationKernel::particle_capability() and the
 * runtime skips the particle setup.
 */
class IParticleCapability
{
public:
    virtual ~IParticleCapability () = default;

    //! Publish the species (and their containers) on the bus.
    virtual void register_species (DataBus& bus) = 0;

    //! Allocate the state the collision handlers declared in the schema.
    virtual void allocate_collision_data () = 0;
};

} // namespace warpx::runtime

#endif // WARPX_RUNTIME_IPARTICLECAPABILITY_H_
