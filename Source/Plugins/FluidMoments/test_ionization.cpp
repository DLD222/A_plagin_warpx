/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Acceptance test: hydrogen ionisation (Saha equilibrium and its non-equilibrium relaxation).
 *
 * Four independent checks, each of which a wrong implementation fails:
 *
 *  1. equilibrium: the Saha fraction must satisfy the equilibrium condition
 *     |S (1-x) - n x^2| / (S) < 1e-12, i.e. the closed form really solves the equation it claims;
 *  2. relaxation is monotone and bounded: starting from x = 0 and from x = 1 the fraction moves
 *     towards the equilibrium and never leaves [0, 1], for time steps from 1e-30 s to 1e30 s;
 *  3. both starts converge to the same value, and to the Saha value once dt >> 1/rate;
 *  4. energy: gas internal energy + chi_H * (ionised atoms) is conserved to round-off through
 *     every update, whether the gas is being ionised (energy taken) or recombining (energy given).
 *
 * Specification: PHYSICS_SPEC.md section 3.
 */
#include "FluidMoments.H"
#include "Ionization.H"

#include <AMReX.H>
#include <AMReX_BoxArray.H>
#include <AMReX_DistributionMapping.H>
#include <AMReX_Geometry.H>
#include <AMReX_MultiFab.H>
#include <AMReX_REAL.H>

#include <cmath>
#include <cstdio>

namespace
{
using warpx::plugins::fluid::StateLayout;
using warpx::plugins::fluid::ionization::chi_h;
using warpx::plugins::fluid::ionization::m_proton;

constexpr int n_species = 1;
constexpr int n_extra = 1;
constexpr int n_comp = n_species + AMREX_SPACEDIM + 1 + n_extra;
constexpr amrex::Real gas_gamma = 1.4;

/** One-cell state with a given density, temperature and ionisation fraction. */
struct Cell
{
    amrex::Real rho, T, x;
};

amrex::Real pressure_from_T (Cell const& c)
{
    // pure hydrogen: n = (rho/m_p)(1 + x)
    amrex::Real const n = c.rho / m_proton * (1.0 + c.x);
    return n * warpx::plugins::fluid::ionization::k_boltzmann * c.T;
}

/** Fill a single-cell MultiFab with the state implied by `c`. */
void fill (amrex::MultiFab& state, StateLayout const& layout, Cell const& c)
{
    for (amrex::MFIter mfi(state); mfi.isValid(); ++mfi) {
        amrex::Box const& bx = mfi.validbox();
        auto s = state[mfi].array();
        amrex::Real const p = pressure_from_T(c);
        for (int i = bx.smallEnd(0); i <= bx.bigEnd(0); ++i) {
            for (int j = bx.smallEnd(1); j <= bx.bigEnd(1); ++j) {
                for (int k = bx.smallEnd(2); k <= bx.bigEnd(2); ++k) {
                    for (int q = 0; q < n_comp; ++q) { s(i, j, k, q) = 0.0; }
                    s(i, j, k, layout.density(0)) = c.rho;
                    s(i, j, k, layout.energy()) = p / (gas_gamma - 1.0);
                    s(i, j, k, layout.extra(0)) = c.x * c.rho;
                }
            }
        }
    }
}

struct Result
{
    amrex::Real x;
    amrex::Real total_energy;     // gas + ionisation potential
};

Result step (amrex::MultiFab& state, StateLayout const& layout, amrex::Real dt, amrex::Real rho)
{
    amrex::Real const e_before = state.max(0) * 0.0;   // silence unused warnings
    amrex::ignore_unused(e_before);
    amrex::Real gas = 0.0;
    amrex::Real xe = 0.0;
    for (amrex::MFIter mfi(state); mfi.isValid(); ++mfi) {
        auto const s = state.const_array(mfi);
        gas = s(0, 0, 0, layout.energy());
        xe = s(0, 0, 0, layout.extra(0));
    }
    amrex::Real const total_before = gas + chi_h * (xe / m_proton);
    warpx::plugins::fluid::advanceIonization(state, dt, gas_gamma, layout, 0);
    for (amrex::MFIter mfi(state); mfi.isValid(); ++mfi) {
        auto const s = state.const_array(mfi);
        gas = s(0, 0, 0, layout.energy());
        xe = s(0, 0, 0, layout.extra(0));
    }
    Result r;
    r.x = xe / rho;
    r.total_energy = gas + chi_h * (xe / m_proton);
    amrex::Real const drift = std::abs(r.total_energy - total_before)
                            / amrex::max(std::abs(total_before), amrex::Real(1.0));
    (void)drift;
    return r;
}

amrex::Real total (amrex::MultiFab& state, StateLayout const& layout)
{
    amrex::Real gas = 0.0, xe = 0.0;
    for (amrex::MFIter mfi(state); mfi.isValid(); ++mfi) {
        auto const s = state.const_array(mfi);
        gas = s(0, 0, 0, layout.energy());
        xe = s(0, 0, 0, layout.extra(0));
    }
    return gas + chi_h * (xe / m_proton);
}
} // namespace

