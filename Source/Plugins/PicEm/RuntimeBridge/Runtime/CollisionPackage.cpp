/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "Runtime/CollisionPackage.H"

#include "Particles/Collision/CollisionHandler.H"
#include "Runtime/CollisionHandlerAdapter.H"

namespace warpx::runtime
{

void
CollisionPackage::register_handler (PluginHost& host, warpx::runtime::ICollisionHandler* handler)
{
    host.add(std::make_shared<CollisionHandlerAdapter>(handler));
}

} // namespace warpx::runtime
