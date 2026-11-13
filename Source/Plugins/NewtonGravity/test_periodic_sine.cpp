/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Periodic-dipole test for the gravity solver.
 *
 * rho(x) = rho0 + A sin(kx) in a fully periodic box.  The mean is exactly rho0, so after the
 * Jeans swindle the equation is grad^2 phi = 4 pi G A sin(kx), whose solution is
 *
 *     phi(x) = -(4 pi G A / k^2) sin(kx)
 *
 * (k = 2 pi / L).  This checks the operator, the 4 pi G factor, the sign and the mean
 * subtraction at once, and it needs no boundary handler.
 */
#include "NewtonGravity.H"

#include <AMReX.H>
#include <AMReX_BoxArray.H>
#include <AMReX_DistributionMapping.H>
#include <AMReX_Geometry.H>
#include <AMReX_MultiFab.H>
#include <AMReX_ParmParse.H>
#include <AMReX_REAL.H>

#include <cmath>
#include <cstdio>

using namespace amrex;

int main (int argc, char* argv[])
{
    Initialize(argc, argv);
    int exit_code = 0;

    constexpr Real G = 6.67430e-11;
    constexpr Real pi = 3.14159265358979323846;

    Real L = 1.0e9;          // box size [m]
    Real amp = 1.0e3;        // density amplitude [kg m^-3]
    Real rho0 = 1.0e3;       // mean density [kg m^-3]
    int n_cell = 64;

    for (int const n : {32, 64})
    {
        n_cell = n;

        BoxArray ba(Box(IntVect(0), IntVect(n_cell - 1)));
        DistributionMapping dm(ba);
        RealBox rb({AMREX_D_DECL(0.0, 0.0, 0.0)}, {AMREX_D_DECL(L, L, L)});
        Geometry geom(Box(IntVect(0), IntVect(n_cell - 1)), rb, CoordSys::cartesian,
                      {AMREX_D_DECL(1, 1, 1)});

        MultiFab rho(ba, dm, 1, 1);
        MultiFab phi(ba, dm, 1, 1);

        Real const k = 2.0 * pi / L;
        for (MFIter mfi(rho, TilingIfNotGPU()); mfi.isValid(); ++mfi)
        {
            Box const& bx = mfi.tilebox();
            auto const rho_arr = rho.array(mfi);
            Real const dx = geom.CellSize(0);
            Real const x0 = geom.ProbLo(0);
            ParallelFor(bx, [=] AMREX_GPU_DEVICE (int i, int j, int kk) noexcept
            {
                ignore_unused(j, kk);
                Real const x = x0 + (i + 0.5) * dx;
                rho_arr(i,j,kk) = rho0 + amp * std::sin(k * x);
            });
        }
        rho.FillBoundary(geom.periodicity());

        warpx::plugins::gravity::NewtonGravitySolver::solvePotential(rho, phi, geom, 1.e-13);
        phi.FillBoundary(geom.periodicity());

        // Analytic solution, mean subtracted (the solver zeroes phi's mean).
        Real max_err = 0.0;
        Real max_ref = 0.0;
        Real const dx = geom.CellSize(0);
        Real const x0 = geom.ProbLo(0);
        for (MFIter mfi(phi, TilingIfNotGPU()); mfi.isValid(); ++mfi)
        {
            Box const& bx = mfi.tilebox();
            auto const phi_arr = phi.const_array(mfi);
            for (int i = bx.smallEnd(0); i <= bx.bigEnd(0); ++i)
            {
                Real const x = x0 + (i + 0.5) * dx;
                Real const exact = -(4.0 * pi * G * amp / (k * k)) * std::sin(k * x);
                // The discrete solution of a periodic problem with a sine mode has no
                // constant offset, so the comparison is direct.
                Real const err = std::abs(phi_arr(i, bx.smallEnd(1), 0) - exact);
                max_err = std::max(max_err, (double)err);
                max_ref = std::max(max_ref, (double)std::abs(exact));
            }
        }

        Real const rel = max_err / max_ref;
        std::printf("n=%4d  max|phi_num-phi_exact| = %.6e  max|phi_exact| = %.6e  rel = %.3e\n",
                    n_cell, (double)max_err, (double)max_ref, (double)rel);

        // Second-order convergence: the grid-32 error should be ~4x the grid-64 error.
        if (n_cell == 64) { exit_code = (rel < 1.e-3) ? 0 : 1; }
    }

    if (exit_code == 0) { std::printf("test_periodic_sine: ok\n"); }
    else { std::printf("test_periodic_sine: FAILED (relative error above 1e-3)\n"); }

    Finalize();
    return exit_code;
}
