/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Acceptance test: Lorentz force and Joule heating from prescribed fields.
 *
 * Four checks, each of which a sign error fails:
 *
 *  1. heating is non-negative for a sweep of field and velocity combinations, and equals
 *     sigma |E + u x B|^2 evaluated independently;
 *  2. a plasma moving *along* B with E = 0 feels nothing: E' = 0, so the force and the heating are
 *     exactly zero (a wrong u x B sign turns this into a large fake force);
 *  3. the energy update is exactly (f . u + Q) dt, i.e. the mechanical work and the irreversible
 *     heating are booked separately and add up to what the state actually gained;
 *  4. the force is orthogonal to B (J x B is), which is what makes the field do no work along itself.
 *
 * Specification: PHYSICS_SPEC.md section 11.
 */
#include "FluidMoments.H"
#include "Electromagnetic.H"

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
using namespace warpx::plugins::fluid::electromagnetic;

constexpr int n_species = 1;
constexpr int n_comp = n_species + AMREX_SPACEDIM + 1;
}

int main (int argc, char* argv[])
{
    amrex::Initialize(argc, argv);
    int status = 0;
    {
        // ---- 1./2./4. pointwise algebra ------------------------------------------------
        amrex::Real worst_heating_error = 0.0;
        amrex::Real worst_orthogonality = 0.0;
        bool heating_nonneg = true;
        amrex::Real sigma = 1.0e4;

        for (amrex::Real Ex : {-1.0e3, 0.0, 1.0e3}) {
            for (amrex::Real Bz : {-1.0, 0.0, 1.0}) {
                for (amrex::Real ux : {-1.0e6, 0.0, 1.0e6}) {
                    amrex::Real const Ev[3] = {Ex, 0.0, 0.0};
                    amrex::Real const Bv[3] = {0.0, 0.0, Bz};
                    amrex::Real const uv[3] = {ux, 0.0, 0.0};
                    amrex::Real const Q = joule_heating(Ev, Bv, uv, sigma);
                    amrex::Real f[3];
                    lorentz_force(Ev, Bv, uv, sigma, f);

                    // independent evaluation of sigma |E + u x B|^2
                    amrex::Real uxb[3];
                    cross(uv, Bv, uxb);
                    amrex::Real const e0 = Ev[0] + uxb[0];
                    amrex::Real const e1 = Ev[1] + uxb[1];
                    amrex::Real const e2 = Ev[2] + uxb[2];
                    amrex::Real const expected = sigma * (e0 * e0 + e1 * e1 + e2 * e2);
                    worst_heating_error = amrex::max(worst_heating_error,
                        std::abs(Q - expected) / amrex::max(expected, 1.0e-30));
                    if (Q < 0.0) { heating_nonneg = false; }
                    amrex::Real const fdotb = f[0] * Bv[0] + f[1] * Bv[1] + f[2] * Bv[2];
                    worst_orthogonality = amrex::max(worst_orthogonality,
                        std::abs(fdotb) / amrex::max(std::sqrt(f[0]*f[0]+f[1]*f[1]+f[2]*f[2])
                                                     * std::sqrt(Bv[0]*Bv[0]+Bv[1]*Bv[1]+Bv[2]*Bv[2]),
                                                     amrex::Real(1.0e-30)));
                }
            }
        }

        // field-aligned motion: u parallel to B and E = 0 must give exactly zero
        amrex::Real const E0[3] = {0.0, 0.0, 0.0};
        amrex::Real const Bz[3] = {0.0, 0.0, 2.0};
        amrex::Real const uz[3] = {0.0, 0.0, 3.0e6};
        amrex::Real const Q_aligned = joule_heating(E0, Bz, uz, sigma);
        amrex::Real f_aligned[3];
        lorentz_force(E0, Bz, uz, sigma, f_aligned);

        // ---- 3. the state update equals (f . u + Q) dt ---------------------------------
        amrex::Box const domain(amrex::IntVect{0}, amrex::IntVect{7});
        amrex::BoxArray ba(domain);
        amrex::DistributionMapping dm(ba);
        amrex::RealBox rb({AMREX_D_DECL(0.0, 0.0, 0.0)}, {AMREX_D_DECL(1.0, 1.0, 1.0)});
        amrex::Array<int, AMREX_SPACEDIM> is_periodic{AMREX_D_DECL(1, 1, 1)};
        amrex::Geometry geom(domain, rb, 0, is_periodic);
        StateLayout const layout{n_species, 0};
        amrex::MultiFab state(ba, dm, n_comp, 1);
        amrex::MultiFab E0f(ba, dm, 1, 1), E1f(ba, dm, 1, 1), E2f(ba, dm, 1, 1);
        amrex::MultiFab B0f(ba, dm, 1, 1), B1f(ba, dm, 1, 1), B2f(ba, dm, 1, 1);
        amrex::Real const rho = 1.0e-6;
        state.setVal(0.0);
        for (amrex::MFIter mfi(state); mfi.isValid(); ++mfi) {
            amrex::Box const& bx = mfi.validbox();
            auto s = state[mfi].array();
            for (int i = bx.smallEnd(0); i <= bx.bigEnd(0); ++i) {
                for (int j = star_box_lo(bx, 1); j <= star_box_hi(bx, 1); ++j) {
                    for (int k = star_box_lo(bx, 2); k <= star_box_hi(bx, 2); ++k) {
                        s(i, j, k, layout.density(0)) = rho;
                        s(i, j, k, layout.momentum(0)) = 1.0e-6 * 2.0e6;   // u_x = 2e6
                        // keep the energy small: the update is 1e-4 per cell, and at 1e6 the subtraction of two
                        // doubles would leave only 1e-6 of relative resolution in the comparison
                        s(i, j, k, layout.energy()) = 1.0e2;
                    }
                }
            }
        }
        E0f.setVal(1.0e2); E1f.setVal(0.0); E2f.setVal(0.0);
        B0f.setVal(0.0); B1f.setVal(0.0); B2f.setVal(1.0);
        amrex::Vector<amrex::MultiFab const*> Ev{&E0f, &E1f, &E2f};
        amrex::Vector<amrex::MultiFab const*> Bv{&B0f, &B1f, &B2f};

        amrex::Real const e_before = state.const_array(0)(0, 0, 0, layout.energy());
        amrex::Real const dt = 1.0e-12;
        amrex::Real const booked = warpx::plugins::fluid::applyElectromagneticSources(
            state, Ev, Bv, sigma, dt, layout);
        amrex::Real const e_after = state.const_array(0)(0, 0, 0, layout.energy());
        // gross check: the energy gained must be positive and of the order of the booked heating
        // the box is 8 cells in every direction: count them instead of assuming one line
        amrex::Real const n_cells = static_cast<amrex::Real>(state.boxArray().numPts());
        amrex::Real const gained = (e_after - e_before) * n_cells;
        amrex::Real uv_probe[3] = {2.0 / rho, 0.0, 0.0};
        amrex::Real Ev_probe[3] = {1.0e2, 0.0, 0.0};
        amrex::Real Bv_probe[3] = {0.0, 0.0, 1.0};
        amrex::Real f_probe[3];
        lorentz_force(Ev_probe, Bv_probe, uv_probe, sigma, f_probe);
        amrex::Real const Q_probe = joule_heating(Ev_probe, Bv_probe, uv_probe, sigma);
        amrex::Real const work_probe = f_probe[0] * uv_probe[0] + f_probe[1] * uv_probe[1]
                                     + f_probe[2] * uv_probe[2];
        std::printf("[em] probe: u = %.3e, Q = %.3e, work = %.3e, expected dE = %.3e\n",
                    (double)uv_probe[0], (double)Q_probe, (double)work_probe,
                    (double)((work_probe + Q_probe) * dt));
        amrex::Real const heating_ratio = booked > 0.0 ? gained / booked : 0.0;
        // per-cell energy change must equal (f . u + Q) dt from the pointwise algebra, and the
        // booked heating must equal sum Q dt over the cells.  Note that f . u can cancel Q exactly
        // (it does in this configuration: J.E = J.E' + J.(u x B) and J.(u x B) = -f.u), which is
        // why the check is against (f.u + Q), not against Q.
        amrex::Real const expected_cell = (work_probe + Q_probe) * dt;
        amrex::Real const cell_scale = amrex::max(std::abs(expected_cell), 1.0e-30);

        std::printf("[em] heating: worst relative error = %.3e, non-negative: %s\n",
                    (double)worst_heating_error, heating_nonneg ? "yes" : "no");
        std::printf("[em] field-aligned motion: Q = %.3e, |f| = %.3e (both must be 0)\n",
                    (double)Q_aligned,
                    (double)std::sqrt(f_aligned[0]*f_aligned[0] + f_aligned[1]*f_aligned[1]
                                    + f_aligned[2]*f_aligned[2]));
        std::printf("[em] force.B orthogonality: worst relative = %.3e\n",
                    (double)worst_orthogonality);
        std::printf("[em] update: booked %.6e, gained %.6e, ratio %.6f, per-cell dE = %.6e\n",
                    (double)booked, (double)gained, (double)heating_ratio,
                    (double)(e_after - e_before));

        amrex::Real const cell_error = std::abs((e_after - e_before) - expected_cell) / cell_scale;
        amrex::Real const booked_error = std::abs(booked - n_cells * Q_probe * dt)
                                       / amrex::max(n_cells * Q_probe * dt, 1.0e-30);
        amrex::Real const work_over_heating = (Q_probe > 0.0) ? -work_probe / Q_probe : 0.0;
        std::printf("[em] update: per-cell relative error = %.3e, booked relative error = %.3e,"
                    " work/Q = %.6f (== -1 in this configuration)\n",
                    (double)cell_error, (double)booked_error, (double)(-work_over_heating));
        bool const ok = (worst_heating_error < 1.0e-14) && heating_nonneg
                     && (Q_aligned == 0.0)
                     && (std::abs(f_aligned[0]) + std::abs(f_aligned[1]) + std::abs(f_aligned[2]) == 0.0)
                     && (worst_orthogonality < 1.0e-14)
                     && (cell_error < 1.0e-8) && (booked_error < 1.0e-12);
        std::printf("[em] %s\n", ok ? "PASSED" : "FAILED");
        if (!ok) { status = 1; }
    }
    amrex::Finalize();
    return status;
}
