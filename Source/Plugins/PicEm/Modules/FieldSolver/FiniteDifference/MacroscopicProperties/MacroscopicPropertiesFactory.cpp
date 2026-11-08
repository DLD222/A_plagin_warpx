/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Material-model plugin entry point: the kernel asks the core, the core
 * forwards here (S6).
 */
#include "FieldSolver/FiniteDifference/MacroscopicProperties/MacroscopicProperties.H"

#include "Runtime/IMacroscopicProperties.H"
#include "Runtime/ServiceFactory.H"

#include <memory>

//! Linker anchor: keeps this registration object in the aggregate archive.
extern "C" void warpx_macroscopic_anchor (void) {}

namespace warpx::runtime
{
namespace
{
std::unique_ptr<IMacroscopicProperties> make_macroscopic_properties ()
{
    return std::make_unique<MacroscopicProperties>();
}

const bool macroscopic_factory_registered = [] {
    register_macroscopic_properties_factory(&make_macroscopic_properties);
    return true;
}();
} // namespace
} // namespace warpx::runtime
