/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
/* Spherical radial hydro: the same HLLC physics, with the r^2 geometry.
 *
 * A stellar core cannot be represented by a Cartesian slab: over the solar radius the pressure scale
 * height changes by two orders of magnitude and the mass inside a radius grows as r^3, so the
 * geometric terms are the dominant part of the momentum balance.  This header adds a genuinely
 * spherical, finite-volume radial update on a uniform dr grid:
 *
 *     d/dt (rho V)     = -(A F_rho)_hi + (A F_rho)_lo
 *     d/dt (rho u V)   = -(A (rho u^2 + p))_hi + (A (rho u^2 + p))_lo + p_c (A_hi - A_lo) + rho g V
 *     d/dt (E V)       = -(A u (E + p))_hi + (A u (E + p))_lo + rho g u V
 *
 * with A = 4 pi r^2 and V = 4 pi (r_hi^3 - r_lo^3)/3.  The p_c (A_hi - A_lo) term is exactly the
 * geometric part of int (dp/dr) dV = [A p] - int p dA; without it a hydrostatic star has a spurious
 * force of order (p / (rho g r)) which is O(1) in the core, not a correction.
 *
 * The Riemann solver is HLLC with Davis wave speeds, first order in space (the core is smooth, and a
 * first-order scheme can be checked against an exact hydrostatic solution, which is what the test
 * does).  Gravity is supplied by the caller.
 *
 * Units are CGS.
 */
#ifndef WARPX_PLUGINS_FLUID_SPHERICAL_H_
#define WARPX_PLUGINS_FLUID_SPHERICAL_H_

#include "FluidMoments.H"

#include <AMReX_MultiFab.H>

#include <cmath>

