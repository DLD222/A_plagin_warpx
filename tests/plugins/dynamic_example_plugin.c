/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Dynamically loaded plugin built as plain C: proves a plugin needs neither a
 * C++ compiler nor any host header beyond Runtime/PluginCApi.H.
 */
#include "Runtime/PluginCApi.H"

static int g_apply_count = 0;

static void
dynamic_apply (WarpXStageContext* ctx, void* user_data)
{
    (void)ctx;
    (void)user_data;
    ++g_apply_count;
}

static void
dynamic_register_handlers (WarpXPluginHost* host)
{
    WarpXHandlerDesc handler = {0};
    handler.struct_size = (uint32_t) sizeof(WarpXHandlerDesc);
    handler.name = "dynamic_example_plugin";
    handler.stage = WARPX_STAGE_FINALIZE;
    handler.priority = 0;
    handler.apply = &dynamic_apply;
    handler.user_data = NULL;
    (void) warpx_host_add_handler(host, &handler);
}

static const WarpXPluginDesc g_plugin = {
    WARPX_PLUGIN_ABI_VERSION,
    (uint32_t) sizeof(WarpXPluginDesc),
    "dynamic_example_plugin",
    "1.0",
    &dynamic_register_handlers
};

const WarpXPluginDesc*
warpx_plugin_entry (void)
{
    return &g_plugin;
}

int
dynamic_example_apply_count (void)
{
    return g_apply_count;
}
