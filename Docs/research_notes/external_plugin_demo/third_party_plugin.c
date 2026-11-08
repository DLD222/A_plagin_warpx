/* A plugin written entirely outside the WarpX repository.
 *
 * It only includes the public C ABI header, declares its own state and a
 * handler, and links no WarpX code: the host resolves the ABI entry points
 * when it loads the library.  Building and loading this proves that a third
 * party can extend the code without modifying the core.
 */
#include "Runtime/PluginCApi.H"

#include <stddef.h>
#include <stdio.h>

static double g_calls = 0.0;

static void third_party_handler (WarpXStageContext* ctx, void* user_data)
{
    (void)user_data;
    g_calls += 1.0;
    /* Visible proof that the core scheduled a plugin it never heard of. */
    printf("[third_party] handler call %.0f at stage AFTER_FIELD_SOLVE, dt=%g\n",
                g_calls, warpx_stage_dt(ctx));
    (void)warpx_stage_set_scalar(ctx, "third_party_calls", g_calls);
}

static void on_schema (WarpXStateRegistry* state)
{
    /* State the core does not know about: it owns the memory. */
    (void)warpx_state_request_field(state, "third_party_score", 1, 0, 0);
}

static void on_init (WarpXStageContext* ctx)
{
    (void)warpx_stage_set_scalar(ctx, "third_party_ready", 1.0);
}

static void register_handlers (WarpXPluginHost* host)
{
    WarpXHandlerDesc desc;
    desc.struct_size = sizeof(WarpXHandlerDesc);
    desc.name = "third_party_handler";
    desc.stage = WARPX_STAGE_AFTER_FIELD_SOLVE;
    desc.sub = 0;
    desc.priority = 500;
    desc.reads_fields = NULL;
    desc.n_reads_fields = 0;
    desc.writes_fields = NULL;
    desc.n_writes_fields = 0;
    desc.reads_species = NULL;
    desc.n_reads_species = 0;
    desc.writes_species = NULL;
    desc.n_writes_species = 0;
    desc.reads_scalars = NULL;
    desc.n_reads_scalars = 0;
    desc.writes_scalars = NULL;
    desc.n_writes_scalars = 0;
    desc.apply = &third_party_handler;
    desc.on_regrid = NULL;
    desc.user_data = NULL;
    (void)warpx_host_add_handler(host, &desc);
}

static const WarpXPluginDesc plugin = {
    WARPX_PLUGIN_ABI_VERSION,
    sizeof(WarpXPluginDesc),
    "third_party",
    "1.0.0",
    &register_handlers,
    "physics",
    NULL, 0,
    NULL, 0,
    &on_schema,
    &on_init,
    NULL
};

const WarpXPluginDesc* warpx_plugin_entry (void)
{
    return &plugin;
}
