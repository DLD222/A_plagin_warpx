/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Compile/link/run check that the plugin ABI header is valid C and that the
 * exported host functions have C linkage.  Built with the C compiler, so a
 * C++-only header would fail here.
 */
#include "Runtime/PluginCApi.H"

#include <stdio.h>

static int g_counter = 0;

static void
count_apply (WarpXStageContext* ctx, void* user_data)
{
    (void) user_data;
    /* step(NULL) must be 0; a real host passes a live context. */
    if (warpx_stage_step(ctx) >= 0) {
        ++g_counter;
    }
}

int
main (void)
{
    WarpXHandlerDesc handler = {0};

    handler.struct_size = (uint32_t) sizeof(WarpXHandlerDesc);
    handler.name = "c_abi_header_probe";
    handler.stage = WARPX_STAGE_PUSH;
    handler.sub = 0;
    handler.priority = 0;
    handler.reads_fields = NULL;
    handler.n_reads_fields = 0;
    handler.writes_fields = NULL;
    handler.n_writes_fields = 0;
    handler.reads_species = NULL;
    handler.n_reads_species = 0;
    handler.writes_species = NULL;
    handler.n_writes_species = 0;
    handler.apply = &count_apply;
    handler.on_regrid = NULL;
    handler.user_data = NULL;

    if (handler.struct_size != sizeof(WarpXHandlerDesc)) {
        printf("FAIL: handler struct_size\n");
        return 1;
    }
    if (WARPX_PLUGIN_ABI_VERSION != 6u) {
        printf("FAIL: unexpected ABI version\n");
        return 2;
    }
    if (handler.stage != WARPX_STAGE_PUSH) {
        printf("FAIL: stage ordinal\n");
        return 3;
    }

    handler.apply(NULL, handler.user_data);
    if (g_counter != 1) {
        printf("FAIL: apply dispatch\n");
        return 4;
    }

    /* C-linkage accessors must be callable and tolerant of NULL. */
    if (warpx_stage_step(NULL) != 0) { return 5; }
    if (warpx_stage_time(NULL) != 0.0) { return 6; }
    if (warpx_stage_dt(NULL) != 0.0) { return 7; }
    if (warpx_stage_has_field(NULL, "Efield_fp", 0) != 0) { return 8; }
    if (warpx_stage_field(NULL, "Efield_fp", 0, NULL) != 0) { return 9; }
    if (warpx_host_add_handler(NULL, NULL) == 0) { return 10; }
    if (warpx_state_request_field(NULL, "x", 1, 0, 0) != 0) { return 11; }

    printf("plugin ABI v%u C header OK\n", (unsigned) WARPX_PLUGIN_ABI_VERSION);
    return 0;
}
