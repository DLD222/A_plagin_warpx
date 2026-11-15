/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
/* Radiation: optically thin cooling, and grey flux-limited diffusion coupled to the energy equation.
 *
 * Two distinct pieces, because a stellar fluid needs both:
 *
 *  1. Optically thin losses.  The gas radiates and nothing comes back, so the energy equation gains
 *     a sink n_e n_i Lambda(T).  Lambda is the usual two-term fit - free-free plus recombination
 *     cooling over the range 1e4 < T < 1e8 K - written here with its coefficients in CGS:
 *
 *         Lambda(T) = 1.7e-27 sqrt(T) + 1.0e-24 T^{-0.7}      [erg cm^3 s^-1]
 *
 *  2. Grey flux-limited diffusion of a radiation energy density E_r, carried as one of the passive
 *     extras.  The diffusion coefficient is D = c lambda / (kappa rho) with the Levermore-Pomraning
 *     limiter lambda(R) = (2 + R) / (6 + 3R + R^2), R = |grad E_r| / (kappa rho E_r), which gives
 *     F = -D grad E_r and saturates at |F| = c E_r in the optically thin limit rather than
 *     exceeding it - the property that makes the diffusion limit usable near a stellar surface.
 *
 *     The diffusion is advanced with explicit sub-steps limited by 0.4 dx^2 / (2 D_max).  That is
 *     honest about the stiffness: D grows like c/(kappa rho), so a dense cell takes many sub-steps
 *     and the caller can bound them.  A linear-solver implicit version is the natural next step and
 *     is not claimed here.
 *
 *  3. Gas <-> radiation exchange: dE_gas/dt = -c kappa rho (E_r - a T^4), the same term with the
 *     opposite sign in the radiation equation.  Both are updated together and the sum is conserved
 *     by construction, which the test measures.
 *
 * Units are CGS throughout.
 *
 * Specification: PHYSICS_SPEC.md section 4 (radiation).
 */
#ifndef WARPX_PLUGINS_FLUID_RADIATION_H_
#define WARPX_PLUGINS_FLUID_RADIATION_H_

#include "FluidMoments.H"

#include <AMReX_Geometry.H>
#include <AMReX_MultiFab.H>

#include <cmath>

using warpx::plugins::fluid::star_box_hi;
using warpx::plugins::fluid::star_box_lo;

