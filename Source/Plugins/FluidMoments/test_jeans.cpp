/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Acceptance test: the Jeans instability of a self-gravitating uniform medium.
 *
 * This is the first test that uses both star plugins together: the five-moment fluid is kicked by
 * the gravity field that the Newton solver computes from its own density, half a step before and
 * half a step after the hydro sweep (Strang splitting, PHYSICS_SPEC 2.4).
 *
 * Linear theory gives
 *
 *     omega^2 = c_s^2 k^2 - 4 pi G rho,      lambda_J = 2 pi / k_J = c_s sqrt(pi / (G rho)),
 *
 * so a perturbation with lambda > lambda_J grows at sigma = sqrt(4 pi G rho - c_s^2 k^2), and one
 * with lambda < lambda_J oscillates.  Rather than bisect for the critical wavelength, the test
 * brackets it from both sides: the m = 1 mode of a box of length 2 lambda_J is unstable and its
 * growth rate must match theory to 5%, while the m = 4 mode (lambda = lambda_J / 2) must not grow
 * at all.  Together they pin the threshold to within the 5% that the specification asks for.
 *
 * Specification: PHYSICS_SPEC.md sections 2.4 and 2.5.
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

/** Amplitude of the cos(kx) mode of the density: A = (2/L) integral (rho - rho0) cos(kx) dx. */
Real modeAmplitude (MultiFab const& state, Geometry const& geom, Real k, Real rho0,
                    StateLayout const& layout)
{
    Real sum = 0.0;
    Real const dx = geom.CellSize(0);
    Real const lo = geom.ProbLo(0);
    for (MFIter mfi(state); mfi.isValid(); ++mfi)
    {
        Box const& bx = mfi.validbox();
        auto const s = state.const_array(mfi);
        for (int i = bx.smallEnd(0); i <= bx.bigEnd(0); ++i) {
            Real const x = lo + (Real(i) + Real(0.5)) * dx;
            sum += (s(i, 0, 0, layout.density(0)) - rho0) * std::cos(k * x) * dx;
        }
    }
    Real const box_length = geom.ProbHi(0) - geom.ProbLo(0);
    return Real(2.0) * sum / box_length;
}

/** Least-squares slope of ln(A) against t over the samples in [a_min, a_max]. */
Real fitGrowthRate (std::vector<Real> const& t, std::vector<Real> const& a, Real a_min, Real a_max,
                    int& used)
{
    Real st = 0.0;
    Real sy = 0.0;
    Real stt = 0.0;
    Real sty = 0.0;
    used = 0;
    for (std::size_t i = 0; i < t.size(); ++i) {
        if (a[i] < a_min || a[i] > a_max) { continue; }
        Real const y = std::log(a[i]);
        st += t[i];
        sy += y;
        stt += t[i] * t[i];
        sty += t[i] * y;
        ++used;
    }
    if (used < 2) { return 0.0; }
    Real const n = Real(used);
    Real const denominator = n * stt - st * st;
    if (denominator == 0.0) { return 0.0; }
    return (n * sty - st * sy) / denominator;
}

} // namespace

