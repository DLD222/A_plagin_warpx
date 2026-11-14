/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_FLUIDMOMENTS_H_
#define WARPX_FLUIDMOMENTS_H_

#include <AMReX_Array.H>
#include <AMReX_Array4.H>
#include <AMReX_BoxArray.H>
#include <AMReX_Box.H>
#include <AMReX_Geometry.H>
#include <AMReX_GpuLaunch.H>
#include <AMReX_MultiFab.H>
#include <AMReX_REAL.H>

#include <cmath>

/** Five-moment (Euler) multi-species fluid, finite volume with MUSCL-Hancock reconstruction and
 *  an HLLC Riemann solver.
 *
 * Specification: Docs/research_notes/star_plugins/PHYSICS_SPEC.md section 2.  Equations numbers
 * in the comments below refer to that document:
 *
 *   2.2   the conservative state [rho_s, rho u, E]
 *   2.7   minmod-limited linear reconstruction
 *   2.8   Hancock half-step predictor
 *   2.9   HLLC flux (Davis wave speeds, contact speed, star state)
 *   2.10  conservative update and the CFL time step
 *
 * The equation of state is a template parameter, not a fixed formula: the solver only needs
 *
 *     eos.pressure(rho, e_int)      and     eos.soundSpeed(rho, p)
 *
 * both of which must be callable on the device.  IdealGasEOS below is the analytic case used by
 * the acceptance tests; a tabulated (OPAL-like) EOS plugs in by providing the same two methods.
 */
namespace warpx::plugins::fluid
{

/** Largest number of transported components the local (stack) buffers hold.  One species in
 *  three dimensions needs 5; a full solar network with a few species stays well below 16. */
constexpr int max_components = 16;

/** Component layout of the state: [rho_0 .. rho_{ns-1}, mom_x, mom_y, mom_z, E]. */
struct StateLayout
{
    int n_species = 1;

    AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE
    int density (int s) const noexcept { return s; }

    AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE
    int momentum (int d) const noexcept { return n_species + d; }

    AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE
    int energy () const noexcept { return n_species + AMREX_SPACEDIM; }

    AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE
    int n_comp () const noexcept { return n_species + AMREX_SPACEDIM + 1; }
};

/** Ideal gas: p = (gamma - 1) e, with e the internal energy per unit volume.
 *
 *  For a monatomic gas gamma = 5/3; the tests use gamma = 1.4 (the standard Sod/shock-tube value)
 *  so that the results can be compared against the textbook solution.
 */
struct IdealGasEOS
{
    amrex::Real gamma = amrex::Real(1.4);

    AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE
    amrex::Real pressure (amrex::Real /*rho*/, amrex::Real e_int) const noexcept
    {
        return (gamma - amrex::Real(1.0)) * e_int;
    }

    AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE
    amrex::Real soundSpeed (amrex::Real rho, amrex::Real p) const noexcept
    {
        return std::sqrt(gamma * p / rho);
    }
};

namespace detail
{

AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE
amrex::Real minmod (amrex::Real a, amrex::Real b) noexcept
{
    // (2.7a): zero when the slopes disagree, otherwise the smaller magnitude with the common sign.
    if (a * b <= amrex::Real(0.0)) { return amrex::Real(0.0); }
    return (std::abs(a) < std::abs(b)) ? a : b;
}

/** Total density, momentum, kinetic energy and internal energy of a state array. */
template <class EOS>
AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE
void primitive (amrex::Real const* U, StateLayout const& layout, EOS const& eos,
                amrex::Real& rho, amrex::Real vel[AMREX_SPACEDIM], amrex::Real& p,
                amrex::Real& E) noexcept
{
    int const ns = layout.n_species;
    rho = amrex::Real(0.0);
    for (int s = 0; s < ns; ++s) { rho += U[layout.density(s)]; }
    amrex::Real mom2 = amrex::Real(0.0);
    for (int d = 0; d < AMREX_SPACEDIM; ++d) {
        vel[d] = U[layout.momentum(d)] / rho;
        mom2 += U[layout.momentum(d)] * U[layout.momentum(d)];
    }
    E = U[layout.energy()];
    amrex::Real const e_int = E - amrex::Real(0.5) * mom2 / rho;
    p = eos.pressure(rho, e_int);
}

/** Physical flux F(U) in the normal direction `dir` (equation 2.3). */
template <class EOS>
AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE
void physicalFlux (amrex::Real const* U, amrex::Real* F, int dir, StateLayout const& layout,
                   EOS const& eos) noexcept
{
    amrex::Real rho, p, E;
    amrex::Real vel[AMREX_SPACEDIM];
    primitive(U, layout, eos, rho, vel, p, E);
    amrex::Real const un = vel[dir];

    for (int s = 0; s < layout.n_species; ++s) {
        F[layout.density(s)] = U[layout.density(s)] * un;
    }
    for (int d = 0; d < AMREX_SPACEDIM; ++d) {
        F[layout.momentum(d)] = U[layout.momentum(d)] * un + ((d == dir) ? p : amrex::Real(0.0));
    }
    F[layout.energy()] = (E + p) * un;
}

/** HLLC flux between the reconstructed states UL and UR (equations 2.9a-2.9d). */
template <class EOS>
AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE
void hllcFlux (amrex::Real const* UL, amrex::Real const* UR, amrex::Real* F, int dir,
               StateLayout const& layout, EOS const& eos) noexcept
{
    amrex::Real rhoL, pL, EL;
    amrex::Real velL[AMREX_SPACEDIM];
    amrex::Real rhoR, pR, ER;
    amrex::Real velR[AMREX_SPACEDIM];
    primitive(UL, layout, eos, rhoL, velL, pL, EL);
    primitive(UR, layout, eos, rhoR, velR, pR, ER);

    amrex::Real const cL = eos.soundSpeed(rhoL, pL);
    amrex::Real const cR = eos.soundSpeed(rhoR, pR);
    amrex::Real const uL = velL[dir];
    amrex::Real const uR = velR[dir];

    // Davis estimates: they only need the sound speeds, so any EOS can supply them.
    amrex::Real const SL = amrex::min(uL - cL, uR - cR);
    amrex::Real const SR = amrex::max(uL + cL, uR + cR);

    amrex::Real FL[max_components];
    amrex::Real FR[max_components];
    physicalFlux(UL, FL, dir, layout, eos);
    physicalFlux(UR, FR, dir, layout, eos);

    int const ncomp = layout.n_comp();
    if (SL >= amrex::Real(0.0)) {
        for (int n = 0; n < ncomp; ++n) { F[n] = FL[n]; }
        return;
    }
    if (SR <= amrex::Real(0.0)) {
        for (int n = 0; n < ncomp; ++n) { F[n] = FR[n]; }
        return;
    }

    amrex::Real const SM = (pR - pL + rhoL * uL * (SL - uL) - rhoR * uR * (SR - uR))
                         / (rhoL * (SL - uL) - rhoR * (SR - uR));

    bool const use_left = (SM >= amrex::Real(0.0));
    amrex::Real const* U = use_left ? UL : UR;
    amrex::Real const* FK = use_left ? FL : FR;
    amrex::Real const S = use_left ? SL : SR;
    amrex::Real const rho = use_left ? rhoL : rhoR;
    amrex::Real const p = use_left ? pL : pR;
    amrex::Real const u = use_left ? uL : uR;
    amrex::Real const E = use_left ? EL : ER;
    amrex::Real const* vel = use_left ? velL : velR;

    amrex::Real const factor = rho * (S - u) / (S - SM);

    for (int s = 0; s < layout.n_species; ++s) {
        amrex::Real const Ustar = U[layout.density(s)] * (S - u) / (S - SM);
        F[layout.density(s)] = FK[layout.density(s)] + S * (Ustar - U[layout.density(s)]);
    }
    for (int d = 0; d < AMREX_SPACEDIM; ++d) {
        amrex::Real const Ustar = (d == dir) ? factor * SM : factor * vel[d];
        F[layout.momentum(d)] = FK[layout.momentum(d)] + S * (Ustar - U[layout.momentum(d)]);
    }
    amrex::Real const Ustar =
        factor * (E / rho + (SM - u) * (SM + p / (rho * (S - u))));
    F[layout.energy()] = FK[layout.energy()] + S * (Ustar - U[layout.energy()]);
}

/** One cell's conserved state, read from the Array4 with the face/cell offset applied. */
AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE
int offset (int i, int j, int k, int dir, int delta, int& oi, int& oj, int& ok) noexcept
{
    oi = i;
    oj = j;
    ok = k;
#if AMREX_SPACEDIM == 3
    if (dir == 0) { oi += delta; }
    else if (dir == 1) { oj += delta; }
    else { ok += delta; }
#elif AMREX_SPACEDIM == 2
    if (dir == 0) { oi += delta; }
    else { oj += delta; }
#else
    amrex::ignore_unused(dir);
    oi += delta;
#endif
    return 0;
}

} // namespace detail

/** Advance `state` by one conservative sweep along `dir` (equation 2.10a).
 *
 * The state MultiFab must carry at least two ghost cells in `dir`; the caller fills them
 * according to its boundary conditions (periodic, transmissive, ...).  The update is a single
 * directional sweep; multi-dimensional evolution is meant to be built from Strang-split sweeps.
 */
template <class EOS>
void advanceSweep (amrex::MultiFab& state, amrex::Geometry const& geom, int dir,
                   amrex::Real dt, EOS const& eos, StateLayout const& layout)
{
    using namespace amrex;

    int const ncomp = layout.n_comp();
    AMREX_ALWAYS_ASSERT_WITH_MESSAGE(
        ncomp <= max_components,
        "FluidMoments: state has more components than the buffers hold");
    AMREX_ALWAYS_ASSERT(state.nGrowVect()[dir] >= 2);

    Real const dx = geom.CellSize(dir);

    // Face-centred flux in the sweep direction, one component per state component.
    MultiFab flux(amrex::convert(state.boxArray(), IntVect::TheDimensionVector(dir)),
                  state.DistributionMap(), ncomp, 0);

    for (MFIter mfi(state); mfi.isValid(); ++mfi)
    {
        Box const& bx = mfi.validbox();
        Box const fbx = amrex::surroundingNodes(bx, dir);
        auto const s = state.const_array(mfi);
        auto const f = flux.array(mfi);

        ParallelFor(fbx, [=] AMREX_GPU_DEVICE (int i, int j, int k) noexcept
        {
            // Face (i,j,k) sits between the cell below it (L) and above it (R).
            int li, lj, lk, ri, rj, rk;
            detail::offset(i, j, k, dir, -1, li, lj, lk);
            detail::offset(i, j, k, dir, 0, ri, rj, rk);

            Real UL[max_components];
            Real UR[max_components];
            for (int n = 0; n < ncomp; ++n) {
                UL[n] = s(li, lj, lk, n);
                UR[n] = s(ri, rj, rk, n);
            }

            // Minmod slopes and the edge states of both cells (2.7a, 2.7b), then the Hancock
            // half-step (2.8).  The predictor needs the slope of the cell on each side, hence
            // two ghost cells.
            Real ULm[max_components];
            Real ULp[max_components];
            Real URm[max_components];
            Real URp[max_components];
            for (int n = 0; n < ncomp; ++n) {
                int li2, lj2, lk2, ri2, rj2, rk2;
                detail::offset(li, lj, lk, dir, -1, li2, lj2, lk2);
                detail::offset(ri, rj, rk, dir, 1, ri2, rj2, rk2);
                Real const slopeL =
                    detail::minmod(UL[n] - s(li2, lj2, lk2, n), UR[n] - UL[n]);
                Real const slopeR =
                    detail::minmod(UR[n] - UL[n], s(ri2, rj2, rk2, n) - UR[n]);
                ULm[n] = UL[n] - Real(0.5) * slopeL;
                ULp[n] = UL[n] + Real(0.5) * slopeL;
                URm[n] = UR[n] - Real(0.5) * slopeR;
                URp[n] = UR[n] + Real(0.5) * slopeR;
            }

            Real FLm[max_components];
            Real FLp[max_components];
            Real FRm[max_components];
            Real FRp[max_components];
            detail::physicalFlux(ULm, FLm, dir, layout, eos);
            detail::physicalFlux(ULp, FLp, dir, layout, eos);
            detail::physicalFlux(URm, FRm, dir, layout, eos);
            detail::physicalFlux(URp, FRp, dir, layout, eos);

            Real const half_dt_dx = Real(0.5) * dt / dx;
            Real UtL[max_components];
            Real UtR[max_components];
            for (int n = 0; n < ncomp; ++n) {
                UtL[n] = ULp[n] + half_dt_dx * (FLm[n] - FLp[n]);
                UtR[n] = URm[n] + half_dt_dx * (FRm[n] - FRp[n]);
            }

            Real F[max_components];
            detail::hllcFlux(UtL, UtR, F, dir, layout, eos);
            for (int n = 0; n < ncomp; ++n) { f(i, j, k, n) = F[n]; }
        });
    }

    // Conservative update (2.10a).
    Real const dt_dx = dt / dx;
    for (MFIter mfi(state); mfi.isValid(); ++mfi)
    {
        Box const& bx = mfi.validbox();
        auto s = state.array(mfi);
        auto const f = flux.const_array(mfi);

        ParallelFor(bx, ncomp, [=] AMREX_GPU_DEVICE (int i, int j, int k, int n) noexcept
        {
            int lo_i, lo_j, lo_k, hi_i, hi_j, hi_k;
            detail::offset(i, j, k, dir, 0, lo_i, lo_j, lo_k);
            detail::offset(i, j, k, dir, 1, hi_i, hi_j, hi_k);
            s(i, j, k, n) -= dt_dx * (f(hi_i, hi_j, hi_k, n) - f(lo_i, lo_j, lo_k, n));
        });
    }
}

/** Largest |u| + c over the valid cells; the caller turns it into dt with (2.10b).
 *
 * A one-component scratch field is filled and reduced, which keeps this independent of the
 * number of species. */
template <class EOS>
amrex::Real maxWaveSpeed (amrex::MultiFab const& state, EOS const& eos, StateLayout const& layout,
                          int dir)
{
    using namespace amrex;
    MultiFab speed(state.boxArray(), state.DistributionMap(), 1, 0);
    for (MFIter mfi(state); mfi.isValid(); ++mfi)
    {
        Box const& bx = mfi.validbox();
        auto const s = state.const_array(mfi);
        auto sp = speed.array(mfi);
        ParallelFor(bx, [=] AMREX_GPU_DEVICE (int i, int j, int k) noexcept
        {
            Real U[max_components];
            for (int n = 0; n < layout.n_comp(); ++n) { U[n] = s(i, j, k, n); }
            Real rho, p, E;
            Real vel[AMREX_SPACEDIM];
            detail::primitive(U, layout, eos, rho, vel, p, E);
            sp(i, j, k) = std::abs(vel[dir]) + eos.soundSpeed(rho, p);
        });
    }
    return speed.norminf(0);
}

/** Compiled entry point for the ideal-gas case, so the plugin has a non-template translation unit
 *  (and an obvious place to grow the EOS-injection layer). */
void advanceIdealGas (amrex::MultiFab& state, amrex::Geometry const& geom, int dir,
                      amrex::Real dt, amrex::Real gamma = amrex::Real(1.4),
                      int n_species = 1);

} // namespace warpx::plugins::fluid

#endif // WARPX_FLUIDMOMENTS_H_
