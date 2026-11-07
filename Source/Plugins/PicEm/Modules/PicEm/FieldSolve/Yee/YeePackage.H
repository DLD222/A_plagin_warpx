/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_MODULES_PICEM_FIELDSOLVE_YEE_YEEPACKAGE_H_
#define WARPX_MODULES_PICEM_FIELDSOLVE_YEE_YEEPACKAGE_H_

#include "Runtime/HandlerSpec.H"

namespace warpx::runtime
{

class YeePackage
{
public:
    static void register_handlers (PluginHost& host);
};

} // namespace warpx::runtime

#endif
