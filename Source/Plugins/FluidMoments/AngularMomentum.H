/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
/* Rotation: angular momentum as a conserved density, with its centrifugal source term.
 *
 * The azimuthal momentum density rho u_phi is carried as one of the passive extras, so it is
 * advected by exactly the same conservative HLLC flux as a species: in an axisymmetric flow with no
 * torques, sum(rho u_phi * r) can only change through the domain boundary, which is a statement the
 * test checks rather than assumes.
 *
 * What the extras cannot do by themselves is the centrifugal term: an azimuthal flow has no radial
 * force in a Cartesian momentum equation, so rotation would not hold a star up at all.  That term is
 * added here, together with the work it does:
 *
 *     d(rho u_r)/dt = + rho u_phi^2 / r          (radial momentum)
 *     dE/dt         = + rho u_phi^2 u_r / r      (the work of that force)
 *
 * With g_r = -Omega^2 r from the gravity plugin the two cancel for a rigidly rotating state, which is
 * the equilibrium the test measures: a rotation law in equilibrium must stay at rest in the radial
 * direction, not drift.
 *
 * The axis is a parameter (the y axis by default), and the radius is measured to it in the plane
 * perpendicular to that axis.  r is floored at half a cell so the term stays finite on the axis.
 *
 * Units are CGS.
 *
 * Specification: PHYSICS_SPEC.md section 7 (rotation).
 */
#ifndef WARPX_PLUGINS_FLUID_ANGULAR_MOMENTUM_H_
#define WARPX_PLUGINS_FLUID_ANGULAR_MOMENTUM_H_

#include "FluidMoments.H"

#include <AMReX_Geometry.H>
#include <AMReX_MultiFab.H>

#include <cmath>

