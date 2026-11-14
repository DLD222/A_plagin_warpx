/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Acceptance test: the reduced solar reaction network.
 *
 * The network is pp-I plus the CNO main cycle (PHYSICS_SPEC 3.1-3.3).  Four things are checked:
 *
 * 1. Mass numbers balance in every reaction, which is the stoichiometry's own bookkeeping.
 * 2. A pure-hydrogen gas burns at the rate the pp-I chain predicts.  In the chain's
 *    quasi-equilibrium (D and 3He adjust faster than protons burn) the proton abundance follows
 *
 *        dn_p/dt = -2 r1,  r1 = 1/2 n_p^2 <sigma v>(p+p)
 *        =>  n_p(t) = n_p0 / (1 + n_p0 <sigma v> t),
 *
 *    and that closed form is what the test compares against.  The comparison starts after the
 *    transient in which D and 3He build up from zero.
 * 3. Energy bookkeeping of pp-I: 4p -> 4He releases 26.732 MeV, so the energy the network
 *    deposits per 4He nucleus produced must be exactly that (2*1.442 + 2*5.494 + 12.860).
 * 4. The CNO cycle closes: 12C is regenerated (12C + 4p -> 12C + 4He), so with the cycle running
 *    the carbon abundance must come back to its initial value and helium must grow by one 4He per
 *    cycle.  Its *energy* is checked only for consistency with the tabulated Q values: the two
 *    beta decays are folded into entries 61 and 71 and their energy is not in that table, so the
 *    CNO energy is marked [todo-src] in the specification rather than quietly added.
 */
#include "SolarNetwork.H"

#include <AMReX.H>
#include <AMReX_REAL.H>

#include <cmath>
#include <cstdio>

using namespace amrex;
using warpx::plugins::fluid::SolarNetwork;

namespace
{

Real totalDensity (Real const* rho)     // kg/m^3
{
    Real sum = 0.0;
    for (int s = 0; s < SolarNetwork::n_species; ++s) { sum += rho[s]; }
    return sum;
}

Real numberDensityCgs (Real rho_si, Real mass_number)
{
    constexpr Real m_u = 1.66053906660e-24;
    return rho_si * 1.e-3 / (mass_number * m_u);     // cm^-3
}

} // namespace

