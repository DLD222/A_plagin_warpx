/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "Runtime/DataBus.H"

#include "Particles/WarpXParticleContainer.H"

#include <stdexcept>

namespace warpx::runtime
{

void
DataBus::declare (FieldSpec spec)
{
    m_registry->declare(spec);
}

const FieldSpec*
DataBus::spec (const std::string& name) const
{
    return m_registry->find(name);
}

bool
DataBus::has (const std::string& name, int lev) const
{
    return m_fields.has(name, lev);
}

bool
DataBus::has (const std::string& name, int dir, int lev) const
{
    return m_fields.has(name, ablastr::fields::Direction{dir}, lev);
}

amrex::MultiFab*
DataBus::field (const std::string& name, int lev)
{
    return m_fields.get(name, lev);
}

const amrex::MultiFab*
DataBus::field (const std::string& name, int lev) const
{
    return m_fields.get(name, lev);
}

amrex::MultiFab*
DataBus::field (const std::string& name, int dir, int lev)
{
    return m_fields.get(name, ablastr::fields::Direction{dir}, lev);
}

const amrex::MultiFab*
DataBus::field (const std::string& name, int dir, int lev) const
{
    return m_fields.get(name, ablastr::fields::Direction{dir}, lev);
}

amrex::MultiFab*
DataBus::alias (const std::string& dst, const std::string& src, int dir, int lev)
{
    if (m_fields.has(dst, ablastr::fields::Direction{dir}, lev)) {
        return m_fields.get(dst, ablastr::fields::Direction{dir}, lev);
    }
    if (!m_fields.has(src, ablastr::fields::Direction{dir}, lev)) {
        throw std::runtime_error("DataBus::alias: missing source field " + src);
    }
    return m_fields.alias_init(dst, src, ablastr::fields::Direction{dir}, lev);
}

void
DataBus::register_species (SpeciesSpec spec, WarpXParticleContainer* pc)
{
    m_species.insert_or_assign(spec.name, SpeciesHandle(spec.name, pc));
}

bool
DataBus::has_species (const std::string& name) const
{
    return m_species.find(name) != m_species.end();
}

SpeciesHandle
DataBus::species (const std::string& name) const
{
    const auto it = m_species.find(name);
    if (it == m_species.end()) {
        throw std::runtime_error("DataBus::species: unknown species " + name);
    }
    return it->second;
}

} // namespace warpx::runtime
