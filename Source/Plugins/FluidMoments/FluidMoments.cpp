/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "FluidMoments.H"

#include "SolarNetwork.H"

namespace warpx::plugins::fluid
{

void advanceIdealGas (amrex::MultiFab& state, amrex::Geometry const& geom, int dir,
                      amrex::Real dt, amrex::Real gamma, int n_species)
{
    IdealGasEOS eos{gamma};
    StateLayout layout{n_species};
    advanceSweep(state, geom, dir, dt, eos, layout);
}

void applySolarNetwork (amrex::MultiFab& state, amrex::Real dt, amrex::Real gamma,
                        StateLayout const& layout, amrex::MultiFab* neutrino_energy)
{
    using namespace amrex;
    int const ns = SolarNetwork::n_species;
    AMREX_ALWAYS_ASSERT_WITH_MESSAGE(
        layout.n_species == ns,
        "applySolarNetwork: the state must have the network's species");
    if (neutrino_energy != nullptr) { neutrino_energy->setVal(Real(0.0)); }

    for (MFIter mfi(state); mfi.isValid(); ++mfi)
    {
        Box const& bx = mfi.validbox();
        auto s = state.array(mfi);
        Array4<Real> nu4;
        if (neutrino_energy != nullptr) { nu4 = neutrino_energy->array(mfi); }
        for (int i = bx.smallEnd(0); i <= bx.bigEnd(0); ++i) {
            for (int j = bx.smallEnd(1); j <= bx.bigEnd(1); ++j) {
                for (int k = bx.smallEnd(2); k <= bx.bigEnd(2); ++k) {
                    Real rho_species[SolarNetwork::n_species];
                    Real total = Real(0.0);
                    for (int a = 0; a < ns; ++a) {
                        rho_species[a] = s(i, j, k, layout.density(a));
                        total += rho_species[a];
                    }
                    if (total <= Real(0.0)) { continue; }

                    Real mass_fractions[SolarNetwork::n_species];
                    for (int a = 0; a < ns; ++a) { mass_fractions[a] = rho_species[a] / total; }

                    Real mom2 = Real(0.0);
                    for (int d = 0; d < AMREX_SPACEDIM; ++d) {
                        Real const m = s(i, j, k, layout.momentum(d));
                        mom2 += m * m;
                    }
                    Real const energy = s(i, j, k, layout.energy());
                    Real const pressure = (gamma - Real(1.0)) * (energy - Real(0.5) * mom2 / total);
                    Real const t9 = SolarNetwork::temperature(pressure, total, mass_fractions)
                                  / Real(1.e9);

                    auto const step = SolarNetwork::advance(rho_species, dt, t9);
                    for (int a = 0; a < ns; ++a) { s(i, j, k, layout.density(a)) = rho_species[a]; }
                    s(i, j, k, layout.energy()) += step.energy_density;
                    if (neutrino_energy != nullptr) { nu4(i, j, k) = step.neutrino_density; }
                }
            }
        }
    }
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
