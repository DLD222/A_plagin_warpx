/* Copyright 2026 Andrew Myers
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause-LBNL
 */
#ifndef WARPX_PARTICLECORE_PARTICLETHERMALIZER_H_
#define WARPX_PARTICLECORE_PARTICLETHERMALIZER_H_

#include "Runtime/IParticleThermalizer.H"

#include <AMReX_Array.H>
#include <AMReX_REAL.H>

#include <string>
#include <vector>

namespace warpx::runtime { class IParticleContainer; }

/**
 * \brief Particle thermalizer (A7.4 unit), implementing the core
 * `IParticleThermalizer` contract.
 *
 * The unit names no kernel type, no concrete particle container and no
 * multi-species container: it reaches species through the generic
 * `ParticleContext` the kernel hands it and iterates the particle tiles with
 * the AMReX tile iterator.  This is what lets it live in its own plugin layer.
 */
class ParticleThermalizer final : public warpx::runtime::IParticleThermalizer
{
public:

    // Construct and read parameters from inputs under the `particle_thermalizer` block.
    ParticleThermalizer ();

    // Returns true if the particle_thermalizer parameters were provided
    // and the thermalizer is therefore on.
    [[nodiscard]] bool defined () const override;

    // Apply the thermalizer to the species reached through `particles`.
    void apply (warpx::runtime::ParticleContext const& particles) const override;

private:

    // Per-species work; operates on the generic core container contract.
    void applyToSpecies (warpx::runtime::IParticleContainer& pc) const;

    bool m_defined{false};

    // The normal direction as an integer USING THE AMREX CONVENTION (0, 1, or 2)
    // in 1D, "z" is 0
    // in 2D, "x" is 0, "z" is 1
    // in 3D, "x" is 0, "y" is 1, "z" is 2
    int m_normal{-1};

    // The start and end positions of the thermalizer region along the normal direction
    amrex::Real m_start;
    amrex::Real m_end;

    // The threshold for each momentum component above which particles are thermalized in normalized units (i.e. u = gamma*beta)
    amrex::Array<amrex::Real, 3> m_momentum_threshold;

    // dimensionless temperature theta = kB*T/(m*c^2) used to sample thermal momentum distribution
    amrex::Real m_theta;

    // If non-empty, only these species are thermalized; if empty, all species are thermalized.
    std::vector<std::string> m_species_names;
};

#endif // WARPX_PARTICLECORE_PARTICLETHERMALIZER_H_
