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
        auto& pc = mypc.GetParticleContainerFromName(name);
        m_species.emplace(name, &pc);
        warpx::runtime::SpeciesSpec spec;
        spec.species_name = name;
        spec.species_charge = pc.getCharge();
        spec.species_mass = pc.getMass();
        spec.count = static_cast<std::int64_t>(pc.TotalNumberOfParticles(false));
        m_handles.emplace(name, warpx::runtime::SpeciesHandle(spec));
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
    const auto hit = m_handles.find(name);
    return CollisionSpeciesView(hit->second, *it->second);
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
