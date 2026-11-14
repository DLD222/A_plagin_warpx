/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
/* Stellar energy sources, applied in one place with an explicit ledger.
 *
 * By this point the fluid layer has several ways to change the energy of a cell: nuclear reactions
 * (CF88, already in SolarNetwork.H), optically thin cooling, radiation exchange with a diffusion
 * field, and conduction.  Applied one by one, each is easy to get right and the *sum* is easy to get
 * wrong: a term applied twice, a loss counted as a gain, a neutrino loss dropped.  This header
 * therefore runs them in a fixed order and records, for every term, what it actually changed in the
 * state rather than what it was supposed to change:
 *
 *     ledger.gas_nuclear      change of gas energy from the network
 *     ledger.neutrino_energy  energy the network carried away as neutrinos
 *     ledger.gas_cooling      change of gas energy from the thin cooling
 *     ledger.gas_radiation    change of gas energy from the radiation exchange
 *     ledger.field_radiation  change of the radiation energy density
 *
 * The test then checks two identities that must hold to round-off no matter what the physics does:
 *
 *     gas_radiation + field_radiation == 0        (the exchange is conservative)
 *     neutrino_energy >= 0                        (a network can emit, never absorb, neutrinos)
 *
 * and that each recorded term equals the difference of the corresponding state between the call and
 * its return - i.e. nothing moves without being booked.
 *
 * Conduction is deliberately not part of this ledger: it is a flux, so it is conservative by
 * construction and its budget is the boundary flux (zero in a periodic box).  It is advanced by
 * advanceThermalConduction and is checked separately in test_conduction.cpp.
 *
 * Units are CGS.
 *
 * Specification: PHYSICS_SPEC.md sections 3-6 (sources and energy accounting).
 */
#ifndef WARPX_PLUGINS_FLUID_STELLAR_SOURCES_H_
#define WARPX_PLUGINS_FLUID_STELLAR_SOURCES_H_

#include "FluidMoments.H"
#include "Ionization.H"
#include "Radiation.H"
#include "SolarNetwork.H"

#include <AMReX_MultiFab.H>

namespace warpx::plugins::fluid
{

/** What each source actually changed, measured from the state, not assumed. */
struct EnergyLedger
{
    amrex::Real gas_nuclear = 0.0;       //!< summed over cells
    amrex::Real neutrino_energy = 0.0;   //!< summed over cells, >= 0
    amrex::Real gas_cooling = 0.0;       //!< <= 0 (a loss)
    amrex::Real gas_radiation = 0.0;
    amrex::Real field_radiation = 0.0;

    /** The exchange must balance and cooling must not heat. */
    [[nodiscard]] amrex::Real exchange_closure () const
    {
        amrex::Real const scale = amrex::max(std::abs(gas_radiation) + std::abs(field_radiation),
                                             amrex::Real(1.0e-300));
        return std::abs(gas_radiation + field_radiation) / scale;
    }
};

namespace detail
{
inline amrex::Real sum_energy (amrex::MultiFab& state, StateLayout const& layout)
{
    amrex::Real sum = 0.0;
    for (amrex::MFIter mfi(state); mfi.isValid(); ++mfi) {
        auto const s = state.const_array(mfi);
        amrex::Box const& bx = mfi.validbox();
        for (int i = bx.smallEnd(0); i <= bx.bigEnd(0); ++i) {
            for (int j = bx.smallEnd(1); j <= bx.bigEnd(1); ++j) {
                for (int k = bx.smallEnd(2); k <= bx.bigEnd(2); ++k) {
                    sum += s(i, j, k, layout.energy());
                }
            }
        }
    }
    return sum;
}

inline amrex::Real sum_field (amrex::MultiFab& field)
{
    amrex::Real sum = 0.0;
    for (amrex::MFIter mfi(field); mfi.isValid(); ++mfi) {
        auto const a = field.const_array(mfi);
        amrex::Box const& bx = mfi.validbox();
        for (int i = bx.smallEnd(0); i <= bx.bigEnd(0); ++i) {
            for (int j = bx.smallEnd(1); j <= bx.bigEnd(1); ++j) {
                for (int k = bx.smallEnd(2); k <= bx.bigEnd(2); ++k) {
                    sum += a(i, j, k);
                }
            }
        }
    }
    return sum;
}
} // namespace detail

/** Options for one application of the source package. */
struct SourceOptions
{
    amrex::Real gamma = 1.4;
    bool nuclear = true;
    bool cooling = true;
    bool radiation = true;
    amrex::Real kappa = 0.0;          //!< opacity for the radiation exchange [cm^2/g]
    amrex::Real coulomb_log = 5.0;    //!< kept for symmetry with the conduction options
};

/** Apply the source package in a fixed order and return what each term did.
 *
 *  Order: nuclear -> cooling -> radiation exchange.  The nuclear step is first because it changes
 *  the composition and hence the temperature every later term uses; the exchange is last because it
 *  is implicit and drives the radiation field towards the final temperature.
 *
 *  \param state       the conserved state (species, momentum, energy, extras)
 *  \param radiation   optional radiation energy density (1 component, >= 1 ghost); may be nullptr
 *  \param neutrino    optional per-cell neutrino energy from the network; may be nullptr
 */
inline EnergyLedger applyStellarSources (amrex::MultiFab& state, amrex::MultiFab* radiation,
                                         amrex::MultiFab* neutrino, amrex::Real dt,
                                         StateLayout const& layout, SourceOptions const& opts)
{
    EnergyLedger ledger;

    if (opts.nuclear) {
        amrex::Real const e0 = detail::sum_energy(state, layout);
        applySolarNetwork(state, dt, opts.gamma, layout, neutrino);
        ledger.gas_nuclear = detail::sum_energy(state, layout) - e0;
        if (neutrino != nullptr) { ledger.neutrino_energy = detail::sum_field(*neutrino); }
    }

    if (opts.cooling) {
        amrex::Real const e0 = detail::sum_energy(state, layout);
        applyOpticallyThinCooling(state, dt, opts.gamma, layout, 0);
        ledger.gas_cooling = detail::sum_energy(state, layout) - e0;
        // A cooling term must not heat: a sign error here is invisible in a conserved total.  The
        // check has to allow round-off, though - when the sink removes nearly the whole internal
        // energy the difference of two sums of order 1e19 carries ~1e3 of noise, and a strict
        // "<= 0" then fails on arithmetic rather than on physics (the solar column hit exactly that).
        amrex::Real const cooling_scale = amrex::max(std::abs(e0), amrex::Real(1.0e-30));
        AMREX_ALWAYS_ASSERT_WITH_MESSAGE(ledger.gas_cooling <= 1.0e-12 * cooling_scale,
            "applyStellarSources: optically thin cooling added energy");
    }

    if (opts.radiation && radiation != nullptr && opts.kappa > 0.0) {
        amrex::Real const e0 = detail::sum_energy(state, layout);
        amrex::Real const f0 = detail::sum_field(*radiation);
        applyRadiationCoupling(state, *radiation, opts.kappa, dt, opts.gamma, layout);
        ledger.gas_radiation = detail::sum_energy(state, layout) - e0;
        ledger.field_radiation = detail::sum_field(*radiation) - f0;
    }

    return ledger;
}

} // namespace warpx::plugins::fluid

#endif // WARPX_PLUGINS_FLUID_STELLAR_SOURCES_H_
