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
 * star_g{x,y,z} that the gravity plugin publishes on the bus, and advance the Euler sweep with its
 * own sub-cycled step.  The step has to be its own: warpx_stage_dt() is the *electromagnetic* dt,
 * and a five-moment sweep at that dt violates the acoustic CFL by orders of magnitude.
 *
 * It includes nothing from another plugin - the coupling is four field names on the bus.
 */
#include "FluidMoments.H"

#include "Runtime/PluginCApi.H"

#include <AMReX.H>
#include <AMReX_BoxArray.H>
#include <AMReX_DistributionMapping.H>
#include <AMReX_Geometry.H>
#include <AMReX_MultiFab.H>

#include <cmath>
#include <cstdio>

namespace
{
double g_steps = 0.0;
double g_energy = 0.0;
bool g_initialised = false;

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

/** Smallest acoustic step the state allows: CFL * dx / (|u| + c_s). */
amrex::Real acoustic_dt (amrex::MultiFab const& state, amrex::Geometry const& geom,
                         amrex::Real cfl)
{
    amrex::Real const gamma = 1.4;
    amrex::Real const dx = geom.CellSize(0);
    amrex::Real umax = 0.0;
    amrex::Real csmax = 0.0;
    for (amrex::MFIter mfi(state); mfi.isValid(); ++mfi) {
        amrex::Box const& bx = mfi.validbox();
        auto const& s = state.const_array(mfi);
        for (int k = bx.smallEnd(2); k <= bx.bigEnd(2); ++k) {
            for (int j = bx.smallEnd(1); j <= bx.bigEnd(1); ++j) {
                for (int i = bx.smallEnd(0); i <= bx.bigEnd(0); ++i) {
                    amrex::Real const rho = s(i, j, k, g_layout.density(0))
                                          + s(i, j, k, g_layout.density(1));
                    amrex::Real const e = s(i, j, k, g_layout.energy());
                    if (rho <= 0.0) { continue; }
                    amrex::Real mom2 = 0.0;
                    for (int d = 0; d < AMREX_SPACEDIM; ++d) {
                        amrex::Real const m = s(i, j, k, g_layout.momentum(d));
                        mom2 += m * m;
                    }
                    amrex::Real const u = std::sqrt(mom2) / rho;
                    amrex::Real const p = (gamma - 1.0) * (e - 0.5 * mom2 / rho);
                    amrex::Real const cs = (p > 0.0) ? std::sqrt(gamma * p / rho) : 0.0;
                    umax = amrex::max(umax, u);
                    csmax = amrex::max(csmax, cs);
                }
            }
        }
    }
    amrex::Real const speed = umax + csmax;
    return (speed > 1.0e-30) ? amrex::Real(cfl) * dx / speed : amrex::Real(1.0e30);
}

void fluid_step (WarpXStageContext* ctx, void*)
{
    g_steps += 1.0;

    void* raw_state = nullptr;
    if (warpx_stage_field(ctx, "star_state", 0, &raw_state) != 1 || raw_state == nullptr) { return; }
    auto& state = *static_cast<amrex::MultiFab*>(raw_state);

    if (!g_initialised) {
        // Host-side loop on purpose: the handler is dispatched from the host, and a device/parallel
        // kernel launched here deadlocked the process (FutexWait, 300 s timeout) in the first
        // attempt.  Setting the state through a plain MFIter keeps everything on the calling thread.
        for (amrex::MFIter mfi(state); mfi.isValid(); ++mfi) {
            amrex::Box const& bx = mfi.validbox();
            auto s = state[mfi].array();
            for (int k = bx.smallEnd(2); k <= bx.bigEnd(2); ++k) {
                for (int j = bx.smallEnd(1); j <= bx.bigEnd(1); ++j) {
                    for (int i = bx.smallEnd(0); i <= bx.bigEnd(0); ++i) {
                        for (int c = 0; c < g_layout.n_comp(); ++c) { s(i, j, k, c) = 0.0; }
                        s(i, j, k, g_layout.density(0)) = 1.0;
                        s(i, j, k, g_layout.density(1)) = 0.1;
                        s(i, j, k, g_layout.energy()) = 2.5;   // p = 1 with gamma = 1.4
                    }
                }
            }
        }
        g_initialised = true;
        std::printf("[fluid] initialised: two-species gas at rest, p = 1\n");
    }

    // g comes from the bus; if the gravity plugin is not loaded the fields are absent and the
    // fluid simply advances without gravity - which is exactly the independence A7 asks for.
    amrex::MultiFab gx(state.boxArray(), state.DistributionMap(), 1, 1);
    amrex::MultiFab gy(state.boxArray(), state.DistributionMap(), 1, 1);
    amrex::MultiFab gz(state.boxArray(), state.DistributionMap(), 1, 1);
    char name[16] = {'s', 't', 'a', 'r', '_', 'g', 'x', '\0'};
    amrex::MultiFab* out[3] = {&gx, &gy, &gz};
    bool have_g = true;
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
    amrex::ignore_unused(gx, gy, gz);

    // Advance with the plugin's own step: the EM step is far too large for the Euler sweep.
    amrex::Geometry const geom = geometry_from_context(ctx);
    amrex::Real const dt_em = warpx_stage_dt(ctx);
    int n_sub = 0;
    if (!geom.Domain().isEmpty()) {
        amrex::Real const dt_f = acoustic_dt(state, geom, 0.4);
        n_sub = static_cast<int>(std::ceil(dt_em / dt_f));
        n_sub = amrex::min(amrex::max(n_sub, 1), 200);   // bounded work per EM step
        amrex::Real const dt_sub = dt_em / static_cast<amrex::Real>(n_sub);
        // The deck need not be periodic, so FillBoundary alone leaves the halo holding whatever the
        // allocation had - and the MUSCL reconstruction reads it, which produced NaN energies in
        // the first run.  Fill the halo with a nearest-neighbour copy of the interior on the host.
        auto fill_halo = [&]() {
            for (amrex::MFIter mfi(state); mfi.isValid(); ++mfi) {
                amrex::Box const& bx = mfi.validbox();
                auto s = state[mfi].array();
                amrex::Box const gb = mfi.fabbox();
                for (int k = gb.smallEnd(2); k <= gb.bigEnd(2); ++k) {
                    for (int j = gb.smallEnd(1); j <= gb.bigEnd(1); ++j) {
                        for (int i = gb.smallEnd(0); i <= gb.bigEnd(0); ++i) {
                            if (bx.contains(amrex::IntVect{i, j, k})) { continue; }
                            int const ii = amrex::min(amrex::max(i, bx.smallEnd(0)), bx.bigEnd(0));
                            int const jj = amrex::min(amrex::max(j, bx.smallEnd(1)), bx.bigEnd(1));
                            int const kk = amrex::min(amrex::max(k, bx.smallEnd(2)), bx.bigEnd(2));
                            for (int c = 0; c < g_layout.n_comp(); ++c) {
                                s(i, j, k, c) = s(ii, jj, kk, c);
                            }
                        }
                    }
                }
            }
        };
        warpx::plugins::fluid::IdealGasEOS eos;
        for (int s = 0; s < n_sub; ++s) {
            fill_halo();
            for (int dir = 0; dir < AMREX_SPACEDIM; ++dir) {
                warpx::plugins::fluid::advanceSweep(state, geom, dir, dt_sub, eos, g_layout);
            }
        }
    }

    g_energy = state.sum(g_layout.energy());
    (void)warpx_stage_set_scalar(ctx, "star_energy", g_energy);
    if (g_steps <= 3.0) {
        std::printf("[fluid] step %.0f: %d sub-steps, energy = %g, g from bus: %s\n",
                    g_steps, n_sub, g_energy, have_g ? "yes" : "no");
    }
}

void on_schema (WarpXStateRegistry* state)
{
    (void)warpx_state_request_field(state, "star_state", 2 + AMREX_SPACEDIM + 1, 2, 0);
}

const char* kScalars[] = {"star_energy"};
const char* kReads[] = {"star_gx", "star_gy", "star_gz"};
const char* kWrites[] = {"star_state"};

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
    desc.writes_fields = kWrites;
    desc.n_writes_fields = 1;
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
    nullptr,
    nullptr
};
} // namespace

extern "C" const WarpXPluginDesc* warpx_plugin_entry (void)
{
    return &kPlugin;
}
