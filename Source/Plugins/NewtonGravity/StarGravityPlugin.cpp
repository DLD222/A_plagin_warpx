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

#include <algorithm>
#include <cstdio>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

// These kernels walk a three-direction box; the helpers clamp the directions a lower-
// dimensional build does not have (see NewtonGravity.H).
using warpx::plugins::gravity::star_box_hi;
using warpx::plugins::gravity::star_box_lo;

namespace
{
double g_steps = 0.0;
double g_phi_min = 0.0;
static bool g_levels_filled = false;
static void fill_all_levels (WarpXStageContext* ctx);

/** Build the Geometry of a level from what the ABI exposes. */
amrex::Geometry geometry_from_context (WarpXStageContext* ctx, int lev = 0)
{
    int lo[3] = {0, 0, 0}, hi[3] = {0, 0, 0}, periodic[3] = {0, 0, 0};
    double prob_lo[3] = {0.0, 0.0, 0.0}, prob_hi[3] = {1.0, 1.0, 1.0}, dx[3] = {0.0, 0.0, 0.0};
    if (warpx_stage_geometry(ctx, lev, lo, hi, prob_lo, prob_hi, dx, periodic) != 1) {
        return amrex::Geometry();
    }
    // AMREX_D_DECL keeps this compiling in 1D (IntVect has exactly AMREX_SPACEDIM entries;
    // a 3-element brace list only works in 3D).
    amrex::Box const domain(amrex::IntVect{AMREX_D_DECL(lo[0], lo[1], lo[2])},
                            amrex::IntVect{AMREX_D_DECL(hi[0], hi[1], hi[2])});
    amrex::RealBox rb(prob_lo, prob_hi);
    amrex::Array<int, AMREX_SPACEDIM> is_periodic{};
    for (int d = 0; d < AMREX_SPACEDIM; ++d) { is_periodic[d] = periodic[d]; }
    return amrex::Geometry(domain, rb, 0 /* Cartesian */, is_periodic);
}

/** Publish g = -grad(phi) for one level on the bus (cgs, same units as the star state).
 *
 *  Does nothing when the level has no star_g* field: the bus exposes the fields the schema asked
 *  for, and the number of levels differs between runs.
 */
static void publish_acceleration (WarpXStageContext* ctx, int lev,
                                  amrex::MultiFab& phi, amrex::Geometry const& geom)
{
    amrex::MultiFab* g[3] = {nullptr, nullptr, nullptr};
    for (int d = 0; d < 3; ++d) {
        char name[16] = {'s', 't', 'a', 'r', '_', 'g', static_cast<char>('x' + d), '\0'};
        void* raw = nullptr;
        if (warpx_stage_field(ctx, name, lev, &raw) == 1 && raw != nullptr) {
            g[d] = static_cast<amrex::MultiFab*>(raw);
        }
    }
    if (g[0] == nullptr || g[1] == nullptr || g[2] == nullptr) { return; }
    warpx::plugins::gravity::NewtonGravitySolver::computeAcceleration(phi, *g[0], *g[1], *g[2], geom);
}

void gravity_step (WarpXStageContext* ctx, void*)
{
    g_steps += 1.0;
    if (!g_levels_filled) {
        g_levels_filled = true;
        fill_all_levels(ctx);
    }

    void* raw_rho = nullptr;
    void* raw_phi = nullptr;
    if (warpx_stage_read_field(ctx, "star_rho", 0, &raw_rho) != 1 || raw_rho == nullptr) { return; }
    if (warpx_stage_field(ctx, "star_phi", 0, &raw_phi) != 1 || raw_phi == nullptr) { return; }

    auto const& rho = *static_cast<amrex::MultiFab*>(raw_rho);
    auto& phi = *static_cast<amrex::MultiFab*>(raw_phi);
    amrex::Geometry geom = geometry_from_context(ctx);
    if (geom.Domain().isEmpty()) { return; }

    // The star state below is Gaussian cgs (the profile is read as g/cm^3 with r in cm, and the
    // fluid integrates in the same units), so the solve must use the cgs gravitational constant.
    // Passing the SI default here multiplies a cgs density by an SI G, which is 1e7 off and is
    // how the earlier version ended up with a potential that no acceleration could be derived
    // from consistently.
    warpx::plugins::gravity::NewtonGravitySolver::solvePotential(
        rho, phi, geom, 1.0e-13, warpx::plugins::gravity::PotentialBoundary::open,
        warpx::plugins::gravity::NewtonGravitySolver::gravitational_constant_cgs);
    g_phi_min = phi.min(0);

    // Hand the acceleration to the bus: the fluid layer reads these, never calls into this one.
    // g = -grad(phi) is what the fluid actually needs; the earlier version published zeros here,
    // so the self-gravity was solved every step and then silently dropped.
    publish_acceleration(ctx, 0, phi, geom);

    // Solve on every level.  This is a per-level solve, not a composite MLMG one: each level sees
    // its own density with its own geometry, so the coarse-fine correction of a true multilevel
    // operator (the reason MLMG exists) is missing.  On the solar case the levels are nested cubes
    // with the same centre, so the neglected term is a boundary effect of the fine levels - it is
    // recorded here as the limitation it is rather than described as a full AMR Poisson solve.
    for (int lev = 1; lev < 8; ++lev) {
        void* raw_rho_l = nullptr;
        try {
            if (warpx_stage_read_field(ctx, "star_rho", lev, &raw_rho_l) != 1 || raw_rho_l == nullptr) {
                break;
            }
        } catch (...) {
            break;
        }
        void* raw_phi_l = nullptr;
        try {
            if (warpx_stage_field(ctx, "star_phi", lev, &raw_phi_l) != 1 || raw_phi_l == nullptr) {
                break;
            }
        } catch (...) {
            break;
        }
        amrex::Geometry const geom_l = geometry_from_context(ctx, lev);
        if (geom_l.Domain().isEmpty()) { break; }
        try {
            warpx::plugins::gravity::NewtonGravitySolver::solvePotential(
                *static_cast<amrex::MultiFab*>(raw_rho_l),
                *static_cast<amrex::MultiFab*>(raw_phi_l), geom_l, 1.0e-13,
                warpx::plugins::gravity::PotentialBoundary::open,
                warpx::plugins::gravity::NewtonGravitySolver::gravitational_constant_cgs);
            publish_acceleration(ctx, lev, *static_cast<amrex::MultiFab*>(raw_phi_l), geom_l);
            double const phi_l = static_cast<amrex::MultiFab*>(raw_phi_l)->min(0);
            std::printf("[gravity] step %.0f level %d: phi_min = %g on %d cells\n",
                        g_steps, lev, phi_l, (int)geom_l.Domain().numPts());
        } catch (std::exception const& e) {
            std::printf("[gravity] step %.0f level %d: solve unavailable (%s)\n",
                        g_steps, lev, e.what());
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

// ---- spherical standard-solar-model initialisation, on every level the bus exposes --------
struct SsmRow { double r, rho, T, P; };

static std::vector<SsmRow> g_rows;
static bool g_loaded = false;

static void load_profile ()
{
    if (g_loaded) { return; }
    g_loaded = true;
    const char* path = std::getenv("WARPX_STAR_PROFILE");
    if (path == nullptr) { return; }
    std::ifstream in(path);
    if (!in) { std::printf("[gravity] cannot open %s\n", path); return; }
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#' || line.rfind("r_over_R", 0) == 0) { continue; }
        std::stringstream ss(line);
        std::string tok;
        std::vector<double> v;
        while (std::getline(ss, tok, ',')) { v.push_back(std::stod(tok)); }
        if (v.size() < 10) { continue; }
        g_rows.push_back(SsmRow{v[0] * 6.957e10, v[3], v[2], v[4]});   // r in cm, rho, T, P
    }
    // the table runs surface -> centre; rho_at() and r_star assume ascending r
    std::sort(g_rows.begin(), g_rows.end(),
              [](SsmRow const& a, SsmRow const& b) { return a.r < b.r; });
    std::printf("[gravity] profile loaded: %d rows, r in [%.3e, %.3e] cm\n",
                (int)g_rows.size(), g_rows.empty() ? 0.0 : g_rows.front().r,
                g_rows.empty() ? 0.0 : g_rows.back().r);
}

/** Density of the spherically symmetric model at radius r [cm], 0 outside the star. */
static double rho_at (double r, double r_star)
{
    if (g_rows.empty() || r >= r_star) { return 0.0; }
    if (r <= g_rows.front().r) { return g_rows.front().rho; }
    std::size_t hi = 1;
    while (hi < g_rows.size() && g_rows[hi].r < r) { ++hi; }
    if (hi >= g_rows.size()) { return g_rows.back().rho; }
    double const t = (r - g_rows[hi - 1].r) / (g_rows[hi].r - g_rows[hi - 1].r);
    return std::exp(std::log(g_rows[hi - 1].rho)
                    + t * (std::log(g_rows[hi].rho) - std::log(g_rows[hi - 1].rho)));
}

/** Fill star_rho on one level with the sphere and return the mass on that level. */
static double fill_sphere (WarpXStageContext* ctx, int lev, double r_star)
{
    void* raw = nullptr;
    // The bus throws when a level has no such field (the register stores the name, not the level).
    // The exception crosses the ABI as C++ and a caller in the plugin can catch it, which is how the
    // probe below finds out that there are no more levels; leaving it uncaught terminates the run.
    try {
        if (warpx_stage_field(ctx, "star_rho", lev, &raw) != 1 || raw == nullptr) { return -1.0; }
    } catch (...) {
        return -1.0;
    }
    auto& rho = *static_cast<amrex::MultiFab*>(raw);

    int lo[3] = {0, 0, 0}, hi[3] = {0, 0, 0}, periodic[3] = {0, 0, 0};
    double plo[3] = {0, 0, 0}, phi[3] = {0, 0, 0}, d[3] = {0, 0, 0};
    try {
        if (warpx_stage_geometry(ctx, lev, lo, hi, plo, phi, d, periodic) != 1) { return -1.0; }
    } catch (...) {
        return -1.0;
    }

    double mass = 0.0;
    amrex::Real const cell_volume = d[0] * d[1] * d[2];
    for (amrex::MFIter mfi(rho); mfi.isValid(); ++mfi) {
        amrex::Box const& bx = mfi.validbox();
        auto a = rho.array(mfi);
        for (int i = bx.smallEnd(0); i <= bx.bigEnd(0); ++i) {
            double const x = plo[0] + (i + 0.5) * d[0];
            for (int j = star_box_lo(bx, 1); j <= star_box_hi(bx, 1); ++j) {
                double const y = plo[1] + (j + 0.5) * d[1];
                for (int k = star_box_lo(bx, 2); k <= star_box_hi(bx, 2); ++k) {
                    double const z = plo[2] + (k + 0.5) * d[2];
                    double const r = std::sqrt(x * x + y * y + z * z);
                    double const value = rho_at(r, r_star);
                    a(i, j, k) = value;
                    mass += value * cell_volume;
                }
            }
        }
    }
    return mass;
}

/** Fill every level and report its mass.  Called from the first handler invocation, because the
 *  level boxes are only cut by the refinement tags after on_init has run. */
static void fill_all_levels (WarpXStageContext* ctx)
{
    load_profile();
    double const r_star = g_rows.empty() ? 0.0 : g_rows.back().r;
    for (int lev = 0; lev < 8; ++lev) {
        double const mass = fill_sphere(ctx, lev, r_star);
        if (mass < 0.0) { break; }
        void* raw = nullptr;
        if (warpx_stage_field(ctx, "star_rho", lev, &raw) == 1 && raw != nullptr) {
            auto& rho = *static_cast<amrex::MultiFab*>(raw);
            // The mass is what lies inside this level's own (already cut) box.  It is close to the
            // whole star on every level because the box is a *cube* centred on the origin: level 1
            // covers 12.5% of the domain volume, but its corners reach r = sqrt(3)/2 x 0.6 R_sun
            // ~ 1.04 R_sun, i.e. past the surface.  An earlier note claimed the levels were not cut
            // at all - the host's own summary shows they are (level 1: 12.5% of the domain, level 2:
            // 5.27%), and this comment records the correction.
            std::printf("[gravity] level %d: %d boxes, mass = %.4e g (%.6f M_sun)\n",
                        lev, (int)rho.boxArray().size(), mass, mass / 1.989e33);
        }
    }
}

void on_init (WarpXStageContext* ctx)
{
    load_profile();
    // the star's radius: the centre is the origin and the surface is where the model ends
    double const r_star = g_rows.empty() ? 0.0 : g_rows.back().r;
    for (int lev = 0; lev < 8; ++lev) {
        double const mass = fill_sphere(ctx, lev, r_star);
        if (mass < 0.0) { break; }   // no such level
        void* raw = nullptr;
        if (warpx_stage_field(ctx, "star_rho", lev, &raw) == 1 && raw != nullptr) {
            auto& rho = *static_cast<amrex::MultiFab*>(raw);
            std::printf("[gravity] level %d: %d boxes, mass = %.4e g (%.6f M_sun)\n",
                        lev, (int)rho.boxArray().size(), mass, mass / 1.989e33);
        }
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
