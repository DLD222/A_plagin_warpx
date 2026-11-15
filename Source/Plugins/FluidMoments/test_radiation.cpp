/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Acceptance test: radiation - optically thin cooling, flux-limited diffusion, gas exchange.
 *
 * Four checks against quantities that can be computed independently of the scheme:
 *
 *  1. cooling: a single cell loses exactly n_e n_i Lambda(T) dt of internal energy (the sink is
 *     evaluated in the test from the same published fit, so this checks the bookkeeping and the
 *     units, not the fit itself);
 *  2. diffusion: a sinusoidal radiation energy density in a periodic box decays as exp(-D k^2 t)
 *     with D = c/(3 kappa rho) in the diffusion limit - the classic analytic solution of the
 *     linear diffusion equation, compared at the level the explicit sub-cycling allows;
 *  3. exchange: gas + radiation energy is conserved through applyRadiationCoupling to round-off,
 *     and the radiation field is driven to a T^4 for dt G >> 1 without overshoot (the reason the
 *     exchange is implicit);
 *  4. flux limiter: lambda R <= 1 for R from 1e-6 to 1e6, i.e. the flux never exceeds c E_r.
 *
 * Specification: PHYSICS_SPEC.md section 4.
 */
#include "FluidMoments.H"
#include "Radiation.H"

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
using namespace warpx::plugins::fluid::radiation;

