/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
/* FluidMoments as a runtime plugin.
 *
 * One plugin, one job: own the two-species five-moment state, read the gravitational acceleration
 * star_g{x,y,z} that the gravity plugin published on the bus, advance one MUSCL-Hancock/HLLC sweep
 * per direction, and report the total energy.
 *
 * It includes nothing from another plugin: the coupling to NewtonGravity is the four field names
 * on the bus, which is the only channel A7 allows between independent plugins.
 */
#include "FluidMoments.H"

#include "Runtime/PluginCApi.H"

#include <AMReX.H>
#include <AMReX_BoxArray.H>
#include <AMReX_DistributionMapping.H>
#include <AMReX_Geometry.H>
#include <AMReX_MultiFab.H>

#include <cstdio>

namespace
{
double g_steps = 0.0;
double g_energy = 0.0;

/** The fluid state this plugin owns: two species, the momentum and the total energy. */
warpx::plugins::fluid::StateLayout const g_layout{2};

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
    return amrex::Geometry(domain, rb, 0, is_periodic);
}

void fluid_step (WarpXStageContext* ctx, void*)
{
    g_steps += 1.0;

    void* raw_state = nullptr;
    if (warpx_stage_field(ctx, "star_state", 0, &raw_state) != 1 || raw_state == nullptr) { return; }
    auto& state = *static_cast<amrex::MultiFab*>(raw_state);
    amrex::Geometry geom = geometry_from_context(ctx);
    if (geom.Domain().isEmpty()) { return; }

    // g comes from the bus.  If the gravity plugin is not loaded these are absent and the fluid
    // simply advances without it - which is exactly the independence A7 asks for.
    amrex::MultiFab gx(state.boxArray(), state.DistributionMap(), 1, 1);
    amrex::MultiFab gy(state.boxArray(), state.DistributionMap(), 1, 1);
    amrex::MultiFab gz(state.boxArray(), state.DistributionMap(), 1, 1);
    bool have_g = true;
    char name[16] = {'s', 't', 'a', 'r', '_', 'g', 'x', '\0'};
    amrex::MultiFab* out[3] = {&gx, &gy, &gz};
    for (int d = 0; d < 3; ++d) {
        name[6] = static_cast<char>('x' + d);
        void* raw_g = nullptr;
        if (warpx_stage_read_field(ctx, name, 0, &raw_g) == 1 && raw_g != nullptr) {
            out[d]->ParallelCopy(*static_cast<amrex::MultiFab*>(raw_g), 0, 0, 1);
        } else {
            out[d]->setVal(0.0);
            have_g = false;
        }
    }
    amrex::ignore_unused(have_g);

    amrex::Real const dt = warpx_stage_dt(ctx);
    amrex::Vector<amrex::MultiFab const*> g{&gx, &gy, &gz};
    warpx::plugins::fluid::applyGravitySource(state, g, dt, g_layout);

    warpx::plugins::fluid::IdealGasEOS eos;
    for (int dir = 0; dir < AMREX_SPACEDIM; ++dir) {
        state.FillBoundary(geom.periodicity());
        warpx::plugins::fluid::advanceSweep(state, geom, dir, dt, eos, g_layout);
    }
    g_energy = state.sum(g_layout.energy());
    (void)warpx_stage_set_scalar(ctx, "star_energy", g_energy);
    if (g_steps <= 3.0) {
        std::printf("[fluid] step %.0f: fluid energy = %g (g from bus: %s)\n",
                    g_steps, g_energy, have_g ? "yes" : "no");
    }
}

void on_schema (WarpXStateRegistry* state)
{
    (void)warpx_state_request_field(state, "star_state", 2 + AMREX_SPACEDIM + 1, 2, 0);
}

void on_init (WarpXStageContext* ctx)
{
    for (int c = 0; c < 2; ++c) {
        void* raw = nullptr;
        if (warpx_stage_field(ctx, "star_state", 0, &raw) == 1 && raw != nullptr) {
            static_cast<amrex::MultiFab*>(raw)->setVal(c == 0 ? 1.0 : 0.1, c);
        }
    }
    void* raw = nullptr;
    if (warpx_stage_field(ctx, "star_state", 0, &raw) == 1 && raw != nullptr) {
        auto& state = *static_cast<amrex::MultiFab*>(raw);
        for (int d = 0; d < AMREX_SPACEDIM; ++d) { state.setVal(0.0, g_layout.momentum(d)); }
        state.setVal(2.5, g_layout.energy());   // p = 1 with gamma = 1.4
    }
    std::printf("[fluid] initialised: two-species gas at rest, p = 1\n");
}

const char* kScalars[] = {"star_energy"};
const char* kReads[] = {"star_gx", "star_gy", "star_gz"};

void register_handlers (WarpXPluginHost* host)
{
    WarpXHandlerDesc desc{};
    desc.struct_size = sizeof(WarpXHandlerDesc);
    desc.name = "star_fluid";
    desc.stage = WARPX_STAGE_AFTER_FIELD_SOLVE;
    desc.sub = 1;                   // after the gravity plugin at the same stage
    desc.apply = &fluid_step;
    desc.reads_fields = kReads;
    desc.n_reads_fields = 3;
    desc.writes_scalars = kScalars;
    desc.n_writes_scalars = 1;
    (void)warpx_host_add_handler(host, &desc);
}

const WarpXPluginDesc kPlugin = {
    WARPX_PLUGIN_ABI_VERSION,
    sizeof(WarpXPluginDesc),
    "star_fluid",
    "1.0",
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
