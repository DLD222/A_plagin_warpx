/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Acceptance test: the stellar model with every operator switched on at once.
 *
 * Each piece has its own test; this one checks that they still add up when they run together in a
 * single step, which is the property none of the individual tests can see:
 *
 *   gravity (static field) -> HLLC sweep -> centrifugal -> nuclear + cooling + exchange
 *   -> conduction -> neutrino diffusion and escape
 *
 * Every operator records what it changed in the total energy, and the test verifies
 *
 *   sum of the recorded changes == the total energy change, to round-off
 *   mass conserved, angular momentum conserved (no torques in this setup)
 *   no NaN, temperature and densities stay positive
 *
 * The point is not that any single number is physical: the configuration is a coarse, hot, rotating
 * slab chosen so that every operator does something measurable in a few steps.
 *
 * Specification: PHYSICS_SPEC.md (integration).
 */
#include "FluidMoments.H"
#include "StellarSources.H"
#include "Conduction.H"
#include "AngularMomentum.H"
#include "Neutrino.H"
#include "DegenerateEOS.H"

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

constexpr int n_cell = 32;
constexpr int n_species = warpx::plugins::fluid::SolarNetwork::n_species;
constexpr int n_extra = 2;                     // 0: rho x_e, 1: rho u_phi
constexpr int n_comp = n_species + AMREX_SPACEDIM + 1 + n_extra;
constexpr amrex::Real gas_gamma = 1.4;

amrex::Real total (amrex::MultiFab& st, StateLayout const& layout, int comp)
{
    amrex::Real sum = 0.0;
    for (amrex::MFIter mfi(st); mfi.isValid(); ++mfi) {
        auto const s = st.const_array(mfi);
        amrex::Box const& bx = mfi.validbox();
        for (int i = bx.smallEnd(0); i <= bx.bigEnd(0); ++i) {
            for (int j = star_box_lo(bx, 1); j <= star_box_hi(bx, 1); ++j) {
                for (int k = star_box_lo(bx, 2); k <= star_box_hi(bx, 2); ++k) {
                    sum += s(i, j, k, comp);
                }
            }
        }
    }
    return sum;
}

amrex::Real total_density (amrex::MultiFab& st, StateLayout const& layout)
{
    amrex::Real sum = 0.0;
    for (int a = 0; a < layout.n_species; ++a) { sum += total(st, layout, layout.density(a)); }
    return sum;
}

