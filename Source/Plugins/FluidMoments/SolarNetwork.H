/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_SOLARNETWORK_H_
#define WARPX_SOLARNETWORK_H_

#include "Cf88Rates.H"

#include <AMReX_GpuQualifiers.H>
#include <AMReX_REAL.H>

#include <algorithm>
#include <cmath>

/** The reduced solar reaction network: pp-I plus the CNO main cycle.
 *
 * Specification: Docs/research_notes/star_plugins/PHYSICS_SPEC.md sections 3.1-3.3.  The two
 * chains are
 *
 *     p + p -> D            (CF88 entry 2)      D + p -> 3He        (4)
 *     3He + 3He -> 4He+2p   (18)                [pp-I complete]
 *
 *     12C + p -> 13C        (61)                13C + p -> 14N      (64)
 *     14N + p -> 15N        (71)                15N + p -> 12C+4He  (78)
 *
 * The two beta decays (13N -> 13C, 15O -> 15N) are folded into entries 61 and 71: their lifetimes
 * are minutes against nuclear timescales of years, so the effective step rate is the nuclear one.
 * The pp-II/pp-III branches and the 15N(p,gamma)16O leak are *not* in this network yet - they
 * need the 7Be electron capture (entry 41) and 7Be(p,gamma)8B (42) with a density-dependent rate
 * law, and they are marked [todo-src] in the specification together with the neutrino energies.
 *
 * Mass numbers balance in every reaction (1+1 = 2, 2+1 = 3, 3+3 = 4+1+1, 12+1 = 13, ...), which
 * the test checks as a bookkeeping invariant.
 *
 * The module works in cgs internally (rates are tabulated in cm^3 s^-1 and energies in MeV) and
 * converts at its boundary, where the state is SI.
 */
namespace warpx::plugins::fluid
{

class SolarNetwork
{
public:
    enum Species : int { p = 0, d = 1, he3 = 2, he4 = 3, c12 = 4, c13 = 5, n14 = 6, n15 = 7 };
    static constexpr int n_species = 8;

    //! Mass numbers, used to turn mass densities into number densities.
    AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE
    static amrex::Real massNumber (int s) noexcept
    {
        constexpr amrex::Real a[n_species] = {1.0, 2.0, 3.0, 4.0, 12.0, 13.0, 14.0, 15.0};
        return a[s];
    }

    AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE
    static const char* speciesName (int s) noexcept
    {
        constexpr const char* names[n_species] = {"p", "d", "he3", "he4", "c12", "c13", "n14",
                                                  "n15"};
        return names[s];
    }

    struct Reaction
    {
        int a;               //!< first reactant
        int b;               //!< second reactant
        int n_products;      //!< number of product entries used
        int c;               //!< first product
        int e;               //!< second product (-1 when unused)
        int third;           //!< third product (-1 when unused)
        amrex::Real q_mev;   //!< total energy release [MeV]
        amrex::Real q_nu_mev;//!< part of it carried away by neutrinos [MeV]
        int cf88_entry;      //!< CF88 entry number, i.e. the fit that drives it
    };

    static constexpr int n_reactions = 7;

    AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE
    static Reaction reaction (int r) noexcept
    {
        // The p+p and 3He+3He reactions have repeated products (2p) or identical reactants; the
        // product list below spells them out so that the stoichiometry is readable.
        // The Q values are the CF88 table's, plus the beta decays that are folded into entries
        // 61 and 71 (PHYSICS_SPEC 3.4 and its transcription notes): 13N -> 13C + e+ + nu with
        // Q = 2.221 MeV and 15O -> 15N + e+ + nu with Q = 2.754 MeV.  The neutrino column holds
        // the mean neutrino energies: 0.265 MeV for p+p, 0.7067 MeV for 13N and 0.9965 MeV for
        // 15O.
        constexpr Reaction table[n_reactions] = {
            {p,   p,   1, d,   -1,  -1,   1.442, 0.265,  2},
            {d,   p,   1, he3, -1,  -1,   5.494, 0.0,    4},
            {he3, he3, 3, he4, p,   p,   12.860, 0.0,   18},
            {c12, p,   1, c13, -1,  -1,   4.165, 0.7067, 61},
            {c13, p,   1, n14, -1,  -1,   7.551, 0.0,   64},
            {n14, p,   1, n15, -1,  -1,  10.051, 0.9965, 71},
            {n15, p,   2, c12, he4, -1,   4.966, 0.0,   78}
        };
        return table[r];
    }

    /** <sigma v> of reaction r in cm^3/s at T9 = T / 1e9 K.
     *
     *  Of the eleven CF88 fits this network uses the seven whose rate law is unambiguously
     *  two-body; the electron captures (41) and the 7Be(p,gamma)8B (42) are [todo-src]. */
    AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE
    static amrex::Real sigmaV (int r, amrex::Real t9) noexcept
    {
        using namespace cf88;
        amrex::Real f = 0.0;
        switch (reaction(r).cf88_entry) {
            case 2:  f = f2(t9);  break;
            case 4:  f = f4(t9);  break;
            case 18: f = f18(t9); break;
            case 61: f = f61(t9); break;
            case 64: f = f64(t9); break;
            case 71: f = f71(t9); break;
            case 78: f = f78(t9); break;
            default: f = 0.0;     break;
        }
        return f / N_A;
    }

