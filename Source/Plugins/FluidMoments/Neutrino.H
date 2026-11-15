/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
/* Neutrino transport: grey flux-limited diffusion, plus escape in the optically thin limit.
 *
 * Neutrinos leave a star's core, and how they leave matters for the energy budget: where the medium
 * is optically thick they diffuse (the transport is the same grey FLD operator the photons use, with
 * the neutrino opacity instead of the photon one), and where it is thin they simply stream out at the
 * speed of light.  Both limits are needed, and they are different operators:
 *
 *     thick:  dE_nu/dt = div( D grad E_nu ),   D = c lambda / (kappa rho)
 *     thin:   dE_nu/dt = -c kappa rho E_nu    (each neutrino escapes, none comes back)
 *
 * The first is the operator in Radiation.H, generalised only in its opacity; reusing it here is
 * deliberate rather than accidental duplication - the numerics that were validated there (out of
 * place update, sub-cycling, flux limiter) are exactly what the neutrino field needs.  The second is
 * a local sink, advanced exactly with an exponential so that any dt is stable and the energy is
 * never driven negative.
 *
 * The network (SolarNetwork.H) already reports the energy it puts into neutrinos; that source feeds
 * the field this header transports, and the ledger in StellarSources.H books both.
 *
 * Units are CGS.
 *
 * Specification: PHYSICS_SPEC.md section 9 (neutrinos).
 */
#ifndef WARPX_PLUGINS_FLUID_NEUTRINO_H_
#define WARPX_PLUGINS_FLUID_NEUTRINO_H_

#include "FluidMoments.H"
#include "Radiation.H"

#include <AMReX_Geometry.H>
#include <AMReX_MultiFab.H>

#include <cmath>

using warpx::plugins::fluid::star_box_hi;
using warpx::plugins::fluid::star_box_lo;

namespace warpx::plugins::fluid
{

namespace neutrino
{
/** Speed of light, used as the free-streaming speed [cm/s]. */
constexpr amrex::Real c_light = radiation::c_light;

/** Effective diffusion coefficient in the optically thick limit [cm^2/s]. */
AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE
amrex::Real diffusion_coefficient (amrex::Real kappa, amrex::Real rho) noexcept
{
    if (kappa <= amrex::Real(0.0) || rho <= amrex::Real(0.0)) { return amrex::Real(0.0); }
    return c_light / (amrex::Real(3.0) * kappa * rho);
}

/** Fraction of the neutrino energy that survives a free-streaming step of length dt. */
AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE
amrex::Real escape_factor (amrex::Real kappa, amrex::Real rho, amrex::Real dt) noexcept
{
    if (kappa <= amrex::Real(0.0) || rho <= amrex::Real(0.0) || dt <= amrex::Real(0.0)) {
        return amrex::Real(1.0);
    }
    return std::exp(-c_light * kappa * rho * dt);
}
} // namespace neutrino

/** Diffuse the neutrino energy density (optically thick limit).
 *
 *  The operator is the grey FLD one from Radiation.H; only the opacity is neutrino-specific.
 *  \return the number of sub-steps taken
 */
inline int advanceNeutrinoDiffusion (amrex::MultiFab& nu, amrex::MultiFab const& rho,
                                     amrex::Real kappa, amrex::Geometry const& geom,
                                     amrex::Real dt, int max_sub = 100000)
{
    return advanceRadiationDiffusion(nu, rho, kappa, geom, dt, max_sub);
}

/** Let the neutrino energy stream out where the medium is thin (local exponential sink). */
inline void applyNeutrinoEscape (amrex::MultiFab& nu, amrex::MultiFab const& rho,
                                 amrex::Real kappa, amrex::Real dt)
{
    using namespace amrex;
    for (MFIter mfi(nu); mfi.isValid(); ++mfi) {
        Box const& bx = mfi.validbox();
        auto a = nu.array(mfi);
        auto const r = rho.const_array(mfi);
        for (int i = bx.smallEnd(0); i <= bx.bigEnd(0); ++i) {
            for (int j = star_box_lo(bx, 1); j <= star_box_hi(bx, 1); ++j) {
                for (int k = star_box_lo(bx, 2); k <= star_box_hi(bx, 2); ++k) {
                    a(i, j, k) *= neutrino::escape_factor(kappa, r(i, j, k, 0), dt);
                }
            }
        }
    }
}

} // namespace warpx::plugins::fluid

#endif // WARPX_PLUGINS_FLUID_NEUTRINO_H_
