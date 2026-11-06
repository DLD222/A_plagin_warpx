/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_RUNTIME_SPECIESREGISTRY_H_
#define WARPX_RUNTIME_SPECIESREGISTRY_H_

#include "Runtime/SpeciesSpec.H"

#include <map>
#include <stdexcept>
#include <string>
#include <vector>

class WarpXParticleContainer;

namespace warpx::runtime
{

/** \brief String-keyed species registry (A4.2). */
class SpeciesRegistry
{
public:
    void register_species (const SpeciesSpec& spec, WarpXParticleContainer* pc)
    {
        const SpeciesHandle handle(spec.name, pc);
        auto [it, inserted] = m_species.emplace(spec.name, handle);
        if (inserted) { return; }
        if (it->second.charge() != handle.charge() || it->second.mass() != handle.mass()) {
            throw std::runtime_error(
                "SpeciesRegistry: conflicting metadata for species '" + spec.name + "'");
        }
    }

    [[nodiscard]] bool contains (const std::string& name) const
    {
        return m_species.find(name) != m_species.end();
    }

    [[nodiscard]] SpeciesHandle get (const std::string& name) const
    {
        const auto it = m_species.find(name);
        if (it == m_species.end()) {
            throw std::runtime_error("SpeciesRegistry: unknown species " + name);
        }
        return it->second;
    }

    [[nodiscard]] std::size_t size () const { return m_species.size(); }

    [[nodiscard]] std::vector<std::string> names () const
    {
        std::vector<std::string> out;
        out.reserve(m_species.size());
        for (const auto& [name, handle] : m_species) {
            (void)handle;
            out.push_back(name);
        }
        return out;
    }

private:
    std::map<std::string, SpeciesHandle> m_species;
};

} // namespace warpx::runtime

#endif // WARPX_RUNTIME_SPECIESREGISTRY_H_
