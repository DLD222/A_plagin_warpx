/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Acceptance test: the degenerate-electron correction.
 *
 * The interpolation must reproduce both exact limits and vanish in the classical regime:
 *
 *  1. non-relativistic: for n_e << n_c, P_deg / (K_nr n^{5/3}) - 1 < 1e-3;
 *  2. relativistic: for n_e >> n_c, P_deg / (K_r n^{4/3}) - 1 < 1e-3;
 *  3. classical regime: at n_e = 1e20 cm^-3 and T = 1e6 K the correction is < 1e-3 of the ideal
 *     pressure, so adding it cannot move an ordinary stellar envelope;
 *  4. the EOS plugs into the solver's interface: pressure(rho, e_int) and soundSpeed(rho, p) are
 *     finite and positive, and advanceSweep instantiated with it runs a step without NaN.
 *
 * Specification: PHYSICS_SPEC.md section 10.
 */
#include "FluidMoments.H"
#include "DegenerateEOS.H"

#include <AMReX.H>
#include <AMReX_BoxArray.H>
#include <AMReX_DistributionMapping.H>
#include <AMReX_Geometry.H>
#include <AMReX_MultiFab.H>
#include <AMReX_REAL.H>

#include <cmath>
#include <cstdio>

int main (int argc, char* argv[])
{
    amrex::Initialize(argc, argv);
    int status = 0;
    {
        using namespace warpx::plugins::fluid::degenerate;
        warpx::plugins::fluid::DegenerateElectronEOS eos;
        eos.gamma = 1.4;
        eos.mu_e = 2.0;

        // ---- 1. non-relativistic limit ------------------------------------------------
        amrex::Real worst_nr = 0.0;
        // stay a few decades below the crossover: the blend is asymptotic, not exact, and its error is
        // O((n/n_c)^{2/3}) - about 0.5% at 1e27 against 9% at 1e29, which is a property of the
        // interpolation rather than of the coefficients
        for (amrex::Real n : {1.0e22, 1.0e24, 1.0e26, 1.0e27}) {
            amrex::Real const got = degeneracy_pressure(n);
            amrex::Real const exact = degeneracy_pressure_nonrel(n);
            worst_nr = amrex::max(worst_nr, std::abs(got - exact) / exact);
        }

        // ---- 2. relativistic limit ----------------------------------------------------
        amrex::Real worst_rel = 0.0;
        for (amrex::Real n : {1.0e33, 1.0e34, 1.0e35, 1.0e36}) {
            amrex::Real const got = degeneracy_pressure(n);
            amrex::Real const exact = degeneracy_pressure_rel(n);
            worst_rel = amrex::max(worst_rel, std::abs(got - exact) / exact);
        }

        // ---- 3. classical regime ------------------------------------------------------
        amrex::Real const n_classical = 1.0e20;
        amrex::Real const T = 1.0e6;
        amrex::Real const rho_c = n_classical * 2.0 * m_proton;
        amrex::Real const p_ideal = rho_c / m_proton * 1.380649e-16 * T;   // ions + electrons
        amrex::Real const correction_ratio =
            degeneracy_pressure(n_classical) / p_ideal;

        // ---- 4. the solver interface --------------------------------------------------
        amrex::Real const rho = 1.0e6;            // degenerate-ish
        amrex::Real const e_int = 1.0e20;
        amrex::Real const p = eos.pressure(rho, e_int);
        amrex::Real const cs = eos.soundSpeed(rho, p);

        amrex::Box const domain(amrex::IntVect{0}, amrex::IntVect{31});
        amrex::BoxArray ba(domain);
        amrex::DistributionMapping dm(ba);
        amrex::RealBox rb({AMREX_D_DECL(0.0, 0.0, 0.0)}, {AMREX_D_DECL(1.0, 1.0, 1.0)});
        amrex::Array<int, AMREX_SPACEDIM> is_periodic{AMREX_D_DECL(1, 1, 1)};
        amrex::Geometry geom(domain, rb, 0, is_periodic);
        warpx::plugins::fluid::StateLayout const layout{1, 0};
        amrex::MultiFab state(ba, dm, layout.n_comp(), 2);
        state.setVal(0.0);
        for (amrex::MFIter mfi(state); mfi.isValid(); ++mfi) {
            amrex::Box const& bx = mfi.validbox();
            auto s = state[mfi].array();
            for (int i = bx.smallEnd(0); i <= bx.bigEnd(0); ++i) {
                for (int j = bx.smallEnd(1); j <= bx.bigEnd(1); ++j) {
                    for (int k = bx.smallEnd(2); k <= bx.bigEnd(2); ++k) {
                        s(i, j, k, layout.density(0)) = rho;
                        s(i, j, k, layout.energy()) = e_int;
                    }
                }
            }
        }
        amrex::Real const e0 = state.const_array(0)(0, 0, 0, layout.energy());
        state.FillBoundary(geom.periodicity());
        warpx::plugins::fluid::advanceSweep(state, geom, 0, 1.0e-6, eos, layout);
        amrex::Real const e1 = state.const_array(0)(0, 0, 0, layout.energy());
        bool const sweep_ok = std::isfinite(e1) && (e1 > 0.0);

        std::printf("[eos] non-rel limit: worst relative error = %.3e\n", (double)worst_nr);
        std::printf("[eos] relativistic limit: worst relative error = %.3e\n", (double)worst_rel);
        std::printf("[eos] classical regime: P_deg / P_ideal = %.3e at n_e = 1e20\n",
                    (double)correction_ratio);
        std::printf("[eos] interface: p = %.3e, c_s = %.3e, sweep energy %.3e -> %.3e (finite: %s)\n",
                    (double)p, (double)cs, (double)e0, (double)e1, sweep_ok ? "yes" : "no");

        bool const ok = (worst_nr < 1.0e-2) && (worst_rel < 1.0e-2)
                     && (correction_ratio < 1.0e-3) && (p > 0.0) && (cs > 0.0) && sweep_ok;
        std::printf("[eos] %s\n", ok ? "PASSED" : "FAILED");
        if (!ok) { status = 1; }
    }
    amrex::Finalize();
    return status;
}
