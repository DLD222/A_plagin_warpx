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
}

} // namespace warpx::runtime
