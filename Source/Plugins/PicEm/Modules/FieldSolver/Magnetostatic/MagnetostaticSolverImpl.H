/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_MAGNETOSTATICSOLVERIMPL_H_
#define WARPX_MAGNETOSTATICSOLVERIMPL_H_

#include "Runtime/IMagnetostaticSolver.H"

#include "MagnetostaticSolver.H"

#include <ablastr/fields/MultiFabRegister.H>

class MagnetostaticSolverImpl : public warpx::runtime::IMagnetostaticSolver
{
public:
    void defineVectorPotentialBCs (const amrex::Geometry& geom) override;

    void computeField (warpx::runtime::MagnetostaticConfig const& config) override;

private:
    void setVectorPotentialBC (
        warpx::runtime::MagnetostaticConfig const& config,
        ablastr::fields::MultiLevelVectorField const& A) const;

    void computeVectorPotential (
        warpx::runtime::MagnetostaticConfig const& config,
        ablastr::fields::MultiLevelVectorField const& curr,
        ablastr::fields::MultiLevelVectorField const& A) const;

    //! Boundary conditions and flags, owned by the solver now.
    mutable MagnetostaticSolver::VectorPoissonBoundaryHandler m_bc;
    //! Field-boundary types the boundary conditions were built from.
    amrex::Array<FieldBoundaryType, AMREX_SPACEDIM> m_boundary_lo{};
    amrex::Array<FieldBoundaryType, AMREX_SPACEDIM> m_boundary_hi{};
};

#endif // WARPX_MAGNETOSTATIC_SOLVERIMPL_H_
