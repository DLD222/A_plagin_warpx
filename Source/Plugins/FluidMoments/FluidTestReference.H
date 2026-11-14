/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef WARPX_FLUIDTESTREFERENCE_H_
#define WARPX_FLUIDTESTREFERENCE_H_

#include <AMReX_BoxArray.H>
#include <AMReX_DistributionMapping.H>
#include <AMReX_Geometry.H>
#include <AMReX_MultiFab.H>
#include <AMReX_REAL.H>

#include <algorithm>
#include <cmath>
#include <cstdio>

/** Analytic reference for the fluid acceptance tests: the exact Riemann solution of the
 *  one-dimensional Euler equations (Toro, *Riemann Solvers and Numerical Methods for Fluid
 *  Dynamics*, 3rd ed., ch. 4, in particular the pressure function 4.39-4.47, the star state
 *  4.55-4.57 and the sampling procedure 4.63-4.68).
 *
 * The implementation is deliberately self-contained and is cross-checked in
 * Tools/star/sod_reference.py, which solves the same star-state equation independently and
 * prints the star state for the tests to confirm.
 */
namespace warpx::plugins::fluid::test
{

/** Exact solution of the Riemann problem for a gamma-law gas. */
struct ExactRiemann
{
    amrex::Real gamma;
    amrex::Real rho_l, u_l, p_l;
    amrex::Real rho_r, u_r, p_r;

    amrex::Real p_star = 0.0;
    amrex::Real u_star = 0.0;
    amrex::Real c_l = 0.0;
    amrex::Real c_r = 0.0;

    ExactRiemann (amrex::Real g, amrex::Real rl, amrex::Real ul, amrex::Real pl,
                  amrex::Real rr, amrex::Real ur, amrex::Real pr)
        : gamma(g), rho_l(rl), u_l(ul), p_l(pl), rho_r(rr), u_r(ur), p_r(pr)
    {
        c_l = std::sqrt(gamma * p_l / rho_l);
        c_r = std::sqrt(gamma * p_r / rho_r);
        solveStarState();
    }

    /** Pressure function f_K(p) of Toro (4.39)-(4.47): its zero, summed over both sides, is p*. */
    amrex::Real pressureFunction (amrex::Real p, bool left) const
    {
        amrex::Real const rho = left ? rho_l : rho_r;
        amrex::Real const p_k = left ? p_l : p_r;
        amrex::Real const c_k = left ? c_l : c_r;
        if (p > p_k) {
            amrex::Real const a = amrex::Real(2.0) / ((gamma + amrex::Real(1.0)) * rho);
            amrex::Real const b = (gamma - amrex::Real(1.0)) / (gamma + amrex::Real(1.0)) * p_k;
            return (p - p_k) * std::sqrt(a / (p + b));
        }
        amrex::Real const expo = (gamma - amrex::Real(1.0)) / (amrex::Real(2.0) * gamma);
        return amrex::Real(2.0) * c_k / (gamma - amrex::Real(1.0))
             * (std::pow(p / p_k, expo) - amrex::Real(1.0));
    }

    amrex::Real pressureFunctionDerivative (amrex::Real p, bool left) const
    {
        amrex::Real const rho = left ? rho_l : rho_r;
        amrex::Real const p_k = left ? p_l : p_r;
        amrex::Real const c_k = left ? c_l : c_r;
        if (p > p_k) {
            amrex::Real const a = amrex::Real(2.0) / ((gamma + amrex::Real(1.0)) * rho);
            amrex::Real const b = (gamma - amrex::Real(1.0)) / (gamma + amrex::Real(1.0)) * p_k;
            return std::sqrt(a / (p + b))
                 * (amrex::Real(1.0) - amrex::Real(0.5) * (p - p_k) / (p + b));
        }
        amrex::Real const expo = (gamma - amrex::Real(1.0)) / (amrex::Real(2.0) * gamma);
        return std::pow(p / p_k, -expo - amrex::Real(1.0)) / (rho * c_k);
    }

