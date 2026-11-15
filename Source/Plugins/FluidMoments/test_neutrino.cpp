/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Acceptance test: neutrino transport in its two limits.
 *
 *  1. optically thick: the energy density obeys the same grey diffusion equation as the photons, so
 *     a sinusoidal mode must decay as exp(-D k^2 t) with D = c/(3 kappa rho);
 *  2. optically thin: each neutrino escapes at the speed of light, so a uniform field must decay
 *     exactly as exp(-c kappa rho t), for any dt (the sink is exponential, not a linear update);
 *  3. the thin sink never returns energy and never goes negative, even for c kappa rho dt >> 1;
 *  4. the transport part is conservative in a closed box (periodic): the total is unchanged by the
 *     diffusion step to round-off.
 *
 * Specification: PHYSICS_SPEC.md section 9.
 */
#include "FluidMoments.H"
#include "Neutrino.H"

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
}

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
        amrex::Real const k = 2.0 * M_PI;
        amrex::Real const rho = 1.0e-3;
        amrex::Real const kappa = 1.0;

        amrex::MultiFab nu(ba, dm, 1, 1);
        amrex::MultiFab rho_f(ba, dm, 1, 0);
        rho_f.setVal(rho);

        auto fill_mode = [&](amrex::Real amp) {
            for (amrex::MFIter mfi(nu); mfi.isValid(); ++mfi) {
                amrex::Box const& bx = mfi.validbox();
                auto a = nu[mfi].array();
                for (int i = bx.smallEnd(0); i <= bx.bigEnd(0); ++i) {
                    amrex::Real const x = (static_cast<amrex::Real>(i) + 0.5) * dx - 0.5;
                    for (int j = star_box_lo(bx, 1); j <= star_box_hi(bx, 1); ++j) {
                        for (int q = star_box_lo(bx, 2); q <= star_box_hi(bx, 2); ++q) {
                            a(i, j, q) = 1.0 + amp * std::cos(k * x);
                        }
                    }
                }
            }
        };
        auto amplitude = [&]() {
            amrex::Real sum = 0.0;
            for (amrex::MFIter mfi(nu); mfi.isValid(); ++mfi) {
                auto const a = nu.const_array(mfi);
                amrex::Box const& bx = mfi.validbox();
                for (int i = bx.smallEnd(0); i <= bx.bigEnd(0); ++i) {
                    amrex::Real const x = (static_cast<amrex::Real>(i) + 0.5) * dx - 0.5;
                    sum += (a(i, 0, 0) - 1.0) * std::cos(k * x);
                }
            }
            return 2.0 * sum / n_cell;
        };
        auto total = [&]() {
            amrex::Real sum = 0.0;
            for (amrex::MFIter mfi(nu); mfi.isValid(); ++mfi) {
                auto const a = nu.const_array(mfi);
                amrex::Box const& bx = mfi.validbox();
                for (int i = bx.smallEnd(0); i <= bx.bigEnd(0); ++i) {
                    for (int j = star_box_lo(bx, 1); j <= star_box_hi(bx, 1); ++j) {
                        for (int q = star_box_lo(bx, 2); q <= star_box_hi(bx, 2); ++q) { sum += a(i, j, q); }
                    }
                }
            }
            return sum;
        };

        // ---- 1./4. thick diffusion: analytic decay and total conservation ---------------
        amrex::Real const D = warpx::plugins::fluid::neutrino::diffusion_coefficient(kappa, rho);
        fill_mode(1.0e-6);
        amrex::Real const amp0 = amplitude();
        amrex::Real const tot0 = total();
        amrex::Real const t = 0.3 / (D * k * k);
        int const nsub = warpx::plugins::fluid::advanceNeutrinoDiffusion(nu, rho_f, kappa, geom,
                                                                        t, 200000);
        amrex::Real const amp1 = amplitude();
        amrex::Real const tot1 = total();
        amrex::Real const decay_measured = amp1 / amp0;
        amrex::Real const decay_exact = std::exp(-D * k * k * t);
        amrex::Real const diff_error = std::abs(decay_measured - decay_exact) / decay_exact;
        amrex::Real const tot_error = std::abs(tot1 - tot0) / tot0;

        // ---- 2./3. thin escape: exponential, stable at any dt ---------------------------
        nu.setVal(1.0);
        amrex::Real const tau = warpx::plugins::fluid::neutrino::c_light * kappa * rho;
        amrex::Real const dt_thin = 1.0 / tau;                 // one e-folding
        warpx::plugins::fluid::applyNeutrinoEscape(nu, rho_f, kappa, dt_thin);
        amrex::Real const after = nu.const_array(0)(0, 0, 0);
        amrex::Real const escape_error = std::abs(after - std::exp(-1.0)) / std::exp(-1.0);

        nu.setVal(1.0);
        warpx::plugins::fluid::applyNeutrinoEscape(nu, rho_f, kappa, 1000.0 / tau);  // huge step
        amrex::Real const after_huge = nu.const_array(0)(0, 0, 0);

        std::printf("[neutrino] thick: %d sub-steps, decay measured %.6f vs exact %.6f,"
                    " relative error = %.3e, total drift = %.3e\n", nsub,
                    (double)decay_measured, (double)decay_exact, (double)diff_error,
                    (double)tot_error);
        std::printf("[neutrino] thin: one e-folding gives %.6f (exact %.6f, error %.3e),"
                    " a 1000-fold step gives %.3e (must be >= 0)\n",
                    (double)after, (double)std::exp(-1.0), (double)escape_error,
                    (double)after_huge);

        bool const ok = (diff_error < 5.0e-3) && (tot_error < 1.0e-12)
                     && (escape_error < 1.0e-12) && (after_huge >= 0.0);
        std::printf("[neutrino] %s\n", ok ? "PASSED" : "FAILED");
        if (!ok) { status = 1; }
    }
    amrex::Finalize();
    return status;
}
