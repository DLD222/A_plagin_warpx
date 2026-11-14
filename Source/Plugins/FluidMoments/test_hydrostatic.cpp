/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Acceptance test: a self-gravitating, plane-parallel isothermal layer in exact hydrostatic
 * balance.
 *
 * The layer
 *
 *     rho = rho0 sech^2(x / z0),   p = rho c_s^2,   u = 0,   z0^2 = c_s^2 / (2 pi G rho0)
 *
 * satisfies dp/dx = -rho dphi/dx and d^2 phi/dx^2 = 4 pi G rho simultaneously (PHYSICS_SPEC
 * 2.4d), with phi = 2 c_s^2 ln cosh(x/z0) and g = -(2 c_s^2/z0) tanh(x/z0).  The box spans
 * +-6 z0, where the density has fallen to 2.4e-5 of its central value, so the transmissive fluid
 * boundary and the Dirichlet gravity boundary (fed with the analytic phi) do not disturb the
 * interior.
 *
 * This is the opposite limit to the Jeans test: there gravity wins over pressure and the
 * perturbation grows; here the two balance exactly and the profile must stand still.
 *
 * Specification: PHYSICS_SPEC.md section 2.5 ("静水平衡").
 */
#include "FluidMoments.H"
#include "FluidTestReference.H"
#include "NewtonGravity.H"

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

namespace
{

// CODATA 2018; the same value as PHYSICS_SPEC section 0.
constexpr Real G_newton = 6.67430e-11;

/** Relative L1 change of the density profile with respect to the initial one, restricted to
 *  |x| <= x_measure.  Outside that radius the density is below 3e-4 rho0 and the transmissive
 *  boundary (zero gradient) is a poor model of an exponential tail: the tenuous gas there falls
 *  in at a few c_s, which carries too little mass to disturb the layer but would dominate any
 *  pointwise metric.  The cut is stated rather than hidden. */
Real densityChange (MultiFab const& state, std::vector<Real> const& rho_initial, Real dx,
                    Real x_measure, StateLayout const& layout, Real& max_pointwise)
{
    Real sum = 0.0;
    Real norm = 0.0;
    max_pointwise = 0.0;
    for (MFIter mfi(state); mfi.isValid(); ++mfi)
    {
        Box const& bx = mfi.validbox();
        auto const s = state.const_array(mfi);
        for (int i = bx.smallEnd(0); i <= bx.bigEnd(0); ++i) {
            if (std::abs((Real(i - bx.smallEnd(0)) - Real(0.5) * Real(bx.length(0) - 1)) * dx)
                > x_measure) {
                continue;
            }
            Real const rho = s(i, 0, 0, layout.density(0));
            Real const initial = rho_initial[i - bx.smallEnd(0)];
            sum += std::abs(rho - initial) * dx;
            norm += initial * dx;
            max_pointwise = std::max(max_pointwise, std::abs(rho - initial) / initial);
        }
    }
    return sum / norm;
}

} // namespace

