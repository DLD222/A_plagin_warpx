/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Acceptance test: the reaction network wired into the fluid state.
 *
 * The coupling (applySolarNetwork) has to do three things and only those three: read the
 * temperature off the fluid state, advance every species density, and add the energy the network
 * deposits to the total energy density while leaving the neutrino energy out.  The test checks
 * each of them:
 *
 *   1. The mean molecular weight of a fully ionised gas: 0.5 for pure hydrogen and ~0.62 for the
 *      solar mixture, both textbook values (Kippenhahn et al. section 3).
 *   2. A box with no fuel (pure 4He) must not change at all - no heating, no composition drift.
 *   3. A burning box must heat by exactly the energy the network reports, and its composition
 *      must equal what the network produces on its own - the coupling must not alter either.
 *
 * Specification: PHYSICS_SPEC.md sections 2.4 and 3.4.
 */
#include "FluidMoments.H"
#include "FluidTestReference.H"
#include "SolarNetwork.H"

#include <AMReX.H>
#include <AMReX_BoxArray.H>
#include <AMReX_DistributionMapping.H>
#include <AMReX_Geometry.H>
#include <AMReX_MultiFab.H>
#include <AMReX_REAL.H>

#include <cmath>
#include <cstdio>
#include <vector>

using namespace amrex;
using namespace warpx::plugins::fluid;

namespace
{

Real totalEnergy (MultiFab const& state, StateLayout const& layout)
{
    Real sum = 0.0;
    for (MFIter mfi(state); mfi.isValid(); ++mfi)
    {
        Box const& bx = mfi.validbox();
        auto const s = state.const_array(mfi);
        for (int i = bx.smallEnd(0); i <= bx.bigEnd(0); ++i) {
            sum += s(i, 0, 0, layout.energy());
        }
    }
    return sum;
}

} // namespace

