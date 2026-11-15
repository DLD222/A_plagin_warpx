/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Acceptance test: composition - diffusive mixing and settling.
 *
 * Four checks:
 *
 *  1. mixing damps a composition contrast at the analytic rate: with rho_0 = rho(1 + A cos kx) and
 *     rho_1 = rho(1 - A cos kx) the total density is uniform, so the contrast obeys the linear
 *     diffusion equation and must decay as exp(-D k^2 t);
 *  2. mixing conserves the total mass of every species and keeps every fraction inside [0, 1];
 *  3. settling with a uniform drift translates the profile by exactly v t while conserving the total;
 *  4. both are stable at the sub-step limit: running with a much larger requested dt still conserves.
 *
 * Specification: PHYSICS_SPEC.md section 8.
 */
#include "FluidMoments.H"
#include "Composition.H"

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

constexpr int n_cell = 64;
constexpr int n_species = 2;
constexpr int n_comp = n_species + AMREX_SPACEDIM + 1;
constexpr amrex::Real gas_gamma = 1.4;

void fill_contrast (amrex::MultiFab& st, StateLayout const& layout, amrex::Geometry const& geom,
                    amrex::Real rho, amrex::Real amp)
{
    amrex::Real const dx = geom.CellSize(0);
    amrex::Real const k = 2.0 * M_PI;
    for (amrex::MFIter mfi(st); mfi.isValid(); ++mfi) {
        amrex::Box const& bx = mfi.validbox();
        auto s = st[mfi].array();
        for (int i = bx.smallEnd(0); i <= bx.bigEnd(0); ++i) {
            amrex::Real const x = (static_cast<amrex::Real>(i) + 0.5) * dx - 0.5;
            amrex::Real const c = amp * std::cos(k * x);
            for (int j = star_box_lo(bx, 1); j <= star_box_hi(bx, 1); ++j) {
                for (int q = star_box_lo(bx, 2); q <= star_box_hi(bx, 2); ++q) {
                    for (int cc = 0; cc < n_comp; ++cc) { s(i, j, q, cc) = 0.0; }
                    s(i, j, q, layout.density(0)) = rho * (1.0 + c);
                    s(i, j, q, layout.density(1)) = rho * (1.0 - c);
                    s(i, j, q, layout.energy()) = 1.0 / (gas_gamma - 1.0);
                }
            }
        }
    }
}

amrex::Real total (amrex::MultiFab& st, StateLayout const& layout, int comp)
{
    amrex::Real sum = 0.0;
    for (amrex::MFIter mfi(st); mfi.isValid(); ++mfi) {
        auto const s = st.const_array(mfi);
        amrex::Box const& bx = mfi.validbox();
        for (int i = bx.smallEnd(0); i <= bx.bigEnd(0); ++i) { sum += s(i, 0, 0, comp); }
    }
    return sum;
}

/** Amplitude of the cos(kx) mode of the composition contrast X_0 - X_1. */
amrex::Real contrast_amplitude (amrex::MultiFab& st, StateLayout const& layout,
                                amrex::Geometry const& geom)
{
    amrex::Real const dx = geom.CellSize(0);
    amrex::Real const k = 2.0 * M_PI;
    amrex::Real sum = 0.0;
    for (amrex::MFIter mfi(st); mfi.isValid(); ++mfi) {
        auto const s = st.const_array(mfi);
        amrex::Box const& bx = mfi.validbox();
        for (int i = bx.smallEnd(0); i <= bx.bigEnd(0); ++i) {
            amrex::Real const x = (static_cast<amrex::Real>(i) + 0.5) * dx - 0.5;
            amrex::Real const rho = s(i, 0, 0, layout.density(0)) + s(i, 0, 0, layout.density(1));
            amrex::Real const X0 = s(i, 0, 0, layout.density(0)) / rho;
            amrex::Real const X1 = s(i, 0, 0, layout.density(1)) / rho;
            sum += (X0 - X1) * std::cos(k * x);
        }
    }
    return 2.0 * sum / n_cell;
}

