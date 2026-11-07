/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "Modules/PicEm/FieldSolve/Yee/YeePackage.H"

#include "Modules/PicEm/FieldSolve/Yee/YeeFieldSolveHandler.H"

#include <memory>

namespace warpx::runtime
{

void
YeePackage::register_handlers (PluginHost& host)
{
    host.add(std::make_shared<YeeFieldSolveHandler>());
}

} // namespace warpx::runtime