    /** Energy released per reaction [erg], from the tabulated Q value. */
    AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE
    static amrex::Real energyPerReaction (int r) noexcept
    {
        return reaction(r).q_mev * amrex::Real(1.602176634e-6);
    }

    /** Part of that energy carried away by neutrinos [erg]; it does not heat the gas. */
    AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE
    static amrex::Real neutrinoEnergyPerReaction (int r) noexcept
    {
        return reaction(r).q_nu_mev * amrex::Real(1.602176634e-6);
    }

    /** Number density [cm^-3] of a species from its mass density [g/cm^3]. */
    AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE
    static amrex::Real numberDensity (amrex::Real rho_g_cm3) noexcept
    {
        constexpr amrex::Real m_u = 1.66053906660e-24;   // g
        return rho_g_cm3 / m_u;
    }

    /** Reaction rate per unit volume of reaction r [cm^-3 s^-1].
     *
     *  r = n_a n_b <sigma v> / (1 + delta_ab): the two identical-particle reactions carry the
     *  symmetry factor 1/2. */
    AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE
    static amrex::Real ratePerVolume (int r, amrex::Real const* rho_species_g_cm3,
                                      amrex::Real t9) noexcept
    {
        Reaction const re = reaction(r);
        amrex::Real const n_a = numberDensity(rho_species_g_cm3[re.a])
                              / massNumber(re.a);
        amrex::Real const n_b = numberDensity(rho_species_g_cm3[re.b])
                              / massNumber(re.b);
        amrex::Real const symmetry = (re.a == re.b) ? amrex::Real(0.5) : amrex::Real(1.0);
        return symmetry * n_a * n_b * sigmaV(r, t9);
    }

    struct StepResult
    {
        amrex::Real energy_density = 0.0;    //!< energy deposited in the gas per volume [J m^-3]
        amrex::Real neutrino_density = 0.0;  //!< energy escaping as neutrinos per volume [J m^-3]
        amrex::Real rate_he4 = 0.0;          //!< net 4He production [cm^-3 s^-1], for the tests
        int iterations = 0;                  //!< Newton iterations used, for diagnostics
    };

    /** Right-hand side of the network: species event rates [cm^-3 s^-1] and the energy release
     *  per volume [erg cm^-3 s^-1], given mass densities in g/cm^3. */
    AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE
    static void rightHandSide (amrex::Real const* rho_cgs, amrex::Real t9,
                               amrex::Real* event_rate, amrex::Real& energy_rate,
                               amrex::Real& neutrino_rate, amrex::Real& rate_he4) noexcept
    {
        for (int s = 0; s < n_species; ++s) { event_rate[s] = 0.0; }
        energy_rate = 0.0;
        neutrino_rate = 0.0;
        rate_he4 = 0.0;
        for (int r = 0; r < n_reactions; ++r)
        {
            Reaction const re = reaction(r);
            amrex::Real const rate = ratePerVolume(r, rho_cgs, t9);
            energy_rate += (energyPerReaction(r) - neutrinoEnergyPerReaction(r)) * rate;
            neutrino_rate += neutrinoEnergyPerReaction(r) * rate;

            event_rate[re.a] -= rate;
            event_rate[re.b] -= rate;
            event_rate[re.c] += rate;
            if (re.e >= 0) { event_rate[re.e] += rate; }
            if (re.third >= 0) { event_rate[re.third] += rate; }

            int const he4_produced = ((re.c == he4) ? 1 : 0) + ((re.e == he4) ? 1 : 0)
                                   + ((re.third == he4) ? 1 : 0);
            rate_he4 += he4_produced * rate;
        }
    }

