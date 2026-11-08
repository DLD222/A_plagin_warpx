/* Copyright 2025 David Grote
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause-LBNL
 */
#ifndef WARPX_PARTICLES_COLLISION_INVERSEBREMSSTRAHLUNG_H_
#define WARPX_PARTICLES_COLLISION_INVERSEBREMSSTRAHLUNG_H_

#include "Collision/CollisionSpeciesTable.H"
#include "Modules/Particles/IParticleIterator.H"
#include "Runtime/IParticleContainer.H"
#include "Modules/Particles/IParticleIterator.H"
#include "Runtime/IParticleContainer.H"

namespace warpx::runtime { class RuntimeData; class RuntimeConfig; }
#include "Collision/CollisionBase.H"
#include "Collision/ScatteringProcess.H"

#include <AMReX_Parser.H>
#include <AMReX_REAL.H>
#include <AMReX_Vector.H>
#include <AMReX_GpuContainers.H>

#include <memory>
#include <string>

class InverseBremsstrahlung final
    : public CollisionBase
{

    using ParticleTileType = warpx::runtime::IParticleContainer::ParticleTileType;
    using ParticleTileDataType = ParticleTileType::ParticleTileDataType;
    using ParticleBins = amrex::DenseBins<ParticleTileDataType>;
    using index_type = ParticleBins::index_type;

public:
    InverseBremsstrahlung (std::string const& collision_name,
                          const CollisionSpeciesTable& species,
                          const warpx::runtime::RuntimeData& data,
                          const warpx::runtime::RuntimeConfig& config);

    ~InverseBremsstrahlung () override = default;

    InverseBremsstrahlung ( InverseBremsstrahlung const &)             = delete;
    InverseBremsstrahlung& operator= ( InverseBremsstrahlung const & ) = delete;
    InverseBremsstrahlung ( InverseBremsstrahlung&& )                  = delete;
    InverseBremsstrahlung& operator= ( InverseBremsstrahlung&& )       = delete;

    /** \brief Perform the collisions
     *
     * @param cur_time Current time
     * @param dt Time step size
     * @param mypc Container of species involved
     *
     */
    void doCollisions (amrex::Real cur_time, amrex::Real dt) override;

    /** \brief Perform inverse Bremsstrahlung within the tile
     *
     * @param dt time step size
     * @param lev refinement lev
     * @param mfi the MFIter for the tile
     * @param photons the photons particle container
     * @param electrons the electrons particle container
     *
     */
    void doInverseBremsstrahlungWithinTile (amrex::Real dt, int lev, amrex::MFIter const& mfi,
                                            warpx::runtime::IParticleContainer& photons,
                                            warpx::runtime::IParticleContainer& electrons) const;

private:

    CollisionSpeciesTable m_species;
    const warpx::runtime::RuntimeData* m_data = nullptr;
    const warpx::runtime::RuntimeConfig* m_config = nullptr;

    amrex::ParticleReal m_energy_fraction = 0.05;

    ParticleShufflingMethod m_shuffling_method;
    int m_modulus_rounds;

};

#endif // WARPX_PARTICLES_COLLISION_INVERSEBREMSSTRAHLUNG_H
