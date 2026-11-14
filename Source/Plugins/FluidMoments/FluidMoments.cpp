/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "FluidMoments.H"

namespace warpx::plugins::fluid
{

void advanceIdealGas (amrex::MultiFab& state, amrex::Geometry const& geom, int dir,
                      amrex::Real dt, amrex::Real gamma, int n_species)
{
    IdealGasEOS eos{gamma};
    StateLayout layout{n_species};
    advanceSweep(state, geom, dir, dt, eos, layout);
}

void computeTotalDensity (amrex::MultiFab const& state, amrex::MultiFab& density,
                          StateLayout const& layout)
{
    using namespace amrex;
    for (MFIter mfi(state); mfi.isValid(); ++mfi)
    {
        Box const& bx = mfi.validbox();
        auto const s = state.const_array(mfi);
        auto rho = density.array(mfi);
        int const ns = layout.n_species;
        ParallelFor(bx, [=] AMREX_GPU_DEVICE (int i, int j, int k) noexcept
        {
            Real total = Real(0.0);
            for (int a = 0; a < ns; ++a) { total += s(i, j, k, layout.density(a)); }
            rho(i, j, k) = total;
        });
    }
}

void applyGravitySource (amrex::MultiFab& state,
                         amrex::Vector<amrex::MultiFab const*> const& g,
                         amrex::Real dt, StateLayout const& layout)
{
    using namespace amrex;
    AMREX_ALWAYS_ASSERT(g.size() == AMREX_SPACEDIM);
    int const ns = layout.n_species;

    for (MFIter mfi(state); mfi.isValid(); ++mfi)
    {
        Box const& bx = mfi.validbox();
        auto s = state.array(mfi);
        Array<Array4<Real const>, AMREX_SPACEDIM> ga;
        for (int d = 0; d < AMREX_SPACEDIM; ++d) { ga[d] = g[d]->const_array(mfi); }
        Real const dtc = dt;
        ParallelFor(bx, [=] AMREX_GPU_DEVICE (int i, int j, int k) noexcept
        {
            Real rho = Real(0.0);
            for (int a = 0; a < ns; ++a) { rho += s(i, j, k, layout.density(a)); }
            // The energy source uses the momentum *before* the kick (2.4a).
            Real momentum_dot_g = Real(0.0);
            for (int d = 0; d < AMREX_SPACEDIM; ++d) {
                Real const gd = ga[d](i, j, k);
                momentum_dot_g += s(i, j, k, layout.momentum(d)) * gd;
                s(i, j, k, layout.momentum(d)) += dtc * rho * gd;
            }
            s(i, j, k, layout.energy()) += dtc * momentum_dot_g;
        });
    }
}

} // namespace warpx::plugins::fluid
