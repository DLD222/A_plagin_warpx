/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "NewtonGravity.H"

#include <ablastr/fields/PoissonSolver.H>

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
                                          amrex::Real relative_tolerance)
{
    BL_PROFILE("NewtonGravitySolver::solvePotential");

    // Poisson's equation is solved as grad^2 phi = 4 pi G rho.  The ablastr solver computes
    // grad^2 phi = rhs, so the right-hand side is 4 pi G rho.  In a periodic domain the
    // monopole must vanish, hence the mean is subtracted (Jeans swindle).
    amrex::MultiFab rhs(rho.boxArray(), rho.DistributionMap(), 1, 0);
    amrex::MultiFab::Copy(rhs, rho, 0, 0, 1, 0);

    amrex::Real const mean = rhs.sum(0) / geom.ProbSize();
    rhs.plus(-mean * 4.0 * M_PI * G_newton, 0);
    rhs.mult(4.0 * M_PI * G_newton, 0);

    // phi itself is what the caller gets; the layout must match what the solver expects
    // (cell-centred, one component, no ghosts beyond what the geometry provides).
    phi.setVal(amrex::Real(0.0));

    amrex::Vector<amrex::Geometry> geom_v{geom};
    amrex::Vector<amrex::DistributionMapping> dmap_v{rhs.DistributionMap()};
    amrex::Vector<amrex::BoxArray> grids_v{rhs.boxArray()};

    ablastr::fields::MultiLevelScalarField rho_ml(1);
    ablastr::fields::MultiLevelScalarField phi_ml(1);
    rho_ml[0] = &rhs;
    phi_ml[0] = &phi;

    ablastr::fields::MLMGOptions options;
    options.relative_tolerance = relative_tolerance;
    options.absolute_tolerance = amrex::Real(0.0);
    options.verbosity = 0;

    std::array<amrex::Real, 3> const beta{1.0, 1.0, 1.0};
    ablastr::fields::computePhi(
        rho_ml, phi_ml, beta, options, geom_v, dmap_v, grids_v,
        ablastr::utils::enums::GridType::Collocated,
        /*is_solver_igf_on_lev0=*/false,
        /*is_igf_2d=*/false);

    // The solve returns phi up to an additive constant; zero the mean so repeated calls are
    // comparable (and so the analytic comparison is well defined).
    amrex::Real const phi_mean = phi.sum(0) / geom.ProbSize();
    phi.plus(-phi_mean, 0, 1);
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
#else
            gx_arr(i,j,k) = amrex::Real(0.0);
            gy_arr(i,j,k) = amrex::Real(0.0);
            gz_arr(i,j,k) = -(phi_arr(i,j+1,k) - phi_arr(i,j-1,k)) / (2.0 * dx[0]);
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