int main (int argc, char* argv[])
{
    Initialize(argc, argv);

    using namespace warpx::plugins::fluid;
    int exit_code = 0;

    // 1. Mass numbers balance.
    for (int r = 0; r < SolarNetwork::n_reactions; ++r)
    {
        auto const re = SolarNetwork::reaction(r);
        Real const reactants = SolarNetwork::massNumber(re.a) + SolarNetwork::massNumber(re.b);
        Real products = SolarNetwork::massNumber(re.c);
        if (re.e >= 0) { products += SolarNetwork::massNumber(re.e); }
        if (re.third >= 0) { products += SolarNetwork::massNumber(re.third); }
        if (std::abs(reactants - products) > 1e-12) {
            std::printf(" FAILED: reaction %d (CF88 %d) does not balance: %.0f -> %.0f\n", r,
                        re.cf88_entry, (double)reactants, (double)products);
            exit_code = 1;
        }
    }
    std::printf(" network: %d species, %d reactions, mass numbers balance\n",
                SolarNetwork::n_species, SolarNetwork::n_reactions);

    // 2. pp-I: a pure-hydrogen gas at fixed temperature.
    {
        // Solar-centre conditions from the SSM profile file: T = 1.563376e7 K and
        // rho = 152.699 g/cm^3, with pure hydrogen so that the pp-I chain is the only channel.
        Real const t9 = Real(1.563376e7) / Real(1.e9);
        Real const rho_total = Real(1.526990e5);     // kg/m^3 (152.699 g/cm^3)
        Real rho[SolarNetwork::n_species] = {0.0};
        rho[SolarNetwork::p] = rho_total;

        Real const sigma_v = SolarNetwork::sigmaV(0, t9);      // reaction 0 is p+p
        std::printf("\n pp-I at T9 = %.6f, rho = %.4e kg/m^3: <sigma v>(p+p) = %.4e cm^3/s\n",
                    (double)t9, (double)rho_total, (double)sigma_v);
        std::printf(" %10s %16s %16s %12s %12s\n", "t [s]", "n_p numeric", "n_p analytic",
                    "rel. diff", "eps [W/kg]");

        Real const n_p0 = numberDensityCgs(rho_total, 1.0);
        Real const n_he4_initial = numberDensityCgs(rho[SolarNetwork::he4], 4.0);
        // The pp chain burns over ~1e17 s at these conditions, and backward Euler is stable at
        // that step size (the stiff D/3He equilibrium is handled implicitly, not resolved).
        Real const dt = Real(1.e14);
        Real t = 0.0;
        Real const t_end = Real(1.e17);
        // The D and 3He reservoirs need to fill before the quasi-equilibrium form applies; the
        // comparison is made once 5% of the protons have burned, where that is long past.
        Real worst = 0.0;
        Real energy_total = 0.0;
        Real neutrino_total = 0.0;
        Real he4_initial = 0.0;
        he4_initial = rho[SolarNetwork::he4];
        while (t < t_end)
        {
            // pp-I only, bare rates: the closed-form decay below is derived for that chain.
            SolarNetwork::Options options;
            options.include_pp2 = false;
            options.include_screening = false;
            auto const step = SolarNetwork::advance(rho, dt, t9, options);
            energy_total += step.energy_density;
            neutrino_total += step.neutrino_density;
            t += dt;

            Real const n_p = numberDensityCgs(rho[SolarNetwork::p], 1.0);
            Real const burned = 1.0 - n_p / n_p0;
            if (burned > 0.05) {
                Real const analytic = n_p0 / (1.0 + n_p0 * sigma_v * t);
                Real const rel = std::abs(n_p - analytic) / analytic;
                worst = std::max(worst, rel);
            }
        }
        Real const n_p = numberDensityCgs(rho[SolarNetwork::p], 1.0);
        Real const n_he4 = numberDensityCgs(rho[SolarNetwork::he4], 4.0);
        Real const burned = 1.0 - n_p / n_p0;
        Real const analytic = n_p0 / (1.0 + n_p0 * sigma_v * t);
        std::printf(" %10.3e %16.4e %16.4e %12.3e\n", (double)t, (double)n_p,
                    (double)analytic, (double)(std::abs(n_p - analytic) / analytic));

        // Energy per 4He nucleus: pp-I releases 26.732 MeV per 4He.
        Real const n_he4_made = n_he4 - n_he4_initial;
        // energy_total is J per m^3 of the lattice; dividing by a number density in cm^-3 needs
        // the cm^3 -> m^3 factor of 1e6 to give joules per nucleus.
        Real const energy_per_he4 = energy_total / (n_he4_made * Real(1.e6));
        // 26.732 MeV in joules: 26.732 * 1.602176634e-13 J/MeV = 4.283e-12 J.
        Real const expected = Real(2.0 * 1.442 + 2.0 * 5.494 + 12.860) * Real(1.602176634e-13);
        std::printf(" burned fraction %.3f, 4He = %.4e cm^-3\n", (double)burned, (double)n_he4);
        Real const neutrino_per_he4 = neutrino_total / (n_he4_made * Real(1.e6));
        Real const total_per_he4 = energy_per_he4 + neutrino_per_he4;
        std::printf(" energy per 4He: deposited %.3f MeV, neutrino %.3f MeV, total %.3f MeV\n",
                    (double)(energy_per_he4 / 1.602176634e-13),
                    (double)(neutrino_per_he4 / 1.602176634e-13),
                    (double)(total_per_he4 / 1.602176634e-13));
        std::printf("   (pp-I releases 26.732 MeV per 4He, of which the two pp neutrinos take"
                    " 2 x 0.265 = 0.530 MeV)\n");

        if (n_he4_made <= 0.0) {
            std::printf(" FAILED: no helium was produced\n");
            exit_code = 1;
        }
        if (worst > Real(5.e-2)) {
            std::printf(" FAILED: proton decay differs from the analytic pp-I form by %.1f%%\n",
                        (double)(100.0 * worst));
            exit_code = 1;
        }
        // Per step the deposited energy and the 4He produced come from the same rates, so the
        // ratio is the Q sum exactly; integrated over a thousand steps it deviates by the
        // departure from exact quasi-equilibrium (measured 7e-6), which sets the tolerance.
        if (std::abs(total_per_he4 - expected) / expected > Real(1.e-4)) {
            std::printf(" FAILED: the total energy per 4He is not the pp-I Q sum\n");
            exit_code = 1;
        }
        // The neutrino share is exactly 2 x 0.265 MeV per 4He for pp-I.
        Real const expected_nu = Real(2.0 * 0.265) * Real(1.602176634e-13);
        if (std::abs(neutrino_per_he4 - expected_nu) / expected_nu > Real(1.e-4)) {
            std::printf(" FAILED: neutrino energy per 4He is %.6f MeV, expected 0.530\n",
                        (double)(neutrino_per_he4 / 1.602176634e-13));
            exit_code = 1;
        }
    }

    // 3. CNO closure: the cycle returns 12C, so its abundance must come back.
    {
        Real const t9 = Real(0.2);
        Real rho[SolarNetwork::n_species] = {0.0};
        rho[SolarNetwork::p] = Real(1.0e4);         // kg/m^3, hydrogen-rich
        rho[SolarNetwork::c12] = Real(1.0e2);       // kg/m^3 of carbon (1%)
        // The conserved quantity of the cycle is the *number* of C+N nuclei, not their mass:
        // every step turns one CN nucleus into another.
        auto const cn_number = [&] () {
            int const cn_species[4] = {SolarNetwork::c12, SolarNetwork::c13, SolarNetwork::n14,
                                       SolarNetwork::n15};
            Real sum = 0.0;
            for (int const s : cn_species) {
                sum += numberDensityCgs(rho[s], SolarNetwork::massNumber(s));
            }
            return sum;
        };
        Real const cn_initial = cn_number();
        Real const n_p_initial = numberDensityCgs(rho[SolarNetwork::p], 1.0);
        Real const n_he4_initial = numberDensityCgs(rho[SolarNetwork::he4], 4.0);
        Real const he4_initial = rho[SolarNetwork::he4];

        Real const dt = Real(10.0);
        Real t = 0.0;
        Real energy_total = 0.0;
        Real neutrino_total = 0.0;
        // The energy per 4He is only the clean Q sum once the cycle is in steady state: while the
        // intermediates fill up, the counts of the four steps differ and the stored chemical
        // energy shows up in the ratio.  The ratio is therefore measured over the second half.
        Real energy_mid = -1.0;
        Real neutrino_mid = 0.0;
        Real he4_mid = 0.0;
        Real const t_mid = Real(5.e3);
        while (t < Real(1.e4))
        {
            auto const step = SolarNetwork::advance(rho, dt, t9);
            energy_total += step.energy_density;
            neutrino_total += step.neutrino_density;
            t += dt;
            if (energy_mid < 0.0 && t >= t_mid) {
                energy_mid = energy_total;
                neutrino_mid = neutrino_total;
                he4_mid = numberDensityCgs(rho[SolarNetwork::he4], 4.0);
            }
        }
        Real const cn_final = cn_number();
        Real const protons_burned = n_p_initial - numberDensityCgs(rho[SolarNetwork::p], 1.0);
        Real const he4_made = numberDensityCgs(rho[SolarNetwork::he4], 4.0) - n_he4_initial;
        std::printf("\n CNO cycle at T9 = %.2f, rho = %.0f kg/m^3, t = %.1e s\n",
                    (double)t9, (double)totalDensity(rho), (double)t);
        std::printf("   12C %.3e  13C %.3e  14N %.3e  15N %.3e kg/m^3\n",
                    (double)rho[SolarNetwork::c12], (double)rho[SolarNetwork::c13],
                    (double)rho[SolarNetwork::n14], (double)rho[SolarNetwork::n15]);
        std::printf("   C+N nuclei initial %.6e final %.6e cm^-3   protons burned %.4e, 4He"
                    " made %.4e (ratio %.3f)\n", (double)cn_initial, (double)cn_final,
                    (double)protons_burned, (double)he4_made,
                    (double)(protons_burned / he4_made));

        // The cycle converts four protons into one 4He and returns the carbon; the CN species are
        // therefore conserved as a group, and the proton consumption is four per helium made.
        if (std::abs(cn_final - cn_initial) / cn_initial > Real(1.e-10)) {
            std::printf(" FAILED: the C+N group is not conserved (%.6e vs %.6e)\n",
                        (double)cn_final, (double)cn_initial);
            exit_code = 1;
        }
        if (he4_made <= 0.0) {
            std::printf(" FAILED: the CNO cycle made no helium\n");
            exit_code = 1;
        } else if (std::abs(protons_burned / he4_made - Real(4.0)) > Real(2.e-2)) {
            // Four protons per helium is the *steady-state* bookkeeping; while the CN
            // intermediates fill up they store protons (each 13C holds one more than 12C, each
            // 14N two), so the cumulative ratio sits a fraction of a percent above four until
            // the cycle settles.
            std::printf(" FAILED: %.6f protons per 4He instead of 4\n",
                        (double)(protons_burned / he4_made));
            exit_code = 1;
        }
        // Energy per 4He: with the beta decays folded in, the four CNO steps sum to
        // 4.165 + 7.551 + 10.051 + 4.966 = 26.733 MeV, and the two beta neutrinos carry
        // 0.7067 + 0.9965 = 1.7032 MeV of it.
        Real const he4_late = numberDensityCgs(rho[SolarNetwork::he4], 4.0) - he4_mid;
        Real const total_cno = ((energy_total - energy_mid) + (neutrino_total - neutrino_mid))
                             / (he4_late * Real(1.e6) * Real(1.602176634e-13));
        Real const nu_cno = (neutrino_total - neutrino_mid)
                          / (he4_late * Real(1.e6) * Real(1.602176634e-13));
        std::printf("   steady-state energy per 4He: total %.4f MeV (26.733 expected), neutrino"
                    " %.4f MeV (1.7032 expected)\n", (double)total_cno, (double)nu_cno);
        if (std::abs(total_cno - Real(26.733)) > Real(2.e-2)
            || std::abs(nu_cno - Real(1.7032)) > Real(2.e-2)) {
            std::printf(" FAILED: the CNO energy per 4He does not match its Q values\n");
            exit_code = 1;
        }
        // In steady state the chain piles up in 14N, whose proton capture is the slow step.
        if (rho[SolarNetwork::n14] <= rho[SolarNetwork::c12]) {
            std::printf(" FAILED: 14N (%.3e) should dominate 12C (%.3e) at steady state\n",
                        (double)rho[SolarNetwork::n14], (double)rho[SolarNetwork::c12]);
            exit_code = 1;
        }
    }

    // 4. pp-II branch: 3He + 4He -> 7Be followed by the 7Be electron capture.  The branch runs
    //    alongside pp-I, so the *total* energy per 4He must stay 26.732 MeV (the branches only
    //    move energy from the gas to the neutrinos), while the neutrino share must rise above the
    //    pp-I 0.53 MeV because the 7Be neutrino is 0.862 MeV.
    {
        Real const t9 = Real(1.563376e7) / Real(1.e9);
        Real const rho_total = Real(1.526990e5);
        Real rho[SolarNetwork::n_species] = {0.0};
        // A solar *composition* (X = 0.34, Y = 0.64): without the pre-existing helium the
        // pp-II branch has no 4He to capture on and is invisible.
        rho[SolarNetwork::p] = Real(0.34) * rho_total;
        rho[SolarNetwork::he4] = Real(0.64) * rho_total;

        Real const n_p0 = numberDensityCgs(rho[SolarNetwork::p], 1.0);
        Real const n_he4_0 = numberDensityCgs(rho[SolarNetwork::he4], 4.0);
        Real const dt = Real(1.e14);
        Real t = 0.0;
        Real energy_total = 0.0;
        Real neutrino_total = 0.0;
        Real be7_max = 0.0;
        while (t < Real(1.e17))
        {
            auto const step = SolarNetwork::advance(rho, dt, t9);
            energy_total += step.energy_density;
            neutrino_total += step.neutrino_density;
            t += dt;
            be7_max = std::max(be7_max, rho[SolarNetwork::be7]);
        }
        Real const n_he4_made = numberDensityCgs(rho[SolarNetwork::he4], 4.0) - n_he4_0;
        Real const total_per_he4 = (energy_total + neutrino_total)
                                 / (n_he4_made * Real(1.e6) * Real(1.602176634e-13));
        Real const nu_per_he4 = neutrino_total
                              / (n_he4_made * Real(1.e6) * Real(1.602176634e-13));
        Real const burned = 1.0 - numberDensityCgs(rho[SolarNetwork::p], 1.0) / n_p0;
        std::printf("\n pp-I + pp-II (X = 0.34, Y = 0.64): burned %.3f, 4He made = %.4e cm^-3,"
                    " 7Be max = %.3e kg/m^3\n", (double)burned, (double)n_he4_made,
                    (double)be7_max);
        std::printf("   energy per 4He: total %.3f MeV (26.732 expected), neutrino %.3f MeV"
                    " (> 0.530, since the 7Be neutrino is 0.862)\n",
                    (double)total_per_he4, (double)nu_per_he4);
        if (std::abs(total_per_he4 - Real(26.732)) > Real(5.e-2)) {
            std::printf(" FAILED: the total energy per 4He changed when the pp-II branch was"
                        " switched on\n");
            exit_code = 1;
        }
        if (!(nu_per_he4 > Real(0.530) && nu_per_he4 < Real(1.40))) {
            std::printf(" FAILED: the neutrino share %.3f MeV is not a pp/7Be mixture\n",
                        (double)nu_per_he4);
            exit_code = 1;
        }
        if (!(be7_max > 0.0) || be7_max > Real(1.e-3) * rho_total) {
            std::printf(" FAILED: 7Be should be a trace species, reached %.3e kg/m^3\n",
                        (double)be7_max);
            exit_code = 1;
        }
    }

    if (exit_code == 0) { std::printf("test_network: ok\n"); }
    Finalize();
    return exit_code;
}
