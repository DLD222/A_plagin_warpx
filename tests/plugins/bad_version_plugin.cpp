/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "Runtime/HandlerSpec.H"
#include "Runtime/PluginABI.H"

namespace
{
void register_handlers (warpx::runtime::PluginHost*) {}
const WarpXPluginDesc desc{
    999u,
    sizeof(WarpXPluginDesc),
    "bad_version_plugin",
    "0",
    &register_handlers
};
}

extern "C" const WarpXPluginDesc* warpx_plugin_entry () { return &desc; }
