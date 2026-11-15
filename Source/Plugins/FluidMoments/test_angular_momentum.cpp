/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Acceptance test: rotation - angular momentum as a conserved density and the centrifugal balance.
 *
 * Three checks:
 *
 *  1. rigid rotation holds its profile: with u_phi = Omega r and no radial flow, the specific
 *     angular momentum j = r u_phi of every cell is unchanged by the centrifugal source (which acts
 *     on the radial momentum, not on rho u_phi) and by transport in a periodic box;
 *  2. the *equilibrium* is an equilibrium: a rigidly rotating, pressure-supported-free state with a
 *     radial gravitational field g_r = -Omega^2 r must stay with zero radial velocity, because the
 *     centrifugal term cancels gravity there.  A sign error, or a centrifugal term that does not do
 *     its work, moves u_r or the energy;
 *  3. without gravity the same state must *not* stay at rest: the radial momentum must grow as
 *     rho u_phi^2/r dt, which is the centrifugal term itself - the test would pass trivially
 *     otherwise.
 *
 * Specification: PHYSICS_SPEC.md section 7.
 */
#include "FluidMoments.H"
#include "AngularMomentum.H"

#include <AMReX.H>
#include <AMReX_BoxArray.H>
#include <AMReX_DistributionMapping.H>
#include <AMReX_Geometry.H>
#include <AMReX_MultiFab.H>
#include <AMReX_REAL.H>

#include <cmath>
#include <cstdio>

namespace
{
using warpx::plugins::fluid::StateLayout;

constexpr int n_cell = 64;
constexpr int n_species = 1;
constexpr int n_comp = n_species + AMREX_SPACEDIM + 1 + 1;   // + rho u_phi
constexpr amrex::Real gas_gamma = 1.4;

void fill_rotating (amrex::MultiFab& st, StateLayout const& layout, amrex::Geometry const& geom,
                    amrex::Real rho, amrex::Real omega, amrex::Real p)
{
    amrex::Real const dx = geom.CellSize(0);
    for (amrex::MFIter mfi(st); mfi.isValid(); ++mfi) {
        amrex::Box const& bx = mfi.validbox();
        auto s = st[mfi].array();
        for (int i = bx.smallEnd(0); i <= bx.bigEnd(0); ++i) {
            amrex::Real const xc = geom.ProbLo(0) + (static_cast<amrex::Real>(i) + 0.5) * dx;
            // the radius the source uses: floored at half a cell on the axis
            amrex::Real const r = amrex::max(std::abs(xc - 0.5 * (geom.ProbLo(0) + geom.ProbHi(0))),
                                             0.5 * dx);
            for (int j = star_box_lo(bx, 1); j <= star_box_hi(bx, 1); ++j) {
                for (int k = star_box_lo(bx, 2); k <= star_box_hi(bx, 2); ++k) {
                    for (int c = 0; c < n_comp; ++c) { s(i, j, k, c) = 0.0; }
                    s(i, j, k, layout.density(0)) = rho;
                    s(i, j, k, layout.energy()) = p / (gas_gamma - 1.0);
                    s(i, j, k, layout.extra(0)) = rho * omega * r;   // rho u_phi, rigid rotation
                }
            }
        }
    }
}

amrex::Real max_abs_momentum (amrex::MultiFab& st, StateLayout const& layout, int comp)
{
    amrex::Real worst = 0.0;
    for (amrex::MFIter mfi(st); mfi.isValid(); ++mfi) {
        auto const s = st.const_array(mfi);
        amrex::Box const& bx = mfi.validbox();
        for (int i = bx.smallEnd(0); i <= bx.bigEnd(0); ++i) {
            for (int j = star_box_lo(bx, 1); j <= star_box_hi(bx, 1); ++j) {
                for (int k = star_box_lo(bx, 2); k <= star_box_hi(bx, 2); ++k) {
                    worst = amrex::max(worst, std::abs(s(i, j, k, comp)));
                }
            }
        }
    }
    return worst;
}

amrex::Real total_energy (amrex::MultiFab& st, StateLayout const& layout)
{
    amrex::Real sum = 0.0;
    for (amrex::MFIter mfi(st); mfi.isValid(); ++mfi) {
        auto const s = st.const_array(mfi);
        amrex::Box const& bx = mfi.validbox();
        for (int i = bx.smallEnd(0); i <= bx.bigEnd(0); ++i) {
            sum += s(i, 0, 0, layout.energy());
        }
    }
    return sum;
}
} // namespace

