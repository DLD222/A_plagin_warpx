/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "Runtime/FactoryConflict.H"
#include "Runtime/IMagnetostaticSolver.H"

namespace warpx::runtime
{
namespace
{
MagnetostaticSolverFactory& registry ()
{
    static MagnetostaticSolverFactory factory = nullptr;
    return factory;
}
} // namespace

namespace
{
std::string& conflict ()
{
    static std::string c;
    return c;
}
} // namespace

void
register_magnetostatic_solver_factory (MagnetostaticSolverFactory factory)
{
    detail::assign_factory(registry(), factory, "magnetostatic-solver", conflict());
}

bool
has_magnetostatic_solver_factory ()
{
    return registry() != nullptr;
}

std::unique_ptr<IMagnetostaticSolver>
create_magnetostatic_solver ()
{
    detail::require_no_conflict(conflict());
    if (registry() == nullptr) { return nullptr; }
    return registry()();
}

void
clear_magnetostatic_solver_factory ()
{
    registry() = nullptr;
}

} // namespace warpx::runtime
