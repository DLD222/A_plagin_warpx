/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "Runtime/DataBus.H"

#include <stdexcept>

namespace warpx::runtime
{

void
DataBus::declare (FieldSpec spec)
{
    const auto it = m_specs.find(spec.name);
    if (it != m_specs.end()) {
        const auto& old = it->second;
        if (old.ncomp != spec.ncomp || old.nodal != spec.nodal || old.ngrow != spec.ngrow ||
            old.checkpoint != spec.checkpoint || old.alias_of != spec.alias_of) {
            throw std::runtime_error("DataBus: conflicting FieldSpec for " + spec.name);
        }
        return;
    }
    m_specs.emplace(spec.name, std::move(spec));
}

const FieldSpec*
DataBus::spec (const std::string& name) const
{
    const auto it = m_specs.find(name);
    return it == m_specs.end() ? nullptr : &it->second;
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

} // namespace warpx::runtime