    /** f(p) = f_L(p) + f_R(p), monotone increasing; bisection keeps the Newton step honest. */
    void solveStarState ()
    {
        amrex::Real lo = 1.e-12;
        amrex::Real hi = amrex::Real(2.0) * std::max(p_l, p_r) + 1.e3;
        auto const f = [this] (amrex::Real p) {
            return pressureFunction(p, true) + pressureFunction(p, false);
        };
        while (f(hi) < 0.0) { hi *= amrex::Real(2.0); }
        amrex::Real p = amrex::Real(0.5) * (p_l + p_r);   // initial guess
        for (int it = 0; it < 200; ++it)
        {
            amrex::Real const value = f(p);
            if (std::abs(value) < 1.e-15 * std::max(p_l, p_r)) { break; }
            if (value < 0.0) { lo = p; } else { hi = p; }
            amrex::Real const deriv = pressureFunctionDerivative(p, true)
                                    + pressureFunctionDerivative(p, false);
            amrex::Real next = p - value / deriv;
            if (!(next > lo && next < hi)) { next = amrex::Real(0.5) * (lo + hi); }
            p = next;
        }
        p_star = p;
        u_star = amrex::Real(0.5) * (u_l + u_r)
               + amrex::Real(0.5) * (pressureFunction(p_star, false)
                                     - pressureFunction(p_star, true));
    }

    /** Star-state density on one side (Toro 4.55-4.57). */
    amrex::Real starDensity (bool left) const
    {
        amrex::Real const rho = left ? rho_l : rho_r;
        amrex::Real const p_k = left ? p_l : p_r;
        if (p_star > p_k) {
            amrex::Real const r = p_star / p_k;
            amrex::Real const gm = (gamma - amrex::Real(1.0)) / (gamma + amrex::Real(1.0));
            return rho * (r + gm) / (gm * r + amrex::Real(1.0));
        }
        return rho * std::pow(p_star / p_k, amrex::Real(1.0) / gamma);
    }

    struct Sample { amrex::Real rho, u, p; };

