/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Plugin with a deliberately wrong ABI version (C).
 */
#include "Runtime/PluginCApi.H"

static void
bad_register_handlers (WarpXPluginHost* host)
{
    (void) host;
}

static const WarpXPluginDesc g_plugin = {
    999u,
    (uint32_t) sizeof(WarpXPluginDesc),
    "bad_version_plugin",
    "0",
    &bad_register_handlers
};

const WarpXPluginDesc*
warpx_plugin_entry (void)
{
    return &g_plugin;
}
