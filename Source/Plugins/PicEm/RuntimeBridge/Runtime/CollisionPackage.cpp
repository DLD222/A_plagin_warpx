/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "Runtime/CollisionPackage.H"

#include "Runtime/CollisionHandlerAdapter.H"
#include "Runtime/ICollisionHandler.H"

namespace warpx::runtime
{

void
CollisionPackage::register_handler (PluginHost& host, warpx::runtime::ICollisionHandler* handler)
{
    // No collision plugin linked: register nothing, so the stage simply has no
    // handler instead of one that throws when invoked.
    if (handler == nullptr) { return; }
    host.add(std::make_shared<CollisionHandlerAdapter>(handler));
    // The handler is created by the kernel, not by a plugin entry: give its
    // state declarations a place in the schema phase (S4).
    host.add_plugin(std::make_shared<CollisionStatePlugin>(handler));
}

} // namespace warpx::runtime
