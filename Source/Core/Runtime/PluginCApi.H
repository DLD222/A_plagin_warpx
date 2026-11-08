/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
/*
 * C ABI v3 for out-of-tree runtime plugins.
 *
 * Everything a plugin needs to touch is declared here with C types only: no
 * std::string, no std::vector, no std::function, no C++ vtable, no host class
 * pointer.  A plugin compiled as C (or by a different C++ compiler / STL)
 * can register handlers through this header and be dlopen'ed by the runtime.
 *
 * The host side wraps each WarpXHandlerDesc in a C++ IHandler internally; the
 * wrapper is invisible to the plugin.
 */
#ifndef WARPX_RUNTIME_PLUGINCAPI_H_
#define WARPX_RUNTIME_PLUGINCAPI_H_

#include <stddef.h>
#include <stdint.h>

/* Version of the layout below.  Bump on any change to WarpXHandlerDesc,
 * WarpXPluginDesc, the WarpXStage ordinals, or the exported functions. */
#define WARPX_PLUGIN_ABI_VERSION 4u

#ifdef __cplusplus
extern "C" {
#endif

/* Opaque handles; deliberately incomplete so a plugin cannot depend on the
 * host's memory layout. */
typedef struct WarpXPluginHost WarpXPluginHost;
typedef struct WarpXStageContext WarpXStageContext;

/* Stage ordinals.  Mirrors warpx::runtime::Stage; the host asserts equality at
 * build time (PluginCAdapter.cpp). */
enum WarpXStage
{
    /* Standard ordinals.  They match warpx::runtime::Stage::* and are spaced
     * by 1000 so that a C++ plugin can register extra stages in between; a C
     * plugin can only use the standard ones (custom stages are registered by
     * name through the C++ API until the plugin manifest lands). */
    WARPX_STAGE_BEFORE_INIT       =     0,
    WARPX_STAGE_INIT              =  1000,
    WARPX_STAGE_AFTER_INIT        =  2000,
    WARPX_STAGE_BEFORE_PUSH       =  3000,
    WARPX_STAGE_PUSH              =  4000,
    WARPX_STAGE_AFTER_PUSH        =  5000,
    WARPX_STAGE_BEFORE_COLLISIONS =  6000,
    WARPX_STAGE_COLLISIONS        =  7000,
    WARPX_STAGE_AFTER_COLLISIONS  =  8000,
    WARPX_STAGE_BEFORE_FIELD_SOLVE=  9000,
    WARPX_STAGE_FIELD_SOLVE       = 10000,
    WARPX_STAGE_AFTER_FIELD_SOLVE = 11000,
    WARPX_STAGE_DIAGNOSTICS       = 12000,
    WARPX_STAGE_FINALIZE          = 13000
};

/* Data contract of one handler.  String arrays are borrowed for the duration
 * of warpx_host_add_handler: the host copies them. */
typedef struct WarpXHandlerDesc
{
    uint32_t struct_size;          /* must be sizeof(WarpXHandlerDesc) */
    const char* name;              /* unique handler name, host copies it */
    int32_t stage;                 /* enum WarpXStage */
    int32_t sub;                   /* sub-stage index, usually 0 */
    int32_t priority;              /* tie-break inside one stage */
    const char* const* reads_fields;
    uint32_t n_reads_fields;
    const char* const* writes_fields;
    uint32_t n_writes_fields;
    const char* const* reads_species;
    uint32_t n_reads_species;
    const char* const* writes_species;
    uint32_t n_writes_species;
    /* Called at the handler's stage. user_data is passed back verbatim. */
    void (*apply) (WarpXStageContext* ctx, void* user_data);
    /* Optional; may be NULL. Called after a successful AMR regrid. */
    void (*on_regrid) (WarpXStageContext* ctx, void* user_data);
    void* user_data;
} WarpXHandlerDesc;

/* Plugin descriptor returned by the warpx_plugin_entry symbol. */
typedef struct WarpXPluginDesc
{
    uint32_t abi_version;          /* must equal WARPX_PLUGIN_ABI_VERSION */
    uint32_t struct_size;          /* must equal sizeof(WarpXPluginDesc) */
    const char* name;
    const char* version;
    /* Registers zero or more handlers through warpx_host_add_handler. */
    void (*register_handlers) (WarpXPluginHost* host);
} WarpXPluginDesc;

/* Type of the exported entry point; the loader resolves the string
 * "warpx_plugin_entry" and calls it. */
typedef const WarpXPluginDesc* (*WarpXPluginEntry) (void);

/* Register one handler.  Returns 0 on success, non-zero if the descriptor is
 * invalid, its struct_size does not match, or the host rejects it (duplicate
 * writer on the same stage, already-registered name, ...). */
int warpx_host_add_handler (WarpXPluginHost* host, const WarpXHandlerDesc* desc);

/* Stage context accessors.  Safe to call with a context the host passed in. */
int32_t warpx_stage_step (const WarpXStageContext* ctx);
double warpx_stage_time (const WarpXStageContext* ctx);
double warpx_stage_dt (const WarpXStageContext* ctx);

/* Field access.  On success returns 1 and, if multifab is non-NULL, stores the
 * amrex::MultiFab pointer for that field/level (advanced plugins also link
 * AMReX to interpret it).  Returns 0 when the field is absent. */
int32_t warpx_stage_has_field (WarpXStageContext* ctx, const char* name, int32_t lev);
int32_t warpx_stage_field (WarpXStageContext* ctx, const char* name, int32_t lev, void** multifab);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* WARPX_RUNTIME_PLUGINCAPI_H_ */
