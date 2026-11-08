/* Copyright 2020 David Grote
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause-LBNL
 */
#ifndef WARPX_PARTICLES_COLLISION_COLLISIONHANDLER_H_
#define WARPX_PARTICLES_COLLISION_COLLISIONHANDLER_H_

#include "Runtime/ICollisionHandler.H"

#include "CollisionBase.H"
#include "CollisionSpeciesTable.H"

namespace warpx::runtime { class HostServices; }

#include "Particles/Collision/CollisionSpeciesTable.H"

namespace warpx::runtime { class HostServices; class RuntimeData; class RuntimeConfig; }

#include <AMReX_REAL.H>
#include <AMReX_Vector.H>

#include <memory>
#include <string>

/* \brief CollisionHandler is a light weight class that contains the
 * list of collisions to be done.
 */
class CollisionHandler : public warpx::runtime::ICollisionHandler
{
public:
    CollisionHandler (const CollisionSpeciesTable& species,
                      const warpx::runtime::RuntimeData& data,
                      const warpx::runtime::RuntimeConfig& config,
                      warpx::runtime::HostServices* host);

    /* Allocate data needed for collision */
    void AllocData () override;

    [[nodiscard]] const CollisionSpeciesTable& speciesTable () const { return m_species; }



    /* Perform all of the collisions */
    void doCollisions (int step, amrex::Real cur_time, amrex::Real dt) override;

private:

    CollisionSpeciesTable m_species;
    const warpx::runtime::RuntimeData* m_data = nullptr;
    const warpx::runtime::RuntimeConfig* m_config = nullptr;
    warpx::runtime::HostServices* m_host = nullptr;

    amrex::Vector<std::string> collision_names;
    amrex::Vector<std::string> collision_types;
    amrex::Vector< std::unique_ptr<CollisionBase> > allcollisions;

    bool m_use_global_debye_length = false;

};

#endif // WARPX_PARTICLES_COLLISION_COLLISIONHANDLER_H_