constexpr int n_cell = 64;
constexpr int n_species = 1;
constexpr int n_extra = 1;
constexpr int n_comp = n_species + AMREX_SPACEDIM + 1 + n_extra;
constexpr amrex::Real gas_gamma = 1.4;
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
        StateLayout const layout{n_species, n_extra};

        // ---- 1. optically thin cooling -------------------------------------------------
        amrex::MultiFab state(ba, dm, n_comp, 1);
        amrex::Real const rho1 = 1.0e-3;
        amrex::Real const T1 = 1.0e6;
        amrex::Real const x1 = 1.0;
        amrex::Real const n1 = rho1 / m_proton;
        amrex::Real const p1 = n1 * (1.0 + x1) * 1.380649e-16 * T1;
        for (amrex::MFIter mfi(state); mfi.isValid(); ++mfi) {
            amrex::Box const& bx = mfi.validbox();
            auto s = state[mfi].array();
            for (int i = bx.smallEnd(0); i <= bx.bigEnd(0); ++i) {
                for (int j = star_box_lo(bx, 1); j <= star_box_hi(bx, 1); ++j) {
                    for (int k = star_box_lo(bx, 2); k <= star_box_hi(bx, 2); ++k) {
                        for (int q = 0; q < n_comp; ++q) { s(i, j, k, q) = 0.0; }
                        s(i, j, k, layout.density(0)) = rho1;
                        s(i, j, k, layout.energy()) = p1 / (gas_gamma - 1.0);
                        s(i, j, k, layout.extra(0)) = x1 * rho1;
                    }
                }
            }
        }
        amrex::Real const E0 = state.const_array(0)(0, 0, 0, layout.energy());
        // The cooling time here is e / (n^2 Lambda) ~ 7e-7 s, so a millisecond step would try to
        // remove a billion times the available energy (the floor then clamps it and the check
        // would measure the clamp).  Use 1e-8 s: a 1.5% loss, which is what the test is about.
        amrex::Real const dt_cool = 1.0e-8;   // s
        warpx::plugins::fluid::applyOpticallyThinCooling(state, dt_cool, gas_gamma, layout, 0);
        amrex::Real const E1 = state.const_array(0)(0, 0, 0, layout.energy());
        amrex::Real const expected_loss = cooling_rate(rho1, T1, x1) * dt_cool;
        amrex::Real const cool_error = std::abs((E0 - E1) - expected_loss) / expected_loss;

        // ---- 2. diffusion of a sinusoidal radiation field ------------------------------
        amrex::MultiFab er(ba, dm, 1, 1);
        amrex::MultiFab rho(ba, dm, 1, 0);
        amrex::Real const kappa = 1.0;        // cm^2/g
        amrex::Real const k = 2.0 * M_PI;
        for (amrex::MFIter mfi(er); mfi.isValid(); ++mfi) {
            amrex::Box const& bx = mfi.validbox();
            auto e = er[mfi].array();
            auto r = rho[mfi].array();
            for (int i = bx.smallEnd(0); i <= bx.bigEnd(0); ++i) {
                amrex::Real const x = (static_cast<amrex::Real>(i) + 0.5) * dx - 0.5;
                for (int j = star_box_lo(bx, 1); j <= star_box_hi(bx, 1); ++j) {
                    for (int q = star_box_lo(bx, 2); q <= star_box_hi(bx, 2); ++q) {
                        e(i, j, q) = 1.0 + 1.0e-6 * std::cos(k * x);
                        r(i, j, q) = rho1;
                    }
                }
            }
        }
        auto amplitude = [&]() {
            amrex::Real worst = 0.0;
            for (amrex::MFIter mfi(er); mfi.isValid(); ++mfi) {
                amrex::Box const& bx = mfi.validbox();
                auto const e = er.const_array(mfi);
                for (int i = bx.smallEnd(0); i <= bx.bigEnd(0); ++i) {
                    amrex::Real const x = (static_cast<amrex::Real>(i) + 0.5) * dx - 0.5;
                    worst += (e(i, 0, 0) - 1.0) * std::cos(k * x);
                }
            }
            return worst / n_cell * 2.0;
        };
        amrex::Real const amp0 = amplitude();
        // Small amplitude on purpose: R = |grad E| / (kappa rho E) must be << 1 so the limiter sits
        // at its diffusion-limit value 1/3, and the analytic exp(-D k^2 t) with D = c/(3 kappa rho)
        // is the right reference.  A large amplitude makes the comparison test the limiter instead.
        amrex::Real const D = c_light / (3.0 * kappa * rho1);
        // Decay the mode by exp(-0.3): large enough to measure, small enough for the explicit
        // sub-cycling to resolve it.
        amrex::Real const t_diff = 0.3 / (D * k * k);
        int const nsub = warpx::plugins::fluid::advanceRadiationDiffusion(er, rho, kappa, geom,
                                                                         t_diff, 200000);
        amrex::Real const amp1 = amplitude();
        amrex::Real const decay_measured = amp1 / amp0;
        amrex::Real const decay_exact = std::exp(-D * k * k * t_diff);
        amrex::Real const diff_error = std::abs(decay_measured - decay_exact) / decay_exact;

        // ---- 3. gas <-> radiation exchange ---------------------------------------------
        amrex::MultiFab er2(ba, dm, 1, 0);
        er2.setVal(0.0);
        amrex::MultiFab state2(ba, dm, n_comp, 0);
        amrex::Real const T2 = 1.0e5;
        amrex::Real const p2 = n1 * 2.0 * 1.380649e-16 * T2;
        for (amrex::MFIter mfi(state2); mfi.isValid(); ++mfi) {
            amrex::Box const& bx = mfi.validbox();
            auto s = state2[mfi].array();
            for (int i = bx.smallEnd(0); i <= bx.bigEnd(0); ++i) {
                for (int j = star_box_lo(bx, 1); j <= star_box_hi(bx, 1); ++j) {
                    for (int q = star_box_lo(bx, 2); q <= star_box_hi(bx, 2); ++q) {
                        for (int c = 0; c < n_comp; ++c) { s(i, j, q, c) = 0.0; }
                        s(i, j, q, layout.density(0)) = rho1;
                        s(i, j, q, layout.energy()) = p2 / (gas_gamma - 1.0);
                        s(i, j, q, layout.extra(0)) = rho1;
                    }
                }
            }
        }
        auto totals = [&](amrex::MultiFab& st, amrex::MultiFab& erx) {
            amrex::Real gas = 0.0, rad = 0.0;
            for (amrex::MFIter mfi(st); mfi.isValid(); ++mfi) {
                gas += st.const_array(mfi)(0, 0, 0, layout.energy());
                rad += erx.const_array(mfi)(0, 0, 0);
            }
            return gas + rad;
        };
        amrex::Real const sum0 = totals(state2, er2);
        warpx::plugins::fluid::applyRadiationCoupling(state2, er2, kappa, 1.0e4, gas_gamma,
                                                      layout);
        amrex::Real const sum1 = totals(state2, er2);
        amrex::Real const sum_error = std::abs(sum1 - sum0) / std::abs(sum0);
        // a very long exchange must land on a T^4 without overshoot
        for (int it = 0; it < 40; ++it) {
            warpx::plugins::fluid::applyRadiationCoupling(state2, er2, kappa, 1.0e8, gas_gamma,
                                                          layout);
        }
        amrex::Real const er_final = er2.const_array(0)(0, 0, 0);
        amrex::Real const e_gas = state2.const_array(0)(0, 0, 0, layout.energy());
        amrex::Real const T_final = (gas_gamma - 1.0) * e_gas / (n1 * 2.0 * 1.380649e-16);
        amrex::Real const b_final = a_rad * T_final * T_final * T_final * T_final;
        amrex::Real const exchange_error = std::abs(er_final - b_final) / b_final;

        // ---- 4. flux limiter -----------------------------------------------------------
        amrex::Real worst_limiter = 0.0;
        for (amrex::Real R : {1.0e-6, 1.0e-3, 1.0, 10.0, 1.0e3, 1.0e6}) {
            worst_limiter = amrex::max(worst_limiter, flux_limiter(R) * R);
        }

        std::printf("[radiation] cooling error = %.3e (expected %.3e, got %.3e)\n",
                    (double)cool_error, (double)expected_loss, (double)(E0 - E1));
        std::printf("[radiation] diffusion: %d sub-steps, amp0 = %.4e, amp1 = %.4e,"
                    " measured %.6f vs exact %.6f, relative error = %.3e\n",
                    nsub, (double)amp0, (double)amp1, (double)decay_measured,
                    (double)decay_exact, (double)diff_error);
        std::printf("[radiation] exchange: total drift = %.3e, E_r/B(T) - 1 = %.3e\n",
                    (double)sum_error, (double)exchange_error);
        std::printf("[radiation] max(lambda R) = %.6f (must be <= 1)\n", (double)worst_limiter);

        bool const ok = (cool_error < 1.0e-12) && (diff_error < 2.0e-2)
                     && (sum_error < 1.0e-12) && (exchange_error < 1.0e-3)
                     && (worst_limiter <= 1.0);
        std::printf("[radiation] %s\n", ok ? "PASSED" : "FAILED");
        if (!ok) { status = 1; }
    }
    amrex::Finalize();
    return status;
}
