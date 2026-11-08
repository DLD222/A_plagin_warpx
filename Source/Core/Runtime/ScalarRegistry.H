/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_RUNTIME_SCALARREGISTRY_H_
#define WARPX_RUNTIME_SCALARREGISTRY_H_

#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace warpx::runtime
{

/** One named, simulation-wide scalar (temperature, phase fraction, ...). */
struct ScalarSpec
{
    std::string name;
    double value = 0.0;
    bool checkpoint = false;
    std::string unit;
};

/**
 * \brief Named scalar state shared between plugins (S3b).
 *
 * Unlike fields, scalars are not per level: one value for the whole
 * simulation (the architecture document's `ScalarKey`).  A plugin declares
 * the scalar it needs through StateRegistry; handlers read/write it through
 * the bus, subject to the same declared-access rule as fields.
 */
class ScalarRegistry
{
public:
    //! Declare a scalar; identical re-declaration is a no-op.
    void declare (const ScalarSpec& spec);

    [[nodiscard]] bool has (const std::string& name) const;

    //! Value of a declared scalar; throws when undeclared.
    [[nodiscard]] double get (const std::string& name) const;

    //! Set the value of a declared scalar; throws when undeclared.
    void set (const std::string& name, double value);

    [[nodiscard]] const std::vector<ScalarSpec>& specs () const { return m_specs; }

    //! Drop everything (tests only).
    void clear ();

private:
    std::vector<ScalarSpec> m_specs;
    std::map<std::string, double> m_values;
};

} // namespace warpx::runtime

#endif // WARPX_RUNTIME_SCALARREGISTRY_H_
