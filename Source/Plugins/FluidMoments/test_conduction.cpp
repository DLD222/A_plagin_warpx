/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Acceptance test: electron thermal conduction.
 *
 * Three checks:
 *
 *  1. constant conductivity: a sinusoidal temperature perturbation in a periodic box decays as
 *     exp(-alpha k^2 t) with alpha = kappa/(rho c_v) - the analytic solution of the linear diffusion
 *     equation, which is why the constant-conductivity entry point exists;
 *  2. energy conservation: with Spitzer conductivity (kappa ~ T^{5/2}, nonlinear) the total energy of
 *     the periodic box must be unchanged to round-off, because the flux form only moves energy
 *     between cells;
 *  3. Spitzer scaling: a large-amplitude temperature wave must decay faster with Spitzer conductivity
 *     than with the constant conductivity evaluated at the mean temperature, because kappa grows like
 *     T^{5/2}; and the temperature must never go negative.
 *
 * Specification: PHYSICS_SPEC.md section 5.
 */
#include "FluidMoments.H"
#include "Conduction.H"

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
using namespace warpx::plugins::fluid::conduction;

constexpr int n_cell = 64;
constexpr int n_species = 1;
constexpr int n_extra = 1;
constexpr int n_comp = n_species + AMREX_SPACEDIM + 1 + n_extra;
constexpr amrex::Real gas_gamma = 1.4;

amrex::Real temperature_of (amrex::Real e_int, amrex::Real rho)
{
    // pure, fully ionised hydrogen: n = (rho/m_p)(1 + x) with x = 1
    return (gas_gamma - 1.0) * e_int / (rho / m_proton * 2.0 * k_boltzmann);
}

void fill_wave (amrex::MultiFab& state, StateLayout const& layout, amrex::Geometry const& geom,
                amrex::Real rho, amrex::Real T0, amrex::Real amp)
{
    amrex::Real const dx = geom.CellSize(0);
    amrex::Real const k = 2.0 * M_PI;
    for (amrex::MFIter mfi(state); mfi.isValid(); ++mfi) {
        amrex::Box const& bx = mfi.validbox();
        auto s = state[mfi].array();
        for (int i = bx.smallEnd(0); i <= bx.bigEnd(0); ++i) {
            amrex::Real const xcoord = (static_cast<amrex::Real>(i) + 0.5) * dx - 0.5;
            amrex::Real const T = T0 + amp * std::cos(k * xcoord);
            amrex::Real const p = rho / m_proton * 2.0 * k_boltzmann * T;
            for (int j = bx.smallEnd(1); j <= bx.bigEnd(1); ++j) {
                for (int q = bx.smallEnd(2); q <= bx.bigEnd(2); ++q) {
                    for (int c = 0; c < n_comp; ++c) { s(i, j, q, c) = 0.0; }
                    s(i, j, q, layout.density(0)) = rho;
                    s(i, j, q, layout.energy()) = p / (gas_gamma - 1.0);
                    s(i, j, q, layout.extra(0)) = rho;   // x = 1
                }
            }
        }
    }
}

amrex::Real total_energy (amrex::MultiFab& state, StateLayout const& layout)
{
    amrex::Real sum = 0.0;
    for (amrex::MFIter mfi(state); mfi.isValid(); ++mfi) {
        auto const s = state.const_array(mfi);
        amrex::Box const& bx = mfi.validbox();
        for (int i = bx.smallEnd(0); i <= bx.bigEnd(0); ++i) {
            for (int j = bx.smallEnd(1); j <= bx.bigEnd(1); ++j) {
                for (int q = bx.smallEnd(2); q <= bx.bigEnd(2); ++q) {
                    sum += s(i, j, q, layout.energy());
                }
            }
        }
    }
    return sum;
}

/** Amplitude of the cos(kx) mode of T. */
amrex::Real mode_amplitude (amrex::MultiFab& state, StateLayout const& layout,
                            amrex::Geometry const& geom)
{
    amrex::Real const dx = geom.CellSize(0);
    amrex::Real const k = 2.0 * M_PI;
    amrex::Real sum = 0.0;
    amrex::Real rho = 0.0;
    for (amrex::MFIter mfi(state); mfi.isValid(); ++mfi) {
        auto const s = state.const_array(mfi);
        rho = s(0, 0, 0, layout.density(0));
        amrex::Box const& bx = mfi.validbox();
        for (int i = bx.smallEnd(0); i <= bx.bigEnd(0); ++i) {
            amrex::Real const xcoord = (static_cast<amrex::Real>(i) + 0.5) * dx - 0.5;
            amrex::Real const e_int = s(i, 0, 0, layout.energy());
            sum += temperature_of(e_int, rho) * std::cos(k * xcoord);
        }
    }
    return 2.0 * sum / n_cell;
}

