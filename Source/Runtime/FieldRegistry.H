/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_RUNTIME_FIELDREGISTRY_H_
#define WARPX_RUNTIME_FIELDREGISTRY_H_

#include "Runtime/FieldSpec.H"

#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace warpx::runtime
{

/**
 * \brief String-keyed, schema-only field registry (A4.1).
 *
 * It never owns a MultiFab; it only records FieldSpec declarations. The same
 * key may be declared repeatedly only with identical metadata. All metadata
 * fields are compared, including checkpoint/moving_window_shift/unit.
 */
class FieldRegistry
{
public:
    void declare (const FieldSpec& spec)
    {
        auto [it, inserted] = m_specs.emplace(spec.name, spec);
        if (inserted) { return; }
        const FieldSpec& old = it->second;
        if (old.ncomp != spec.ncomp || old.nodal != spec.nodal ||
            old.ngrow != spec.ngrow || old.checkpoint != spec.checkpoint ||
            old.moving_window_shift != spec.moving_window_shift ||
            old.unit != spec.unit || old.alias_of != spec.alias_of) {
            throw std::runtime_error(
                "FieldRegistry: conflicting declaration for '" + spec.name + "'");
        }
    }

    [[nodiscard]] bool contains (const std::string& name) const
    {
        return m_specs.find(name) != m_specs.end();
    }

    [[nodiscard]] const FieldSpec& get (const std::string& name) const
    {
        return m_specs.at(name);
    }

    [[nodiscard]] const FieldSpec* find (const std::string& name) const
    {
        const auto it = m_specs.find(name);
        return it == m_specs.end() ? nullptr : &it->second;
    }

    [[nodiscard]] std::size_t size () const { return m_specs.size(); }

    [[nodiscard]] std::vector<std::string> checkpointNames () const
    {
        std::vector<std::string> out;
        for (const auto& [name, spec] : m_specs) {
            if (spec.checkpoint) { out.push_back(name); }
        }
        return out;
    }

    [[nodiscard]] std::vector<std::string> names () const
    {
        std::vector<std::string> out;
        out.reserve(m_specs.size());
        for (const auto& [name, spec] : m_specs) {
            (void)spec;
            out.push_back(name);
        }
        return out;
    }

private:
    std::map<std::string, FieldSpec> m_specs;
};

} // namespace warpx::runtime

#endif // WARPX_RUNTIME_FIELDREGISTRY_H_