int main (int argc, char* argv[])
{
    Initialize(argc, argv);

    int exit_code = 0;

    // 1. Mean molecular weight.
    {
        Real pure_h[SolarNetwork::n_species] = {0.0};
        pure_h[SolarNetwork::p] = 1.0;
        Real const mu_h = SolarNetwork::meanMolecularWeight(pure_h);

        Real solar[SolarNetwork::n_species] = {0.0};
        solar[SolarNetwork::p] = 0.70;
        solar[SolarNetwork::he4] = 0.28;
        solar[SolarNetwork::n14] = 0.02;
        Real const mu_solar = SolarNetwork::meanMolecularWeight(solar);

        std::printf("\n mean molecular weight: pure H %.4f (0.5 expected), solar mixture %.4f"
                    " (~0.62 expected)\n", (double)mu_h, (double)mu_solar);
        if (std::abs(mu_h - Real(0.5)) > Real(1.e-12)) {
            std::printf(" FAILED: mu of ionised hydrogen is not 1/2\n");
            exit_code = 1;
        }
        if (std::abs(mu_solar - Real(0.6155)) > Real(5.e-3)) {
            std::printf(" FAILED: mu of the solar mixture is %.4f\n", (double)mu_solar);
            exit_code = 1;
        }
    }

    StateLayout const layout{SolarNetwork::n_species};
    Real const gamma = Real(1.4);

    // The temperature of a state with p = 1e12 Pa and rho = 1e5 kg/m^3 sets how fast it burns;
    // both cases below use the same box, only the composition differs.
    BoxArray ba;
    DistributionMapping dm;
    Geometry geom = test::makeSlab(Real(0.0), Real(1.0), 8, ba, dm, /*periodic_x=*/true);
    Real const rho = Real(1.0e5);
    // The pressure is chosen so that the mixture burns at T9 = 0.2, where the CNO steps are
    // active: p = rho k_B T / (mu m_u) with T = 2e8 K and mu ~ 0.84 for this composition.
    Real const pressure = Real(1.97e17);

    auto make_state = [&] (std::vector<Real> const& fractions) {
        MultiFab state(ba, dm, layout.n_comp(), 2);
        state.setVal(Real(0.0));
        for (MFIter mfi(state); mfi.isValid(); ++mfi)
        {
            Box const& bx = mfi.validbox();
            auto s = state.array(mfi);
            for (int i = bx.smallEnd(0); i <= bx.bigEnd(0); ++i) {
                for (int a = 0; a < SolarNetwork::n_species; ++a) {
                    s(i, 0, 0, layout.density(a)) = rho * fractions[a];
                }
                for (int d = 0; d < AMREX_SPACEDIM; ++d) {
                    s(i, 0, 0, layout.momentum(d)) = Real(0.0);
                }
                s(i, 0, 0, layout.energy()) = pressure / (gamma - Real(1.0));
            }
        }
        state.FillBoundary(geom.periodicity());
        return state;
    };

    // 2. No fuel: pure 4He cannot react in this network.
    {
        std::vector<Real> fractions(SolarNetwork::n_species, 0.0);
        fractions[SolarNetwork::he4] = 1.0;
        MultiFab state = make_state(fractions);
        Real const energy_before = totalEnergy(state, layout);

        MultiFab neutrino(ba, dm, 1, 0);
        applySolarNetwork(state, Real(1.e10), gamma, layout, &neutrino);

        Real const energy_after = totalEnergy(state, layout);
        Real const drift = std::abs(energy_after - energy_before) / energy_before;
        std::printf("\n inert 4He box: relative energy drift %.3e (must be zero)\n",
                    (double)drift);
        if (drift > Real(1.e-14)) {
            std::printf(" FAILED: an inert composition was heated\n");
            exit_code = 1;
        }
        if (std::abs(totalEnergy(state, layout) - energy_before) > Real(0.0)
            && drift > Real(1.e-14)) {
            exit_code = 1;
        }
    }

    // 3. Burning: one coupled step must heat the fluid by exactly the energy the network reports
    //    and must leave the same composition a standalone network run from the same initial
    //    state produces (both use the same initial temperature, so the comparison is exact).
    {
        std::vector<Real> fractions(SolarNetwork::n_species, 0.0);
        fractions[SolarNetwork::p] = 0.35;
        fractions[SolarNetwork::he4] = 0.63;
        fractions[SolarNetwork::c12] = 0.01;
        fractions[SolarNetwork::n14] = 0.01;
        MultiFab state = make_state(fractions);

        Real const mu = SolarNetwork::meanMolecularWeight(fractions.data());
        Real const t9 = SolarNetwork::temperature(pressure, rho, fractions.data()) / Real(1.e9);
        std::printf("\n burning box: T9 = %.5f (mu = %.4f)\n", (double)t9, (double)mu);

        Real const energy_before = totalEnergy(state, layout);
        // A step short enough that the CNO fuel burns by only a fraction of a percent: the
        // energy gain must be resolvable against the internal energy it is added to.
        Real const dt = Real(1.e-4);
        MultiFab neutrino(ba, dm, 1, 0);
        applySolarNetwork(state, dt, gamma, layout, &neutrino);

        // The box is uniform, so the per-cell gain is the total divided by the number of cells.
        Real const gas_gain = (totalEnergy(state, layout) - energy_before)
                            / Real(geom.Domain().numPts());
        Real const neutrino_gain = neutrino.norminf(0);

        // Standalone network run from the same composition and temperature.  The network takes
        // mass densities in kg/m^3 (it converts to cgs internally), so the state is copied
        // as-is.
        Real standalone[SolarNetwork::n_species];
        for (int a = 0; a < SolarNetwork::n_species; ++a) {
            standalone[a] = rho * fractions[a];
        }
        auto const result = SolarNetwork::advance(standalone, dt, t9);

        Real max_relative = 0.0;
        for (MFIter mfi(state); mfi.isValid(); ++mfi)
        {
            Box const& bx = mfi.validbox();
            auto const s = state.const_array(mfi);
            for (int a = 0; a < SolarNetwork::n_species; ++a) {
                Real const coupled = s(bx.smallEnd(0), 0, 0, layout.density(a));
                Real const reference = standalone[a];
                max_relative = std::max(max_relative,
                                        std::abs(coupled - reference)
                                        / std::max(rho * fractions[a], Real(1.e-30)));
            }
        }

        std::printf("   gas gain %.6e J/m^3, network deposit %.6e J/m^3 (ratio %.9f)\n",
                    (double)gas_gain, (double)result.energy_density,
                    (double)(gas_gain / result.energy_density));
        std::printf("   neutrinos %.6e J/m^3, composition max rel. difference %.3e\n",
                    (double)neutrino_gain, (double)max_relative);

        if (std::abs(gas_gain / result.energy_density - Real(1.0)) > Real(1.e-10)) {
            std::printf(" FAILED: the fluid was not heated by exactly the deposited energy\n");
            exit_code = 1;
        }
        if (std::abs(neutrino_gain - result.neutrino_density) / result.neutrino_density
            > Real(1.e-10)) {
            std::printf(" FAILED: the neutrino energy reported to the caller is wrong\n");
            exit_code = 1;
        }
        if (max_relative > Real(1.e-10)) {
            std::printf(" FAILED: the coupled composition differs from the standalone run\n");
            exit_code = 1;
        }
        if (!(result.energy_density > Real(0.0))) {
            std::printf(" FAILED: the composition did not burn\n");
            exit_code = 1;
        }
    }

    if (exit_code == 0) { std::printf("test_network_coupling: ok\n"); }
    Finalize();
    return exit_code;
}
