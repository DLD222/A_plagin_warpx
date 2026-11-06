/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_COLLISION_SPECIESTABLE_H_
#define WARPX_COLLISION_SPECIESTABLE_H_

#include <functional>
#include <map>
#include <string>
#include <vector>

class MultiParticleContainer;
class WarpXParticleContainer;

/**
 * \brief Collision-internal species lookup (A3.3b8).
 *
 * Centralizes the `GetParticleContainerFromName` call sites and the few global
 * collision operations (currently CalculateNuei). Container pointers stay
 * internal to the collision package; they are not part of IHandler.
 */
class CollisionSpeciesTable
{
public:
    using NueiFunction = std::function<void(const std::string&)>;

    explicit CollisionSpeciesTable (const MultiParticleContainer& mypc,
                                    NueiFunction calculate_nuei = {});

    [[nodiscard]] WarpXParticleContainer* get (const std::string& name) const;
    [[nodiscard]] int size () const { return static_cast<int>(m_species.size()); }
    [[nodiscard]] std::vector<std::string> names () const;

    void calculateNuei (const std::string& name) const
    {
        if (m_calculate_nuei) { m_calculate_nuei(name); }
    }

private:
    std::map<std::string, WarpXParticleContainer*> m_species;
    NueiFunction m_calculate_nuei;
};

#endif // WARPX_COLLISION_SPECIESTABLE_H_
