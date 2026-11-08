/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_RUNTIME_STATEREGISTRY_H_
#define WARPX_RUNTIME_STATEREGISTRY_H_

#include "Runtime/FieldSpec.H"

#include <string>
#include <vector>

namespace warpx::runtime
{

/**
 * \brief State a plugin asks the core to own (S3).
 *
 * The plugin declares *what* it needs; the core decides *when* to create,
 * recreate and checkpoint it.  A plugin must never allocate its own fields:
 * that is what keeps "level lifecycle belongs to the core" (I3) true and lets
 * a field survive regrid, load balance and restart without plugin code.
 *
 * Today the registry owns fields only; species, materials, scalars and
 * particle attributes follow in S3b.
 */
class StateRegistry
{
public:
    /**
     * \brief Declare a field owned by a plugin.
     *
     * Re-declaring the same name with identical metadata is harmless;
     * declaring it with different metadata throws.
     */
    void request_field (const FieldSpec& spec);

    [[nodiscard]] const std::vector<FieldSpec>& requested_fields () const
    {
        return m_fields;
    }

    [[nodiscard]] const FieldSpec* find_field (const std::string& name) const;

    [[nodiscard]] bool has_field (const std::string& name) const
    {
        return find_field(name) != nullptr;
    }

    //! Drop every request (tests only).
    void clear ();

private:
    std::vector<FieldSpec> m_fields;
};

} // namespace warpx::runtime

#endif // WARPX_RUNTIME_STATEREGISTRY_H_
