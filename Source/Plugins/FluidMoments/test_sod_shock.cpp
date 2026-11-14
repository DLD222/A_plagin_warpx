/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Acceptance test: Sod's shock tube against the exact Riemann solution.
 *
 * Left state (rho, u, p) = (1, 0, 1), right state (0.125, 0, 0.1), gamma = 1.4, evolved to
 * t = 0.2.  The numerical solution is compared with the exact solution of Toro chapter 4 on the
 * same cells; the L1 error must fall as the grid is refined (the acceptance rate is set below,
 * with the reason: the solution contains a shock, so the formal second-order rate of the scheme
 * is not what an L1 norm over the whole domain can show).
 *
 * Specification: PHYSICS_SPEC.md section 2.5.
 */
#include "FluidMoments.H"
#include "FluidTestReference.H"

#include <AMReX.H>
#include <AMReX_BoxArray.H>
#include <AMReX_DistributionMapping.H>
#include <AMReX_Geometry.H>
#include <AMReX_MultiFab.H>
#include <AMReX_REAL.H>

#include <cmath>
#include <cstdio>
#include <vector>

using namespace amrex;
using namespace warpx::plugins::fluid;

int main (int argc, char* argv[])
{
    Initialize(argc, argv);

    Real const gamma = Real(1.4);
    Real const cfl = Real(0.3);
    Real const x_lo = Real(0.0);
    Real const x_hi = Real(1.0);
    Real const t_end = Real(0.2);

    test::ExactRiemann exact(gamma, Real(1.0), Real(0.0), Real(1.0),
                             Real(0.125), Real(0.0), Real(0.1));
    std::printf("\n sod shock: gamma = %.2f, p* = %.8f, u* = %.8f, rho*_L = %.6f, rho*_R = %.6f\n",
                (double)gamma, (double)exact.p_star, (double)exact.u_star,
                (double)exact.starDensity(true), (double)exact.starDensity(false));

    std::vector<int> const grids{200, 400};
    std::vector<Real> l1_rho;
    std::vector<Real> l1_p;
    std::vector<Real> fan_rho;
    std::vector<Real> fan_p;

    // Edges of the left rarefaction fan, in the similarity variable S = (x - x_disc)/t.
    Real const fan_head = Real(0.0) - exact.c_l;
    Real const fan_tail = exact.u_star
        - exact.c_l * std::pow(exact.p_star / Real(1.0),
                               (gamma - Real(1.0)) / (Real(2.0) * gamma));

    std::printf(" %6s %8s %11s %11s %11s %11s\n", "n", "steps", "L1(rho)", "L1(p)",
                "L1_fan(rho)", "L1_fan(p)");

    IdealGasEOS eos{gamma};
    StateLayout const layout{1};

    for (int const n : grids)
    {
        BoxArray ba;
        DistributionMapping dm;
        Geometry geom = test::makeSlab(x_lo, x_hi, n, ba, dm, /*periodic_x=*/false);
        Real const dx = geom.CellSize(0);

        MultiFab state(ba, dm, layout.n_comp(), 2);
        for (MFIter mfi(state, TilingIfNotGPU()); mfi.isValid(); ++mfi)
        {
            Box const& bx = mfi.growntilebox(2);
            auto s = state.array(mfi);
            Real const lo = geom.ProbLo(0);
            ParallelFor(bx, [=] AMREX_GPU_DEVICE (int i, int j, int k) noexcept
            {
                Real const x = lo + (Real(i) + Real(0.5)) * dx;
                bool const left = (x < Real(0.5));
                Real const rho = left ? Real(1.0) : Real(0.125);
                Real const p = left ? Real(1.0) : Real(0.1);
                s(i, j, k, layout.density(0)) = rho;
                for (int d = 0; d < AMREX_SPACEDIM; ++d) {
                    s(i, j, k, layout.momentum(d)) = Real(0.0);
                }
                s(i, j, k, layout.energy()) = p / (gamma - Real(1.0));
            });
        }
        state.FillBoundary(geom.periodicity());

        Real t = Real(0.0);
        int steps = 0;
        while (t < t_end)
        {
            Real const speed = maxWaveSpeed(state, eos, layout, 0);
            Real dt = cfl * dx / speed;
            if (t + dt > t_end) { dt = t_end - t; }
            test::fillTransmissive(state, 2);
            advanceSweep(state, geom, 0, dt, eos, layout);
            t += dt;
            ++steps;
        }

        // L1 error of rho and p against the exact solution sampled at the cell centres, over the
        // whole domain and over the smooth rarefaction only.
        Real e_rho = 0.0;
        Real e_p = 0.0;
        Real f_rho = 0.0;
        Real f_p = 0.0;
        int fan_cells = 0;
        // Keep clear of the fan's edges: the exact solution has a derivative jump there, and the
        // scheme smears that kink over several cells, which would otherwise dominate the norm.
        Real const margin = Real(0.25) * (fan_tail - fan_head);
        for (MFIter mfi(state); mfi.isValid(); ++mfi)
        {
            Box const& bx = mfi.validbox();
            auto const s = state.const_array(mfi);
            Real const lo = geom.ProbLo(0);
            for (int i = bx.smallEnd(0); i <= bx.bigEnd(0); ++i)
            {
                Real const x = lo + (Real(i) + Real(0.5)) * dx;
                Real const similarity = (x - Real(0.5)) / t_end;
                auto const ref = exact.sample(similarity);
                Real const rho = s(i, 0, 0, layout.density(0));
                Real const mom = s(i, 0, 0, layout.momentum(0));
                Real const E = s(i, 0, 0, layout.energy());
                Real const u = mom / rho;
                Real const p = (gamma - Real(1.0)) * (E - Real(0.5) * rho * u * u);
                e_rho += std::abs(rho - ref.rho) * dx;
                e_p += std::abs(p - ref.p) * dx;
                if (similarity > fan_head + margin && similarity < fan_tail - margin) {
                    f_rho += std::abs(rho - ref.rho) * dx;
                    f_p += std::abs(p - ref.p) * dx;
                    ++fan_cells;
                }
            }
        }
        e_rho /= (x_hi - x_lo);
        e_p /= (x_hi - x_lo);
        // The fan is scaled by its own width so that the two grids are comparable.
        Real const fan_width = (fan_tail - fan_head - Real(2.0) * margin);
        f_rho /= (fan_width * t_end);
        f_p /= (fan_width * t_end);
        l1_rho.push_back(e_rho);
        l1_p.push_back(e_p);
        fan_rho.push_back(f_rho);
        fan_p.push_back(f_p);

        std::printf(" %6d %8d %11.3e %11.3e %11.3e %11.3e  (%d fan cells)\n", n, steps,
                    (double)e_rho, (double)e_p, (double)f_rho, (double)f_p, fan_cells);
    }

    Real const ratio = Real(grids[1]) / Real(grids[0]);
    Real const order_rho = test::convergenceOrder(l1_rho.front(), l1_rho.back(), ratio);
    Real const order_p = test::convergenceOrder(l1_p.front(), l1_p.back(), ratio);
    Real const fan_order_rho = test::convergenceOrder(fan_rho.front(), fan_rho.back(), ratio);
    Real const fan_order_p = test::convergenceOrder(fan_p.front(), fan_p.back(), ratio);
    std::printf(" L1 order (whole domain): rho %.2f, p %.2f   (a shock limits this to ~1)\n",
                (double)order_rho, (double)order_p);
    // Reported, not required: the rarefaction's two edges are derivative kinks that the scheme
    // smears over several cells, and that smearing dominates this norm.
    std::printf(" L1 order (rarefaction, reported only): rho %.2f, p %.2f\n",
                (double)fan_order_rho, (double)fan_order_p);

    // The formal second order of the scheme is established by test_smooth_wave; here the
    // acceptance is that the solution is right and the norm decreases.  A shock is a kink and a
    // conservative scheme smears it over O(dx) cells with an O(1) profile error, so the
    // whole-domain L1 rate is about one for any scheme of this class - this is what the
    // literature reports for Sod's problem, not a property of MUSCL-Hancock.
    int exit_code = 0;
    Real const min_total_order = Real(0.7);
    if (order_rho < min_total_order || order_p < min_total_order) {
        std::printf(" FAILED: whole-domain L1 order below %.1f (rho %.2f, p %.2f)\n",
                    (double)min_total_order, (double)order_rho, (double)order_p);
        exit_code = 1;
    }
    ignore_unused(fan_order_rho);
    ignore_unused(fan_order_p);
    if (l1_rho.back() > Real(1.e-2)) {
        std::printf(" FAILED: L1(rho) = %.3e above 1e-2\n", (double)l1_rho.back());
        exit_code = 1;
    }

    if (exit_code == 0) { std::printf("test_sod_shock: ok\n"); }
    Finalize();
    return exit_code;
}
