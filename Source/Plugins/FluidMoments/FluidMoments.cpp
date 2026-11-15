/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "FluidMoments.H"

#include <cmath>

#include "SolarNetwork.H"

using warpx::plugins::fluid::star_box_hi;
using warpx::plugins::fluid::star_box_lo;

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
            for (int j = star_box_lo(bx, 1); j <= star_box_hi(bx, 1); ++j) {
                for (int k = star_box_lo(bx, 2); k <= star_box_hi(bx, 2); ++k) {
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

                    // The network is an explicit integrator with a fixed dt: outside the
                    // temperature range it was fitted for it returns non-finite rates, and the
                    // NaN then lives in the state forever (this is exactly what a solar-column
                    // deck did at rho ~ 1e2 g/cm^3 with dt ~ 1 s: 4e19 erg/cm^3 booked in the
                    // first step, NaN in the second).  Refuse the step with the cell's numbers
                    // instead of poisoning the state -- a silent clamp would hide the regime
                    // error just as well as the fake assertion that used to sit here.
                    AMREX_ALWAYS_ASSERT_WITH_MESSAGE(
                        t9 > Real(1.0e-6) && t9 < Real(1.0e5),
                        "applySolarNetwork: temperature T9 = " + std::to_string(t9)
                        + " (T = " + std::to_string(t9 * 1.0e9) + " K) is outside the range the "
                        "CF88 network is valid for (1e3 .. 1e14 K).  rho = " + std::to_string(total)
                        + " g/cm^3, dt = " + std::to_string(dt) + " s.");
                    auto const step = SolarNetwork::advance(rho_species, dt, t9);
                    AMREX_ALWAYS_ASSERT_WITH_MESSAGE(
                        std::isfinite(step.energy_density)
                        && std::isfinite(static_cast<double>(step.neutrino_density)),
                        "applySolarNetwork: the network returned a non-finite energy at T = "
                        + std::to_string(t9 * 1.0e9) + " K, rho = " + std::to_string(total)
                        + " g/cm^3, dt = " + std::to_string(dt) + " s");
                    for (int a = 0; a < ns; ++a) {
                        AMREX_ALWAYS_ASSERT_WITH_MESSAGE(
                            std::isfinite(static_cast<double>(rho_species[a]))
                            && rho_species[a] >= Real(0.0),
                            "applySolarNetwork: species " + std::to_string(a)
                            + " became negative or non-finite");
                        s(i, j, k, layout.density(a)) = rho_species[a];
                    }
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
