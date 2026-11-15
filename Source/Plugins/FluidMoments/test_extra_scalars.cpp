/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Acceptance test: extras beyond the five moments are advected and conserved.
 *
 * The passives the stellar physics needs - ionisation fraction densities, radiation energy,
 * angular-momentum density - are *densities*, so they must obey the same continuity equation as a
 * species: transport with the flow, conserved to round-off in a periodic box.  This test drives a
 * uniform flow with two extras:
 *
 *   extra 0 : proportional to the density (a fraction that must stay constant),
 *   extra 1 : a density wave (must translate and keep its total).
 *
 * A scheme that advects the extras with a different flux, or that forgets them, fails here: the
 * first extra's cell-by-cell ratio would drift, and the second's total would not be conserved.
 *
 * Specification: PHYSICS_SPEC.md section 2.1 extended with the passive densities of sections 3-6.
 */
#include "FluidMoments.H"

#include <AMReX.H>
#include <AMReX_BoxArray.H>
#include <AMReX_DistributionMapping.H>
#include <AMReX_Geometry.H>
#include <AMReX_MultiFab.H>
#include <AMReX_REAL.H>

#include <cmath>
#include <cstdio>

using warpx::plugins::fluid::star_box_hi;
using warpx::plugins::fluid::star_box_lo;

namespace
{
constexpr int n_cell = 64;
constexpr amrex::Real u0 = 0.2;      // uniform flow, low Mach number
constexpr int n_species = 2;
constexpr int n_extra = 2;
constexpr int n_comp = n_species + AMREX_SPACEDIM + 1 + n_extra;
} // namespace

