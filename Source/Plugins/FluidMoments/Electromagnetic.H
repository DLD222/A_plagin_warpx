/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
/* Electromagnetic coupling for the stellar fluid: Lorentz force and Joule heating.
 *
 * The fields themselves are NOT computed here and no field solver is added: E and B are read from the
 * existing electromagnetic plugin through the runtime bus (the ABI's warpx_stage_read_field_dir), and
 * this header turns them into the two things a fluid needs - a force and a heating rate - with a
 * resistive Ohm's law:
 *
 *     E' = E + u x B          (electric field in the fluid frame)
 *     J  = sigma E'           (Ohm's law, scalar conductivity)
 *     f  = J x B              (Lorentz force density)
 *     Q  = sigma |E'|^2       (Joule heating)
 *
 * Two properties follow from this form and are what the test checks, because both are easy to lose
 * with a sign error:
 *
 *   - Q >= 0 always.  Heating that can cool is the classic sign mistake, and the fluid-frame
 *     formulation makes the positivity a theorem (Q = sigma |E'|^2), not an accident of the data;
 *   - Q = 0 and f = 0 when E' = 0, i.e. a plasma moving *along* B with no electric field feels
 *     nothing.  That is the case a wrong u x B sign turns into a large fake force.
 *
 * The work done by the force is booked separately from the heating: the energy update is
 * (f . u + Q) dt, so the mechanical work and the irreversible heating are distinguishable in the
 * ledger.
 *
 * What this is not: the field is a prescribed source, not something the fluid pushes back on.  A
 * self-consistent treatment would need the current fed back into the field solver, which is the EM
 * plugin's job; here the coupling is one-way and that is stated rather than implied.
 *
 * Units are CGS.
 *
 * Specification: PHYSICS_SPEC.md section 11 (electromagnetic coupling).
 */
#ifndef WARPX_PLUGINS_FLUID_ELECTROMAGNETIC_H_
#define WARPX_PLUGINS_FLUID_ELECTROMAGNETIC_H_

#include "FluidMoments.H"

#include <AMReX_MultiFab.H>

#include <cmath>

using warpx::plugins::fluid::star_box_hi;
using warpx::plugins::fluid::star_box_lo;