int main (int argc, char* argv[])
{
    Initialize(argc, argv);

    Real const gamma = Real(1.4);
    Real const cfl = Real(0.3);
    Real const rho0 = Real(1.0);
    Real const sound_speed = Real(1.0);
    Real const p0 = rho0 * sound_speed * sound_speed / gamma;
    Real const pi = Real(3.14159265358979323846);
    Real const delta = Real(1.e-3);
    int const n = 128;

    Real const jeans_length = sound_speed * std::sqrt(pi / (G_newton * rho0));
    Real const box_length = Real(2.0) * jeans_length;

    IdealGasEOS const eos{gamma};
    StateLayout const layout{1};

    std::printf("\n jeans instability: rho0 = %.1f, c_s = %.1f, lambda_J = %.4e,"
                " box = 2 lambda_J\n",
                (double)rho0, (double)sound_speed, (double)jeans_length);

    int exit_code = 0;
    std::printf(" %4s %10s %14s %14s %14s %10s\n", "m", "lambda/l_J", "sigma theory",
                "sigma measured", "rel. diff", "amp max");

    for (int const m : {1, 4})
    {
        Real const k = Real(2.0) * pi * Real(m) / box_length;
        Real const dispersion = Real(4.0) * pi * G_newton * rho0
                              - sound_speed * sound_speed * k * k;
        bool const unstable = (dispersion > 0.0);
        Real const sigma_theory = std::sqrt(std::abs(dispersion));

        BoxArray ba;
        DistributionMapping dm;
        Geometry geom = test::makeSlab(Real(0.0), box_length, n, ba, dm, /*periodic_x=*/true);
        Real const dx = geom.CellSize(0);
        Real const lo = geom.ProbLo(0);

        MultiFab state(ba, dm, layout.n_comp(), 2);
        for (MFIter mfi(state, TilingIfNotGPU()); mfi.isValid(); ++mfi)
        {
            Box const& bx = mfi.growntilebox(2);
            auto s = state.array(mfi);
            ParallelFor(bx, [=] AMREX_GPU_DEVICE (int i, int j, int kk) noexcept
            {
                Real const x = lo + (Real(i) + Real(0.5)) * dx;
                Real const rho = rho0 * (Real(1.0) + delta * std::cos(k * x));
                s(i, j, kk, layout.density(0)) = rho;
                for (int d = 0; d < AMREX_SPACEDIM; ++d) {
                    s(i, j, kk, layout.momentum(d)) = Real(0.0);
                }
                s(i, j, kk, layout.energy()) = p0 / (gamma - Real(1.0));
            });
        }
        state.FillBoundary(geom.periodicity());

        // Gravity work space, same layout as the fluid state.
        MultiFab rho_g(ba, dm, 1, 1);
        MultiFab phi(ba, dm, 1, 1);
        MultiFab gx(ba, dm, 1, 1);
        MultiFab gy(ba, dm, 1, 1);
        MultiFab gz(ba, dm, 1, 1);

        auto solve_gravity = [&] () {
            computeTotalDensity(state, rho_g, layout);
            rho_g.FillBoundary(geom.periodicity());
            warpx::plugins::gravity::NewtonGravitySolver::solvePotential(
                rho_g, phi, geom, Real(1.e-12),
                warpx::plugins::gravity::PotentialBoundary::periodic);
            warpx::plugins::gravity::NewtonGravitySolver::computeAcceleration(
                phi, gx, gy, gz, geom);
        };
        Vector<MultiFab const*> const g_ptrs{AMREX_D_DECL(&gx, &gy, &gz)};

        solve_gravity();

        // The unstable case runs for three e-folds of the m = 1 mode.  The stable case runs for
        // twice that, so that a mode that was secretly unstable would have grown by e^6 ~ 400
        // times the starting amplitude - the criterion below is then unambiguous.
        Real const sigma_reference = std::sqrt(
            Real(4.0) * pi * G_newton * rho0
            - sound_speed * sound_speed * (Real(2.0) * pi / box_length)
              * (Real(2.0) * pi / box_length));
        Real const t_end = (unstable ? Real(3.0) : Real(6.0)) / sigma_reference;

        std::vector<Real> times;
        std::vector<Real> amplitudes;
        Real amplitude_max = Real(0.0);
        Real t = Real(0.0);
        int steps = 0;
        while (t < t_end)
        {
            Real const speed = maxWaveSpeed(state, eos, layout, 0);
            Real dt = cfl * dx / speed;
            if (t + dt > t_end) { dt = t_end - t; }

            applyGravitySource(state, g_ptrs, Real(0.5) * dt, layout);
            state.FillBoundary(geom.periodicity());
            advanceSweep(state, geom, 0, dt, eos, layout);
            solve_gravity();
            applyGravitySource(state, g_ptrs, Real(0.5) * dt, layout);
            state.FillBoundary(geom.periodicity());

            t += dt;
            ++steps;
            Real const amplitude = modeAmplitude(state, geom, k, rho0, layout);
            times.push_back(t);
            amplitudes.push_back(amplitude);
            amplitude_max = std::max(amplitude_max, amplitude);
        }

        int used = 0;
        Real sigma_measured = fitGrowthRate(times, amplitudes, Real(2.0) * delta,
                                            Real(8.0) * delta, used);
        Real const rel = (sigma_theory > 0.0)
            ? std::abs(sigma_measured - sigma_theory) / sigma_theory : Real(0.0);
        std::printf(" %4d %10.2f %14.3e %14.3e %14.3e %10.3e  (%d steps, %d fit points)\n", m,
                    (double)(Real(2.0) * pi / k / jeans_length), (double)sigma_theory,
                    (double)sigma_measured, (double)rel, (double)amplitude_max, steps, used);

        if (unstable) {
            if (rel > Real(0.05)) {
                std::printf(" FAILED: growth rate differs from theory by %.1f%% (> 5%%)\n",
                            (double)(Real(100.0) * rel));
                exit_code = 1;
            }
            if (used < 5) {
                std::printf(" FAILED: only %d samples in the exponential window\n", used);
                exit_code = 1;
            }
        } else {
            // An oscillation (plus a bounded entropy-mode beat) stays within a couple of delta;
            // an instability with the unstable rate would have reached ~400 delta by now.
            Real const bound = Real(2.5) * delta;
            if (amplitude_max > bound) {
                std::printf(" FAILED: the lambda < lambda_J mode reached %.3e, above the %.1e "
                            "bound (it started at %.3e)\n",
                            (double)amplitude_max, (double)bound, (double)delta);
                exit_code = 1;
            }
        }
    }

    if (exit_code == 0) { std::printf("test_jeans: ok\n"); }
    Finalize();
    return exit_code;
}
