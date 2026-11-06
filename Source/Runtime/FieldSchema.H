/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_RUNTIME_FIELDSCHEMA_H_
#define WARPX_RUNTIME_FIELDSCHEMA_H_

#include "Runtime/FieldSpec.H"

#include <map>
#include <string>

namespace warpx::runtime
{

/**
 * \brief Registry of per-field allocation schemas (A2.4).
 *
 * The schema describes how a register entry is allocated; it never owns the
 * MultiFab. Declaring the same key twice with different parameters is an error.
 */
class FieldSchema
{
public:
    void declare (const FieldSpec& spec)
    {
        auto [it, inserted] = m_specs.emplace(spec.name, spec);
        if (!inserted) {
            const FieldSpec& old = it->second;
            AMREX_ALWAYS_ASSERT_WITH_MESSAGE(
                old.ncomp == spec.ncomp && old.nodal == spec.nodal &&
                old.ngrow == spec.ngrow && old.checkpoint == spec.checkpoint &&
                old.moving_window_shift == spec.moving_window_shift &&
                old.unit == spec.unit && old.alias_of == spec.alias_of,
                "FieldSchema: conflicting declaration for " + spec.name);
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

    [[nodiscard]] std::size_t size () const { return m_specs.size(); }

private:
    std::map<std::string, FieldSpec> m_specs;
};

} // namespace warpx::runtime

#endif // WARPX_RUNTIME_FIELDSCHEMA_H_
