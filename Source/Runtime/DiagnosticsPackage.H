/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_RUNTIME_DIAGNOSTICSPACKAGE_H_
#define WARPX_RUNTIME_DIAGNOSTICSPACKAGE_H_

#include "Runtime/DiagnosticsHandler.H"

#include <memory>

namespace warpx::runtime
{

class DiagnosticsPackage
{
public:
    static void register_handlers (PluginHost& host)
    {
        host.add(std::make_shared<DiagnosticsHandler>());
    }
};

} // namespace warpx::runtime

#endif // WARPX_RUNTIME_DIAGNOSTICSPACKAGE_H_
