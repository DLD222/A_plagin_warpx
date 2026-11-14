/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Acceptance test: a source-free periodic run conserves mass, momentum and energy.
 *
 * The finite-volume update (2.10a) is a flux difference, so on a periodic domain every interior
 * flux cancels pairwise and the only remaining term is the wrap-around flux, which periodicity
 * makes identical.  The sums must therefore agree to round-off (~1e-15 relative), not merely to
 * a loosened tolerance: anything above 1e-12 means the update is not conservative.
 *
 * Specification: PHYSICS_SPEC.md section 2.5 ("守恒（无源）").
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

using namespace amrex;
using namespace warpx::plugins::fluid;

namespace
{

struct Conserved
{
    Real mass = 0.0;
    Real momentum = 0.0;
    Real energy = 0.0;
};

Conserved sumConserved (MultiFab const& state, Real dx, StateLayout const& layout)
{
    Conserved total;
    for (MFIter mfi(state); mfi.isValid(); ++mfi)
    {
        Box const& bx = mfi.validbox();
        auto const s = state.const_array(mfi);
        Real mass = 0.0;
        Real momentum = 0.0;
        Real energy = 0.0;
        for (int i = bx.smallEnd(0); i <= bx.bigEnd(0); ++i) {
            mass += s(i, 0, 0, layout.density(0));
            momentum += s(i, 0, 0, layout.momentum(0));
            energy += s(i, 0, 0, layout.energy());
        }
        total.mass += mass * dx;
        total.momentum += momentum * dx;
        total.energy += energy * dx;
    }
    ParallelDescriptor::ReduceRealSum(total.mass);
    ParallelDescriptor::ReduceRealSum(total.momentum);
    ParallelDescriptor::ReduceRealSum(total.energy);
    return total;
}

Real relativeDifference (Real a, Real b)
{
    return std::abs(a - b) / std::max(std::abs(a), 1.e-300);
}

} // namespace

int main (int argc, char* argv[])
{
    Initialize(argc, argv);

    Real const gamma = Real(1.4);
    Real const cfl = Real(0.3);
    int const n = 256;
    Real const x_lo = Real(0.0);
    Real const x_hi = Real(1.0);

    BoxArray ba;
    DistributionMapping dm;
    Geometry geom = test::makeSlab(x_lo, x_hi, n, ba, dm, /*periodic_x=*/true);
    Real const dx = geom.CellSize(0);

    IdealGasEOS const eos{gamma};
    StateLayout const layout{1};

    MultiFab state(ba, dm, layout.n_comp(), 2);
    for (MFIter mfi(state, TilingIfNotGPU()); mfi.isValid(); ++mfi)
    {
        Box const& bx = mfi.growntilebox(2);
        auto s = state.array(mfi);
        Real const lo = geom.ProbLo(0);
        ParallelFor(bx, [=] AMREX_GPU_DEVICE (int i, int j, int k) noexcept
        {
            Real const x = lo + (Real(i) + Real(0.5)) * dx;
            Real const pi = Real(3.14159265358979323846);
            Real const rho = Real(1.0) + Real(0.30) * std::sin(Real(2.0) * pi * x);
            Real const u = Real(0.10) * std::sin(Real(2.0) * pi * x);
            Real const p = Real(1.0) + Real(0.20) * std::cos(Real(2.0) * pi * x);
            s(i, j, k, layout.density(0)) = rho;
            for (int d = 0; d < AMREX_SPACEDIM; ++d) {
                s(i, j, k, layout.momentum(d)) = (d == 0) ? rho * u : Real(0.0);
            }
            s(i, j, k, layout.energy()) = p / (gamma - Real(1.0))
                                        + Real(0.5) * rho * u * u;
        });
    }
    state.FillBoundary(geom.periodicity());

    Conserved const before = sumConserved(state, dx, layout);

    // A first-order in time space is fine here: conservation does not depend on the order, only
    // on the update being a flux difference.
    int const n_steps = 40;
    for (int step = 0; step < n_steps; ++step)
    {
        Real const speed = maxWaveSpeed(state, eos, layout, 0);
        Real const dt = cfl * dx / speed;
        state.FillBoundary(geom.periodicity());
        advanceSweep(state, geom, 0, dt, eos, layout);
    }

    Conserved const after = sumConserved(state, dx, layout);

    Real const d_mass = relativeDifference(before.mass, after.mass);
    Real const d_momentum = relativeDifference(before.momentum, after.momentum);
    Real const d_energy = relativeDifference(before.energy, after.energy);

    std::printf("\n conservation over %d steps on %d cells\n", n_steps, n);
    std::printf("   mass     %.15e -> %.15e   rel %.3e\n",
                (double)before.mass, (double)after.mass, (double)d_mass);
    std::printf("   momentum %.15e -> %.15e   rel %.3e\n",
                (double)before.momentum, (double)after.momentum, (double)d_momentum);
    std::printf("   energy   %.15e -> %.15e   rel %.3e\n",
                (double)before.energy, (double)after.energy, (double)d_energy);

    int exit_code = 0;
    Real const tol = Real(1.e-12);
    if (d_mass > tol || d_momentum > tol || d_energy > tol) {
        std::printf(" FAILED: relative drift above %.1e\n", (double)tol);
        exit_code = 1;
    }

    if (exit_code == 0) { std::printf("test_conservation: ok\n"); }
    Finalize();
    return exit_code;
}
