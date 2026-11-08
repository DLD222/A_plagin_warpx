/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "Runtime/DataBus.H"

#include "Runtime/KeyMatch.H"

#include <stdexcept>

namespace warpx::runtime
{

void
DataBus::require_field_access (const std::string& name, bool write) const
{
    if (m_active == nullptr) { return; }   // kernel / legacy path
    const bool declared = write
        ? key_declared(name, m_active->writes_fields)
        : (key_declared(name, m_active->reads_fields) ||
           key_declared(name, m_active->writes_fields));
    if (!declared) {
        throw std::runtime_error(
            "DataBus: handler '" + m_active->name + "' accessed field '" + name +
            "' without declaring it as " + (write ? "written" : "read") +
            ". Add it to HandlerSpec." + (write ? "writes_fields" : "reads_fields") + ".");
    }
}

void
DataBus::require_species_access (const std::string& name, bool write) const
{
    if (m_active == nullptr) { return; }
    const bool declared = write
        ? key_declared(name, m_active->writes_species)
        : (key_declared(name, m_active->reads_species) ||
           key_declared(name, m_active->writes_species));
    if (!declared) {
        throw std::runtime_error(
            "DataBus: handler '" + m_active->name + "' accessed species '" + name +
            "' without declaring it. Add it to HandlerSpec.");
    }
}

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
    require_field_access(name, /*write=*/true);
    return m_fields.get(name, lev);
}

const amrex::MultiFab*
DataBus::field (const std::string& name, int lev) const
{
    require_field_access(name, /*write=*/false);
    return m_fields.get(name, lev);
}

amrex::MultiFab*
DataBus::field (const std::string& name, int dir, int lev)
{
    require_field_access(name, /*write=*/true);
    return m_fields.get(name, ablastr::fields::Direction{dir}, lev);
}

const amrex::MultiFab*
DataBus::field (const std::string& name, int dir, int lev) const
{
    require_field_access(name, /*write=*/false);
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
    require_field_access(dst, /*write=*/true);
    require_field_access(src, /*write=*/false);
    return m_fields.alias_init(dst, src, ablastr::fields::Direction{dir}, lev);
}

void
DataBus::register_species (const SpeciesSpec& spec)
{
    m_species.register_species(spec);
}

bool
DataBus::has_species (const std::string& name) const
{
    return m_species.contains(name);
}

SpeciesHandle
DataBus::species (const std::string& name) const
{
    require_species_access(name, /*write=*/false);
    return m_species.get(name);
}

} // namespace warpx::runtime
