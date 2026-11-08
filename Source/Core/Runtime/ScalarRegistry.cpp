/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "Runtime/ScalarRegistry.H"

#include <algorithm>

namespace warpx::runtime
{

void
ScalarRegistry::declare (const ScalarSpec& spec)
{
    if (spec.name.empty()) {
        throw std::runtime_error("ScalarRegistry: scalar without a name");
    }
    const auto it = std::find_if(m_specs.begin(), m_specs.end(),
        [&spec](const ScalarSpec& existing) { return existing.name == spec.name; });
    if (it != m_specs.end()) {
        if (it->checkpoint != spec.checkpoint || it->unit != spec.unit) {
            throw std::runtime_error(
                "ScalarRegistry: conflicting declaration for '" + spec.name + "'");
        }
        return;
    }
    m_specs.push_back(spec);
    m_values.emplace(spec.name, spec.value);
}

bool
ScalarRegistry::has (const std::string& name) const
{
    return m_values.find(name) != m_values.end();
}

double
ScalarRegistry::get (const std::string& name) const
{
    const auto it = m_values.find(name);
    if (it == m_values.end()) {
        throw std::runtime_error("ScalarRegistry: undeclared scalar '" + name + "'");
    }
    return it->second;
}

void
ScalarRegistry::set (const std::string& name, double value)
{
    const auto it = m_values.find(name);
    if (it == m_values.end()) {
        throw std::runtime_error("ScalarRegistry: undeclared scalar '" + name + "'");
    }
    it->second = value;
}

void
ScalarRegistry::clear ()
{
    m_specs.clear();
    m_values.clear();
}

} // namespace warpx::runtime
