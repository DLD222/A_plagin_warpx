/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
/* Electron thermal conduction: Spitzer conductivity in the energy equation.
 *
 * The conduction term is div(kappa grad T) added to the internal energy, with the Spitzer
 * conductivity
 *
 *     kappa_e(T) = 1.84e-5 T^{5/2} / ln(Lambda)        [erg cm^-1 s^-1 K^-1]
 *
 * (the coefficient already carries the electron mass and the 1/4 pi eps0 factors; only the Coulomb
 * logarithm is left as a parameter).  Writing it in flux form and updating out of place keeps the
 * scheme conservative: total energy changes only through the boundary flux, which is zero in a
 * periodic box and is exactly what the test measures.
 *
 * The T^{5/2} dependence makes this a nonlinear diffusion, and in a stellar interior it is stiff:
 * alpha = kappa/(rho c_v) can be enormous.  Explicit sub-stepping with dt <= 0.4 dx^2/(2 alpha_max)
 * is used, with a caller-supplied cap on the number of sub-steps, so a run can trade cost for
 * stability but never silently take an unstable step.  A linear-solver implicit version is the
 * natural next step and is not claimed here.
 *
 * A constant-conductivity entry point exists as well, because that is the case with an analytic
 * solution (the sine decay) and therefore the one the scheme can be checked against.
 *
 * Units are CGS.
 *
 * Specification: PHYSICS_SPEC.md section 5 (conductivity).
 */
#ifndef WARPX_PLUGINS_FLUID_CONDUCTION_H_
#define WARPX_PLUGINS_FLUID_CONDUCTION_H_

#include "FluidMoments.H"

#include <AMReX_Geometry.H>
#include <AMReX_MultiFab.H>

#include <cmath>

using warpx::plugins::fluid::star_box_hi;
using warpx::plugins::fluid::star_box_lo;

