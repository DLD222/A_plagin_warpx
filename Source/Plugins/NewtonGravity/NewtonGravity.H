/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_NEWTONGRAVITY_H_
#define WARPX_NEWTONGRAVITY_H_

#include <ablastr/fields/MultiFabRegister.H>

#include <AMReX_Array.H>
#include <AMReX_BoxArray.H>
#include <AMReX_DistributionMapping.H>
#include <AMReX_Geometry.H>
#include <AMReX_MultiFab.H>
#include <AMReX_REAL.H>
#include <AMReX_Vector.H>

namespace warpx::plugins::gravity
{

/** How the potential is closed at the domain boundary.
 *
 * Specification: Docs/research_notes/star_plugins/PHYSICS_SPEC.md sections 1.2 and 1.3.
 */
enum class PotentialBoundary
{
    /** Periodic in every direction.  A periodic domain cannot carry a net monopole, so the
     *  mean density is subtracted before the solve (the Jeans swindle, Binney & Tremaine
     *  section 2.1) and the returned potential has zero mean. */
    periodic,
    /** Dirichlet phi = phi_boundary in every direction.  The caller supplies the boundary
     *  values on the domain faces in the ghost cells of `phi` (see solvePotential). */
    open
};

/** Newtonian gravity: grad^2 phi = 4 pi G rho, then g = -grad phi.
 *
 * Specification: Docs/research_notes/star_plugins/PHYSICS_SPEC.md sections 1.1-1.3.
 *
 * Units are SI throughout: rho in kg m^-3, phi in m^2 s^-2, g in m s^-2.  The field solver the
 * rest of the code uses works in whatever units the caller supplies, so nothing here assumes
 * SI beyond the value of G.
 */
class NewtonGravitySolver
{
public:
    /** Solve for the potential on one level.
     *
     * The `open` case imposes Dirichlet data taken from the ghost cells of `phi`.  AMReX places
     * physical boundary data on the domain *face* (MLMGBndry::setBoxBC fixes the offset to zero),
     * so the caller must fill a boundary ghost cell with the potential evaluated on that face -
     * the normal coordinate clamped to the domain, the tangential ones at the cell centre.  The
     * operator then extrapolates the ghost value as 2 phi_face - phi_interior, which is what keeps
     * the interior stencil second order near the wall.  Filling ghost cells with cell-centre
     * values instead is a half-cell shift and shows up as a first-order error; this convention is
     * recorded in PHYSICS_SPEC section 6.
     *
     * @param[in]  rho     mass density [kg m^-3]
     * @param[out] phi     potential [m^2 s^-2], same layout as rho
     * @param[in]  geom    geometry of this level
     * @param[in]  relative_tolerance  MLMG relative tolerance
     * @param[in]  boundary  periodic (Jeans swindle) or open (Dirichlet from phi's ghost cells)
     */
    static void solvePotential (amrex::MultiFab const& rho,
                                amrex::MultiFab& phi,
                                amrex::Geometry const& geom,
                                amrex::Real relative_tolerance = 1.e-11,
                                PotentialBoundary boundary = PotentialBoundary::periodic);

    /** g = -grad phi on one level, cell-centred, stored in the same layout as phi.
     *
     * The gradient uses the same stencil as the Laplacian in solvePotential (second-order
     * central differences on a cell-centred field), which is what makes the pair consistent:
     * a uniform-density sphere then has div g = -4 pi G rho to the same order as the solve.
     */
    static void computeAcceleration (amrex::MultiFab const& phi,
                                     amrex::MultiFab& gx,
                                     amrex::MultiFab& gy,
                                     amrex::MultiFab& gz,
                                     amrex::Geometry const& geom);

    /** Total gravitational energy 1/2 integral rho phi dV [J] on one level. */
    [[nodiscard]] static amrex::Real gravitationalEnergy (amrex::MultiFab const& rho,
                                                          amrex::MultiFab const& phi,
                                                          amrex::Geometry const& geom);
};

} // namespace warpx::plugins::gravity

#endif // WARPX_NEWTONGRAVITY_H_
