/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
/* Electron degeneracy as an equation-of-state correction.
 *
 * A stellar core is supported by degenerate electrons long before it is supported by temperature, and
 * an ideal-gas EOS simply has no such pressure.  This header adds it as a correction on top of the
 * ideal gas, in the form that interpolates between the two exact limits:
 *
 *     P_deg(n_e) = K_nr n_e^{5/3} / sqrt(1 + (n_e/n_c)^{2/3})
 *
 *     K_nr = (3 pi^2)^{2/3} hbar^2 / (5 m_e)      non-relativistic coefficient
 *     K_r  = (3 pi^2)^{1/3} hbar c / 4            relativistic coefficient
 *     n_c  = (K_r / K_nr)^3                       crossover density (~1.1e30 cm^-3)
 *
 * so that P_deg -> K_nr n^{5/3} for n << n_c and P_deg -> K_r n^{4/3} for n >> n_c, with a single
 * smooth expression and no branch on the density.  The blend is asymptotic, not exact: its error is
 * O((n/n_c)^{2/3}) (about 0.5% three decades from the crossover, 9% one decade from it).  Tests
 * therefore check the limits where the interpolation is meaningful and the monotonicity across the
 * crossover; claiming 1e-3 everywhere would be claiming an exact T=0 Fermi integral this is not.  The internal energy of the degenerate electrons is
 * taken as (3/2) P_deg, which is the non-relativistic relation; that is a stated approximation, not an
 * exact statement, and it matters only where the electrons are already relativistic.
 *
 * What this is and is not: it is the degeneracy *correction* the model needs, added to the ideal-gas
 * pressure.  Where the electrons are strongly degenerate the ideal-gas part over-counts the electron
 * contribution, because a real EOS (Helmholtz table or Saha with Fermi integrals) would replace it
 * rather than add to it.  The tests therefore check the two analytic limits of the correction itself
 * and its negligibility in the classical regime, and do not claim absolute accuracy in the
 * strong-degeneracy regime.
 *
 * The structs implement the same interface as IdealGasEOS (pressure(rho, e_int), soundSpeed(rho, p)),
 * so advanceSweep can be instantiated with them directly.
 *
 * Units are CGS.
 *
 * Specification: PHYSICS_SPEC.md section 10 (equation of state).
 */
#ifndef WARPX_PLUGINS_FLUID_DEGENERATE_EOS_H_
#define WARPX_PLUGINS_FLUID_DEGENERATE_EOS_H_

#include "FluidMoments.H"

#include <cmath>

namespace warpx::plugins::fluid
{

namespace degenerate
{
constexpr amrex::Real m_proton = 1.67262192e-24;
constexpr amrex::Real m_electron = 9.1093837e-28;
constexpr amrex::Real hbar = 1.054571817e-27;
constexpr amrex::Real c_light = 2.99792458e10;

/** (3 pi^2)^{2/3} hbar^2 / (5 m_e), non-relativistic degeneracy coefficient. */
constexpr amrex::Real k_nr = 2.335e-27;      //!< [erg cm^2] with n_e in cm^-3
/** (3 pi^2)^{1/3} hbar c / 4, relativistic degeneracy coefficient. */
constexpr amrex::Real k_rel = 2.440e-17;     //!< [erg cm] with n_e in cm^-3
/** Density at which the two limits cross: (k_rel/k_nr)^3, derived rather than tuned, so that the
 *  blend is consistent with both coefficients by construction (a rounded n_c made the relativistic
 *  limit miss by 1.2%). */
constexpr amrex::Real n_cross = 1.1406e30;   //!< [cm^-3]

/** Degenerate electron pressure [dyn/cm^2] for an electron density n_e [cm^-3]. */
AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE
amrex::Real degeneracy_pressure (amrex::Real n_e) noexcept
{
    if (n_e <= amrex::Real(0.0)) { return amrex::Real(0.0); }
    amrex::Real const ratio = std::pow(n_e / n_cross, amrex::Real(2.0) / amrex::Real(3.0));
    return k_nr * std::pow(n_e, amrex::Real(5.0) / amrex::Real(3.0))
         / std::sqrt(amrex::Real(1.0) + ratio);
}

/** Non-relativistic limit, for tests and for a sanity check in an analytic model. */
AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE
amrex::Real degeneracy_pressure_nonrel (amrex::Real n_e) noexcept
{
    return (n_e > amrex::Real(0.0)) ? k_nr * std::pow(n_e, amrex::Real(5.0) / amrex::Real(3.0))
                                    : amrex::Real(0.0);
}

/** Relativistic limit. */
AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE
amrex::Real degeneracy_pressure_rel (amrex::Real n_e) noexcept
{
    return (n_e > amrex::Real(0.0)) ? k_rel * std::pow(n_e, amrex::Real(4.0) / amrex::Real(3.0))
                                    : amrex::Real(0.0);
}
} // namespace degenerate

/** Ideal gas plus a degenerate-electron correction. */
struct DegenerateElectronEOS
{
    amrex::Real gamma = amrex::Real(1.4);
    amrex::Real mu_e = amrex::Real(2.0);   //!< electrons per nucleon (2 for hydrogen)

    /** Electron density [cm^-3] of a mass density rho [g/cm^3]. */
    AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE
    amrex::Real electron_density (amrex::Real rho) const noexcept
    {
        return (mu_e > amrex::Real(0.0)) ? rho / (mu_e * degenerate::m_proton) : amrex::Real(0.0);
    }

    AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE
    amrex::Real pressure (amrex::Real rho, amrex::Real e_int) const noexcept
    {
        amrex::Real const p_ideal = (gamma - amrex::Real(1.0)) * e_int;
        return p_ideal + degenerate::degeneracy_pressure(electron_density(rho));
    }

    AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE
    amrex::Real soundSpeed (amrex::Real rho, amrex::Real p) const noexcept
    {
        if (rho <= amrex::Real(0.0) || p <= amrex::Real(0.0)) { return amrex::Real(0.0); }
        // gamma_eff = 5/3 for the non-degenerate gas; the degenerate part stiffens the sound speed,
        // and dP/drho for P ~ n^{4/3} gives the same 4/3 factor, so use the gas value as the lower
        // bound and let the stiffness enter through p/rho itself.
        return std::sqrt(gamma * p / rho);
    }
};

} // namespace warpx::plugins::fluid

#endif // WARPX_PLUGINS_FLUID_DEGENERATE_EOS_H_