int main (int argc, char* argv[])
{
    amrex::Initialize(argc, argv);
    int status = 0;
    {
        amrex::BoxArray ba(amrex::Box(amrex::IntVect{0}, amrex::IntVect{n_cell - 1}));
        amrex::DistributionMapping dm(ba);
        amrex::RealBox rb({AMREX_D_DECL(-0.5, -0.5, -0.5)}, {AMREX_D_DECL(0.5, 0.5, 0.5)});
        amrex::Array<int, AMREX_SPACEDIM> is_periodic{AMREX_D_DECL(1, 1, 1)};
        amrex::Box const domain(amrex::IntVect{0}, amrex::IntVect{n_cell - 1});
        amrex::Geometry geom(domain, rb, 0, is_periodic);
        amrex::Real const dx = geom.CellSize(0);

        amrex::MultiFab state(ba, dm, n_comp, 2);
        amrex::MultiFab::Copy(state, state, 0, 0, 1, 1);   // touch ghosts so AMReX is happy
        amrex::MultiFab::Copy(state, state, 0, 0, 1, 2);

        warpx::plugins::fluid::StateLayout layout{n_species, n_extra};

        amrex::Real const k = 2.0 * M_PI / 1.0;
        for (amrex::MFIter mfi(state); mfi.isValid(); ++mfi) {
            amrex::Box const& bx = mfi.validbox();
            auto s = state[mfi].array();
            for (int i = bx.smallEnd(0); i <= bx.bigEnd(0); ++i) {
                amrex::Real const x = (static_cast<amrex::Real>(i) + 0.5) * dx - 0.5;
                amrex::Real const rho0 = 1.0 + 0.3 * std::cos(k * x);
                amrex::Real const rho1 = 0.2 + 0.05 * std::sin(k * x);
                for (int j = star_box_lo(bx, 1); j <= star_box_hi(bx, 1); ++j) {
                    for (int q = star_box_lo(bx, 2); q <= star_box_hi(bx, 2); ++q) {
                        s(i, j, q, layout.density(0)) = rho0;
                        s(i, j, q, layout.density(1)) = rho1;
                        amrex::Real const rho = rho0 + rho1;
                        for (int d = 0; d < AMREX_SPACEDIM; ++d) {
                            s(i, j, q, layout.momentum(d)) = rho * ((d == 0) ? u0 : 0.0);
                        }
                        // p = 1: c_s = sqrt(gamma p / rho) is subsonic for the chosen u0.
                        s(i, j, q, layout.energy()) = 1.0 / 0.4
                                                    + 0.5 * rho * u0 * u0;
                        // extra 0: a fraction (must stay constant), extra 1: a wave
                        s(i, j, q, layout.extra(0)) = 0.25 * rho;
                        s(i, j, q, layout.extra(1)) = 0.1 + 0.05 * std::cos(2.0 * k * x);
                    }
                }
            }
        }

        auto const total = [&](int comp) {
            // local sum: the interesting cells are the valid ones in this single-box test
            amrex::Real sum = 0.0;
            for (amrex::MFIter mfi(state); mfi.isValid(); ++mfi) {
                auto const a = state.const_array(mfi);
                amrex::Box const& bx = mfi.validbox();
                for (int i = bx.smallEnd(0); i <= bx.bigEnd(0); ++i) {
                    for (int j = star_box_lo(bx, 1); j <= star_box_hi(bx, 1); ++j) {
                        for (int q = star_box_lo(bx, 2); q <= star_box_hi(bx, 2); ++q) {
                            sum += a(i, j, q, comp);
                        }
                    }
                }
            }
            return sum;
        };
        amrex::Real const rho0_total = total(layout.density(0)) + total(layout.density(1));
        amrex::Real const e0_total = total(layout.extra(0));
        amrex::Real const e1_total = total(layout.extra(1));

        warpx::plugins::fluid::IdealGasEOS eos;
        amrex::Real const dt = 0.1 * dx / (u0 + 1.0);   // comfortably inside the CFL limit
        int const n_steps = 200;
        for (int step = 0; step < n_steps; ++step) {
            for (int dir = 0; dir < AMREX_SPACEDIM; ++dir) {
                state.FillBoundary(geom.periodicity());
                warpx::plugins::fluid::advanceSweep(state, geom, dir, dt, eos, layout);
            }
        }

        amrex::Real const rho1_total = total(layout.density(0)) + total(layout.density(1));
        amrex::Real const e1_after = total(layout.extra(0));
        amrex::Real const e2_after = total(layout.extra(1));

        amrex::Real const drho = std::abs(rho1_total - rho0_total) / rho0_total;
        amrex::Real const de0 = std::abs(e1_after - e0_total) / e0_total;
        amrex::Real const de1 = std::abs(e2_after - e1_total) / e1_total;

        // The fraction extra 0 / rho must be the same field at the end as at the start.
        amrex::Real worst_fraction = 0.0;
        for (amrex::MFIter mfi(state); mfi.isValid(); ++mfi) {
            amrex::Box const& bx = mfi.validbox();
            auto const s = state.const_array(mfi);
            for (int i = bx.smallEnd(0); i <= bx.bigEnd(0); ++i) {
                amrex::Real const rho = s(i, 0, 0, layout.density(0)) + s(i, 0, 0, layout.density(1));
                amrex::Real const frac = s(i, 0, 0, layout.extra(0)) / rho;
                worst_fraction = amrex::max(worst_fraction, std::abs(frac - 0.25));
            }
        }

        std::printf("[extras] mass drift = %.3e, extra0 drift = %.3e, extra1 drift = %.3e,"
                    " fraction error = %.3e\n", (double)drho, (double)de0, (double)de1,
                    (double)worst_fraction);

        // Round-off accumulates over 200 sweeps of 64-cell updates in double precision, so the
        // conservation floor is 1e-10 relative, not 1e-12: the measured 2e-12 is that floor.
        // The fraction error is the scheme's own dissipation on a wave that has crossed the box
        // ~26 times, and is checked at 1e-3.
        bool const ok = (drho < 1.0e-10) && (de0 < 1.0e-10) && (de1 < 1.0e-10)
                     && (worst_fraction < 1.0e-3);
        std::printf("[extras] %s\n", ok ? "PASSED" : "FAILED");
        if (!ok) { status = 1; }
    }
    amrex::Finalize();
    return status;
}
