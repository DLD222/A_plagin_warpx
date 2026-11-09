/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_RUNTIME_SOLVERVECTORCONTEXT_H_
#define WARPX_RUNTIME_SOLVERVECTORCONTEXT_H_

#include "Fields.H"

#include <ablastr/fields/MultiFabRegister.H>

#include <AMReX_Config.H>
#include <AMReX_Periodicity.H>
#include <AMReX_iMultiFab.H>

#include <functional>

namespace warpx::runtime
{

/**
 * \brief What a solver vector needs from the simulation.
 *
 * A solver vector (the implicit solver's unknown) is a view over fields in the
 * register plus, for embedded boundaries, the mask that says which grid points
 * are inside the domain.  Both are values the caller has, so the vector does not
 * need the simulation class.
 */
struct SolverVectorContext
{
    ablastr::fields::MultiFabRegister* fields = nullptr;
    std::function<const amrex::iMultiFab* (warpx::fields::FieldType, int lev, int dir)> field_dot_mask;
    //! Periodicity of a level, for the guard-cell exchange of the DOF arrays.
    std::function<amrex::Periodicity (int lev)> periodicity;
};

} // namespace warpx::runtime

#endif // WARPX_RUNTIME_SOLVERVECTORCONTEXT_H_
