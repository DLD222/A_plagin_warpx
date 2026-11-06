/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "CollisionSpeciesTable.H"

#include "Particles/MultiParticleContainer.H"

#include <stdexcept>

CollisionSpeciesTable::CollisionSpeciesTable (const MultiParticleContainer& mypc,
                                              NueiFunction calculate_nuei)
    : m_calculate_nuei(std::move(calculate_nuei))
{
    for (const auto& name : mypc.GetSpeciesNames()) {
        m_species.emplace(name, &mypc.GetParticleContainerFromName(name));
    }
}

WarpXParticleContainer*
CollisionSpeciesTable::get (const std::string& name) const
{
    const auto it = m_species.find(name);
    if (it == m_species.end()) {
        throw std::runtime_error("CollisionSpeciesTable: unknown species " + name);
    }
    return it->second;
}

std::vector<std::string>
CollisionSpeciesTable::names () const
{
    std::vector<std::string> out;
    out.reserve(m_species.size());
    for (const auto& [name, pc] : m_species) {
        amrex::ignore_unused(pc);
        out.push_back(name);
    }
    return out;
}
