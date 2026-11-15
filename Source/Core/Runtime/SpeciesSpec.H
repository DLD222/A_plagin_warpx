/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_RUNTIME_SPECIESSPEC_H_
#define WARPX_RUNTIME_SPECIESSPEC_H_

#include "Runtime/IParticleSpecies.H"

#include <AMReX_REAL.H>

#include <cstdint>
#include <string>


class CollisionSpeciesTable;

namespace warpx::runtime
{

struct SpeciesSpec : public IParticleSpecies
{
    std::string species_name;
    amrex::Real species_charge = 0.0;
    amrex::Real species_mass = 0.0;
    std::int64_t count = 0;

    [[nodiscard]] const std::string& name () const override { return species_name; }
    [[nodiscard]] std::int64_t numberOfParticles () const override { return count; }
    [[nodiscard]] amrex::Real charge () const override { return species_charge; }
    [[nodiscard]] amrex::Real mass () const override { return species_mass; }
};

/**
 * \brief Opaque species description for handlers (A3.3b2).
 *
 * The underlying WarpXParticleContainer is hidden; a handler sees the species' name, charge and
 * mass, which are static for the run.
 *
 * `numberOfParticles()` is the count at the moment the kernel registered the species (before the
 * first step) -- a snapshot, not a live view.  A handler that needs the current count has to ask
 * the data service (the particle container); this accessor is named `registeredNumberOfParticles`
 * to stop it from being mistaken for a live count.
 */
class SpeciesHandle
{
public:
    SpeciesHandle () = default;
    explicit SpeciesHandle (SpeciesSpec spec) : m_spec(std::move(spec)) {}

    [[nodiscard]] const std::string& name () const { return m_spec.name(); }
    //! Particle count captured at registration time -- NOT the current one.
    [[nodiscard]] std::int64_t registeredNumberOfParticles () const {
        return m_spec.numberOfParticles();
    }
    [[nodiscard]] amrex::Real charge () const { return m_spec.charge(); }
    [[nodiscard]] amrex::Real mass () const { return m_spec.mass(); }

private:
    SpeciesSpec m_spec;
};

} // namespace warpx::runtime

#endif // WARPX_RUNTIME_SPECIESSPEC_H_
