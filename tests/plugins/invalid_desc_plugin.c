/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Plugin with a malformed descriptor (no name, no register function) (C).
 */
#include "Runtime/PluginCApi.H"

static const WarpXPluginDesc g_plugin = {
    WARPX_PLUGIN_ABI_VERSION,
    (uint32_t) sizeof(WarpXPluginDesc),
    NULL,
    "0",
    NULL
};

const WarpXPluginDesc*
warpx_plugin_entry (void)
{
    return &g_plugin;
}
