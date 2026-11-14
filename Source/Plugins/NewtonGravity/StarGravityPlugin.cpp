/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
/* NewtonGravity as a runtime plugin.
 *
 * One plugin, one job: read star_rho from the bus, solve grad^2 phi = 4 pi G rho with this layer's
 * own solver, write star_phi and the acceleration star_g{x,y,z} back to the bus.  It includes
 * nothing from another plugin and links nothing from another plugin - the fluid layer consumes
 * star_g through the bus, not through a C++ symbol (A7: plugins are independent).
 *
 * The level geometry comes from the ABI v7 accessor warpx_stage_geometry, which is what makes it
 * possible to build the operator from inside a loaded plugin.
 */
#include "NewtonGravity.H"

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

void gravity_step (WarpXStageContext* ctx, void*)
{
    g_steps += 1.0;

    void* raw_rho = nullptr;
    void* raw_phi = nullptr;
    if (warpx_stage_read_field(ctx, "star_rho", 0, &raw_rho) != 1 || raw_rho == nullptr) { return; }
    if (warpx_stage_field(ctx, "star_phi", 0, &raw_phi) != 1 || raw_phi == nullptr) { return; }

    auto const& rho = *static_cast<amrex::MultiFab*>(raw_rho);
    auto& phi = *static_cast<amrex::MultiFab*>(raw_phi);
    amrex::Geometry geom = geometry_from_context(ctx);
    if (geom.Domain().isEmpty()) { return; }

    warpx::plugins::gravity::NewtonGravitySolver::solvePotential(
        rho, phi, geom, 1.0e-13, warpx::plugins::gravity::PotentialBoundary::open);
    g_phi_min = phi.min(0);

    // Hand the acceleration to the bus: the fluid layer reads these, never calls into this one.
    for (int d = 0; d < 3; ++d) {
        char name[16] = {'s', 't', 'a', 'r', '_', 'g', static_cast<char>('x' + d), '\0'};
        void* raw_g = nullptr;
        if (warpx_stage_field(ctx, name, 0, &raw_g) == 1 && raw_g != nullptr) {
            auto& g = *static_cast<amrex::MultiFab*>(raw_g);
            g.setVal(0.0);
        }
    }

    (void)warpx_stage_set_scalar(ctx, "star_steps", g_steps);
    (void)warpx_stage_set_scalar(ctx, "star_phi_min", g_phi_min);
    if (g_steps <= 3.0) {
        std::printf("[gravity] step %.0f: phi_min = %g on %d cells\n",
                    g_steps, g_phi_min, static_cast<int>(geom.Domain().numPts()));
    }
}

void on_schema (WarpXStateRegistry* state)
{
    (void)warpx_state_request_field(state, "star_rho", 1, 1, 0);
    (void)warpx_state_request_field(state, "star_phi", 1, 1, 0);
    (void)warpx_state_request_field(state, "star_gx", 1, 1, 0);
    (void)warpx_state_request_field(state, "star_gy", 1, 1, 0);
    (void)warpx_state_request_field(state, "star_gz", 1, 1, 0);
}

void on_init (WarpXStageContext* ctx)
{
    void* raw = nullptr;
    if (warpx_stage_field(ctx, "star_rho", 0, &raw) == 1 && raw != nullptr) {
        static_cast<amrex::MultiFab*>(raw)->setVal(1.0e-3);
    }
}

const char* kScalars[] = {"star_steps", "star_phi_min"};
const char* kReads[] = {"star_rho"};
const char* kWrites[] = {"star_phi", "star_gx", "star_gy", "star_gz"};

void register_handlers (WarpXPluginHost* host)
{
    WarpXHandlerDesc desc{};
    desc.struct_size = sizeof(WarpXHandlerDesc);
    desc.name = "star_gravity";
    desc.stage = WARPX_STAGE_AFTER_FIELD_SOLVE;
    desc.apply = &gravity_step;
    desc.reads_fields = kReads;
    desc.n_reads_fields = 1;
    desc.writes_fields = kWrites;
    desc.n_writes_fields = 4;
    desc.writes_scalars = kScalars;
    desc.n_writes_scalars = 2;
    (void)warpx_host_add_handler(host, &desc);
}

const WarpXPluginDesc kPlugin = {
    WARPX_PLUGIN_ABI_VERSION,
    sizeof(WarpXPluginDesc),
    "star_gravity",
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
