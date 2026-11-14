/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Acceptance test: two species advected by a common velocity.
 *
 * The multi-species state carries one continuity equation per species and shares one velocity
 * and one pressure.  The cleanest check of that layout is a flow where the answer is known
 * exactly: uniform pressure and velocity, with one species carrying a density wave and the other
 * uniform.  Then each species obeys its own advection equation, so species 0 must reproduce its
 * travelling wave and species 1 must stay flat, while the total density is their sum and the
 * pressure stays at 1.
 *
 * Specification: PHYSICS_SPEC.md section 2.1 ("多组分：每个组分 s 有自己的 ρ_s 方程，共享 u 与 p").
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
    Real const pi = Real(3.14159265358979323846);
    Real const x_lo = Real(0.0);
    Real const x_hi = Real(1.0);
    Real const t_end = Real(0.5);
    Real const delta = Real(0.2);
    Real const mean_species_0 = Real(0.5);
    Real const mean_species_1 = Real(0.5);

    IdealGasEOS const eos{gamma};
    StateLayout const layout{2};

    std::vector<int> const grids{64, 128};
    std::vector<Real> l1_species0;
    std::vector<Real> l1_species1;

    std::printf("\n two species advected: rho_0 = 0.5(1 + %.1f sin 2pi x), rho_1 = 0.5, u = 1, p = 1\n",
                (double)delta);
    std::printf(" %6s %10s %14s %14s %14s\n", "n", "steps", "L1(rho_0)", "L1(rho_1)",
                "min rho_1");

    for (int const n : grids)
    {
        BoxArray ba;
        DistributionMapping dm;
        Geometry geom = test::makeSlab(x_lo, x_hi, n, ba, dm, /*periodic_x=*/true);
        Real const dx = geom.CellSize(0);
        Real const lo = geom.ProbLo(0);

        MultiFab state(ba, dm, layout.n_comp(), 2);
        for (MFIter mfi(state, TilingIfNotGPU()); mfi.isValid(); ++mfi)
        {
            Box const& bx = mfi.growntilebox(2);
            auto s = state.array(mfi);
            ParallelFor(bx, [=] AMREX_GPU_DEVICE (int i, int j, int k) noexcept
            {
                Real const x = lo + (Real(i) + Real(0.5)) * dx;
                Real const rho0 = mean_species_0
                    * (Real(1.0) + delta * std::sin(Real(2.0) * pi * x));
                Real const rho1 = mean_species_1;
                Real const total = rho0 + rho1;
                s(i, j, k, layout.density(0)) = rho0;
                s(i, j, k, layout.density(1)) = rho1;
                for (int d = 0; d < AMREX_SPACEDIM; ++d) {
                    s(i, j, k, layout.momentum(d)) = (d == 0) ? total : Real(0.0);
                }
                // p = 1 with u = 1: E = p/(gamma-1) + rho u^2/2.
                s(i, j, k, layout.energy()) = Real(1.0) / (gamma - Real(1.0))
                                            + Real(0.5) * total;
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
            state.FillBoundary(geom.periodicity());
            advanceSweep(state, geom, 0, dt, eos, layout);
            t += dt;
            ++steps;
        }

        Real e0 = 0.0;
        Real e1 = 0.0;
        Real min_rho1 = 1.e30;
        for (MFIter mfi(state); mfi.isValid(); ++mfi)
        {
            Box const& bx = mfi.validbox();
            auto const s = state.const_array(mfi);
            for (int i = bx.smallEnd(0); i <= bx.bigEnd(0); ++i) {
                Real const x = lo + (Real(i) + Real(0.5)) * dx;
                Real const exact0 = mean_species_0
                    * (Real(1.0) + delta * std::sin(Real(2.0) * pi * (x - t_end)));
                Real const rho0 = s(i, 0, 0, layout.density(0));
                Real const rho1 = s(i, 0, 0, layout.density(1));
                e0 += std::abs(rho0 - exact0) * dx;
                e1 += std::abs(rho1 - mean_species_1) * dx;
                min_rho1 = std::min(min_rho1, rho1);
            }
        }
        e0 /= (x_hi - x_lo);
        e1 /= (x_hi - x_lo);
        l1_species0.push_back(e0);
        l1_species1.push_back(e1);

        std::printf(" %6d %10d %14.3e %14.3e %14.3e\n", n, steps, (double)e0, (double)e1,
                    (double)min_rho1);
    }

    Real const order = test::convergenceOrder(l1_species0.front(), l1_species0.back(), Real(2.0));
    std::printf(" L1 order of species 0: %.2f   (require >= 1.8)\n", (double)order);

    int exit_code = 0;
    if (order < Real(1.8)) {
        std::printf(" FAILED: the species are not advected at second order\n");
        exit_code = 1;
    }
    // The uniform species must stay uniform and positive: nothing should leak between species.
    if (l1_species1.back() > Real(1.e-12)) {
        std::printf(" FAILED: the uniform species was disturbed (L1 %.3e)\n",
                    (double)l1_species1.back());
        exit_code = 1;
    }

    if (exit_code == 0) { std::printf("test_species_advection: ok\n"); }
    Finalize();
    return exit_code;
}
