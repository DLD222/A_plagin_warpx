/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "Runtime/HandlerSpec.H"
#include "Runtime/PluginLoader.H"

#include <stdexcept>

int main ()
{
    warpx::runtime::PluginHost host;
    try {
        warpx::runtime::PluginLoader::load("/nonexistent/warpx_plugin.so", host);
    } catch (const std::runtime_error&) {
        return host.size() == 0 ? 0 : 1;
    }
    return 1;
}
