/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "CollisionSpeciesTable.H"

#include "Modules/Particles/IParticleIterator.H"
#include "Runtime/IParticleContainer.H"
#include "Runtime/IParticleContainerAccess.H"

#include <stdexcept>

CollisionSpeciesTable::CollisionSpeciesTable (const warpx::runtime::IParticleContainerAccess& mypc,
                                              NueiFunction calculate_nuei)
    : m_calculate_nuei(std::move(calculate_nuei))
{
    for (const auto& name : mypc.GetSpeciesNames()) {
        m_species.emplace(name, &mypc.GetParticleContainerFromName(name));
    }
}

CollisionSpeciesView
CollisionSpeciesTable::get (const std::string& name) const
{
    return view(name);
}

CollisionSpeciesView
CollisionSpeciesTable::view (const std::string& name) const
{
    const auto it = m_species.find(name);
    if (it == m_species.end()) {
        throw std::runtime_error("CollisionSpeciesTable: unknown species " + name);
    }
    return CollisionSpeciesView(*it->second);
}
