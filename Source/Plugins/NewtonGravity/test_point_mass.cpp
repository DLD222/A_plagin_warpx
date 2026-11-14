/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Acceptance test: point-mass potential with open (Dirichlet) boundaries.
 *
 * A mass M concentrated in a small sphere (radius 1.5 dx) must reproduce
 *
 *     phi(r) = -G M / r,     |g|(r) = G M / r^2
 *
 * outside it (PHYSICS_SPEC section 1.4).  The comparison is made far enough out that the
 * quadrupole and octupole of the source are below the discretisation error, and the box is big
 * enough that the Dirichlet data at the boundary (the exact -GM/r) is not what is being measured.
 *
 * Both grids are compared against the same physical radii: the source radius shrinks with dx,
 * which is exactly the point-mass limit, so the observed order is the convergence of the pair
 * (grid, source) towards a delta function.
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

    Real const mass = 1.0e30;            // [kg]
    Real const side = 1.0e11;            // [m]
    Real const source_radius_cells = Real(2.0);

    std::vector<int> const grids{64, 96};
    std::vector<Real> phi_errors;
    std::vector<Real> g_errors;
    std::vector<Real> phi_rms;
    std::vector<Real> g_rms;

    std::printf("\n point mass: M = %.3e kg, box = %.2e m, source radius = %.1f dx\n",
                (double)mass, (double)side, (double)source_radius_cells);
    std::printf(" %6s %12s %14s %14s %14s %14s\n", "n", "dx [m]", "max rel phi", "max rel |g|",
                "rms rel phi", "rms rel |g|");

    for (int const n : grids)
    {
        BoxArray ba;
        DistributionMapping dm;
        Geometry geom = test::makeCube(side, n, ba, dm);
        Real const dx = geom.CellSize(0);
        Real const source_radius = source_radius_cells * dx;
        Real const source_density =
            mass / (Real(4.0 / 3.0) * test::pi * source_radius * source_radius * source_radius);

        MultiFab rho(ba, dm, 1, 1);
        MultiFab phi(ba, dm, 1, 1);

        for (MFIter mfi(rho, TilingIfNotGPU()); mfi.isValid(); ++mfi)
        {
            Box const& bx = mfi.growntilebox(1);
            auto const rho_arr = rho.array(mfi);
            auto const prob_lo = geom.ProbLoArray();
            ParallelFor(bx, [=] AMREX_GPU_DEVICE (int i, int j, int k) noexcept
            {
                Real const x = prob_lo[0] + (Real(i) + Real(0.5)) * dx;
                Real const y = prob_lo[1] + (Real(j) + Real(0.5)) * dx;
                Real const z = prob_lo[2] + (Real(k) + Real(0.5)) * dx;
                rho_arr(i, j, k) = source_density * test::sphereVolumeFraction(
                    x, y, z, dx, Real(0.0), Real(0.0), Real(0.0), source_radius);
            });
        }
        rho.FillBoundary(geom.periodicity());

        // The source is only a few cells across, so its digitised mass is not exact.  The test is
        // about the *shape* of the far field, not about digitising a tiny sphere, so the density
        // is rescaled to carry exactly M.  (The uniform-sphere test deliberately does not do
        // this: there the volume fractions themselves are under test.)
        Real const mass_discrete = rho.sum(0) * dx * dx * dx;
        rho.mult(mass / mass_discrete, 0);

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
                phi_arr(i, j, k) = test::phiPointMass(r, mass);
            });
        }

        NewtonGravitySolver::solvePotential(rho, phi, geom, 1.e-12, PotentialBoundary::open);
        phi.FillBoundary(geom.periodicity());

        MultiFab gx(ba, dm, 1, 1);
        MultiFab gy(ba, dm, 1, 1);
        MultiFab gz(ba, dm, 1, 1);
        NewtonGravitySolver::computeAcceleration(phi, gx, gy, gz, geom);

        // Far enough out that the source looks like a point, and away from the boundary.
        test::CompareRegion region;
        region.bands = {{Real(0.30) * side, Real(0.47) * side}};
        region.surfaces = {source_radius};
        region.surface_margin = Real(2.0) * dx;
        region.boundary_margin = Real(3.0) * dx;

        auto const phi_exact = [&] (Real r) { return test::phiPointMass(r, mass); };
        auto const g_exact = [&] (Real r) { return mass * test::G_newton / (r * r); };

        test::ErrorStats const err_phi = test::comparePotential(phi, geom, phi_exact, region);
        test::ErrorStats const err_g = test::compareAcceleration(gx, gy, gz, geom, g_exact, region);

        phi_errors.push_back(err_phi.max_rel);
        g_errors.push_back(err_g.max_rel);
        phi_rms.push_back(err_phi.rms_rel);
        g_rms.push_back(err_g.rms_rel);

        std::printf(" %6d %12.3e %14.3e %14.3e %14.3e %14.3e\n", n, (double)dx,
                    (double)err_phi.max_rel, (double)err_g.max_rel,
                    (double)err_phi.rms_rel, (double)err_g.rms_rel);
    }

    Real const ratio = Real(grids[1]) / Real(grids[0]);
    Real const order_phi = test::convergenceOrder(phi_rms.front(), phi_rms.back(), ratio);
    Real const order_g = test::convergenceOrder(g_rms.front(), g_rms.back(), ratio);
    std::printf(" order from the RMS error: phi %.2f, |g| %.2f   (require >= 1.8)\n",
                (double)order_phi, (double)order_g);

    // The RMS relative error is the acceptance quantity, as in the uniform-sphere test: the
    // pointwise maximum of |g| is limited by the central-difference gradient, whose relative
    // truncation error is (dx/r)^2 and is largest at the inner edge of the band.
    int exit_code = 0;
    Real const tol = 1.e-3;
    Real const tol_max = 5.e-3;
    if (phi_rms.back() > tol) {
        std::printf(" FAILED: phi RMS relative error %.3e > %.1e\n",
                    (double)phi_rms.back(), (double)tol);
        exit_code = 1;
    }
    if (g_rms.back() > tol) {
        std::printf(" FAILED: |g| RMS relative error %.3e > %.1e\n",
                    (double)g_rms.back(), (double)tol);
        exit_code = 1;
    }
    if (g_errors.back() > tol_max) {
        std::printf(" FAILED: |g| max relative error %.3e > %.1e\n",
                    (double)g_errors.back(), (double)tol_max);
        exit_code = 1;
    }
    if (order_phi < 1.8) {
        std::printf(" FAILED: phi convergence order %.2f < 1.8\n", (double)order_phi);
        exit_code = 1;
    }
    if (order_g < 1.8) {
        std::printf(" FAILED: |g| convergence order %.2f < 1.8\n", (double)order_g);
        exit_code = 1;
    }

    if (exit_code == 0) { std::printf("test_point_mass: ok\n"); }
    Finalize();
    return exit_code;
}