    /** Advance the species mass densities [kg m^-3] by dt.
     *
     *  The network is stiff by many orders of magnitude: in the pp chain D and 3He reach their
     *  quasi-equilibrium on timescales ~1e-4 s while the protons burn over ~1e8 s, so an explicit
     *  step would have to be of the fast timescale forever.  The step is therefore a backward
     *  Euler solve,
     *
     *      rho^{n+1} = rho^n + dt * f(rho^{n+1}) ,
     *
     *  solved by Newton with a numerically differenced Jacobian and a positivity-preserving
     *  damping.  Backward Euler is first order in dt, which is all this project needs (the
     *  reaction and hydrodynamic timescales are far apart), and it keeps every density positive
     *  for a network of this form.
     */
    static StepResult advance (amrex::Real* rho_si, amrex::Real dt, amrex::Real t9)
    {
        constexpr amrex::Real m_u = 1.66053906660e-24;      // g
        constexpr int n = n_species;

        // SI (kg/m^3) -> cgs (g/cm^3); the network works in cgs throughout.
        amrex::Real y[n];
        for (int s = 0; s < n; ++s) { y[s] = rho_si[s] * amrex::Real(1.e-3); }

        // f(rho)_s = A_s m_u * event_rate_s  [g cm^-3 s^-1]
        amrex::Real neutrino_rate = 0.0;
        auto rhs = [&] (amrex::Real const* rho, amrex::Real* f) {
            amrex::Real event_rate[n];
            amrex::Real energy_rate = 0.0;
            amrex::Real he4_rate = 0.0;
            rightHandSide(rho, t9, event_rate, energy_rate, neutrino_rate, he4_rate);
            for (int s = 0; s < n; ++s) {
                f[s] = event_rate[s] * massNumber(s) * m_u;
            }
            return energy_rate;
        };

        amrex::Real residual[n];
        amrex::Real jacobian[n][n];
        amrex::Real f[n];

        int iterations = 0;
        StepResult result;
        for (int iter = 0; iter < 60; ++iter)
        {
            iterations = iter;
            amrex::Real const energy_rate = rhs(y, f);
            amrex::Real residual_norm = 0.0;
            for (int s = 0; s < n; ++s) {
                residual[s] = y[s] - rho_si[s] * amrex::Real(1.e-3) - dt * f[s];
                residual_norm = std::max(residual_norm, std::abs(residual[s]));
            }
            amrex::Real scale = 0.0;
            for (int s = 0; s < n; ++s) { scale = std::max(scale, std::abs(y[s])); }
            if (residual_norm <= amrex::Real(1.e-14) * scale) {
                // erg/cm^3 -> J/m^3 is a factor 0.1; times dt for the step's energy.
                result.energy_density = energy_rate * amrex::Real(0.1) * dt;
                result.neutrino_density = neutrino_rate * amrex::Real(0.1) * dt;
                break;
            }

            // Numerical Jacobian of the residual: dR_s/dy_t = delta_st - dt * df_s/dy_t.
            for (int t = 0; t < n; ++t) {
                amrex::Real const h = std::max(amrex::Real(1.e-8) * std::abs(y[t]),
                                                 amrex::Real(1.e-30));
                amrex::Real y_plus[n];
                for (int s = 0; s < n; ++s) { y_plus[s] = y[s]; }
                y_plus[t] += h;
                amrex::Real f_plus[n];
                rhs(y_plus, f_plus);
                for (int s = 0; s < n; ++s) {
                    jacobian[s][t] = -dt * (f_plus[s] - f[s]) / h;
                }
                jacobian[t][t] += amrex::Real(1.0);
            }

            // Solve J * delta = -residual by Gaussian elimination with partial pivoting.
            amrex::Real matrix[n][n + 1];
            for (int s = 0; s < n; ++s) {
                for (int t = 0; t < n; ++t) { matrix[s][t] = jacobian[s][t]; }
                matrix[s][n] = -residual[s];
            }
            for (int col = 0; col < n; ++col) {
                int pivot = col;
                for (int row = col + 1; row < n; ++row) {
                    if (std::abs(matrix[row][col]) > std::abs(matrix[pivot][col])) { pivot = row; }
                }
                if (std::abs(matrix[pivot][col]) < amrex::Real(1.e-300)) { break; }
                for (int t = col; t <= n; ++t) {
                    amrex::Real const tmp = matrix[col][t];
                    matrix[col][t] = matrix[pivot][t];
                    matrix[pivot][t] = tmp;
                }
                for (int row = col + 1; row < n; ++row) {
                    amrex::Real const factor = matrix[row][col] / matrix[col][col];
                    for (int t = col; t <= n; ++t) { matrix[row][t] -= factor * matrix[col][t]; }
                }
            }
            amrex::Real delta[n];
            for (int s = n - 1; s >= 0; --s) {
                amrex::Real sum = matrix[s][n];
                for (int t = s + 1; t < n; ++t) { sum -= matrix[s][t] * delta[t]; }
                delta[s] = (std::abs(matrix[s][s]) > amrex::Real(1.e-300))
                    ? sum / matrix[s][s] : amrex::Real(0.0);
            }

            // Damped update: halve the step until every density stays positive.
            amrex::Real damping = amrex::Real(1.0);
            bool accepted = false;
            for (int attempt = 0; attempt < 30; ++attempt) {
                bool positive = true;
                amrex::Real trial[n];
                for (int s = 0; s < n; ++s) {
                    trial[s] = y[s] + damping * delta[s];
                    if (trial[s] < amrex::Real(0.0)) { positive = false; }
                }
                if (positive) {
                    for (int s = 0; s < n; ++s) { y[s] = trial[s]; }
                    accepted = true;
                    break;
                }
                damping *= amrex::Real(0.5);
            }
            if (!accepted) { break; }
        }
        result.iterations = iterations;

        // cgs -> SI (1 g/cm^3 = 1e3 kg/m^3).
        for (int s = 0; s < n; ++s) { rho_si[s] = y[s] * amrex::Real(1.e3); }
        return result;
    }
};

} // namespace warpx::plugins::fluid

#endif // WARPX_SOLARNETWORK_H_
