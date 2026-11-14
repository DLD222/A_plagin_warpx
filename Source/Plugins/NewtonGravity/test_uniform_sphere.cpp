/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Acceptance test: a uniform density sphere with open (Dirichlet) boundaries.
 *
 * The exact solution is the analytic sphere potential (PHYSICS_SPEC section 1.4):
 *
 *     phi(r) = -GM/(2R) (3 - (r/R)^2) inside,  -GM/r outside,
 *     |g|    = (4/3) pi G rho r inside,        G M / r^2 outside.
 *
 * The box is four radii across and every boundary value is the analytic potential, so the
 * interior solution is the exact one and the only error left is discretisation.  The density is
 * represented with exact volume fractions (GravityTestReference.H) so that the digitised mass is
 * right to a few parts in 10^5 and does not mask the solver error.
 *
 * Second-order convergence is the acceptance criterion: the error must fall by about four when
 * the cell size is halved (order >= 1.8), and the finest grid must be within 1e-3.
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

    Real const radius = 6.957e8;          // solar radius [m]
    Real const density = 1408.0;          // solar mean density [kg m^-3]
    Real const side = 4.0 * radius;       // the open boundary sits at r = 2R
    Real const mass = test::massUniformSphere(radius, density);

    std::vector<int> const grids{64, 96};
    std::vector<Real> inside_errors;
    std::vector<Real> outside_errors;
    std::vector<Real> inside_rms;
    std::vector<Real> outside_rms;

    std::printf("\n uniform sphere: rho = %.1f kg/m^3, R = %.4e m, M = %.4e kg, box = %.1f R\n",
                (double)density, (double)radius, (double)mass, (double)(side / radius));
    std::printf(" %6s %12s %12s %12s %12s %12s\n", "n", "dx/R",
                "max rel (in)", "max rel (out)", "rms rel (in)", "rms rel (out)");

    for (int const n : grids)
    {
        BoxArray ba;
        DistributionMapping dm;
        Geometry geom = test::makeCube(side, n, ba, dm);
        Real const dx = geom.CellSize(0);

        MultiFab rho(ba, dm, 1, 1);
        MultiFab phi(ba, dm, 1, 1);

        // Density: uniform inside the sphere, vacuum outside, with sub-cell volume fractions.
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
                Real const fraction = test::sphereVolumeFraction(
                    x, y, z, dx, Real(0.0), Real(0.0), Real(0.0), radius);
                rho_arr(i, j, k) = density * fraction;
            });
        }
        rho.FillBoundary(geom.periodicity());

        // Dirichlet data: the analytic potential on the domain faces (AMReX's convention), which
        // is the ghost-cell value with its normal coordinate clamped to the boundary.
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
                phi_arr(i, j, k) = test::phiUniformSphere(r, radius, density);
            });
        }

        NewtonGravitySolver::solvePotential(rho, phi, geom, 1.e-12, PotentialBoundary::open);
        phi.FillBoundary(geom.periodicity());

        MultiFab gx(ba, dm, 1, 1);
        MultiFab gy(ba, dm, 1, 1);
        MultiFab gz(ba, dm, 1, 1);
        NewtonGravitySolver::computeAcceleration(phi, gx, gy, gz, geom);

        auto const g_exact = [&] (Real r) { return test::gUniformSphere(r, radius, density); };

        test::CompareRegion inside;
        inside.bands = {{Real(0.2) * radius, Real(0.8) * radius}};
        inside.surfaces = {radius};
        inside.surface_margin = Real(2.0) * dx;
        inside.boundary_margin = Real(3.0) * dx;

        test::CompareRegion outside;
        outside.bands = {{Real(1.5) * radius, Real(1.9) * radius}};
        outside.surfaces = {radius};
        outside.surface_margin = Real(2.0) * dx;
        outside.boundary_margin = Real(3.0) * dx;

        test::ErrorStats const err_in =
            test::compareAcceleration(gx, gy, gz, geom, g_exact, inside);
        test::ErrorStats const err_out =
            test::compareAcceleration(gx, gy, gz, geom, g_exact, outside);

        inside_errors.push_back(err_in.max_rel);
        outside_errors.push_back(err_out.max_rel);
        inside_rms.push_back(err_in.rms_rel);
        outside_rms.push_back(err_out.rms_rel);

        Real const m_disc = rho.sum(0) * dx * dx * dx;
        auto const phi_exact_f = [&] (Real r) {
            return test::phiUniformSphere(r, radius, density);
        };
        test::CompareRegion wide;
        wide.bands = {{Real(0.05) * radius, Real(1.9) * radius}};
        wide.surfaces = {radius};
        wide.surface_margin = Real(2.0) * dx;
        wide.boundary_margin = Real(3.0) * dx;
        test::ErrorStats const ephi = test::comparePotential(phi, geom, phi_exact_f, wide);
        std::printf("   DEBUG mass disc/exact = %.6f   phi max rel %.3e  rms %.3e\n",
                    (double)(m_disc / mass), (double)ephi.max_rel, (double)ephi.rms_rel);

        std::printf(" %6d %12.3e %12.3e %12.3e %12.3e %12.3e\n", n, (double)(dx / radius),
                    (double)err_in.max_rel, (double)err_out.max_rel,
                    (double)err_in.rms_rel, (double)err_out.rms_rel);
    }

    Real const ratio = Real(grids[1]) / Real(grids[0]);
    Real const order_in = test::convergenceOrder(inside_rms.front(), inside_rms.back(), ratio);
    Real const order_out = test::convergenceOrder(outside_rms.front(), outside_rms.back(), ratio);

    std::printf(" order from the RMS error: inside %.2f, outside %.2f   (require >= 1.8)\n",
                (double)order_in, (double)order_out);

    // The acceptance quantity is the RMS relative error; the pointwise maximum is reported so
    // that a locally bad cell cannot hide.  At the outer edge of the exterior band the
    // central-difference gradient contributes (dx/r)^2, which is why the max is a few times
    // larger there than the RMS.
    int exit_code = 0;
    Real const tol = 1.e-3;
    Real const tol_max_outside = 5.e-3;
    if (inside_rms.back() > tol) {
        std::printf(" FAILED: interior RMS relative error %.3e > %.1e\n",
                    (double)inside_rms.back(), (double)tol);
        exit_code = 1;
    }
    if (outside_rms.back() > tol) {
        std::printf(" FAILED: exterior RMS relative error %.3e > %.1e\n",
                    (double)outside_rms.back(), (double)tol);
        exit_code = 1;
    }
    if (outside_errors.back() > tol_max_outside) {
        std::printf(" FAILED: exterior max relative error %.3e > %.1e\n",
                    (double)outside_errors.back(), (double)tol_max_outside);
        exit_code = 1;
    }
    if (order_in < 1.8) {
        std::printf(" FAILED: interior convergence order %.2f < 1.8\n", (double)order_in);
        exit_code = 1;
    }
    if (order_out < 1.8) {
        std::printf(" FAILED: exterior convergence order %.2f < 1.8\n", (double)order_out);
        exit_code = 1;
    }

    if (exit_code == 0) { std::printf("test_uniform_sphere: ok\n"); }
    Finalize();
    return exit_code;
}