namespace warpx::plugins::fluid
{

namespace radiation
{
constexpr amrex::Real c_light = 2.99792458e10;     //!< [cm/s]
constexpr amrex::Real a_rad = 7.5657e-15;          //!< radiation constant [erg cm^-3 K^-4]
constexpr amrex::Real m_proton = 1.67262192e-24;

/** Optically thin cooling function [erg cm^3 s^-1]. */
AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE
amrex::Real cooling_function (amrex::Real T) noexcept
{
    if (T <= amrex::Real(0.0)) { return amrex::Real(0.0); }
    return amrex::Real(1.7e-27) * std::sqrt(T) + amrex::Real(1.0e-24) * std::pow(T, -0.7);
}

/** Volumetric cooling rate n_e n_i Lambda [erg cm^-3 s^-1] for ionisation fraction x. */
AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE
amrex::Real cooling_rate (amrex::Real rho, amrex::Real T, amrex::Real x) noexcept
{
    amrex::Real const n = rho / m_proton;
    return (n * x) * (n * x) * cooling_function(T);
}

/** Levermore-Pomraning flux limiter. */
AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE
amrex::Real flux_limiter (amrex::Real R) noexcept
{
    amrex::Real const r = amrex::max(R, amrex::Real(0.0));
    return (amrex::Real(2.0) + r) / (amrex::Real(6.0) + amrex::Real(3.0) * r + r * r);
}
} // namespace radiation

/** Optically thin cooling of the gas energy, with the ionisation fraction read from the state. */
inline void applyOpticallyThinCooling (amrex::MultiFab& state, amrex::Real dt, amrex::Real gamma,
                                       StateLayout const& layout, int x_comp)
{
    using namespace amrex;
    AMREX_ALWAYS_ASSERT_WITH_MESSAGE(layout.n_extra > x_comp,
        "applyOpticallyThinCooling: the state has no ionisation fraction");
    for (MFIter mfi(state); mfi.isValid(); ++mfi) {
        Box const& bx = mfi.validbox();
        auto s = state.array(mfi);
        for (int i = bx.smallEnd(0); i <= bx.bigEnd(0); ++i) {
            for (int j = star_box_lo(bx, 1); j <= star_box_hi(bx, 1); ++j) {
                for (int k = star_box_lo(bx, 2); k <= star_box_hi(bx, 2); ++k) {
                    amrex::Real rho = Real(0.0);
                    for (int a = 0; a < layout.n_species; ++a) {
                        rho += s(i, j, k, layout.density(a));
                    }
                    if (rho <= Real(0.0)) { continue; }
                    amrex::Real mom2 = Real(0.0);
                    for (int d = 0; d < AMREX_SPACEDIM; ++d) {
                        amrex::Real const m = s(i, j, k, layout.momentum(d));
                        mom2 += m * m;
                    }
                    amrex::Real e_int = s(i, j, k, layout.energy()) - Real(0.5) * mom2 / rho;
                    amrex::Real const x = s(i, j, k, layout.extra(x_comp)) / rho;
                    amrex::Real const T = (gamma - Real(1.0)) * e_int
                                        / (rho / radiation::m_proton * (Real(1.0) + x)
                                           * Real(1.380649e-16));
                    amrex::Real dE = radiation::cooling_rate(rho, T, x) * dt;
                    // A sink must never add energy.  Clamping at a small positive floor did exactly
                    // that when the requested loss exceeded the available internal energy (the
                    // ledger asserts the sign, which is how this was caught), and a non-finite rate
                    // has to be treated the same way rather than propagating.
                    if (!std::isfinite(dE) || dE > e_int) { dE = e_int; }
                    e_int = amrex::max(e_int - dE, Real(0.0));
                    s(i, j, k, layout.energy()) = e_int + Real(0.5) * mom2 / rho;
                }
            }
        }
    }
}

/** Grey flux-limited diffusion of the radiation energy density `er`.
 *
 *  \param er       radiation energy density (one MultiFab, at least 1 ghost cell)
 *  \param kappa    opacity [cm^2/g], read per cell from `kappa_arr` through `kappa_comp`
 *  \param rho      gas density used for the diffusion coefficient
 *  \param dt       time step; it is sub-cycled internally for stability
 *  \param max_sub  upper bound on the number of sub-steps (0 = no bound)
 *  \return the number of sub-steps actually taken
 */
inline int advanceRadiationDiffusion (amrex::MultiFab& er, amrex::MultiFab const& rho,
                                      amrex::Real kappa, amrex::Geometry const& geom,
                                      amrex::Real dt, int max_sub = 100000)
{
    using namespace amrex;
    amrex::Real const dx = geom.CellSize(0);
    amrex::Real const cfl = Real(0.4);
    int taken = 0;
    amrex::Real remaining = dt;
    while (remaining > Real(0.0)) {
        // largest D in the field sets the sub-step (flux limiter bounded by 1/3 in the diffusion
        // limit, so D <= c/(3 kappa rho) there)
        amrex::Real dmax = 0.0;
        for (MFIter mfi(er); mfi.isValid(); ++mfi) {
            Box const& bx = mfi.validbox();
            auto const e = er.const_array(mfi);
            auto const r = rho.const_array(mfi);
            for (int i = bx.smallEnd(0); i <= bx.bigEnd(0); ++i) {
                for (int j = star_box_lo(bx, 1); j <= star_box_hi(bx, 1); ++j) {
                    for (int k = star_box_lo(bx, 2); k <= star_box_hi(bx, 2); ++k) {
                        amrex::Real const rr = r(i, j, k, 0);
                        if (rr <= 0.0 || kappa <= 0.0) { continue; }
                        dmax = amrex::max(dmax, radiation::c_light / (Real(3.0) * kappa * rr));
                        amrex::ignore_unused(e);
                    }
                }
            }
        }
        if (dmax <= 0.0) { return taken; }
        // Multi-dimensional von Neumann limit: h <= dx^2 / (2 d D).
        amrex::Real const dt_stable =
            cfl * dx * dx / (Real(2.0) * Real(AMREX_SPACEDIM) * dmax);
        amrex::Real const h = amrex::min(remaining, dt_stable);
        er.FillBoundary(geom.periodicity());
        // Out of place: updating `er` in place makes each cell read neighbours that have already
        // been advanced (Gauss-Seidel instead of Jacobi), which is a different operator and decayed
        // a test mode 2.3x too fast.
        // Zero it: the update loop `continue`s for cells with rho <= 0 or kappa <= 0, and AMReX
        // does not zero fresh memory -- those cells would otherwise go back into `er` with
        // whatever the allocation happened to contain.
        amrex::MultiFab er_new(er.boxArray(), er.DistributionMap(), 1, 0);
        er_new.setVal(0.0);
        for (MFIter mfi(er); mfi.isValid(); ++mfi) {
            Box const& bx = mfi.validbox();
            auto const e = er.const_array(mfi);
            auto const r = rho.const_array(mfi);
            auto e_new = er_new.array(mfi);
            for (int i = bx.smallEnd(0); i <= bx.bigEnd(0); ++i) {
                for (int j = star_box_lo(bx, 1); j <= star_box_hi(bx, 1); ++j) {
                    for (int k = star_box_lo(bx, 2); k <= star_box_hi(bx, 2); ++k) {
                        amrex::Real const rr = r(i, j, k, 0);
                        if (rr <= 0.0 || kappa <= 0.0) {
                            e_new(i, j, k) = e(i, j, k);  // no diffusion here: keep the value
                            continue;
                        }
                        amrex::Real const ec = e(i, j, k);
                        amrex::Real flux_sum = 0.0;
                        for (int d = 0; d < AMREX_SPACEDIM; ++d) {
                            int off[3] = {0, 0, 0};
                            off[d] = 1;
                            amrex::Real const ep = e(i + off[0], j + off[1], k + off[2]);
                            amrex::Real const em = e(i - off[0], j - off[1], k - off[2]);
                            // face gradients, limiter evaluated from the local gradient
                            amrex::Real const grad = std::abs(ep - em) / (Real(2.0) * dx);
                            amrex::Real const R = (ec > 0.0 && kappa * rr * ec > 0.0)
                                ? grad / (kappa * rr * ec) : Real(0.0);
                            amrex::Real const D = radiation::c_light
                                                * radiation::flux_limiter(R) / (kappa * rr);
                            flux_sum += D * (ep - Real(2.0) * ec + em) / (dx * dx);
                        }
                        e_new(i, j, k) = amrex::max(ec + h * flux_sum, Real(0.0));
                    }
                }
            }
        }
        er.ParallelCopy(er_new);
        remaining -= h;
        ++taken;
        if (max_sub > 0 && taken >= max_sub) {
            // Do not fall silent: stopping here means the operator advanced less than the
            // requested interval, and a caller that does not look at the return value would
            // integrate the wrong time.
            amrex::Print() << "[fluid] explicit diffusion reached max_sub = " << max_sub
                           << " with " << remaining << " of " << dt
                           << " still to integrate (narrow the step or raise max_sub)\n";
            break;
        }
    }
    return taken;
}

/** Gas <-> radiation exchange, conserving the sum of gas and radiation energy.
 *
 *  Implicit in the exchange: with G = c kappa rho and B = a T^4,
 *
 *      E_r^{n+1} = (E_r^n + dt G B^n) / (1 + dt G)
 *
 *  and the gas takes exactly the opposite amount, so the total is conserved to round-off even at
 *  dt G >> 1 (the limit where an explicit exchange would overshoot into negative energies).
 */
inline void applyRadiationCoupling (amrex::MultiFab& state, amrex::MultiFab& er,
                                    amrex::Real kappa, amrex::Real dt, amrex::Real gamma,
                                    StateLayout const& layout)
{
    using namespace amrex;
    for (MFIter mfi(state); mfi.isValid(); ++mfi) {
        Box const& bx = mfi.validbox();
        auto s = state.array(mfi);
        auto const rho_a = state.const_array(mfi);
        auto e = er.array(mfi);
        for (int i = bx.smallEnd(0); i <= bx.bigEnd(0); ++i) {
            for (int j = star_box_lo(bx, 1); j <= star_box_hi(bx, 1); ++j) {
                for (int k = star_box_lo(bx, 2); k <= star_box_hi(bx, 2); ++k) {
                    amrex::Real rho = Real(0.0);
                    for (int a = 0; a < layout.n_species; ++a) {
                        rho += rho_a(i, j, k, layout.density(a));
                    }
                    if (rho <= Real(0.0)) { continue; }
                    amrex::Real mom2 = Real(0.0);
                    for (int d = 0; d < AMREX_SPACEDIM; ++d) {
                        amrex::Real const m = s(i, j, k, layout.momentum(d));
                        mom2 += m * m;
                    }
                    amrex::Real const e_int = s(i, j, k, layout.energy()) - Real(0.5) * mom2 / rho;
                    // blackbody energy at the gas temperature; the mean molecular weight is not
                    // carried here, so use the hydrogen+electron count like the ionisation module
                    amrex::Real const x = (layout.n_extra > 0) ? s(i, j, k, layout.extra(0)) / rho
                                                               : Real(0.0);
                    amrex::Real const T = (gamma - Real(1.0)) * e_int
                                        / (rho / radiation::m_proton * (Real(1.0) + x)
                                           * Real(1.380649e-16));
                    amrex::Real const b_planck = radiation::a_rad * T * T * T * T;
                    amrex::Real const G = radiation::c_light * kappa * rho;
                    amrex::Real const old = e(i, j, k);
                    amrex::Real const updated = (old + dt * G * b_planck) / (Real(1.0) + dt * G);
                    e(i, j, k) = amrex::max(updated, Real(0.0));
                    // the gas takes exactly the opposite of what the radiation gained
                    amrex::Real const dE = e(i, j, k) - old;
                    s(i, j, k, layout.energy()) -= dE;
                }
            }
        }
    }
}

} // namespace warpx::plugins::fluid

#endif // WARPX_PLUGINS_FLUID_RADIATION_H_