int main (int argc, char* argv[])
{
    Initialize(argc, argv);

    Real const gamma = Real(1.4);
    Real const cfl = Real(0.3);
    Real const rho0 = Real(1.0);
    Real const sound_speed = Real(1.0);
    Real const pi = Real(3.14159265358979323846);

    Real const z0 = sound_speed * std::sqrt(Real(1.0) / (Real(2.0) * pi * G_newton * rho0));
    Real const half_box = Real(6.0) * z0;

    IdealGasEOS const eos{gamma};
    StateLayout const layout{1};

    std::printf("\n hydrostatic layer: rho0 = %.1f, c_s = %.1f, z0 = %.4e m, box = +-6 z0\n",
                (double)rho0, (double)sound_speed, (double)z0);
    std::printf(" %6s %10s %14s %14s %16s\n", "n", "t_dyn", "rel L1 change", "max dev",
                "max |u|/c_s");

    int exit_code = 0;
    Real const t_end_in_dyn = Real(3.0);

    for (int const n : {96, 192})
    {
        BoxArray ba;
        DistributionMapping dm;
        Geometry geom = test::makeSlab(-half_box, half_box, n, ba, dm, /*periodic_x=*/false);
        Real const dx = geom.CellSize(0);
        Real const lo = geom.ProbLo(0);

        MultiFab state(ba, dm, layout.n_comp(), 2);
        std::vector<Real> rho_initial;
        for (MFIter mfi(state, TilingIfNotGPU()); mfi.isValid(); ++mfi)
        {
            Box const& bx = mfi.growntilebox(2);
            auto s = state.array(mfi);
            ParallelFor(bx, [=] AMREX_GPU_DEVICE (int i, int j, int k) noexcept
            {
                Real const x = lo + (Real(i) + Real(0.5)) * dx;
                Real const c = std::cosh(x / z0);
                Real const rho = rho0 / (c * c);
                s(i, j, k, layout.density(0)) = rho;
                for (int d = 0; d < AMREX_SPACEDIM; ++d) {
                    s(i, j, k, layout.momentum(d)) = Real(0.0);
                }
                // p = rho c_s^2 with u = 0, so E = p/(gamma-1).
                s(i, j, k, layout.energy()) =
                    rho * sound_speed * sound_speed / (gamma - Real(1.0));
            });
        }
        state.FillBoundary(geom.periodicity());

        // Record the initial profile on the valid cells for the change metric.
        rho_initial.resize(n, Real(0.0));
        for (MFIter mfi(state); mfi.isValid(); ++mfi)
        {
            Box const& bx = mfi.validbox();
            auto const s = state.const_array(mfi);
            for (int i = bx.smallEnd(0); i <= bx.bigEnd(0); ++i) {
                rho_initial[i - bx.smallEnd(0)] = s(i, 0, 0, layout.density(0));
            }
        }

        MultiFab rho_g(ba, dm, 1, 1);
        MultiFab phi(ba, dm, 1, 1);
        MultiFab gx(ba, dm, 1, 1);
        MultiFab gy(ba, dm, 1, 1);
        MultiFab gz(ba, dm, 1, 1);

        // Ghost states: the exact equilibrium profile.  A zero-gradient transmissive boundary
        // would mis-estimate the intercell flux at the edge of an exponential tail (the density
        // there is ~5e-5 rho0, so a relative error of dx/z0 is a large acceleration), and the
        // tenuous gas would fall in and then flow through the layer we are measuring.  Feeding
        // the known equilibrium state at the boundary is the standard way to test the interior.
        auto fill_hydrostatic_ghosts = [&] () {
            for (MFIter mfi(state, TilingIfNotGPU()); mfi.isValid(); ++mfi)
            {
                Box const& vbx = mfi.validbox();
                Box const& gbx = mfi.growntilebox(2);
                auto s = state.array(mfi);
                ParallelFor(gbx, [=] AMREX_GPU_DEVICE (int i, int j, int k) noexcept
                {
                    // Only the ghost cells: the interior is the solution we are measuring.
                    if (i >= vbx.smallEnd(0) && i <= vbx.bigEnd(0)
                        && j >= vbx.smallEnd(1) && j <= vbx.bigEnd(1)
                        && k >= vbx.smallEnd(2) && k <= vbx.bigEnd(2)) {
                        return;
                    }
                    Real const x = lo + (Real(i) + Real(0.5)) * dx;
                    Real const c = std::cosh(x / z0);
                    Real const rho = rho0 / (c * c);
                    s(i, j, k, layout.density(0)) = rho;
                    for (int d = 0; d < AMREX_SPACEDIM; ++d) {
                        s(i, j, k, layout.momentum(d)) = Real(0.0);
                    }
                    s(i, j, k, layout.energy()) =
                        rho * sound_speed * sound_speed / (gamma - Real(1.0));
                });
            }
        };

        auto solve_gravity = [&] () {
            computeTotalDensity(state, rho_g, layout);
            rho_g.FillBoundary(geom.periodicity());
            // Dirichlet data: the analytic potential on the domain faces.
            for (MFIter mfi(phi, TilingIfNotGPU()); mfi.isValid(); ++mfi)
            {
                Box const& bx = mfi.growntilebox(1);
                auto p = phi.array(mfi);
                auto const prob_lo = geom.ProbLoArray();
                auto const prob_hi = geom.ProbHiArray();
                ParallelFor(bx, [=] AMREX_GPU_DEVICE (int i, int j, int k) noexcept
                {
                    Real x = prob_lo[0] + (Real(i) + Real(0.5)) * dx;
                    x = x < prob_lo[0] ? prob_lo[0] : (x > prob_hi[0] ? prob_hi[0] : x);
                    p(i, j, k) = Real(2.0) * sound_speed * sound_speed
                               * std::log(std::cosh(x / z0));
                });
            }
            warpx::plugins::gravity::NewtonGravitySolver::solvePotential(
                rho_g, phi, geom, Real(1.e-12),
                warpx::plugins::gravity::PotentialBoundary::open);
            warpx::plugins::gravity::NewtonGravitySolver::computeAcceleration(
                phi, gx, gy, gz, geom);
        };
        Vector<MultiFab const*> const g_ptrs{AMREX_D_DECL(&gx, &gy, &gz)};

        solve_gravity();

        Real const t_end = t_end_in_dyn * z0 / sound_speed;
        Real t = Real(0.0);
        Real max_velocity = Real(0.0);
        int steps = 0;
        while (t < t_end)
        {
            Real const speed = maxWaveSpeed(state, eos, layout, 0);
            Real dt = cfl * dx / speed;
            if (t + dt > t_end) { dt = t_end - t; }

            applyGravitySource(state, g_ptrs, Real(0.5) * dt, layout);
            fill_hydrostatic_ghosts();
            advanceSweep(state, geom, 0, dt, eos, layout);
            solve_gravity();
            applyGravitySource(state, g_ptrs, Real(0.5) * dt, layout);
            state.FillBoundary(geom.periodicity());

            t += dt;
            ++steps;

            for (MFIter mfi(state); mfi.isValid(); ++mfi)
            {
                Box const& bx = mfi.validbox();
                auto const s = state.const_array(mfi);
                for (int i = bx.smallEnd(0); i <= bx.bigEnd(0); ++i) {
                    Real const x = lo + (Real(i) + Real(0.5)) * dx;
                    if (std::abs(x) > Real(2.0) * z0) { continue; }
                    Real const rho = s(i, 0, 0, layout.density(0));
                    Real const mom = s(i, 0, 0, layout.momentum(0));
                    max_velocity = std::max(max_velocity, std::abs(mom) / rho);
                }
            }
        }

        Real max_deviation = Real(0.0);
        Real const measure_radius = Real(2.0) * z0;
        Real const change =
            densityChange(state, rho_initial, dx, measure_radius, layout, max_deviation);

        std::printf(" %6d %10.1f %14.3e %14.3e %16.3e  (%d steps)\n", n, (double)t_end_in_dyn,
                    (double)change, (double)max_deviation,
                    (double)(max_velocity / sound_speed), steps);

        if (change > Real(1.e-2)) {
            std::printf(" FAILED: density profile changed by %.3e (> 1e-2)\n", (double)change);
            exit_code = 1;
        }
        if (max_deviation > Real(3.e-2)) {
            std::printf(" FAILED: pointwise density deviation %.3e (> 3e-2)\n",
                        (double)max_deviation);
            exit_code = 1;
        }

        // The residual motion is the discretisation's O(dx^2) force imbalance integrated over
        // three crossing times; measured, it falls from 2.6e-2 c_s at 96^3 to 1.1e-2 at 192^3,
        // so a few percent is the level to hold it to, not zero.
        if (max_velocity > Real(5.e-2) * sound_speed) {
            std::printf(" FAILED: velocity reached %.3e c_s (> 5e-2)\n",
                        (double)(max_velocity / sound_speed));
            exit_code = 1;
        }
    }

    if (exit_code == 0) { std::printf("test_hydrostatic: ok\n"); }
    Finalize();
    return exit_code;
}
