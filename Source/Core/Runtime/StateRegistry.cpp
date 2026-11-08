/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "Runtime/StateRegistry.H"

#include <algorithm>
#include <stdexcept>

namespace warpx::runtime
{

void
StateRegistry::request_field (const FieldSpec& spec)
{
    if (spec.name.empty()) {
        throw std::runtime_error("StateRegistry: field without a name");
    }
    if (spec.ncomp <= 0) {
        throw std::runtime_error("StateRegistry: field '" + spec.name +
                                 "' must have at least one component");
    }

    const auto it = std::find_if(m_fields.begin(), m_fields.end(),
        [&spec](const FieldSpec& existing) { return existing.name == spec.name; });
    if (it == m_fields.end()) {
        m_fields.push_back(spec);
        return;
    }

    const FieldSpec& old = *it;
    if (old.ncomp != spec.ncomp || old.nodal != spec.nodal ||
        old.ngrow != spec.ngrow || old.checkpoint != spec.checkpoint ||
        old.moving_window_shift != spec.moving_window_shift ||
        old.unit != spec.unit || old.alias_of != spec.alias_of) {
        throw std::runtime_error(
            "StateRegistry: conflicting request for field '" + spec.name + "'");
    }
}

const FieldSpec*
StateRegistry::find_field (const std::string& name) const
{
    const auto it = std::find_if(m_fields.begin(), m_fields.end(),
        [&name](const FieldSpec& existing) { return existing.name == name; });
    return it == m_fields.end() ? nullptr : &(*it);
}

void
StateRegistry::clear ()
{
    m_fields.clear();
}

} // namespace warpx::runtime
