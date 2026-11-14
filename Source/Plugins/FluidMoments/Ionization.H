/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
/* Hydrogen ionisation: Saha equilibrium and its non-equilibrium relaxation.
 *
 * The ionisation fraction is a *density* (rho * x_e) carried as one of the passive components, so
 * it is advected conservatively by the same HLLC flux as a species density (see StateLayout).  What
 * this header adds is the local chemistry that changes it in place:
 *
 *   dn_e/dt = a (n - n_e) - b n_e^2,   a = S(T)/n,  b = 1,  (per unit volume, pure hydrogen)
 *
 * with S(T) the Saha function.  Discretised with backward Euler because the recombination term is
 * quadratic: the scheme then reduces to a quadratic whose positive root exists for every dt, is
 * monotone in x, stays inside [0, 1], and tends to the Saha value as dt grows - all three are
 * checked in test_ionization.cpp.  Note that this is exactly the behaviour wanted for a stellar
 * fluid, where the ionisation time scale can be far shorter than the hydrodynamic one.
 *
 * Energy: ionising one atom costs chi_H and recombining one releases it, so the update moves
 * chi_H * (n_e_new - n_e_old) between the ionisation potential and the gas internal energy.  The
 * sum is conserved to round-off by construction, which the test measures.
 *
 * Units are CGS (erg, g, cm, K) like the rest of the fluid layer.
 *
 * Specification: PHYSICS_SPEC.md section 3 (ionisation).
 */
#ifndef WARPX_PLUGINS_FLUID_IONIZATION_H_
#define WARPX_PLUGINS_FLUID_IONIZATION_H_

#include "FluidMoments.H"

#include <AMReX_MultiFab.H>

#include <cmath>

namespace warpx::plugins::fluid
{

namespace ionization
{
constexpr amrex::Real chi_h = 2.179e-11;        //!< hydrogen ionisation potential [erg]
constexpr amrex::Real k_boltzmann = 1.380649e-16;
constexpr amrex::Real m_proton = 1.67262192e-24;
constexpr amrex::Real m_electron = 9.1093837e-28;
constexpr amrex::Real h_planck = 6.62607015e-27;

/** Saha function S(T) = 2 (2 pi m_e k T / h^2)^{3/2} exp(-chi/kT)  [cm^-3]. */
AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE
amrex::Real saha_function (amrex::Real T) noexcept
{
    if (T <= amrex::Real(0.0)) { return amrex::Real(0.0); }
    amrex::Real const lam = amrex::Real(2.0) * M_PI * m_electron * k_boltzmann * T
                          / (h_planck * h_planck);
    return amrex::Real(2.0) * lam * std::sqrt(lam) * std::exp(-chi_h / (k_boltzmann * T));
}

/** Saha equilibrium fraction for pure hydrogen at total nuclei density n [cm^-3].
 *  x^2/(1-x) = S/n  solved in closed form (the positive root). */
AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE
amrex::Real saha_fraction (amrex::Real n, amrex::Real T) noexcept
{
    if (n <= amrex::Real(0.0)) { return amrex::Real(0.0); }
    amrex::Real const s = saha_function(T) / n;
    if (s <= amrex::Real(0.0)) { return amrex::Real(0.0); }
    // x = (-s + sqrt(s^2 + 4 s)) / 2, written to stay accurate for both small and large s
    return amrex::Real(2.0) * s / (s + std::sqrt(s * s + amrex::Real(4.0) * s));
}

/** Backward-Euler step of dx/dt = a(1-x) - b x^2, returning x_new in [0, 1]. */
AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE
amrex::Real relax_fraction (amrex::Real x_old, amrex::Real a, amrex::Real b, amrex::Real dt) noexcept
{
    if (dt <= amrex::Real(0.0)) { return x_old; }
    // b x^2 + (1/dt + a) x - (a + x_old/dt) = 0, take the positive root
    amrex::Real const A = b;
    amrex::Real const B = amrex::Real(1.0) / dt + a;
    amrex::Real const C = -(a + x_old / dt);
    amrex::Real x = x_old;
    if (A > amrex::Real(0.0)) {
        amrex::Real const disc = B * B - amrex::Real(4.0) * A * C;
        x = (disc >= amrex::Real(0.0))
          ? amrex::Real(2.0) * (-C) / (B + std::sqrt(disc))     // stable form for C < 0
          : x_old;
    } else {
        x = -C / B;
    }
    return amrex::min(amrex::max(x, amrex::Real(0.0)), amrex::Real(1.0));
}

/** Temperature of a pure-hydrogen gas from its pressure, density and ionisation fraction. */
AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE
amrex::Real temperature (amrex::Real rho, amrex::Real p, amrex::Real x) noexcept
{
    amrex::Real const n = rho / m_proton * (amrex::Real(1.0) + x);   // ions + electrons
    if (n <= amrex::Real(0.0)) { return amrex::Real(0.0); }
    return p / (n * k_boltzmann);
}
} // namespace ionization

/** One ionisation/relaxation update of the state.
 *
 *  \param state       conserved state, extras: x_comp holds rho * x_e
 *  \param dt          time step [s]
 *  \param gamma       adiabatic index of the gas
 *  \param layout      component layout (n_extra must be >= x_comp + 1)
 *  \param x_comp      index of the ionisation extra (0-based among the extras)
 */
inline void advanceIonization (amrex::MultiFab& state, amrex::Real dt, amrex::Real gamma,
                               StateLayout const& layout, int x_comp)
{
    using namespace amrex;
    AMREX_ALWAYS_ASSERT_WITH_MESSAGE(layout.n_extra > x_comp,
        "advanceIonization: the state has no component for the ionisation fraction");
    amrex::Real const cH = ionization::chi_h;
    amrex::Real const mp = ionization::m_proton;

    for (MFIter mfi(state); mfi.isValid(); ++mfi) {
        Box const& bx = mfi.validbox();
        auto s = state.array(mfi);
        for (int i = bx.smallEnd(0); i <= bx.bigEnd(0); ++i) {
            for (int j = bx.smallEnd(1); j <= bx.bigEnd(1); ++j) {
                for (int k = bx.smallEnd(2); k <= bx.bigEnd(2); ++k) {
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
                    amrex::Real const energy = s(i, j, k, layout.energy());
                    amrex::Real const p = (gamma - Real(1.0)) * (energy - Real(0.5) * mom2 / rho);

                    amrex::Real const x_old = s(i, j, k, layout.extra(x_comp)) / rho;
                    amrex::Real const T = ionization::temperature(rho, p, x_old);
                    amrex::Real const n = rho / mp;
                    amrex::Real const a = ionization::saha_function(T) / n;   // per particle rate
                    amrex::Real const b = Real(1.0);                          // quadratic coefficient

                    amrex::Real x_new = ionization::relax_fraction(x_old, a, b, dt);
                    x_new = amrex::min(amrex::max(x_new, Real(0.0)), Real(1.0));

                    // Ionising costs chi_H per atom, recombining releases it: move the difference
                    // between the gas internal energy and the ionisation potential.
                    amrex::Real const dn = (x_new - x_old) * n;
                    s(i, j, k, layout.energy()) -= cH * dn;
                    s(i, j, k, layout.extra(x_comp)) = x_new * rho;
                }
            }
        }
    }
}

} // namespace warpx::plugins::fluid

#endif // WARPX_PLUGINS_FLUID_IONIZATION_H_