namespace warpx::plugins::fluid
{

namespace electromagnetic
{
/** Cross product a x b. */
AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE
void cross (amrex::Real const a[3], amrex::Real const b[3], amrex::Real out[3]) noexcept
{
    out[0] = a[1] * b[2] - a[2] * b[1];
    out[1] = a[2] * b[0] - a[0] * b[2];
    out[2] = a[0] * b[1] - a[1] * b[0];
}

/** Joule heating rate sigma |E + u x B|^2 [erg cm^-3 s^-1]; never negative. */
AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE
amrex::Real joule_heating (amrex::Real const E[3], amrex::Real const B[3],
                           amrex::Real const u[3], amrex::Real sigma) noexcept
{
    amrex::Real uxb[3];
    cross(u, B, uxb);
    amrex::Real const e0 = E[0] + uxb[0];
    amrex::Real const e1 = E[1] + uxb[1];
    amrex::Real const e2 = E[2] + uxb[2];
    return amrex::max(sigma, amrex::Real(0.0)) * (e0 * e0 + e1 * e1 + e2 * e2);
}

/** Lorentz force density J x B / c with J = sigma (E + u x B) [dyn cm^-3].
 *
 *  Gaussian cgs: J = sigma E' is in statA/cm^2 and the force density is J x B / c, not J x B
 *  (the 1/c = 3.34e-11 s/cm is what turns statA G/cm^2 into dyn/cm^3).  Omitting it made the
 *  force 3e10 times too large, while the Joule heating sigma |E'|^2 is the same in both
 *  conventions, which is why the unit tests (they only check f.B = 0 and Q >= 0) did not see it.
 *  The state itself is cgs (erg/cm^3, g cm^-2 s^-1), so cgs is the convention to use here.
 */
constexpr amrex::Real c_light_cgs = 2.99792458e10;  // cm/s

AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE
void lorentz_force (amrex::Real const E[3], amrex::Real const B[3], amrex::Real const u[3],
                    amrex::Real sigma, amrex::Real f[3]) noexcept
{
    amrex::Real uxb[3];
    cross(u, B, uxb);
    amrex::Real const J[3] = {sigma * (E[0] + uxb[0]), sigma * (E[1] + uxb[1]),
                              sigma * (E[2] + uxb[2])};
    amrex::Real jxb[3];
    cross(J, B, jxb);
    for (int d = 0; d < 3; ++d) { f[d] = jxb[d] / c_light_cgs; }
}
} // namespace electromagnetic

/** Add the Lorentz force and the Joule heating of prescribed fields to the fluid state.
 *
 *  \param state  conserved state
 *  \param E      electric field, one component per direction (valid cells)
 *  \param B      magnetic field, one component per direction
 *  \param sigma  scalar electrical conductivity [s^-1]
 *  \param dt     time step
 *  \param layout component layout
 *  \return the total Joule heating [erg], for the ledger
 */
inline amrex::Real applyElectromagneticSources (amrex::MultiFab& state,
                                                amrex::Vector<amrex::MultiFab const*> const& E,
                                                amrex::Vector<amrex::MultiFab const*> const& B,
                                                amrex::Real sigma, amrex::Real dt,
                                                StateLayout const& layout)
{
    using namespace amrex;
    AMREX_ALWAYS_ASSERT_WITH_MESSAGE(E.size() == AMREX_SPACEDIM && B.size() == AMREX_SPACEDIM,
        "applyElectromagneticSources: need one E and one B component per direction");
    amrex::Real total_heating = 0.0;

    for (MFIter mfi(state); mfi.isValid(); ++mfi) {
        Box const& bx = mfi.validbox();
        auto s = state.array(mfi);
        Array<Array4<Real const>, AMREX_SPACEDIM> ea, ba;
        for (int d = 0; d < AMREX_SPACEDIM; ++d) {
            ea[d] = E[d]->const_array(mfi);
            ba[d] = B[d]->const_array(mfi);
        }
        amrex::Real heating_sum = 0.0;
        for (int i = bx.smallEnd(0); i <= bx.bigEnd(0); ++i) {
            for (int j = star_box_lo(bx, 1); j <= star_box_hi(bx, 1); ++j) {
                for (int k = star_box_lo(bx, 2); k <= star_box_hi(bx, 2); ++k) {
                    amrex::Real rho = Real(0.0);
                    for (int a = 0; a < layout.n_species; ++a) {
                        rho += s(i, j, k, layout.density(a));
                    }
                    if (rho <= Real(0.0)) { continue; }
                    // `ea`/`ba` hold AMREX_SPACEDIM entries: reading [1] and [2] unconditionally is
                    // an out-of-bounds read of the Array in a 1-D or 2-D build (it returned
                    // whichever Array4 followed in memory, so E came out as (Ex,Ex,Ex)).  The
                    // directions this build does not have stay zero, like the momentum below.
                    amrex::Real Ev[3] = {Real(0.0), Real(0.0), Real(0.0)};
                    amrex::Real Bv[3] = {Real(0.0), Real(0.0), Real(0.0)};
                    amrex::Real uv[3] = {Real(0.0), Real(0.0), Real(0.0)};
                    for (int d = 0; d < AMREX_SPACEDIM; ++d) {
                        Ev[d] = ea[d](i, j, k);
                        Bv[d] = ba[d](i, j, k);
                        uv[d] = s(i, j, k, layout.momentum(d)) / rho;
                    }
                    amrex::Real f[3];
                    electromagnetic::lorentz_force(Ev, Bv, uv, sigma, f);
                    amrex::Real const Q = electromagnetic::joule_heating(Ev, Bv, uv, sigma);
                    amrex::Real const work = f[0] * uv[0] + f[1] * uv[1] + f[2] * uv[2];
                    for (int d = 0; d < AMREX_SPACEDIM; ++d) {
                        s(i, j, k, layout.momentum(d)) += f[d] * dt;
                    }
                    s(i, j, k, layout.energy()) += (work + Q) * dt;
                    heating_sum += Q * dt;
                }
            }
        }
        total_heating += heating_sum;
    }
    return total_heating;
}

} // namespace warpx::plugins::fluid

#endif // WARPX_PLUGINS_FLUID_ELECTROMAGNETIC_H_
