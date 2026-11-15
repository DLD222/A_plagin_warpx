/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "Runtime/ScalarRegistry.H"

#include <algorithm>

#include <ablastr/warn_manager/WarnManager.H>

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
        // Compare *every* field, including the initial value: two plugins declaring
        // "temperature" with 1.0 and 1e6 used to be silently resolved in favour of whichever ran
        // first (the value half of the declaration was not compared, while MaterialRegistry and
        // StateRegistry::request_field do compare it).
        if (it->checkpoint != spec.checkpoint || it->unit != spec.unit
            || it->value != spec.value) {
            throw std::runtime_error(
                "ScalarRegistry: conflicting declaration for '" + spec.name
                + "' (value/unit/checkpoint differ)");
        }
        return;
    }
    // `checkpoint = true` is accepted but nothing writes or reads scalars on checkpoint/restart
    // (only fields go through FieldRegistry::checkpointNames()).  Saying so is better than a
    // declaration that silently does not survive a restart.
    if (spec.checkpoint) {
        ablastr::warn_manager::WMRecordWarning(
            "ScalarRegistry",
            "scalar '" + spec.name + "' is declared with checkpoint = true, but scalars are not "
            "written to or restored from checkpoints in this build; the value will be reset on "
            "restart",
            ablastr::warn_manager::WarnPriority::medium);
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