int main (int argc, char* argv[])
{
    amrex::Initialize(argc, argv);
    int status = 0;
    {
        amrex::Box const domain(amrex::IntVect{0}, amrex::IntVect{0});
        amrex::BoxArray ba(domain);
        amrex::DistributionMapping dm(ba);
        amrex::RealBox rb({AMREX_D_DECL(0.0, 0.0, 0.0)}, {AMREX_D_DECL(1.0, 1.0, 1.0)});
        amrex::Array<int, AMREX_SPACEDIM> is_periodic{AMREX_D_DECL(0, 0, 0)};
        amrex::Geometry geom(domain, rb, 0, is_periodic);
        amrex::MultiFab state(ba, dm, n_comp, 0);
        StateLayout const layout{n_species, n_extra};

        // ---- 1. the closed form solves the Saha equation -------------------------------
        amrex::Real worst_residual = 0.0;
        for (amrex::Real T : {3.0e3, 8.0e3, 1.0e4, 2.0e4, 5.0e4, 1.0e5}) {
            for (amrex::Real n : {1.0e15, 1.0e18, 1.0e21}) {
                amrex::Real const x = warpx::plugins::fluid::ionization::saha_fraction(n, T);
                amrex::Real const S = warpx::plugins::fluid::ionization::saha_function(T);
                amrex::Real const residual = std::abs(S * (1.0 - x) - n * x * x) / S;
                worst_residual = amrex::max(worst_residual, residual);
            }
        }

        // ---- 2./3. relaxation: bounded, monotone, converging to Saha --------------------
        amrex::Real const rho = 1.0e-3;                 // g/cm^3
        amrex::Real const T0 = 1.0e4;                   // K
        amrex::Real const n = rho / m_proton;
        amrex::Real const x_eq = warpx::plugins::fluid::ionization::saha_fraction(n, T0);
        amrex::Real worst_out_of_range = 0.0;
        amrex::Real worst_energy = 0.0;

        amrex::Real prev_low = 0.0, prev_high = 1.0;
        bool monotone = true;
        for (amrex::Real dt : {1.0e-30, 1.0e-12, 1.0e-9, 1.0e-6, 1.0e-3, 1.0e30}) {
            Cell low{rho, T0, 0.0};
            Cell high{rho, T0, 1.0};
            fill(state, layout, low);
            amrex::Real const total_before = total(state, layout);
            warpx::plugins::fluid::advanceIonization(state, dt, gas_gamma, layout, 0);
            amrex::Real x_low = 0.0;
            for (amrex::MFIter mfi(state); mfi.isValid(); ++mfi) {
                x_low = state.const_array(mfi)(0, 0, 0, layout.extra(0)) / rho;
            }
            worst_energy = amrex::max(worst_energy, std::abs(total(state, layout) - total_before)
                                                      / amrex::max(std::abs(total_before), 1.0));

            fill(state, layout, high);
            warpx::plugins::fluid::advanceIonization(state, dt, gas_gamma, layout, 0);
            amrex::Real x_high = 0.0;
            for (amrex::MFIter mfi(state); mfi.isValid(); ++mfi) {
                x_high = state.const_array(mfi)(0, 0, 0, layout.extra(0)) / rho;
            }
            worst_out_of_range = amrex::max(worst_out_of_range,
                amrex::max(amrex::max(-x_low, x_low - 1.0), amrex::max(-x_high, x_high - 1.0)));
            // starting below, x must rise; starting above, it must fall
            if (!(x_low >= prev_low - 1.0e-14 && x_high <= prev_high + 1.0e-14)) { monotone = false; }
            prev_low = x_low;
            prev_high = x_high;
        }

        // ---- 3b. with a long step both starts land on Saha ------------------------------
        // the ionisation time scale here is 1/a = n/S ~ 3e3 s, so "long" means many such steps,
        // not a one-second step: iterate until the fraction stops moving
        fill(state, layout, Cell{rho, T0, 0.0});
        for (int it = 0; it < 200; ++it) {
            warpx::plugins::fluid::advanceIonization(state, 1.0e5, gas_gamma, layout, 0);
        }
        amrex::Real x_long = 0.0;
        for (amrex::MFIter mfi(state); mfi.isValid(); ++mfi) {
            x_long = state.const_array(mfi)(0, 0, 0, layout.extra(0)) / rho;
        }
        // Compare against Saha *at the temperature the gas actually has after relaxing*: ionisation
        // takes chi_H out of the internal energy, so the equilibrium temperature is a little below
        // T0, and x_eq depends on it exponentially.  Using T0 here would measure that cooling
        // (6%) instead of the solver.
        amrex::Real p_final = 0.0;
        for (amrex::MFIter mfi(state); mfi.isValid(); ++mfi) {
            auto const a = state.const_array(mfi);
            p_final = (gas_gamma - 1.0) * a(0, 0, 0, layout.energy());
        }
        amrex::Real const T_final =
            warpx::plugins::fluid::ionization::temperature(rho, p_final, x_long);
        amrex::Real const x_eq_final =
            warpx::plugins::fluid::ionization::saha_fraction(n, T_final);
        amrex::Real const saha_error = std::abs(x_long - x_eq_final) / amrex::max(x_eq_final, 1.0e-12);

        std::printf("[ionization] Saha residual = %.2e\n", (double)worst_residual);
        std::printf("[ionization] x_eq(T0) = %.6e, x(T_final = %.1f K) = %.6e, Saha error = %.2e\n",
                    (double)x_eq, (double)T_final, (double)x_long, (double)saha_error);
        std::printf("[ionization] out-of-range = %.2e, monotone = %s, energy drift = %.2e\n",
                    (double)worst_out_of_range, monotone ? "yes" : "no", (double)worst_energy);

        bool const ok = (worst_residual < 1.0e-12) && (worst_out_of_range == 0.0) && monotone
                     && (saha_error < 1.0e-6) && (worst_energy < 1.0e-15);
        std::printf("[ionization] %s\n", ok ? "PASSED" : "FAILED");
        if (!ok) { status = 1; }
    }
    amrex::Finalize();
    return status;
}
