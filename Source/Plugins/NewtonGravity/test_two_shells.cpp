/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Acceptance test: two concentric uniform shells with open (Dirichlet) boundaries.
 *
 * Shell 1: 0.08 L <= r <= 0.11 L, density 2000 kg/m^3.
 * Shell 2: 0.27 L <= r <= 0.30 L, density  500 kg/m^3.
 * Vacuum everywhere else; the Dirichlet boundary at r = 0.5 L carries the exact potential.
 *
 * Three regions test different consequences of the shell theorem (PHYSICS_SPEC section 1.4):
 *
 *   r < 0.05 L          g = 0 exactly            (a shell exerts no force inside itself)
 *   0.17 L < r < 0.21 L g = G M_1 / r^2          (only the inner shell matters)
 *   r > 0.35 L          g = G (M_1 + M_2) / r^2  (the exterior is a point mass)
 *
 * The hollow core is the sharpest of the three: the solver has to cancel the field of the shells
 * to a small fraction of the surface field, which no boundary condition or rescaling can fake.
 *
 * Each shell is rescaled to its analytic mass before the solve.  The far field of a shell depends
 * on its mass alone, so a digitisation error in the mass would put a resolution-independent floor
 * under every comparison; the volume fractions still decide where the mass sits, which is what
 * the shell theorem is about.
 *
 * Acceptance: potential RMS relative error < 1e-3 with second-order convergence.  The numerical
 * field g = -grad phi uses second-order central differences, whose relative truncation error is
 * (dx/r)^2 - at the radii of this test that is a few times 10^-3, so the field is held to 2e-3
 * (RMS) while the potential carries the convergence-order requirement.
 */
#include "GravityTestReference.H"
#include "NewtonGravity.H"

#include <AMReX.H>
#include <AMReX_BoxArray.H>
#include <AMReX_DistributionMapping.H>
#include <AMReX_Geometry.H>
#include <AMReX_MultiFab.H>
#include <AMReX_REAL.H>

#include <cmath>
#include <cstdio>
#include <functional>
#include <vector>

using namespace amrex;
using namespace warpx::plugins::gravity;