namespace warpx::plugins::fluid
{

namespace conduction
{
constexpr amrex::Real kappa0 = 1.84e-5;    //!< Spitzer coefficient [erg cm^-1 s^-1 K^-1]
constexpr amrex::Real m_proton = 1.67262192e-24;
constexpr amrex::Real k_boltzmann = 1.380649e-16;
constexpr amrex::Real default_coulomb_log = 5.0;

/** Spitzer electron conductivity [erg cm^-1 s^-1 K^-1]. */
AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE
amrex::Real spitzer_kappa (amrex::Real T, amrex::Real coulomb_log = default_coulomb_log) noexcept
{
    if (T <= amrex::Real(0.0) || coulomb_log <= amrex::Real(0.0)) { return amrex::Real(0.0); }
    return kappa0 * T * T * std::sqrt(T) / coulomb_log;
}
} // namespace conduction

/** Conduction sub-step count helper: the largest stable step given a maximum conductivity. */
AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE
amrex::Real conduction_dt_stable (amrex::Real alpha_max, amrex::Real dx) noexcept
{
    if (alpha_max <= amrex::Real(0.0)) { return amrex::Real(1.0e30); }
    return amrex::Real(0.4) * dx * dx / (amrex::Real(2.0) * alpha_max);
}

/** Add div(kappa grad T) * dt to the internal energy, conservatively.
 *
 *  \param state        conserved state; the energy component is updated
 *  \param gamma        adiabatic index (E_int = p/(gamma-1) is not assumed: T comes from E_int)
 *  \param layout       component layout
 *  \param x_comp       index of the ionisation extra, used for the electron mean molecular weight
 *  \param kappa_const  if > 0, use this constant conductivity instead of Spitzer
 *  \param coulomb_log  Coulomb logarithm for the Spitzer branch
 *  \param max_sub      cap on sub-steps (0 = uncapped)
 *  \return number of sub-steps taken
 */
inline int advanceThermalConduction (amrex::MultiFab& state, amrex::Geometry const& geom,
                                     amrex::Real dt, amrex::Real gamma, StateLayout const& layout,
                                     int x_comp, amrex::Real kappa_const = 0.0,
                                     amrex::Real coulomb_log = conduction::default_coulomb_log,
                                     int max_sub = 1000)
{
    using namespace amrex;
    amrex::Real const dx = geom.CellSize(0);

    auto internal_energy_and_T = [&](amrex::Array4<amrex::Real const> const& s, int i, int j, int k,
                                     amrex::Real& e_int, amrex::Real& T, amrex::Real& rho) {
        rho = Real(0.0);
        for (int a = 0; a < layout.n_species; ++a) { rho += s(i, j, k, layout.density(a)); }
        amrex::Real mom2 = Real(0.0);
        for (int d = 0; d < AMREX_SPACEDIM; ++d) {
            amrex::Real const m = s(i, j, k, layout.momentum(d));
            mom2 += m * m;
        }
        e_int = s(i, j, k, layout.energy()) - Real(0.5) * mom2 / amrex::max(rho, Real(1.0e-300));
        amrex::Real const x = (layout.n_extra > x_comp)
                            ? s(i, j, k, layout.extra(x_comp)) / amrex::max(rho, Real(1.0e-300))
                            : Real(0.0);
        T = (rho > Real(0.0))
          ? (gamma - Real(1.0)) * e_int
            / (rho / conduction::m_proton * (Real(1.0) + x) * conduction::k_boltzmann)
          : Real(0.0);
    };

    int taken = 0;
    amrex::Real remaining = dt;
    // c_v per unit mass: E_int = (1 + x) rho k T / (m_p (gamma - 1)), so c_v = (1+x) k/(m_p (gamma-1)).
    // It depends on the ionisation fraction, and using the fully ionised value everywhere would
    // under-estimate alpha (it is twice the neutral one) and let a neutral cell take a step it
    // should not.  The per-cell value is used instead.

    while (remaining > Real(0.0)) {
        // largest alpha in the field sets the sub-step
        amrex::Real alpha_max = 0.0;
        for (MFIter mfi(state); mfi.isValid(); ++mfi) {
            Box const& bx = mfi.validbox();
            auto const s = state.const_array(mfi);
            for (int i = bx.smallEnd(0); i <= bx.bigEnd(0); ++i) {
                for (int j = star_box_lo(bx, 1); j <= star_box_hi(bx, 1); ++j) {
                    for (int k = star_box_lo(bx, 2); k <= star_box_hi(bx, 2); ++k) {
                        amrex::Real e_int, T, rho;
                        internal_energy_and_T(s, i, j, k, e_int, T, rho);
                        amrex::Real const kap = (kappa_const > 0.0) ? kappa_const
                            : conduction::spitzer_kappa(T, coulomb_log);
                        amrex::Real const x = (layout.n_extra > x_comp)
                            ? s(i, j, k, layout.extra(x_comp)) / amrex::max(rho, Real(1.0e-300))
                            : Real(0.0);
                        amrex::Real const cv_local = (Real(1.0) + x) * conduction::k_boltzmann
                            / (conduction::m_proton * (gamma - Real(1.0)));
                        if (rho > Real(0.0)) {
                            alpha_max = amrex::max(alpha_max, kap / (rho * cv_local));
                        }
                    }
                }
            }
        }
        if (alpha_max <= Real(0.0)) { return taken; }
        amrex::Real const h = amrex::min(remaining, conduction_dt_stable(alpha_max, dx));

        // Out of place, like the radiation diffusion: an in-place update reads neighbours that
        // have already moved and is a different (faster) operator.
        amrex::MultiFab state_new(state.boxArray(), state.DistributionMap(), state.nComp(), 0);
        state.FillBoundary(geom.periodicity());
        for (MFIter mfi(state); mfi.isValid(); ++mfi) {
            Box const& bx = mfi.validbox();
            auto const s = state.const_array(mfi);
            auto s_new = state_new.array(mfi);
            for (int i = bx.smallEnd(0); i <= bx.bigEnd(0); ++i) {
                for (int j = star_box_lo(bx, 1); j <= star_box_hi(bx, 1); ++j) {
                    for (int k = star_box_lo(bx, 2); k <= star_box_hi(bx, 2); ++k) {
                        for (int c = 0; c < state.nComp(); ++c) { s_new(i, j, k, c) = s(i, j, k, c); }
                        amrex::Real e_int, T, rho;
                        internal_energy_and_T(s, i, j, k, e_int, T, rho);
                        amrex::Real div_flux = Real(0.0);
                        for (int d = 0; d < AMREX_SPACEDIM; ++d) {
                            int off[3] = {0, 0, 0};
                            off[d] = 1;
                            amrex::Real Tp, Tpi, rp;
                            amrex::Real Tm, Tmi, rm;
                            internal_energy_and_T(s, i + off[0], j + off[1], k + off[2],
                                                  Tpi, Tp, rp);
                            internal_energy_and_T(s, i - off[0], j - off[1], k - off[2],
                                                  Tmi, Tm, rm);
                            amrex::Real const kp = (kappa_const > 0.0) ? kappa_const
                                : Real(0.5) * (conduction::spitzer_kappa(T, coulomb_log)
                                             + conduction::spitzer_kappa(Tp, coulomb_log));
                            amrex::Real const km = (kappa_const > 0.0) ? kappa_const
                                : Real(0.5) * (conduction::spitzer_kappa(T, coulomb_log)
                                             + conduction::spitzer_kappa(Tm, coulomb_log));
                            div_flux += (kp * (Tp - T) - km * (T - Tm)) / (dx * dx);
                        }
                        amrex::Real const e_new = e_int + h * div_flux;
                        // never let conduction push the internal energy negative
                        s_new(i, j, k, layout.energy()) = s(i, j, k, layout.energy())
                                                        - e_int + amrex::max(e_new, Real(0.0));
                    }
                }
            }
        }
        state.ParallelCopy(state_new);
        remaining -= h;
        ++taken;
        if (max_sub > 0 && taken >= max_sub) { break; }
    }
    return taken;
}

} // namespace warpx::plugins::fluid

#endif // WARPX_PLUGINS_FLUID_CONDUCTION_H_
