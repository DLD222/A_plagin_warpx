/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Out-of-tree example module written in plain C against the v3 plugin ABI.
 */
#include "example_plugin.h"

static int g_apply_count = 0;

static void
example_apply (WarpXStageContext* ctx, void* user_data)
{
    (void)ctx;
    (void)user_data;
    ++g_apply_count;
}

static void
example_register_handlers (WarpXPluginHost* host)
{
    WarpXHandlerDesc handler = {0};
    handler.struct_size = (uint32_t) sizeof(WarpXHandlerDesc);
    handler.name = "example_plugin";
    handler.stage = WARPX_STAGE_DIAGNOSTICS;
    handler.sub = 0;
    handler.priority = 0;
    handler.apply = &example_apply;
    handler.on_regrid = NULL;
    handler.user_data = NULL;
    (void) warpx_host_add_handler(host, &handler);
}

static const WarpXPluginDesc g_example_plugin = {
    WARPX_PLUGIN_ABI_VERSION,
    (uint32_t) sizeof(WarpXPluginDesc),
    "example_plugin",
    "1.0",
    &example_register_handlers
};

const WarpXPluginDesc*
warpx_example_plugin_desc (void)
{
    return &g_example_plugin;
}

int
warpx_example_plugin_apply_count (void)
{
    return g_apply_count;
}