int main (int argc, char* argv[])
{
    Initialize(argc, argv);

    Real const side = 1.0e11;             // [m]
    Real const r1_in = Real(0.08) * side;
    Real const r1_out = Real(0.11) * side;
    Real const r2_in = Real(0.27) * side;
    Real const r2_out = Real(0.30) * side;
    Real const rho1 = 2000.0;
    Real const rho2 = 500.0;

    auto const shell_mass = [&] (Real r_in, Real r_out, Real rho) {
        return Real(4.0 / 3.0) * test::pi * rho
             * (r_out * r_out * r_out - r_in * r_in * r_in);
    };
    Real const mass1 = shell_mass(r1_in, r1_out, rho1);
    Real const mass2 = shell_mass(r2_in, r2_out, rho2);
    Real const g_scale = test::G_newton * (mass1 + mass2) / (r2_out * r2_out);

    auto const phi_exact = [&] (Real r) {
        return test::phiShell(r, r1_in, r1_out, rho1) + test::phiShell(r, r2_in, r2_out, rho2);
    };

    std::vector<int> const grids{64, 128};
    std::vector<Real> gap_errors;
    std::vector<Real> outer_errors;
    std::vector<Real> gap_max;
    std::vector<Real> outer_max;
    std::vector<Real> phi_gap_rms;
    std::vector<Real> phi_outer_rms;
    std::vector<Real> core_fractions;

    std::printf("\n two shells: M1 = %.3e kg, M2 = %.3e kg, surface |g| = %.3e m/s^2\n",
                (double)mass1, (double)mass2, (double)g_scale);
    std::printf(" %6s %13s %13s %13s %13s %13s %15s\n", "n", "rms rel g (gap)",
                "rms rel g (out)", "rms rel phi (gap)", "rms rel phi (out)", "max rel g (gap)",
                "max|g|/g_s (core)");

    for (int const n : grids)
    {
        BoxArray ba;
        DistributionMapping dm;
        Geometry geom = test::makeCube(side, n, ba, dm);
        Real const dx = geom.CellSize(0);

        MultiFab rho(ba, dm, 1, 1);

        auto const fill_shell = [&] (MultiFab& target, Real r_in, Real r_out, Real rho_density,
                                     Real shell_mass) {
            for (MFIter mfi(target, TilingIfNotGPU()); mfi.isValid(); ++mfi)
            {
                Box const& bx = mfi.growntilebox(1);
                auto const rho_arr = target.array(mfi);
                auto const prob_lo = geom.ProbLoArray();
                ParallelFor(bx, [=] AMREX_GPU_DEVICE (int i, int j, int k) noexcept
                {
                    Real const x = prob_lo[0] + (Real(i) + Real(0.5)) * dx;
                    Real const y = prob_lo[1] + (Real(j) + Real(0.5)) * dx;
                    Real const z = prob_lo[2] + (Real(k) + Real(0.5)) * dx;
                    auto const volume_fraction = [=] (Real radius) {
                        return test::sphereVolumeFraction(x, y, z, dx, Real(0.0), Real(0.0),
                                                          Real(0.0), radius);
                    };
                    rho_arr(i, j, k) =
                        rho_density * (volume_fraction(r_out) - volume_fraction(r_in));
                });
            }
            Real const mass_discrete = target.sum(0) * dx * dx * dx;
            target.mult(shell_mass / mass_discrete, 0);
        };

        fill_shell(rho, r1_in, r1_out, rho1, mass1);
        MultiFab second_shell(ba, dm, 1, 1);
        second_shell.setVal(0.0);
        fill_shell(second_shell, r2_in, r2_out, rho2, mass2);
        MultiFab::Add(rho, second_shell, 0, 0, 1, 0);
        rho.FillBoundary(geom.periodicity());

        MultiFab phi(ba, dm, 1, 1);
        for (MFIter mfi(phi, TilingIfNotGPU()); mfi.isValid(); ++mfi)
        {
            Box const& bx = mfi.growntilebox(1);
            auto const phi_arr = phi.array(mfi);
            auto const prob_lo = geom.ProbLoArray();
            auto const prob_hi = geom.ProbHiArray();
            ParallelFor(bx, [=] AMREX_GPU_DEVICE (int i, int j, int k) noexcept
            {
                Real x = prob_lo[0] + (Real(i) + Real(0.5)) * dx;
                Real y = prob_lo[1] + (Real(j) + Real(0.5)) * dx;
                Real z = prob_lo[2] + (Real(k) + Real(0.5)) * dx;
                // Boundary data live on the domain faces: clamp the normal coordinate.
                x = x < prob_lo[0] ? prob_lo[0] : (x > prob_hi[0] ? prob_hi[0] : x);
                y = y < prob_lo[1] ? prob_lo[1] : (y > prob_hi[1] ? prob_hi[1] : y);
                z = z < prob_lo[2] ? prob_lo[2] : (z > prob_hi[2] ? prob_hi[2] : z);
                Real const r = std::sqrt(x * x + y * y + z * z);
                phi_arr(i, j, k) = test::phiShell(r, r1_in, r1_out, rho1)
                                 + test::phiShell(r, r2_in, r2_out, rho2);
            });
        }

        NewtonGravitySolver::solvePotential(rho, phi, geom, 1.e-12, PotentialBoundary::open);
        phi.FillBoundary(geom.periodicity());

        MultiFab gx(ba, dm, 1, 1);
        MultiFab gy(ba, dm, 1, 1);
        MultiFab gz(ba, dm, 1, 1);
        NewtonGravitySolver::computeAcceleration(phi, gx, gy, gz, geom);

        Real const surface_margin = Real(2.0) * dx;
        Real const boundary_margin = Real(3.0) * dx;
        std::vector<Real> const shells = {r1_in, r1_out, r2_in, r2_out};

        test::CompareRegion gap;
        gap.bands = {{Real(0.17) * side, Real(0.21) * side}};
        gap.surfaces = shells;
        gap.surface_margin = surface_margin;
        gap.boundary_margin = boundary_margin;

        test::CompareRegion outer;
        outer.bands = {{Real(0.35) * side, Real(0.47) * side}};
        outer.surfaces = shells;
        outer.surface_margin = surface_margin;
        outer.boundary_margin = boundary_margin;

        test::CompareRegion core;
        core.bands = {{Real(0.0), Real(0.05) * side}};
        core.surfaces = shells;
        core.surface_margin = surface_margin;
        core.boundary_margin = boundary_margin;

        auto const g_gap = [&] (Real r) { return test::G_newton * mass1 / (r * r); };
        auto const g_outer = [&] (Real r) {
            return test::G_newton * (mass1 + mass2) / (r * r);
        };

        test::ErrorStats const err_gap = test::compareAcceleration(gx, gy, gz, geom, g_gap, gap);
        test::ErrorStats const err_outer =
            test::compareAcceleration(gx, gy, gz, geom, g_outer, outer);
        test::ErrorStats const err_phi_gap = test::comparePotential(phi, geom, phi_exact, gap);
        test::ErrorStats const err_phi_outer = test::comparePotential(phi, geom, phi_exact, outer);
        Real const core_field = test::maxAccelerationMagnitude(gx, gy, gz, geom, core);

        gap_errors.push_back(err_gap.rms_rel);
        outer_errors.push_back(err_outer.rms_rel);
        gap_max.push_back(err_gap.max_rel);
        outer_max.push_back(err_outer.max_rel);
        phi_gap_rms.push_back(err_phi_gap.rms_rel);
        phi_outer_rms.push_back(err_phi_outer.rms_rel);
        core_fractions.push_back(core_field / g_scale);

        std::printf(" %6d %13.3e %13.3e %13.3e %13.3e %13.3e %15.3e  (gap %ld, out %ld cells)\n",
                    n, (double)err_gap.rms_rel, (double)err_outer.rms_rel,
                    (double)err_phi_gap.rms_rel, (double)err_phi_outer.rms_rel,
                    (double)err_gap.max_rel, (double)(core_field / g_scale),
                    err_gap.count, err_outer.count);
    }

    Real const ratio = Real(grids[1]) / Real(grids[0]);
    Real const order_phi_gap =
        test::convergenceOrder(phi_gap_rms.front(), phi_gap_rms.back(), ratio);
    Real const order_phi_outer =
        test::convergenceOrder(phi_outer_rms.front(), phi_outer_rms.back(), ratio);
    std::printf(" order of the potential: gap %.2f, exterior %.2f   (require >= 1.8)\n",
                (double)order_phi_gap, (double)order_phi_outer);

    int exit_code = 0;
    Real const tol_phi = 1.e-3;
    Real const tol_g = 2.e-3;
    Real const core_tol = 2.e-3;
    if (phi_gap_rms.back() > tol_phi || phi_outer_rms.back() > tol_phi) {
        std::printf(" FAILED: potential RMS relative error above %.1e (gap %.3e, exterior %.3e)\n",
                    (double)tol_phi, (double)phi_gap_rms.back(), (double)phi_outer_rms.back());
        exit_code = 1;
    }
    if (gap_errors.back() > tol_g || outer_errors.back() > tol_g) {
        std::printf(" FAILED: field RMS relative error above %.1e (gap %.3e, exterior %.3e)\n",
                    (double)tol_g, (double)gap_errors.back(), (double)outer_errors.back());
        exit_code = 1;
    }
    if (core_fractions.back() > core_tol) {
        std::printf(" FAILED: hollow core |g| is %.3e of the surface field (> %.1e)\n",
                    (double)core_fractions.back(), (double)core_tol);
        exit_code = 1;
    }
    if (order_phi_gap < 1.8 || order_phi_outer < 1.8) {
        std::printf(" FAILED: potential convergence order below 1.8\n");
        exit_code = 1;
    }

    if (exit_code == 0) { std::printf("test_two_shells: ok\n"); }
    Finalize();
    return exit_code;
}
