/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Acceptance test: second-order accuracy on a smooth solution.
 *
 * A density wave in a gas moving at constant velocity,
 *
 *     rho = 1 + 0.2 sin(2 pi (x - t)/L),  u = 1,  p = 1,
 *
 * is an exact solution of the Euler equations: with uniform pressure and velocity the momentum
 * equation is satisfied identically and mass, momentum and energy all reduce to advection of the
 * density profile.  The domain is periodic and L = 1, so the wave simply travels around it.
 *
 * This is the test that pins the formal order of the scheme: the Sod test (gravity-free case)
 * contains a shock, whose L1 error falls at first order for *any* conservative scheme.
 *
 * Specification: PHYSICS_SPEC.md section 2.5 ("Sod 激波" and the second-order requirement).
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
    Real const t_end = Real(0.5);
    Real const pi = Real(3.14159265358979323846);

    std::vector<int> const grids{64, 128, 256};
    std::vector<Real> l1_rho;

    IdealGasEOS const eos{gamma};
    StateLayout const layout{1};

    std::printf("\n smooth density wave: rho = 1 + 0.2 sin(2 pi x), u = 1, p = 1, t = %.1f\n",
                (double)t_end);
    std::printf(" %6s %8s %13s\n", "n", "steps", "L1(rho)");

    for (int const n : grids)
    {
        BoxArray ba;
        DistributionMapping dm;
        Geometry geom = test::makeSlab(x_lo, x_hi, n, ba, dm, /*periodic_x=*/true);
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
                Real const rho = Real(1.0) + Real(0.2) * std::sin(Real(2.0) * pi * x);
                s(i, j, k, layout.density(0)) = rho;
                for (int d = 0; d < AMREX_SPACEDIM; ++d) {
                    s(i, j, k, layout.momentum(d)) = (d == 0) ? rho : Real(0.0);
                }
                s(i, j, k, layout.energy()) = Real(1.0) / (gamma - Real(1.0))
                                            + Real(0.5) * rho;
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

        Real error = 0.0;
        for (MFIter mfi(state); mfi.isValid(); ++mfi)
        {
            Box const& bx = mfi.validbox();
            auto const s = state.const_array(mfi);
            Real const lo = geom.ProbLo(0);
            for (int i = bx.smallEnd(0); i <= bx.bigEnd(0); ++i)
            {
                Real const x = lo + (Real(i) + Real(0.5)) * dx;
                // The wave has travelled a distance t_end at speed 1.
                Real const exact_rho =
                    Real(1.0) + Real(0.2) * std::sin(Real(2.0) * pi * (x - t_end));
                error += std::abs(s(i, 0, 0, layout.density(0)) - exact_rho) * dx;
            }
        }
        error /= (x_hi - x_lo);
        l1_rho.push_back(error);
        std::printf(" %6d %8d %13.3e\n", n, steps, (double)error);
    }

    Real const order_1 = test::convergenceOrder(l1_rho[0], l1_rho[1], Real(2.0));
    Real const order_2 = test::convergenceOrder(l1_rho[1], l1_rho[2], Real(2.0));
    std::printf(" L1 order: %.2f (n0->n1), %.2f (n1->n2)   (require >= 1.8)\n",
                (double)order_1, (double)order_2);

    int exit_code = 0;
    if (order_1 < Real(1.8) || order_2 < Real(1.8)) {
        std::printf(" FAILED: the scheme is not second order on a smooth solution\n");
        exit_code = 1;
    }
    if (exit_code == 0) { std::printf("test_smooth_wave: ok\n"); }
    Finalize();
    return exit_code;
}