namespace warpx::plugins::fluid
{

namespace spherical
{
AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE
amrex::Real area (amrex::Real r) noexcept { return amrex::Real(4.0) * M_PI * r * r; }

AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE
amrex::Real volume (amrex::Real r_lo, amrex::Real r_hi) noexcept
{
    return amrex::Real(4.0) * M_PI / amrex::Real(3.0) * (r_hi * r_hi * r_hi - r_lo * r_lo * r_lo);
}

/** HLLC flux for the radial direction; U = (rho, rho u, E) per unit volume. */
template <class EOS>
AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE
void hllc_radial (amrex::Real const UL[3], amrex::Real const UR[3], amrex::Real gamma,
                  EOS const& eos, amrex::Real F[3]) noexcept
{
    amrex::Real const rhoL = UL[0], rhoR = UR[0];
    amrex::Real const uL = (rhoL > 0.0) ? UL[1] / rhoL : 0.0;
    amrex::Real const uR = (rhoR > 0.0) ? UR[1] / rhoR : 0.0;
    amrex::Real const pL = eos.pressure(rhoL, UL[2] - 0.5 * UL[1] * uL);
    amrex::Real const pR = eos.pressure(rhoR, UR[2] - 0.5 * UR[1] * uR);
    amrex::Real const cL = eos.soundSpeed(rhoL, pL);
    amrex::Real const cR = eos.soundSpeed(rhoR, pR);
    amrex::Real const SL = amrex::min(uL - cL, uR - cR);
    amrex::Real const SR = amrex::max(uL + cL, uR + cR);
    amrex::Real FL[3] = {rhoL * uL, rhoL * uL * uL + pL, uL * (UL[2] + pL)};
    amrex::Real FR[3] = {rhoR * uR, rhoR * uR * uR + pR, uR * (UR[2] + pR)};
    if (SL >= 0.0) { F[0] = FL[0]; F[1] = FL[1]; F[2] = FL[2]; return; }
    if (SR <= 0.0) { F[0] = FR[0]; F[1] = FR[1]; F[2] = FR[2]; return; }
    amrex::Real const SM = (pR - pL + rhoL * uL * (SL - uL) - rhoR * uR * (SR - uR))
                         / (rhoL * (SL - uL) - rhoR * (SR - uR));
    bool const left = (SM >= 0.0);
    amrex::Real const rho = left ? rhoL : rhoR;
    amrex::Real const u = left ? uL : uR;
    amrex::Real const p = left ? pL : pR;
    amrex::Real const E = left ? UL[2] : UR[2];
    amrex::Real const S = left ? SL : SR;
    amrex::Real const* U = left ? UL : UR;
    amrex::Real const* FK = left ? FL : FR;
    amrex::Real const factor = rho * (S - u) / (S - SM);
    amrex::Real const Ustar[3] = {
        U[0] * (S - u) / (S - SM),
        factor * SM,
        factor * (E / rho + (SM - u) * (SM + p / (rho * (S - u))))};
    for (int n = 0; n < 3; ++n) { F[n] = FK[n] + S * (Ustar[n] - U[n]); }
    amrex::ignore_unused(gamma);
}
} // namespace spherical

/** One first-order spherical radial update.  Gravity is read from `gr` (per cell, negative inward). */
template <class EOS>
inline void advanceSphericalRadial (amrex::MultiFab& state, amrex::MultiFab const& gr,
                                    amrex::Real r_lo, amrex::Real dr, amrex::Real dt,
                                    EOS const& eos, StateLayout const& layout, int species = 0)
{
    using namespace amrex;
    AMREX_ALWAYS_ASSERT_WITH_MESSAGE(layout.n_species >= species + 1,
        "advanceSphericalRadial: species out of range");

    // the cell state is (rho_s, rho u, E) with u radial
    auto const cell = [&](Array4<Real const> const& s, int i, Real U[3]) {
        U[0] = s(i, 0, 0, layout.density(species));
        U[1] = s(i, 0, 0, layout.momentum(0));
        U[2] = s(i, 0, 0, layout.energy());
    };

    amrex::MultiFab state_new(state.boxArray(), state.DistributionMap(), state.nComp(), 0);
    for (MFIter mfi(state); mfi.isValid(); ++mfi) {
        Box const& bx = mfi.validbox();
        auto const s = state.const_array(mfi);
        auto const g = gr.const_array(mfi);
        auto s_new = state_new.array(mfi);
        int const lo = bx.smallEnd(0);
        int const hi = bx.bigEnd(0);
        for (int i = lo; i <= hi; ++i) {
            for (int c = 0; c < state.nComp(); ++c) { s_new(i, 0, 0, c) = s(i, 0, 0, c); }
            Real const r_c = r_lo + (Real(i) + Real(0.5)) * dr;
            Real const r_l = r_c - Real(0.5) * dr;
            Real const r_h = r_c + Real(0.5) * dr;
            Real const A_l = spherical::area(r_l);
            Real const A_h = spherical::area(r_h);
            Real const V = spherical::volume(r_l, r_h);

            // face states: first order (the cell it comes from), zero-gradient at the grid ends
            Real UL[3], UC[3], UR[3];
            cell(s, amrex::max(i - 1, lo), UL);
            cell(s, i, UC);
            cell(s, amrex::min(i + 1, hi), UR);
            Real F_lo[3], F_hi[3];
            spherical::hllc_radial< EOS >(UL, UC, Real(1.0), eos, F_lo);
            spherical::hllc_radial< EOS >(UC, UR, Real(1.0), eos, F_hi);

            Real const u_c = (UC[0] > 0.0) ? UC[1] / UC[0] : 0.0;
            Real const p_c = eos.pressure(UC[0], UC[2] - Real(0.5) * UC[0] * u_c * u_c);
            Real const g_c = g(i, 0, 0);

            Real const drho = -(A_h * F_hi[0] - A_l * F_lo[0]) * dt / V;
            Real const dmom = (-(A_h * F_hi[1] - A_l * F_lo[1]) + p_c * (A_h - A_l)) * dt / V
                            + UC[0] * g_c * dt;
            Real const dE = -(A_h * F_hi[2] - A_l * F_lo[2]) * dt / V + UC[0] * g_c * u_c * dt;

            s_new(i, 0, 0, layout.density(species)) = amrex::max(UC[0] + drho, Real(0.0));
            for (int d = 1; d < layout.n_species; ++d) {
                s_new(i, 0, 0, layout.density(d)) = s(i, 0, 0, layout.density(d));
            }
            s_new(i, 0, 0, layout.momentum(0)) = UC[1] + dmom;
            for (int d = 1; d < AMREX_SPACEDIM; ++d) {
                s_new(i, 0, 0, layout.momentum(d)) = s(i, 0, 0, layout.momentum(d));
            }
            s_new(i, 0, 0, layout.energy()) = amrex::max(UC[2] + dE, Real(0.0));
        }
    }
    state.ParallelCopy(state_new);
}

} // namespace warpx::plugins::fluid

#endif // WARPX_PLUGINS_FLUID_SPHERICAL_H_
