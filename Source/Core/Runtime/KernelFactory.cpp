/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "Runtime/KernelFactory.H"

#include "Runtime/IPicKernel.H"

#include <stdexcept>

namespace warpx::runtime
{
namespace
{
// Function-local static: registration from a plugin's static initializer runs
// before main(), but this storage is created on first use, so there is no
// static-initialization-order dependency between Core and a plugin.
SolverKernelPlugin& registry ()
{
    static SolverKernelPlugin plugin{};
    return plugin;
}
} // namespace

void
register_solver_kernel_plugin (const SolverKernelPlugin& plugin)
{
    registry() = plugin;
}

bool
has_solver_kernel_plugin ()
{
    return registry().create != nullptr;
}

std::unique_ptr<IPicKernel>
create_solver_kernel (int argc, char* argv[])
{
    if (registry().create == nullptr) {
        throw std::runtime_error(
            "Runtime: no PIC kernel plugin is registered. Link a kernel plugin "
            "(e.g. warpx_picem_<dim>) or register a SolverKernelPlugin.");
    }
    return registry().create(argc, argv);
}

void
initialize_runtime_libraries (int argc, char* argv[])
{
    if (registry().initialize_libraries != nullptr) {
        registry().initialize_libraries(argc, argv);
    }
}

void
finalize_runtime_libraries ()
{
    if (registry().finalize_libraries != nullptr) {
        registry().finalize_libraries();
    }
}

void
clear_solver_kernel_plugin ()
{
    registry() = SolverKernelPlugin{};
}

} // namespace warpx::runtime
