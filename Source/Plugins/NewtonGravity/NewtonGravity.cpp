/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "NewtonGravity.H"

#include <AMReX_MLABecLaplacian.H>
#include <AMReX_MLMG.H>

#include <AMReX_Array4.H>
#include <AMReX_BLProfiler.H>
#include <AMReX_GpuLaunch.H>
#include <AMReX_MultiFabUtil.H>
#include <AMReX_ParallelDescriptor.H>
#include <AMReX_ParmParse.H>

namespace warpx::plugins::gravity
{

namespace
{
    // CODATA 2018; the specification's constants table quotes the same value.
    constexpr amrex::Real G_newton = 6.67430e-11;
}

void NewtonGravitySolver::solvePotential (amrex::MultiFab const& rho,
                                          amrex::MultiFab& phi,
                                          amrex::Geometry const& geom,
                                          amrex::Real relative_tolerance,
                                          PotentialBoundary boundary)
{
    BL_PROFILE("NewtonGravitySolver::solvePotential");

    // Solved directly with AMReX's MLABecLaplacian rather than through ablastr's computePhi:
    // ablastr carries an EB callback contract this plugin has no use for, and defaults the
    // domain to Dirichlet when no handler is given (PoissonSolver.H:396-403), which a periodic
    // box rejects.  Going straight to AMReX keeps the plugin self-contained.
    //
    //   periodic:  grad^2 phi = 4 pi G (rho - <rho>)   [the mean is removed: Jeans swindle]
    //   open:      grad^2 phi = 4 pi G rho,  phi = given on the boundary
    //
    // With MLABecLaplacian the operator is a*alpha*phi - b*div(beta grad phi), so alpha = 0,
    // b = -1 and beta = 1 reproduce the plain Laplacian.

    amrex::MultiFab rhs(rho.boxArray(), rho.DistributionMap(), 1, 1);
    amrex::MultiFab::Copy(rhs, rho, 0, 0, 1, 0);

    if (boundary == PotentialBoundary::periodic) {
        amrex::Real const mean = rho.sum(0) / geom.ProbSize();
        rhs.plus(-mean, 0, 1);
    }
    rhs.mult(4.0 * M_PI * G_newton, 0);
    rhs.FillBoundary(geom.periodicity());

    // The interior is the initial guess: the open case must keep the caller's ghost cells,
    // which hold the Dirichlet data, so only the valid cells are touched here.
    phi.setVal(amrex::Real(0.0), 0, 1, 0);
    if (boundary == PotentialBoundary::periodic) { phi.FillBoundary(geom.periodicity()); }

    // One level: the plugin solves on the level the caller hands it.
    amrex::BoxArray const& ba = rhs.boxArray();
    amrex::DistributionMapping const& dm = rhs.DistributionMap();

    amrex::LPInfo info;
#if defined(WARPX_DIM_RZ)
    amrex::Abort("NewtonGravity: this plugin solves the Poisson equation with the Cartesian "
                 "multigrid operator, which does not carry the 1/r measure of an RZ geometry.  "
                 "The potential, and therefore the acceleration, would be wrong.  Use a 1D, XZ or "
                 "3D build, or wire a cylindrical Poisson solve (PHYSICS_SPEC section 1.4).");
#endif
    amrex::MLABecLaplacian mlabec({geom}, {ba}, {dm}, info);
    mlabec.setMaxOrder(2);

    // A periodic direction stays periodic even in "open" mode: AMReX asserts that the operator
    // type matches the geometry, and a slab that is periodic across its width is exactly the
    // case that needs it.  "open" therefore means Dirichlet on the non-periodic faces only.
    amrex::Array<amrex::LinOpBCType, AMREX_SPACEDIM> lobc;
    for (int d = 0; d < AMREX_SPACEDIM; ++d) {
        lobc[d] = ((boundary == PotentialBoundary::periodic) || geom.isPeriodic(d))
                      ? amrex::LinOpBCType::Periodic
                      : amrex::LinOpBCType::Dirichlet;
    }
    mlabec.setDomainBC(lobc, lobc);

    amrex::MultiFab beta_fab(ba, dm, 1, 1);
    beta_fab.setVal(amrex::Real(1.0));
    mlabec.setLevelBC(0, &phi);
    mlabec.setScalars(amrex::Real(0.0), amrex::Real(-1.0));
    mlabec.setACoeffs(0, amrex::Real(0.0));
    mlabec.setBCoeffs(0, amrex::Array<amrex::MultiFab const*, AMREX_SPACEDIM>{
        AMREX_D_DECL(&beta_fab, &beta_fab, &beta_fab)});

    amrex::MLMG mlmg(mlabec);
    mlmg.setVerbose(0);
    mlmg.setMaxIter(200);
    mlmg.solve({&phi}, {&rhs}, relative_tolerance, amrex::Real(0.0));

    if (boundary == PotentialBoundary::periodic) {
        // The operator annihilates constants, so the periodic solution is fixed by asking for
        // zero mean.
        amrex::Real const phi_mean = phi.sum(0) / geom.ProbSize();
        phi.plus(-phi_mean, 0, 1);
    }
}

void NewtonGravitySolver::computeAcceleration (amrex::MultiFab const& phi,
                                               amrex::MultiFab& gx,
                                               amrex::MultiFab& gy,
                                               amrex::MultiFab& gz,
                                               amrex::Geometry const& geom)
{
    BL_PROFILE("NewtonGravitySolver::computeAcceleration");

    auto const dx = geom.CellSizeArray();

    for (amrex::MFIter mfi(phi, amrex::TilingIfNotGPU()); mfi.isValid(); ++mfi)
    {
        amrex::Box const& bx = mfi.tilebox();
        auto const phi_arr = phi.const_array(mfi);
        auto gx_arr = gx.array(mfi);
        auto gy_arr = gy.array(mfi);
        auto gz_arr = gz.array(mfi);

        amrex::ParallelFor(bx, [=] AMREX_GPU_DEVICE (int i, int j, int k) noexcept
        {
            amrex::ignore_unused(j, k);
            // g = -grad phi, central differences.
#if defined(WARPX_DIM_3D)
            gx_arr(i,j,k) = -(phi_arr(i+1,j,k) - phi_arr(i-1,j,k)) / (2.0 * dx[0]);
            gy_arr(i,j,k) = -(phi_arr(i,j+1,k) - phi_arr(i,j-1,k)) / (2.0 * dx[1]);
            gz_arr(i,j,k) = -(phi_arr(i,j,k+1) - phi_arr(i,j,k-1)) / (2.0 * dx[2]);
#elif defined(WARPX_DIM_XZ) || defined(WARPX_DIM_RZ)
            gx_arr(i,j,k) = -(phi_arr(i+1,j,k) - phi_arr(i-1,j,k)) / (2.0 * dx[0]);
            gy_arr(i,j,k) = amrex::Real(0.0);
            gz_arr(i,j,k) = -(phi_arr(i,j+1,k) - phi_arr(i,j-1,k)) / (2.0 * dx[1]);
#elif defined(WARPX_DIM_1D_Z) || (AMREX_SPACEDIM == 1)
            // One-dimensional builds have a single index, i, and the physical axis is z.  Using
            // j +/- 1 here (as an earlier version did) is an out-of-bounds read that AMReX's
            // linear indexing aliases onto neighbouring i cells, so the difference is taken along
            // the wrong "axis" and the result is silently wrong rather than a crash.
            gx_arr(i,j,k) = amrex::Real(0.0);
            gy_arr(i,j,k) = amrex::Real(0.0);
            gz_arr(i,j,k) = -(phi_arr(i+1,j,k) - phi_arr(i-1,j,k)) / (2.0 * dx[0]);
#endif
        });
    }
}

amrex::Real NewtonGravitySolver::gravitationalEnergy (amrex::MultiFab const& rho,
                                                      amrex::MultiFab const& phi,
                                                      amrex::Geometry const& geom)
{
    BL_PROFILE("NewtonGravitySolver::gravitationalEnergy");

    auto const cell_size = geom.CellSizeArray();
    amrex::Real cell_volume = 1.0;
    for (int d = 0; d < AMREX_SPACEDIM; ++d) { cell_volume *= cell_size[d]; }
    amrex::MultiFab rho_phi(rho.boxArray(), rho.DistributionMap(), 1, 0);
    amrex::MultiFab::Copy(rho_phi, rho, 0, 0, 1, 0);
    amrex::MultiFab::Multiply(rho_phi, phi, 0, 0, 1, 0);

    // 1/2 integral rho phi dV (Binney & Tremaine section 2.2).
    amrex::Real const integral = rho_phi.sum(0) * cell_volume;
    return amrex::Real(0.5) * integral;
}

} // namespace warpx::plugins::gravity