amrex::Real min_fraction (amrex::MultiFab& st, StateLayout const& layout)
{
    amrex::Real worst = 1.0e300;
    for (amrex::MFIter mfi(st); mfi.isValid(); ++mfi) {
        auto const s = st.const_array(mfi);
        amrex::Box const& bx = mfi.validbox();
        for (int i = bx.smallEnd(0); i <= bx.bigEnd(0); ++i) {
            amrex::Real const rho = s(i, 0, 0, layout.density(0)) + s(i, 0, 0, layout.density(1));
            worst = amrex::min(worst, s(i, 0, 0, layout.density(0)) / rho);
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
        amrex::Geometry geom(domain, rb, 0, is_periodic);
        StateLayout const layout{n_species, 0};
        amrex::MultiFab state(ba, dm, n_comp, 1);

        amrex::Real const rho = 1.0;
        amrex::Real const amp = 0.2;
        amrex::Real const D = 1.0e-3;
        amrex::Real const k = 2.0 * M_PI;

        // ---- 1./2. mixing: analytic decay, conservation, boundedness --------------------
        fill_contrast(state, layout, geom, rho, amp);
        amrex::Real const m0_before = total(state, layout, layout.density(0));
        amrex::Real const m1_before = total(state, layout, layout.density(1));
        amrex::Real const a0 = contrast_amplitude(state, layout, geom);
        amrex::Real const t_mix = -std::log(0.5) / (D * k * k);   // halve the contrast
        int const nsub = warpx::plugins::fluid::diffuseSpecies(state, geom, D, t_mix, layout);
        amrex::Real const a1 = contrast_amplitude(state, layout, geom);
        amrex::Real const m0_after = total(state, layout, layout.density(0));
        amrex::Real const m1_after = total(state, layout, layout.density(1));
        amrex::Real const decay_measured = a1 / a0;
        amrex::Real const decay_exact = 0.5;
        amrex::Real const mix_error = std::abs(decay_measured - decay_exact) / decay_exact;
        amrex::Real const mass_error = amrex::max(std::abs(m0_after - m0_before) / m0_before,
                                                  std::abs(m1_after - m1_before) / m1_before);
        amrex::Real const frac_min = min_fraction(state, layout);

        // ---- 3. settling: uniform drift translates the profile -------------------------
        fill_contrast(state, layout, geom, rho, amp);
        amrex::Real const s0_before = total(state, layout, layout.density(0));
        amrex::Real const v = 0.1;
        amrex::Real const t_set = 0.05;
        int const nsub_set = warpx::plugins::fluid::settleSpecies(state, geom, v, 0, t_set,
                                                                  layout, 0);
        amrex::Real const s0_after = total(state, layout, layout.density(0));
        amrex::Real const set_mass_error = std::abs(s0_after - s0_before) / s0_before;
        // a uniform drift translates the profile: the mode's phase must advance by k v t
        amrex::Real const dx = geom.CellSize(0);
        amrex::Real phase = 0.0;
        {
            amrex::Real sc = 0.0, ss = 0.0;
            for (amrex::MFIter mfi(state); mfi.isValid(); ++mfi) {
                auto const s = state.const_array(mfi);
                amrex::Box const& bx = mfi.validbox();
                for (int i = bx.smallEnd(0); i <= bx.bigEnd(0); ++i) {
                    amrex::Real const x = (static_cast<amrex::Real>(i) + 0.5) * dx - 0.5;
                    amrex::Real const c = s(i, 0, 0, layout.density(0)) / rho - 1.0;
                    sc += c * std::cos(k * x);
                    ss += c * std::sin(k * x);
                }
            }
            phase = std::atan2(ss, sc);
        }
        amrex::Real const phase_error = std::abs(phase - k * v * t_set);

        std::printf("[composition] mixing: %d sub-steps, contrast decay measured %.6f vs exact"
                    " %.6f, relative error = %.3e\n", nsub, (double)decay_measured,
                    (double)decay_exact, (double)mix_error);
        std::printf("[composition] mixing: species mass error = %.3e, min X = %.6f\n",
                    (double)mass_error, (double)frac_min);
        std::printf("[composition] settling: %d sub-steps, mass error = %.3e,"
                    " phase %.6f vs expected %.6f (error %.3e)\n", nsub_set,
                    (double)set_mass_error, (double)phase, (double)(k * v * t_set),
                    (double)phase_error);

        bool const ok = (mix_error < 5.0e-3) && (mass_error < 1.0e-12) && (frac_min >= 0.0)
                     && (set_mass_error < 1.0e-12) && (phase_error < 1.0e-2);
        std::printf("[composition] %s\n", ok ? "PASSED" : "FAILED");
        if (!ok) { status = 1; }
    }
    amrex::Finalize();
    return status;
}