amrex::Real min_temperature (amrex::MultiFab& state, StateLayout const& layout)
{
    amrex::Real worst = 1.0e300;
    amrex::Real rho = 0.0;
    for (amrex::MFIter mfi(state); mfi.isValid(); ++mfi) {
        auto const s = state.const_array(mfi);
        rho = s(0, 0, 0, layout.density(0));
        amrex::Box const& bx = mfi.validbox();
        for (int i = bx.smallEnd(0); i <= bx.bigEnd(0); ++i) {
            worst = amrex::min(worst, temperature_of(s(i, 0, 0, layout.energy()), rho));
        }
    }
    return worst;
}
} // namespace

int main (int argc, char* argv[])
{
    amrex::Initialize(argc, argv);
    int status = 0;
    {
        amrex::Box const domain(amrex::IntVect{0}, amrex::IntVect{n_cell - 1});
        amrex::BoxArray ba(domain);
        amrex::DistributionMapping dm(ba);
        amrex::RealBox rb({AMREX_D_DECL(-0.5, -0.5, -0.5)}, {AMREX_D_DECL(0.5, 0.5, 0.5)});
        amrex::Array<int, AMREX_SPACEDIM> is_periodic{AMREX_D_DECL(1, 1, 1)};
        amrex::Real const dx = 1.0 / n_cell;
        amrex::Geometry geom(domain, rb, 0, is_periodic);
        StateLayout const layout{n_species, n_extra};
        amrex::MultiFab state(ba, dm, n_comp, 1);

        amrex::Real const rho = 1.0e-3;
        amrex::Real const T0 = 1.0e6;
        amrex::Real const k = 2.0 * M_PI;
        // fully ionised hydrogen: ions + electrons contribute, so c_v = 2 k / (m_p (gamma-1))
        amrex::Real const cv = 2.0 * k_boltzmann / (m_proton * (gas_gamma - 1.0));

        // ---- 1. constant conductivity: analytic sine decay ------------------------------
        amrex::Real const amp = 1.0e2;                 // small: kappa is essentially constant
        amrex::Real const kappa_c = spitzer_kappa(T0);
        amrex::Real const alpha = kappa_c / (rho * cv);
        fill_wave(state, layout, geom, rho, T0, amp);
        amrex::Real const a_const_0 = mode_amplitude(state, layout, geom);
        amrex::Real const t_c = 0.3 / (alpha * k * k);  // decay the mode by exp(-0.3)
        int const nsub_c = warpx::plugins::fluid::advanceThermalConduction(
            state, geom, t_c, gas_gamma, layout, 0, kappa_c);
        amrex::Real const a_const_1 = mode_amplitude(state, layout, geom);
        amrex::Real const decay_const = a_const_1 / a_const_0;
        amrex::Real const decay_exact = std::exp(-alpha * k * k * t_c);
        amrex::Real const const_error = std::abs(decay_const - decay_exact) / decay_exact;

        // ---- 2. Spitzer: energy conservation and positivity ----------------------------
        amrex::Real const amp2 = 5.0e5;                 // T from 5e5 to 1.5e6 K
        fill_wave(state, layout, geom, rho, T0, amp2);
        amrex::Real const e_before = total_energy(state, layout);
        amrex::Real const a_spitz_0 = mode_amplitude(state, layout, geom);
        amrex::Real const t_s = t_c;                    // same time window
        int const nsub_s = warpx::plugins::fluid::advanceThermalConduction(
            state, geom, t_s, gas_gamma, layout, 0);
        amrex::Real const e_after = total_energy(state, layout);
        amrex::Real const energy_drift = std::abs(e_after - e_before) / std::abs(e_before);
        amrex::Real const a_spitz_1 = mode_amplitude(state, layout, geom);
        amrex::Real const decay_spitz = a_spitz_1 / a_spitz_0;
        amrex::Real const T_min = min_temperature(state, layout);

        // ---- 3. Spitzer decays faster than a constant kappa at T0 ---------------------
        // (kappa(T) ~ T^{5/2}, and the hot half of the wave conducts more)
        bool const faster = (decay_spitz < decay_const);

        std::printf("[conduction] constant kappa: %d sub-steps, measured %.6f vs exact %.6f,"
                    " relative error = %.3e\n", nsub_c, (double)decay_const,
                    (double)decay_exact, (double)const_error);
        std::printf("[conduction] Spitzer: %d sub-steps, energy drift = %.3e, min T = %.3e K\n",
                    nsub_s, (double)energy_drift, (double)T_min);
        std::printf("[conduction] decay: Spitzer %.6f vs constant %.6f (faster: %s)\n",
                    (double)decay_spitz, (double)decay_const, faster ? "yes" : "no");

        bool const ok = (const_error < 5.0e-3) && (energy_drift < 1.0e-12)
                     && (T_min > 0.0) && faster;
        std::printf("[conduction] %s\n", ok ? "PASSED" : "FAILED");
        if (!ok) { status = 1; }
    }
    amrex::Finalize();
    return status;
}
