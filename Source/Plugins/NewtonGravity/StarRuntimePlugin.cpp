/* In-tree runtime plugin that drives the star-physics layers through the C ABI bus.
 *
 * This is the §15.4 wiring: a plugin that the build produces as a loadable module and that the
 * host picks up through WARPX_PLUGIN_PATH, with no core file touched.  It is built by the same
 * CMake project as everything else (so it gets the right AMReX configuration) but it is *loaded*,
 * not linked into the executable - the host never hears of a "star" type.
 *
 * It does two things per step at AFTER_FIELD_SOLVE:
 *   1. builds an amrex::Geometry from the ABI v7 geometry accessor (warpx_stage_geometry);
 *   2. solves the Newton-gravity potential of star_rho into star_phi, and reports the result.
 * The fluid step is the same pattern and is left for the next round.
 */
#include "Runtime/PluginCApi.H"

#include "NewtonGravity.H"

#include <AMReX.H>
#include <AMReX_BoxArray.H>
#include <AMReX_DistributionMapping.H>
#include <AMReX_Geometry.H>
#include <AMReX_MultiFab.H>

#include <cstdio>
#include <cstring>

namespace
{
double g_steps = 0.0;
double g_phi_min = 0.0;

/** Build a level-0 Geometry from what the ABI exposes. */
amrex::Geometry geometry_from_context (WarpXStageContext* ctx)
{
    int lo[3] = {0, 0, 0}, hi[3] = {0, 0, 0}, periodic[3] = {0, 0, 0};
    double prob_lo[3] = {0.0, 0.0, 0.0}, prob_hi[3] = {1.0, 1.0, 1.0}, dx[3] = {0.0, 0.0, 0.0};
    if (warpx_stage_geometry(ctx, 0, lo, hi, prob_lo, prob_hi, dx, periodic) != 1) {
        return amrex::Geometry();
    }
    amrex::Box const domain(amrex::IntVect{lo[0], lo[1], lo[2]},
                            amrex::IntVect{hi[0], hi[1], hi[2]});
    amrex::RealBox rb(prob_lo, prob_hi);
    amrex::Array<int, AMREX_SPACEDIM> is_periodic{};
    for (int d = 0; d < AMREX_SPACEDIM; ++d) { is_periodic[d] = periodic[d]; }
    return amrex::Geometry(domain, rb, 0 /* Cartesian */, is_periodic);
}

void star_step (WarpXStageContext* ctx, void*)
{
    g_steps += 1.0;

    void* raw_rho = nullptr;
    void* raw_phi = nullptr;
    if (warpx_stage_read_field(ctx, "star_rho", 0, &raw_rho) != 1 || raw_rho == nullptr) { return; }
    if (warpx_stage_field(ctx, "star_phi", 0, &raw_phi) != 1 || raw_phi == nullptr) { return; }

    auto& rho = *static_cast<amrex::MultiFab*>(raw_rho);
    auto& phi = *static_cast<amrex::MultiFab*>(raw_phi);
    amrex::Geometry geom = geometry_from_context(ctx);
    if (!geom.isAllPeriodic() && geom.Domain().isEmpty()) { return; }

    // The deck's geometry decides the boundary: "open" means Dirichlet on the non-periodic faces
    // and keeps periodic directions periodic, which is what the AMReX operator requires.
    warpx::plugins::gravity::NewtonGravitySolver::solvePotential(
        rho, phi, geom, 1.0e-13, warpx::plugins::gravity::PotentialBoundary::open);
    g_phi_min = phi.min(0);

    (void)warpx_stage_set_scalar(ctx, "star_steps", g_steps);
    (void)warpx_stage_set_scalar(ctx, "star_phi_min", g_phi_min);
    if (g_steps <= 3.0) {
        std::printf("[star] step %.0f: solved Poisson on a %d-cell domain, phi_min = %g\n",
                    g_steps, (int)geom.Domain().numPts(), g_phi_min);
    }
}

void on_schema (WarpXStateRegistry* state)
{
    (void)warpx_state_request_field(state, "star_rho", 1, 1, 0);
    (void)warpx_state_request_field(state, "star_phi", 1, 1, 0);
}

void on_init (WarpXStageContext* ctx)
{
    void* raw = nullptr;
    if (warpx_stage_field(ctx, "star_rho", 0, &raw) == 1 && raw != nullptr) {
        static_cast<amrex::MultiFab*>(raw)->setVal(1.0e-3);
    }
    std::printf("[star] initialised: star_rho set to 1e-3, waiting for the first step\n");
}

const char* kScalars[] = {"star_steps", "star_phi_min"};
const char* kReads[] = {"star_rho"};
const char* kWrites[] = {"star_phi"};

void register_handlers (WarpXPluginHost* host)
{
    WarpXHandlerDesc desc{};
    desc.struct_size = sizeof(WarpXHandlerDesc);
    desc.name = "star_gravity_step";
    desc.stage = WARPX_STAGE_AFTER_FIELD_SOLVE;
    desc.apply = &star_step;
    desc.reads_fields = kReads;
    desc.n_reads_fields = 1;
    desc.writes_fields = kWrites;
    desc.n_writes_fields = 1;
    desc.writes_scalars = kScalars;
    desc.n_writes_scalars = 2;
    (void)warpx_host_add_handler(host, &desc);
}

const WarpXPluginDesc kPlugin = {
    WARPX_PLUGIN_ABI_VERSION,
    sizeof(WarpXPluginDesc),
    "star_runtime",
    "0.2",
    &register_handlers,
    "physics",
    nullptr, 0,
    nullptr, 0,
    &on_schema,
    &on_init,
    nullptr
};
} // namespace

extern "C" const WarpXPluginDesc* warpx_plugin_entry (void)
{
    return &kPlugin;
}