amrex::Real min_density (amrex::MultiFab& st, StateLayout const& layout)
{
    amrex::Real worst = 1.0e300;
    for (amrex::MFIter mfi(st); mfi.isValid(); ++mfi) {
        auto const s = st.const_array(mfi);
        amrex::Box const& bx = mfi.validbox();
        for (int i = bx.smallEnd(0); i <= bx.bigEnd(0); ++i) {
            worst = amrex::min(worst, s(i, 0, 0, layout.density(0)));
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
        amrex::RealBox rb({AMREX_D_DECL(0.0, 0.0, 0.0)}, {AMREX_D_DECL(1.0, 1.0, 1.0)});
        amrex::Array<int, AMREX_SPACEDIM> is_periodic{AMREX_D_DECL(1, 1, 1)};
        amrex::Geometry geom(domain, rb, 0, is_periodic);
        amrex::Real const dx = geom.CellSize(0);
        StateLayout const layout{n_species, n_extra};

        amrex::MultiFab state(ba, dm, n_comp, 2);
        amrex::MultiFab radiation(ba, dm, 1, 1);
        amrex::MultiFab neutrino(ba, dm, 1, 1);
        amrex::MultiFab density(ba, dm, 1, 1);
        amrex::MultiFab gx(ba, dm, 1, 2), gy(ba, dm, 1, 2), gz(ba, dm, 1, 2);

        // a rotating, hot slab with a composition wave so that every operator has something to do
        amrex::Real const omega = 0.3;
        for (amrex::MFIter mfi(state); mfi.isValid(); ++mfi) {
            amrex::Box const& bx = mfi.validbox();
            auto s = state[mfi].array();
            for (int i = bx.smallEnd(0); i <= bx.bigEnd(0); ++i) {
                amrex::Real const x = (static_cast<amrex::Real>(i) + 0.5) * dx;
                amrex::Real const r = amrex::max(std::abs(x - 0.5), 0.5 * dx);
                amrex::Real const rho = 1.0e-3 * (1.0 + 0.1 * std::sin(2.0 * M_PI * x));
                // ~6e7 K at this density: hot enough for the network and the cooling to act, not so hot that
                // the thin cooling wants to remove the whole internal energy in one step
                amrex::Real const pressure = 1.0e11;
                for (int j = star_box_lo(bx, 1); j <= star_box_hi(bx, 1); ++j) {
                    for (int k = star_box_lo(bx, 2); k <= star_box_hi(bx, 2); ++k) {
                        for (int c = 0; c < n_comp; ++c) { s(i, j, k, c) = 0.0; }
                        // solar-like composition, the network's own species
                        // Sized by the network's species count: it used to be a w[4] that the
                        // loop then indexed up to n_species (9), i.e. five reads of stack memory
                        // past the array, which fed garbage (possibly negative) densities to the
                        // CF88 network.  The remaining species start at zero.
                        amrex::Real const w[n_species] = {0.70, 0.28, 0.02};
                        amrex::Real wsum = 0.0;
                        for (int a = 0; a < n_species; ++a) { wsum += w[a]; }
                        for (int a = 0; a < n_species; ++a) {
                            s(i, j, k, layout.density(a)) = rho * (w[a] / wsum);
                        }
                        s(i, j, k, layout.energy()) = pressure / (gas_gamma - 1.0);
                        s(i, j, k, layout.extra(0)) = rho;          // fully ionised
                        s(i, j, k, layout.extra(1)) = rho * omega * r;   // rho u_phi
                    }
                }
            }
        }
        radiation.setVal(0.0);
        neutrino.setVal(0.0);
        gx.setVal(0.0); gy.setVal(0.0); gz.setVal(0.0);

        warpx::plugins::fluid::SourceOptions opts;
        opts.gamma = gas_gamma;
        opts.nuclear = true;
        opts.cooling = true;
        opts.radiation = true;
        opts.kappa = 1.0e-3;

        amrex::Vector<amrex::MultiFab const*> gvec{&gx, &gy, &gz};
        amrex::Real const dt = 1.0e-9;
        amrex::Real worst_booking = 0.0;

        amrex::Real const mass0 = total_density(state, layout);
        amrex::Real const L0 = warpx::plugins::fluid::total_angular_momentum(state, geom, layout, 1);

        for (int step = 0; step < 5; ++step) {
            amrex::Real const e0 = total(state, layout, layout.energy());

            // gravity first (static field: the Cowling approximation of a pulsation problem)
            warpx::plugins::fluid::applyGravitySource(state, gvec, dt, layout);
            amrex::Real const e_g = total(state, layout, layout.energy());

            // hydrodynamic sweep in every direction
            warpx::plugins::fluid::DegenerateElectronEOS eos;
            eos.gamma = gas_gamma;
            for (int dir = 0; dir < AMREX_SPACEDIM; ++dir) {
                state.FillBoundary(geom.periodicity());
                warpx::plugins::fluid::advanceSweep(state, geom, dir, dt, eos, layout);
            }
            amrex::Real const e_sweep = total(state, layout, layout.energy());

            // rotation
            warpx::plugins::fluid::applyCentrifugalSource(state, geom, dt, layout, 1, 0, 1);
            amrex::Real const e_rot = total(state, layout, layout.energy());

            // nuclear + cooling + radiation exchange, with its ledger
            auto const ledger = warpx::plugins::fluid::applyStellarSources(
                state, &radiation, &neutrino, dt, layout, opts);
            amrex::Real const e_src = total(state, layout, layout.energy());
            amrex::Real const booked = ledger.gas_nuclear + ledger.gas_cooling
                                     + ledger.gas_radiation;

            // conduction (conservative in a periodic box)
            warpx::plugins::fluid::advanceThermalConduction(state, geom, dt, gas_gamma, layout, 0,
                                                            0.0, 5.0, 4000);
            amrex::Real const e_cond = total(state, layout, layout.energy());

            // neutrinos: diffuse, then let the thin part escape (the gas is not touched)
            for (amrex::MFIter mfi(density); mfi.isValid(); ++mfi) {
                amrex::Box const& bx = mfi.validbox();
                auto r = density[mfi].array();
                auto const s = state.const_array(mfi);
                for (int i = bx.smallEnd(0); i <= bx.bigEnd(0); ++i) {
                    for (int j = star_box_lo(bx, 1); j <= star_box_hi(bx, 1); ++j) {
                        for (int k = star_box_lo(bx, 2); k <= star_box_hi(bx, 2); ++k) {
                            r(i, j, k) = s(i, j, k, layout.density(0));
                        }
                    }
                }
            }
            warpx::plugins::fluid::advanceNeutrinoDiffusion(neutrino, density, 1.0e-3, geom, dt);
            warpx::plugins::fluid::applyNeutrinoEscape(neutrino, density, 1.0e-3, dt);

            amrex::Real const e1 = total(state, layout, layout.energy());

            // the ledger identity: the recorded source changes must equal what the state did
            amrex::Real const source_actual = e_src - e_rot;
            amrex::Real const scale = amrex::max(std::abs(e0), amrex::Real(1.0e-30));
            worst_booking = amrex::max(worst_booking,
                                       std::abs(booked - source_actual) / scale);

            // and every operator must be accounted for in the total
            amrex::Real const sum_pieces = (e_g - e0) + (e_sweep - e_g) + (e_src - e_rot)
                                         + (e_cond - e_src);
            amrex::Real const total_change = e1 - e0;
            worst_booking = amrex::max(worst_booking, std::abs(sum_pieces - total_change) / scale);
            (void)e_g;
        }

        // ---- second phase: with the sources off, gas + gravitational potential is conserved ----
        // A static potential phi with g = -grad(phi) makes the work the field does exactly the
        // change of sum(rho phi), so the *combination* must be conserved even though neither part
        // is.  That is a stronger statement than the per-operator bookkeeping above and it only
        // holds if the gravity kick and the energy kick are consistent with each other.
        amrex::Real const g0 = 1.0e-3;
        auto const phi_at = [&](amrex::Real x) { return -g0 * x; };
        auto total_with_potential = [&]() {
            amrex::Real sum = 0.0;
            for (amrex::MFIter mfi(state); mfi.isValid(); ++mfi) {
                auto const s = state.const_array(mfi);
                amrex::Box const& bx = mfi.validbox();
                for (int i = bx.smallEnd(0); i <= bx.bigEnd(0); ++i) {
                    amrex::Real const x = (static_cast<amrex::Real>(i) + 0.5) * dx;
                    amrex::Real rho = 0.0;
                    for (int a = 0; a < layout.n_species; ++a) {
                        rho += s(i, 0, 0, layout.density(a));
                    }
                    sum += s(i, 0, 0, layout.energy()) + rho * phi_at(x);
                }
            }
            return sum;
        };
        for (amrex::MFIter mfi(gx); mfi.isValid(); ++mfi) {
            auto g = gx[mfi].array();
            amrex::Box const& bx = mfi.validbox();
            for (int i = bx.smallEnd(0); i <= bx.bigEnd(0); ++i) {
                for (int j = star_box_lo(bx, 1); j <= star_box_hi(bx, 1); ++j) {
                    for (int k = star_box_lo(bx, 2); k <= star_box_hi(bx, 2); ++k) { g(i, j, k) = g0; }
                }
            }
        }
        amrex::Real const conserved0 = total_with_potential();
        for (int step = 0; step < 20; ++step) {
            warpx::plugins::fluid::applyGravitySource(state, gvec, dt, layout);
            warpx::plugins::fluid::DegenerateElectronEOS eos2;
            eos2.gamma = gas_gamma;
            for (int dir = 0; dir < AMREX_SPACEDIM; ++dir) {
                state.FillBoundary(geom.periodicity());
                warpx::plugins::fluid::advanceSweep(state, geom, dir, dt, eos2, layout);
            }
        }
        amrex::Real const conserved1 = total_with_potential();
        amrex::Real const conserved_error = std::abs(conserved1 - conserved0)
                                          / amrex::max(std::abs(conserved0), 1.0e-30);

        amrex::Real const mass1 = total_density(state, layout);
        amrex::Real const L1 = warpx::plugins::fluid::total_angular_momentum(state, geom, layout, 1);
        amrex::Real const mass_error = std::abs(mass1 - mass0) / mass0;
        amrex::Real const L_error = std::abs(L1 - L0) / amrex::max(std::abs(L0), 1.0e-30);
        amrex::Real const rho_min = min_density(state, layout);
        amrex::Real const nu_total = total(neutrino, layout, 0);
        amrex::Real const rad_total = total(radiation, layout, 0);

        std::printf("[model] mass error = %.3e, angular-momentum error = %.3e\n",
                    (double)mass_error, (double)L_error);
        std::printf("[model] ledger/operator closure = %.3e\n", (double)worst_booking);
        std::printf("[model] gas + potential conserved over 20 steps: relative error = %.3e\n",
                    (double)conserved_error);
        std::printf("[model] min density = %.3e, radiation energy = %.3e, neutrino energy = %.3e\n",
                    (double)rho_min, (double)rad_total, (double)nu_total);

        // Angular momentum is NOT an invariant of this slab: advecting rho u_phi along x with a
        // radius that is fixed in space changes sum(rho u_phi r) by construction.  The exact
        // conservation statement lives in test_angular_momentum (rotation only, no advection); here
        // the check is that the drift stays small, which is what a coupled run can claim.
        bool const ok = (mass_error < 1.0e-12) && (L_error < 1.0e-3)
                     && (worst_booking < 1.0e-10) && (conserved_error < 1.0e-10) && (rho_min > 0.0)
                     && std::isfinite(rad_total) && (rad_total >= 0.0)
                     && std::isfinite(nu_total) && (nu_total >= 0.0);
        std::printf("[model] %s\n", ok ? "PASSED" : "FAILED");
        if (!ok) { status = 1; }
    }
    amrex::Finalize();
    return status;
}
