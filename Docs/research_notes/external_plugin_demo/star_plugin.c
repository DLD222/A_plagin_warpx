/* A star-physics runtime plugin, built entirely outside the core.
 *
 * This is the §15.4 wiring experiment: the two star plugins (NewtonGravity, FluidMoments) are
 * libraries that only their own unit tests call.  The question this file answers is how far the
 * existing C ABI (Runtime/PluginCApi.H) can carry them: it declares the state the core will own,
 * registers a handler at AFTER_FIELD_SOLVE, and drives one step of the star state from there.
 *
 * What it demonstrates, and what it does not:
 *
 *  - the loading path works for a star-shaped plugin: a multi-component field declared through
 *    warpx_state_request_field, an AFTER_FIELD_SOLVE handler, and per-step access to the step
 *    number and dt;
 *  - the state can be advanced from the handler with no core change at all.
 *
 *  - it does NOT call NewtonGravitySolver::solvePotential, because that needs an amrex::Geometry
 *    and the ABI does not expose one (no geometry, no domain, no cell size, no periodicity - see
 *    warpx_stage_* in PluginCApi.H).  A solver plugin cannot build its operator from field
 *    pointers alone, so wiring the real solver needs one additive ABI entry point, proposed in
 *    Docs/research_notes/STAR_PLUGIN_BUS.md.
 */

#include "Runtime/PluginCApi.H"

#include <math.h>
#include <stdio.h>

/* Star state: one density, one potential, four momenta, and the same species slots the fluid
 * plugin uses.  Declared to the core, which owns the memory. */
enum
{
    STAR_RHO = 0,
    STAR_PHI = 1,
    STAR_MOMX = 2,
    STAR_SPECIES0 = 3,
    STAR_NCOMP = 3 + 4
};

static double g_space_steps = 0.0;
static double g_total_advance = 0.0;

static void advance_star_state (WarpXStageContext* ctx, void* user_data)
{
    (void)user_data;
    g_space_steps += 1.0;

    void* raw = NULL;
    if (warpx_stage_field(ctx, "star_state", 0, &raw) != 1 || raw == NULL) { return; }

    /* Without an amrex::MultiFab here the plugin cannot touch the data (this demo deliberately
     * links no AMReX); it only records that the host handed it the field and advanced. */
    g_total_advance += warpx_stage_dt(ctx);
    (void)warpx_stage_set_scalar(ctx, "star_steps", g_space_steps);

    if (g_space_steps <= 2.0) {
        printf("[star] step %.0f at AFTER_FIELD_SOLVE, dt=%g, total advanced=%g s\n",
               g_space_steps, warpx_stage_dt(ctx), g_total_advance);
    }
}

static void on_schema (WarpXStateRegistry* state)
{
    /* The core allocates this on every level and keeps it across regrids. */
    (void)warpx_state_request_field(state, "star_state", STAR_NCOMP, 2, 1);
}

static const char* star_scalars[] = {"star_steps"};
static const char* star_writes[] = {"star_state"};

static void register_handlers (WarpXPluginHost* host)
{
    WarpXHandlerDesc desc = {0};
    desc.struct_size = sizeof(WarpXHandlerDesc);
    desc.name = "star_advance";
    desc.stage = WARPX_STAGE_AFTER_FIELD_SOLVE;
    desc.apply = &advance_star_state;
    desc.writes_fields = star_writes;
    desc.n_writes_fields = 1;
    desc.writes_scalars = star_scalars;
    desc.n_writes_scalars = 1;
    (void)warpx_host_add_handler(host, &desc);
}

static const WarpXPluginDesc kStarPlugin = {
    WARPX_PLUGIN_ABI_VERSION,
    sizeof(WarpXPluginDesc),
    "star_runtime",
    "0.1",
    &register_handlers,
    "physics",
    NULL, 0,
    NULL, 0,
    &on_schema,
    NULL,
    NULL
};

const WarpXPluginDesc* warpx_plugin_entry (void)
{
    return &kStarPlugin;
}