    /** Solution at the similarity coordinate S = x/t (Toro 4.63-4.68 plus the fan 4.56). */
    Sample sample (amrex::Real S) const
    {
        if (S <= u_star) {
            // Left side: shock or rarefaction, both compared against the wave speeds.
            if (p_star > p_l) {
                amrex::Real const s_shock = u_l - c_l * std::sqrt(
                    (gamma + amrex::Real(1.0)) / (amrex::Real(2.0) * gamma) * p_star / p_l
                    + (gamma - amrex::Real(1.0)) / (amrex::Real(2.0) * gamma));
                if (S < s_shock) { return {rho_l, u_l, p_l}; }
                return {starDensity(true), u_star, p_star};
            }
            amrex::Real const expo = (gamma - amrex::Real(1.0)) / (amrex::Real(2.0) * gamma);
            amrex::Real const head = u_l - c_l;
            amrex::Real const tail = u_star - c_l * std::pow(p_star / p_l, expo);
            if (S < head) { return {rho_l, u_l, p_l}; }
            if (S > tail) { return {starDensity(true), u_star, p_star}; }
            amrex::Real const fac = amrex::Real(2.0) / (gamma + amrex::Real(1.0))
                + (gamma - amrex::Real(1.0)) / ((gamma + amrex::Real(1.0)) * c_l) * (u_l - S);
            return {rho_l * std::pow(fac, amrex::Real(2.0) / (gamma - amrex::Real(1.0))),
                    amrex::Real(2.0) / (gamma + amrex::Real(1.0))
                        * (c_l + amrex::Real(0.5) * (gamma - amrex::Real(1.0)) * u_l + S),
                    p_l * std::pow(fac, amrex::Real(2.0) * gamma / (gamma - amrex::Real(1.0)))};
        }
        // Right side.
        if (p_star > p_r) {
            amrex::Real const s_shock = u_r + c_r * std::sqrt(
                (gamma + amrex::Real(1.0)) / (amrex::Real(2.0) * gamma) * p_star / p_r
                + (gamma - amrex::Real(1.0)) / (amrex::Real(2.0) * gamma));
            if (S > s_shock) { return {rho_r, u_r, p_r}; }
            return {starDensity(false), u_star, p_star};
        }
        amrex::Real const expo = (gamma - amrex::Real(1.0)) / (amrex::Real(2.0) * gamma);
        amrex::Real const head = u_r + c_r;
        amrex::Real const tail = u_star + c_r * std::pow(p_star / p_r, expo);
        if (S > head) { return {rho_r, u_r, p_r}; }
        if (S < tail) { return {starDensity(false), u_star, p_star}; }
        amrex::Real const fac = amrex::Real(2.0) / (gamma + amrex::Real(1.0))
            + (gamma - amrex::Real(1.0)) / ((gamma + amrex::Real(1.0)) * c_r) * (S - u_r);
        return {rho_r * std::pow(fac, amrex::Real(2.0) / (gamma - amrex::Real(1.0))),
                amrex::Real(2.0) / (gamma + amrex::Real(1.0))
                    * (-c_r + amrex::Real(0.5) * (gamma - amrex::Real(1.0)) * u_r + S),
                p_r * std::pow(fac, amrex::Real(2.0) * gamma / (gamma - amrex::Real(1.0)))};
    }
};

/** A one-dimensional domain inside whatever AMREX_SPACEDIM the build uses: n cells along x and
 *  a single cell in the transverse directions, with cell size dx everywhere.  The transverse
 *  directions are periodic, which is exact for a state that does not vary along them. */
inline amrex::Geometry makeSlab (amrex::Real x_lo, amrex::Real x_hi, int n_x,
                                 amrex::BoxArray& ba, amrex::DistributionMapping& dm,
                                 bool periodic_x)
{
    using namespace amrex;
    Real const dx = (x_hi - x_lo) / Real(n_x);
    IntVect const lo(0);
    IntVect const hi(AMREX_D_DECL(n_x - 1, 0, 0));
    ba.define(Box(lo, hi));
    dm.define(ba);
    RealBox rb({AMREX_D_DECL(x_lo, Real(0.0), Real(0.0))},
               {AMREX_D_DECL(x_hi, dx, dx)});
    Array<int, AMREX_SPACEDIM> is_periodic{AMREX_D_DECL(periodic_x ? 1 : 0, 1, 1)};
    return Geometry(Box(lo, hi), rb, CoordSys::cartesian, is_periodic);
}

/** Copy the nearest interior cell outward in x: a transmissive (outflow) boundary. */
inline void fillTransmissive (amrex::MultiFab& state, int n_ghost)
{
    using namespace amrex;
    for (MFIter mfi(state); mfi.isValid(); ++mfi)
    {
        Box const& bx = mfi.validbox();
        auto s = state.array(mfi);
        int const ncomp = state.nComp();
        int const lo = bx.smallEnd(0);
        int const hi = bx.bigEnd(0);
        Box const ghost_lo(IntVect(AMREX_D_DECL(lo - n_ghost, 0, 0)),
                           IntVect(AMREX_D_DECL(lo - 1, 0, 0)));
        Box const ghost_hi(IntVect(AMREX_D_DECL(hi + 1, 0, 0)),
                           IntVect(AMREX_D_DECL(hi + n_ghost, 0, 0)));
        ParallelFor(ghost_lo, [=] AMREX_GPU_DEVICE (int i, int j, int k) noexcept {
            amrex::ignore_unused(j, k);
            for (int n = 0; n < ncomp; ++n) { s(i, 0, 0, n) = s(lo, 0, 0, n); }
        });
        ParallelFor(ghost_hi, [=] AMREX_GPU_DEVICE (int i, int j, int k) noexcept {
            amrex::ignore_unused(j, k);
            for (int n = 0; n < ncomp; ++n) { s(i, 0, 0, n) = s(hi, 0, 0, n); }
        });
    }
}

/** Observed convergence order between two grids. */
inline amrex::Real convergenceOrder (amrex::Real e_coarse, amrex::Real e_fine, amrex::Real ratio)
{
    if (e_fine <= 0.0) { return 0.0; }
    return std::log(e_coarse / e_fine) / std::log(ratio);
}

} // namespace warpx::plugins::fluid::test

#endif // WARPX_FLUIDTESTREFERENCE_H_
