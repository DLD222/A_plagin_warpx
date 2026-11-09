/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "Runtime/SpectralSolverFactory.H"

namespace warpx::runtime
{
namespace
{
// Function-local static, like the kernel factory: the field-solver plugin
// registers from its static initializer, whose order relative to Core we do not
// control.
SpectralSolverFactory& registry ()
{
    static SpectralSolverFactory factory = nullptr;
    return factory;
}
} // namespace

void
register_spectral_solver_factory (SpectralSolverFactory factory)
{
    registry() = factory;
}

bool
has_spectral_solver_factory ()
{
    return registry() != nullptr;
}

std::unique_ptr<ISpectralSolver>
create_spectral_solver (SpectralSolverConfig const& config)
{
    if (registry() == nullptr) { return nullptr; }
    return registry()(config);
}

void
clear_spectral_solver_factory ()
{
    registry() = nullptr;
}

} // namespace warpx::runtime
