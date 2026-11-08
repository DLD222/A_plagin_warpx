/* Copyright 2022 David Grote
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause-LBNL
 */
#ifndef WARPX_PARTICLES_COLLISION_BACKGROUNSTOPPING_H_
#define WARPX_PARTICLES_COLLISION_BACKGROUNSTOPPING_H_

#include "Particles/Collision/CollisionSpeciesTable.H"
#include "Modules/Particles/IParticleIterator.H"
#include "Runtime/IParticleContainer.H"

#include <AMReX_Parser.H>

namespace warpx::runtime { class RuntimeData; class RuntimeConfig; }
#include "Particles/Collision/CollisionBase.H"

#include <AMReX_REAL.H>

#include <string>

enum class BackgroundStoppingType {
    INVALID,
    ELECTRONS,
    IONS,
};

class BackgroundStopping final
    : public CollisionBase
{
public:
    BackgroundStopping (const std::string& collision_name,
                        const CollisionSpeciesTable& species,
                        const warpx::runtime::RuntimeData& data,
                        const warpx::runtime::RuntimeConfig& config);

    ~BackgroundStopping () override = default;

    BackgroundStopping ( BackgroundStopping const &)             = delete;
    BackgroundStopping& operator= ( BackgroundStopping const & ) = delete;
    BackgroundStopping ( BackgroundStopping&& )                  = delete;
    BackgroundStopping& operator= ( BackgroundStopping&& )       = delete;

    /** Perform the stopping calculation
     *
     * @param cur_time Current time
     * @param dt Time step size
     * @param mypc Container of species involved
     *
     */
    void doCollisions (amrex::Real cur_time, amrex::Real dt) override;

    /** Perform the stopping calculation within a tile for stopping on electrons
     *
     * @param pti particle iterator
     * @param dt time step size
     * @param t current simulation time
     * @param species_mass mass of the active species
     * @param species_charge charge of the active species
     *
     */
    void doBackgroundStoppingOnElectronsWithinTile (warpx::runtime::IParticleIterator& pti, amrex::Real dt, amrex::Real t,
                                                    amrex::ParticleReal species_mass, amrex::ParticleReal species_charge);

    /** Perform the stopping calculation within a tile for stopping on ions
     *
     * @param pti particle iterator
     * @param dt time step size
     * @param t current simulation time
     * @param species_mass mass of the active species
     * @param species_charge charge of the active species
     *
     */
    void doBackgroundStoppingOnIonsWithinTile (warpx::runtime::IParticleIterator& pti, amrex::Real dt, amrex::Real t,
                                               amrex::ParticleReal species_mass, amrex::ParticleReal species_charge);

private:

    amrex::ParticleReal m_background_mass;
    amrex::ParticleReal m_background_charge_state;
    CollisionSpeciesTable m_species;
    const warpx::runtime::RuntimeData* m_data = nullptr;
    const warpx::runtime::RuntimeConfig* m_config = nullptr;

    BackgroundStoppingType m_background_type;

    amrex::Parser m_background_density_parser;
    amrex::Parser m_background_temperature_parser;

    amrex::ParserExecutor<4> m_background_density_func;
    amrex::ParserExecutor<4> m_background_temperature_func;

};

#endif // WARPX_PARTICLES_COLLISION_BACKGROUNSTOPPING_H_