namespace warpx::plugins::fluid
{

namespace angular_momentum
{
/** Specific angular momentum of an azimuthal flow [cm^2/s]. */
AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE
amrex::Real specific_angular_momentum (amrex::Real r, amrex::Real u_phi) noexcept
{
    return r * u_phi;
}

/** Angular velocity [1/s]. */
AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE
amrex::Real angular_velocity (amrex::Real r, amrex::Real u_phi) noexcept
{
    return (r > amrex::Real(0.0)) ? u_phi / r : amrex::Real(0.0);
}

/** Centrifugal acceleration [cm/s^2]. */
AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE
amrex::Real centrifugal_acceleration (amrex::Real r, amrex::Real u_phi) noexcept
{
    return (r > amrex::Real(0.0)) ? u_phi * u_phi / r : amrex::Real(0.0);
}
} // namespace angular_momentum

/** Add the centrifugal force (and its work) for an azimuthal flow carried in an extra component.
 *
 *  \param state     conserved state; momentum(axis) and the energy are updated
 *  \param geom      geometry, used for the cell centres and the radius floor
 *  \param layout    component layout
 *  \param uphi_comp index (among the extras) of the azimuthal momentum density rho u_phi
 *  \param axis      momentum direction the centrifugal force acts along (0=x, 1=y, 2=z)
 *  \param axis_dir  direction of the rotation axis (the radius is measured perpendicular to it)
 */
inline void applyCentrifugalSource (amrex::MultiFab& state, amrex::Geometry const& geom,
                                    amrex::Real dt, StateLayout const& layout, int uphi_comp,
                                    int axis = 0, int axis_dir = 1)
{
    using namespace amrex;
    AMREX_ALWAYS_ASSERT_WITH_MESSAGE(layout.n_extra > uphi_comp,
        "applyCentrifugalSource: the state has no azimuthal momentum component");
    AMREX_ALWAYS_ASSERT_WITH_MESSAGE(axis < AMREX_SPACEDIM && axis_dir < AMREX_SPACEDIM,
        "applyCentrifugalSource: axis out of range");

    amrex::Real const dx = geom.CellSize(0);
    amrex::Real const r_floor = amrex::Real(0.5) * dx;

    for (MFIter mfi(state); mfi.isValid(); ++mfi) {
        Box const& bx = mfi.validbox();
        auto s = state.array(mfi);
        for (int i = bx.smallEnd(0); i <= bx.bigEnd(0); ++i) {
            for (int j = bx.smallEnd(1); j <= bx.bigEnd(1); ++j) {
                for (int k = bx.smallEnd(2); k <= bx.bigEnd(2); ++k) {
                    // cell centre in every direction, then the radius perpendicular to the axis;
                    // using a zero for a direction that is not being looped over silently added
                    // half a box to r (the z coordinate of a 3D test with one cell in z)
                    amrex::Real const coord[3] = {
                        geom.ProbLo(0) + (static_cast<amrex::Real>(i) + Real(0.5)) * geom.CellSize(0),
                        (AMREX_SPACEDIM > 1)
                            ? geom.ProbLo(1) + (static_cast<amrex::Real>(j) + Real(0.5)) * geom.CellSize(1)
                            : Real(0.0),
                        (AMREX_SPACEDIM > 2)
                            ? geom.ProbLo(2) + (static_cast<amrex::Real>(k) + Real(0.5)) * geom.CellSize(2)
                            : Real(0.0)};
                    amrex::Real r2 = Real(0.0);
                    for (int d = 0; d < AMREX_SPACEDIM; ++d) {
                        if (d == axis_dir) { continue; }
                        amrex::Real const c = coord[d]
                            - Real(0.5) * (geom.ProbLo(d) + geom.ProbHi(d));
                        r2 += c * c;
                    }
                    amrex::Real const r = amrex::max(std::sqrt(r2), r_floor);

                    amrex::Real rho = Real(0.0);
                    for (int a = 0; a < layout.n_species; ++a) {
                        rho += s(i, j, k, layout.density(a));
                    }
                    if (rho <= Real(0.0)) { continue; }
                    amrex::Real const u_phi = s(i, j, k, layout.extra(uphi_comp)) / rho;
                    amrex::Real const mom = s(i, j, k, layout.momentum(axis));
                    amrex::Real const u_r = mom / rho;
                    amrex::Real const a_c = angular_momentum::centrifugal_acceleration(r, u_phi);

                    s(i, j, k, layout.momentum(axis)) += rho * a_c * dt;
                    // the force does work rho a_c u_r, so the energy must follow the momentum
                    s(i, j, k, layout.energy()) += rho * a_c * u_r * dt;
                }
            }
        }
    }
}


/** Fill a gravitational acceleration that exactly balances the centrifugal term of `state`.
 *
 *  This exists so that a caller (and the test) cannot disagree with the implementation about the
 *  radius: the same cell-centre convention and the same floor are used on both sides, so the two
 *  forces cancel cell by cell rather than on average.
 */
inline void balanced_gravity (amrex::MultiFab& state, amrex::Geometry const& geom,
                              StateLayout const& layout, int uphi_comp,
                              amrex::Vector<amrex::MultiFab*> const& g, int axis = 0,
                              int axis_dir = 1)
{
    using namespace amrex;
    for (int d = 0; d < AMREX_SPACEDIM; ++d) { g[d]->setVal(0.0); }
    amrex::Real const dx = geom.CellSize(0);
    amrex::Real const r_floor = amrex::Real(0.5) * dx;
    for (MFIter mfi(state); mfi.isValid(); ++mfi) {
        Box const& bx = mfi.validbox();
        auto const s = state.const_array(mfi);
        auto ga = g[axis]->array(mfi);
        for (int i = bx.smallEnd(0); i <= bx.bigEnd(0); ++i) {
            for (int j = bx.smallEnd(1); j <= bx.bigEnd(1); ++j) {
                for (int k = bx.smallEnd(2); k <= bx.bigEnd(2); ++k) {
                    amrex::Real const coord[3] = {
                        geom.ProbLo(0) + (static_cast<amrex::Real>(i) + Real(0.5)) * geom.CellSize(0),
                        (AMREX_SPACEDIM > 1)
                            ? geom.ProbLo(1) + (static_cast<amrex::Real>(j) + Real(0.5)) * geom.CellSize(1)
                            : Real(0.0),
                        (AMREX_SPACEDIM > 2)
                            ? geom.ProbLo(2) + (static_cast<amrex::Real>(k) + Real(0.5)) * geom.CellSize(2)
                            : Real(0.0)};
                    amrex::Real r2 = Real(0.0);
                    for (int d = 0; d < AMREX_SPACEDIM; ++d) {
                        if (d == axis_dir) { continue; }
                        amrex::Real const c = coord[d]
                            - Real(0.5) * (geom.ProbLo(d) + geom.ProbHi(d));
                        r2 += c * c;
                    }
                    amrex::Real const r = amrex::max(std::sqrt(r2), r_floor);
                    amrex::Real rho = Real(0.0);
                    for (int a = 0; a < layout.n_species; ++a) {
                        rho += s(i, j, k, layout.density(a));
                    }
                    if (rho <= Real(0.0)) { continue; }
                    amrex::Real const u_phi = s(i, j, k, layout.extra(uphi_comp)) / rho;
                    ga(i, j, k) = -angular_momentum::centrifugal_acceleration(r, u_phi);
                }
            }
        }
    }
}

/** Total angular momentum sum(rho u_phi * r) over the valid cells, for conservation checks. */
inline amrex::Real total_angular_momentum (amrex::MultiFab& state, amrex::Geometry const& geom,
                                           StateLayout const& layout, int uphi_comp,
                                           int axis_dir = 1)
{
    using namespace amrex;
    amrex::Real const dx = geom.CellSize(0);
    amrex::Real total = 0.0;
    for (MFIter mfi(state); mfi.isValid(); ++mfi) {
        Box const& bx = mfi.validbox();
        auto const s = state.const_array(mfi);
        for (int i = bx.smallEnd(0); i <= bx.bigEnd(0); ++i) {
            for (int j = bx.smallEnd(1); j <= bx.bigEnd(1); ++j) {
                for (int k = bx.smallEnd(2); k <= bx.bigEnd(2); ++k) {
                    amrex::Real const coord[3] = {
                        geom.ProbLo(0) + (static_cast<amrex::Real>(i) + Real(0.5)) * geom.CellSize(0),
                        (AMREX_SPACEDIM > 1)
                            ? geom.ProbLo(1) + (static_cast<amrex::Real>(j) + Real(0.5)) * geom.CellSize(1)
                            : Real(0.0),
                        (AMREX_SPACEDIM > 2)
                            ? geom.ProbLo(2) + (static_cast<amrex::Real>(k) + Real(0.5)) * geom.CellSize(2)
                            : Real(0.0)};
                    amrex::Real r2 = Real(0.0);
                    for (int d = 0; d < AMREX_SPACEDIM; ++d) {
                        if (d == axis_dir) { continue; }
                        amrex::Real const c = coord[d]
                            - Real(0.5) * (geom.ProbLo(d) + geom.ProbHi(d));
                        r2 += c * c;
                    }
                    total += s(i, j, k, layout.extra(uphi_comp)) * std::sqrt(r2) * dx;
                }
            }
        }
    }
    return total;
}

} // namespace warpx::plugins::fluid

#endif // WARPX_PLUGINS_FLUID_ANGULAR_MOMENTUM_H_