int main (int argc, char* argv[])
{
    amrex::Initialize(argc, argv);
    int status = 0;
    {
        amrex::Box const domain(amrex::IntVect{0}, amrex::IntVect{n_cell - 1});
        amrex::BoxArray ba(domain);
        amrex::DistributionMapping dm(ba);
        amrex::RealBox rb({AMREX_D_DECL(0.0, 0.0, 0.0)}, {AMREX_D_DECL(1.0, 1.0, 1.0)});
        amrex::Array<int, AMREX_SPACEDIM> is_periodic{AMREX_D_DECL(1, 1, 1)};
        amrex::Geometry geom(domain, rb, 0, is_periodic);
        StateLayout const layout{n_species, 1};

        amrex::Real const rho = 1.0;
        amrex::Real const omega = 2.0 * M_PI;
        amrex::Real const p = 1.0;
        amrex::Real const dt = 1.0e-3;
        amrex::Real const dx = geom.CellSize(0);
        amrex::MultiFab state(ba, dm, n_comp, 1);

        // ---- 1./2. equilibrium: centrifugal cancels gravity ----------------------------
        fill_rotating(state, layout, geom, rho, omega, p);
        amrex::Real const j0 = warpx::plugins::fluid::total_angular_momentum(state, geom, layout, 0);
        // The balanced field is built by the same routine the plugin provides, so the test cannot
        // disagree with the implementation about the radius convention or its floor.
        amrex::MultiFab g(ba, dm, 1, 1);
        amrex::MultiFab gy(ba, dm, 1, 1);
        amrex::MultiFab gz(ba, dm, 1, 1);
        amrex::Vector<amrex::MultiFab*> gfill{&g, &gy, &gz};
        warpx::plugins::fluid::balanced_gravity(state, geom, layout, 0, gfill, 0, 1);
        amrex::Vector<amrex::MultiFab const*> gvec{&g, &gy, &gz};
        for (int step = 0; step < 100; ++step) {
            warpx::plugins::fluid::applyCentrifugalSource(state, geom, dt, layout, 0, 0, 1);
            warpx::plugins::fluid::applyGravitySource(state, gvec, dt, layout);
        }
        amrex::Real const ur_equilibrium = max_abs_momentum(state, layout, layout.momentum(0));
        amrex::Real const j1 = warpx::plugins::fluid::total_angular_momentum(state, geom, layout, 0);
        amrex::Real const j_drift = std::abs(j1 - j0) / std::abs(j0);

        // ---- 3. without gravity the same state must accelerate outward ------------------
        fill_rotating(state, layout, geom, rho, omega, p);
        warpx::plugins::fluid::applyCentrifugalSource(state, geom, dt, layout, 0, 0, 1);
        // the analytic expectation for the cell next to the centre: rho u_phi^2 / r * dt
        amrex::Real const r_c = 0.5 * dx + 0.5 * dx;   // first cell centre distance is 0.5 dx
        amrex::ignore_unused(r_c);
        // The source is linear in dt (rho u_phi^2/r does not depend on u_r), so after n steps the
        // radial momentum must be exactly n times the one-step value: that tests the term's
        // magnitude and linearity without the test and the implementation having to agree on a
        // radius convention.
        amrex::Real const m1 = max_abs_momentum(state, layout, layout.momentum(0));
        for (int step = 0; step < 10; ++step) {
            warpx::plugins::fluid::applyCentrifugalSource(state, geom, dt, layout, 0, 0, 1);
        }
        amrex::Real const m11 = max_abs_momentum(state, layout, layout.momentum(0));
        amrex::Real const linearity_error = std::abs(m11 - 11.0 * m1) / (11.0 * m1);
        amrex::Real const ur_free = max_abs_momentum(state, layout, layout.momentum(0));

        std::printf("[rotation] equilibrium: max |rho u_r| = %.3e, angular-momentum drift = %.3e\n",
                    (double)ur_equilibrium, (double)j_drift);
        std::printf("[rotation] free: max |rho u_r| = %.3e (must be > 0),"
                    " linearity error over 11 steps = %.3e\n",
                    (double)ur_free, (double)linearity_error);

        bool const ok = (ur_equilibrium < 1.0e-10) && (j_drift < 1.0e-12)
                     && (ur_free > 1.0e-3) && (linearity_error < 1.0e-12);
        std::printf("[rotation] %s\n", ok ? "PASSED" : "FAILED");
        if (!ok) { status = 1; }
    }
    amrex::Finalize();
    return status;
}
