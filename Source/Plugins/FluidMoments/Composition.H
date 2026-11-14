/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
/* Composition: mixing by diffusion, and settling of one species relative to the rest.
 *
 * Convection is already conservative - the species densities ride the HLLC flux with the fluid - so
 * what a stellar model still needs is the motion *relative* to the flow:
 *
 *  1. mixing.  Turbulent or radiative-region mixing is modelled as a diffusion of the species mass
 *     fraction, written in conservative form
 *
 *         d(rho_s)/dt = div(rho D grad X_s),      X_s = rho_s / rho,
 *
 *     so the total mass of every species is conserved when D is uniform and only the contrast
 *     between species is destroyed.  With a single D the sum of the species fluxes is
 *     div(rho D grad sum X_s) = div(rho D grad 1) = 0, i.e. the composition stays normalised by
 *     construction rather than by renormalising afterwards.
 *
 *  2. settling.  A species can drift relative to the bulk, which is first order in the drift
 *     velocity:
 *
 *         d(rho_s)/dt = -div(rho_s v_s),
 *
 *     again in flux form, so a uniform drift just translates the profile and conserves the total.
 *
 * Both are explicit with a sub-step limited by 0.4 dx^2/(2 D) (mixing) or the advective CFL
 * (settling) and updated out of place, which is the lesson from the radiation bug: an in-place
 * stencil update reads neighbours that have already moved.
 *
 * Units are CGS.
 *
 * Specification: PHYSICS_SPEC.md section 8 (composition).
 */
#ifndef WARPX_PLUGINS_FLUID_COMPOSITION_H_
#define WARPX_PLUGINS_FLUID_COMPOSITION_H_

#include "FluidMoments.H"

#include <AMReX_Geometry.H>
#include <AMReX_MultiFab.H>

#include <cmath>

namespace warpx::plugins::fluid
{

/** Diffuse the species fractions with a uniform diffusivity D [cm^2/s].
 *
 *  \return the number of sub-steps taken
 */
inline int diffuseSpecies (amrex::MultiFab& state, amrex::Geometry const& geom, amrex::Real D,
                           amrex::Real dt, StateLayout const& layout, int max_sub = 1000)
{
    using namespace amrex;
    if (D <= 0.0) { return 0; }
    amrex::Real const dx = geom.CellSize(0);
    amrex::Real const dt_stable = Real(0.4) * dx * dx / (Real(2.0) * D);

    int taken = 0;
    amrex::Real remaining = dt;
    while (remaining > Real(0.0)) {
        amrex::Real const h = amrex::min(remaining, dt_stable);
        state.FillBoundary(geom.periodicity());
        // out of place: the stencil reads neighbours, and an in-place update is a different operator
        amrex::MultiFab state_new(state.boxArray(), state.DistributionMap(), state.nComp(), 0);
        for (MFIter mfi(state); mfi.isValid(); ++mfi) {
            Box const& bx = mfi.validbox();
            auto const s = state.const_array(mfi);
            auto s_new = state_new.array(mfi);
            for (int i = bx.smallEnd(0); i <= bx.bigEnd(0); ++i) {
                for (int j = bx.smallEnd(1); j <= bx.bigEnd(1); ++j) {
                    for (int k = bx.smallEnd(2); k <= bx.bigEnd(2); ++k) {
                        for (int c = 0; c < state.nComp(); ++c) { s_new(i, j, k, c) = s(i, j, k, c); }
                        amrex::Real rho = Real(0.0);
                        for (int a = 0; a < layout.n_species; ++a) {
                            rho += s(i, j, k, layout.density(a));
                        }
                        if (rho <= Real(0.0)) { continue; }
                        for (int a = 0; a < layout.n_species; ++a) {
                            amrex::Real div_flux = Real(0.0);
                            for (int d = 0; d < AMREX_SPACEDIM; ++d) {
                                int off[3] = {0, 0, 0};
                                off[d] = 1;
                                amrex::Real rho_p = Real(0.0), rho_m = Real(0.0);
                                for (int b = 0; b < layout.n_species; ++b) {
                                    rho_p += s(i + off[0], j + off[1], k + off[2], layout.density(b));
                                    rho_m += s(i - off[0], j - off[1], k - off[2], layout.density(b));
                                }
                                amrex::Real const Xp = (rho_p > Real(0.0))
                                    ? s(i + off[0], j + off[1], k + off[2], layout.density(a)) / rho_p
                                    : Real(0.0);
                                amrex::Real const Xm = (rho_m > Real(0.0))
                                    ? s(i - off[0], j - off[1], k - off[2], layout.density(a)) / rho_m
                                    : Real(0.0);
                                amrex::Real const X = s(i, j, k, layout.density(a)) / rho;
                                amrex::Real const fp = Real(0.5) * D * (rho + rho_p) * (Xp - X);
                                amrex::Real const fm = Real(0.5) * D * (rho + rho_m) * (X - Xm);
                                div_flux += (fp - fm) / (dx * dx);
                            }
                            s_new(i, j, k, layout.density(a)) = amrex::max(
                                s(i, j, k, layout.density(a)) + h * div_flux, Real(0.0));
                        }
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

/** Settle species `a` relative to the bulk with drift velocity v [cm/s] along `dir`.
 *
 *  \return the number of sub-steps taken
 */
inline int settleSpecies (amrex::MultiFab& state, amrex::Geometry const& geom, amrex::Real v,
                          int dir, amrex::Real dt, StateLayout const& layout, int species,
                          int max_sub = 1000)
{
    using namespace amrex;
    amrex::Real const dx = geom.CellSize(dir);
    amrex::Real const dt_stable = Real(0.4) * dx / amrex::max(std::abs(v), Real(1.0e-30));

    int taken = 0;
    amrex::Real remaining = dt;
    while (remaining > Real(0.0)) {
        amrex::Real const h = amrex::min(remaining, dt_stable);
        state.FillBoundary(geom.periodicity());
        amrex::MultiFab state_new(state.boxArray(), state.DistributionMap(), state.nComp(), 0);
        for (MFIter mfi(state); mfi.isValid(); ++mfi) {
            Box const& bx = mfi.validbox();
            auto const s = state.const_array(mfi);
            auto s_new = state_new.array(mfi);
            for (int i = bx.smallEnd(0); i <= bx.bigEnd(0); ++i) {
                for (int j = bx.smallEnd(1); j <= bx.bigEnd(1); ++j) {
                    for (int k = bx.smallEnd(2); k <= bx.bigEnd(2); ++k) {
                        for (int c = 0; c < state.nComp(); ++c) { s_new(i, j, k, c) = s(i, j, k, c); }
                        int off[3] = {0, 0, 0};
                        off[dir] = 1;
                        // upwind flux at the two faces: the drift velocity is constant and positive
                        amrex::Real const rho_c = s(i, j, k, layout.density(species));
                        amrex::Real const rho_m = s(i - off[0], j - off[1], k - off[2],
                                                    layout.density(species));
                        amrex::Real const flux_lo = v * rho_m;   // inflow from below
                        amrex::Real const flux_hi = v * rho_c;   // outflow above
                        s_new(i, j, k, layout.density(species)) =
                            amrex::max(rho_c + h * (flux_lo - flux_hi) / dx, Real(0.0));
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

#endif // WARPX_PLUGINS_FLUID_COMPOSITION_H_
