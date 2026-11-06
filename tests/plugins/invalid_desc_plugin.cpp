/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "Runtime/PluginABI.H"

namespace
{
const WarpXPluginDesc desc{
    warpx_plugin_abi_version,
    sizeof(WarpXPluginDesc),
    nullptr,
    "0",
    nullptr
};
}

extern "C" const WarpXPluginDesc* warpx_plugin_entry () { return &desc; }
