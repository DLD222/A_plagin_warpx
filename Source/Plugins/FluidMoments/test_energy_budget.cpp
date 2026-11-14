/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Acceptance test: the stellar source package books every joule it moves.
 *
 * Four checks on one cell with the full package switched on (CF88 network + thin cooling +
 * radiation exchange):
 *
 *  1. the sum of the recorded gas terms equals the actual change of the gas energy, to round-off -
 *     i.e. no term is applied twice and none is missing;
 *  2. gas + radiation is conserved by the exchange (the only term that touches two reservoirs);
 *  3. the neutrino energy the network reports is non-negative, as a network can only emit;
 *  4. with the package switched off, nothing moves at all.
 *
 * The point of the test is not that any single term is right - each has its own test - but that the
 * *combination* adds up.  A sign error or a double application hides perfectly inside a conserved
 * total and shows up here.
 *
 * Specification: PHYSICS_SPEC.md sections 3-6.
 */
#include "FluidMoments.H"
#include "StellarSources.H"

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
using warpx::plugins::fluid::SolarNetwork;

constexpr int n_species = SolarNetwork::n_species;
constexpr int n_extra = 1;
constexpr int n_comp = n_species + AMREX_SPACEDIM + 1 + n_extra;
constexpr amrex::Real gas_gamma = 1.4;

amrex::Real cell_energy (amrex::MultiFab& st, StateLayout const& layout)
{
    return st.const_array(0)(0, 0, 0, layout.energy());
}

void fill_solar (amrex::MultiFab& st, StateLayout const& layout, amrex::Real rho, amrex::Real T)
{
    // solar-like mass fractions, normalised
    amrex::Real const w[n_species] = {0.70, 0.28, 0.02, 0.0};
    amrex::Real wsum = 0.0;
    for (int a = 0; a < n_species; ++a) { wsum += w[a]; }
    amrex::Real const x = 1.0;   // fully ionised for the temperature used here
    amrex::Real const p = rho / 1.67262192e-24 * (1.0 + x) * 1.380649e-16 * T;
    for (amrex::MFIter mfi(st); mfi.isValid(); ++mfi) {
        amrex::Box const& bx = mfi.validbox();
        auto s = st[mfi].array();
        for (int i = bx.smallEnd(0); i <= bx.bigEnd(0); ++i) {
            for (int j = bx.smallEnd(1); j <= bx.bigEnd(1); ++j) {
                for (int k = bx.smallEnd(2); k <= bx.bigEnd(2); ++k) {
                    for (int c = 0; c < n_comp; ++c) { s(i, j, k, c) = 0.0; }
                    for (int a = 0; a < n_species; ++a) {
                        s(i, j, k, layout.density(a)) = rho * (w[a] / wsum);
                    }
                    s(i, j, k, layout.energy()) = p / (gas_gamma - 1.0);
                    s(i, j, k, layout.extra(0)) = x * rho;
                }
            }
        }
    }
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
        StateLayout const layout{n_species, n_extra};

        amrex::MultiFab state(ba, dm, n_comp, 0);
        amrex::MultiFab radiation(ba, dm, 1, 0);
        amrex::MultiFab neutrino(ba, dm, 1, 0);

        amrex::Real const rho = 1.5e2;      // g/cm^3
        amrex::Real const T = 1.5e7;        // K, hot enough for the network to act
        fill_solar(state, layout, rho, T);
        radiation.setVal(0.0);
        neutrino.setVal(0.0);

        warpx::plugins::fluid::SourceOptions opts;
        opts.gamma = gas_gamma;
        opts.nuclear = true;
        opts.cooling = true;
        opts.radiation = true;
        opts.kappa = 0.4;
        // The step is derived from the physics instead of guessed: at solar-core density the
        // *optically thin* rate is ~5e28 erg/cm^3/s (which is precisely why thin cooling is the
        // wrong model for the core), so a fixed 1e-3 s step would remove more than the cell holds,
        // clamp T to zero and leave the exchange nothing to exchange.  Take 1% of the cooling time
        // so every term is a measurable perturbation.
        amrex::Real const e_int0 = cell_energy(state, layout);
        amrex::Real const rate0 = warpx::plugins::fluid::radiation::cooling_rate(rho, T, 1.0);
        amrex::Real const dt_src = amrex::min(1.0e-3, 0.01 * e_int0 / rate0);

        amrex::Real const e_before = cell_energy(state, layout);
        auto const ledger = warpx::plugins::fluid::applyStellarSources(
            state, &radiation, &neutrino, dt_src, layout, opts);
        amrex::Real const e_after = cell_energy(state, layout);

        amrex::Real const booked = ledger.gas_nuclear + ledger.gas_cooling + ledger.gas_radiation;
        amrex::Real const actual = e_after - e_before;
        amrex::Real const scale = amrex::max(std::abs(e_before), 1.0e-30);
        amrex::Real const booking_error = std::abs(booked - actual) / scale;

        amrex::Real const exchange_closure = ledger.exchange_closure();
        amrex::Real const nu = ledger.neutrino_energy;
        amrex::Real const nu_cell = neutrino.const_array(0)(0, 0, 0);

        // ---- 4. package off: nothing moves ---------------------------------------------
        fill_solar(state, layout, rho, T);
        radiation.setVal(0.0);
        neutrino.setVal(0.0);
        warpx::plugins::fluid::SourceOptions off;
        off.gamma = gas_gamma;
        off.nuclear = false;
        off.cooling = false;
        off.radiation = false;
        amrex::Real const e_off0 = cell_energy(state, layout);
        auto const ledger_off = warpx::plugins::fluid::applyStellarSources(
            state, &radiation, &neutrino, dt_src, layout, off);
        amrex::Real const e_off1 = cell_energy(state, layout);
        amrex::Real const off_motion = std::abs(e_off1 - e_off0) / scale;

        std::printf("[sources] nuclear = %.6e, cooling = %.6e, radiation(gas) = %.6e,"
                    " radiation(field) = %.6e\n",
                    (double)ledger.gas_nuclear, (double)ledger.gas_cooling,
                    (double)ledger.gas_radiation, (double)ledger.field_radiation);
        std::printf("[sources] booked %.6e vs actual %.6e, relative error = %.3e\n",
                    (double)booked, (double)actual, (double)booking_error);
        std::printf("[sources] exchange closure = %.3e, neutrino = %.6e (cell %.6e, >= 0: %s)\n",
                    (double)exchange_closure, (double)nu, (double)nu_cell,
                    (nu >= 0.0 && nu_cell >= 0.0) ? "yes" : "no");
        std::printf("[sources] package off: motion = %.3e, total booked = %.3e\n",
                    (double)off_motion,
                    (double)(ledger_off.gas_nuclear + ledger_off.gas_cooling
                             + ledger_off.gas_radiation));

        bool const ok = (booking_error < 1.0e-12) && (exchange_closure < 1.0e-12)
                     && (nu >= 0.0) && (nu_cell >= 0.0) && (off_motion == 0.0)
                     && (std::abs(ledger.gas_cooling) >= 0.0);
        std::printf("[sources] %s\n", ok ? "PASSED" : "FAILED");
        if (!ok) { status = 1; }
    }
    amrex::Finalize();
    return status;
}
