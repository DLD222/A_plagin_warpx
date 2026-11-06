/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_COLLISION_SPECIESTABLE_H_
#define WARPX_COLLISION_SPECIESTABLE_H_

#include <map>
#include <string>
#include <vector>

class MultiParticleContainer;
class WarpXParticleContainer;

/**
 * \brief Collision-internal species lookup (A3.3b8).
 *
 * Centralizes the 39 `GetParticleContainerFromName` call sites. The container
 * pointer stays internal to the collision package; it is not part of IHandler.
 */
class CollisionSpeciesTable
{
public:
    explicit CollisionSpeciesTable (const MultiParticleContainer& mypc);

    [[nodiscard]] WarpXParticleContainer* get (const std::string& name) const;
    [[nodiscard]] int size () const { return static_cast<int>(m_species.size()); }
    [[nodiscard]] std::vector<std::string> names () const;

private:
    std::map<std::string, WarpXParticleContainer*> m_species;
};

#endif // WARPX_COLLISION_SPECIESTABLE_H_
