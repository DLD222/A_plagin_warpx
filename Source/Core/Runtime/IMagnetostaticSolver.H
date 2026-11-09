/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_RUNTIME_IMAGNETOSTATICSOLVER_H_
#define WARPX_RUNTIME_IMAGNETOSTATICSOLVER_H_

#include <ablastr/fields/MultiFabRegister.H>

#include "Utils/WarpXAlgorithmSelection.H"

#include <AMReX_Array.H>
#include <AMReX_BoxArray.H>
#include <AMReX_DistributionMapping.H>
#include <AMReX_Geometry.H>
#include <AMReX_FArrayBox.H>
#include <AMReX_Vector.H>
#include <AMReX_REAL.H>

#include <functional>

namespace warpx::runtime
{

class IParticleContainer;

/**
 * \brief Everything a magnetostatic solve needs from the simulation.
 *
 * The magnetostatic solver deposits the current into the field register, syncs
 * it, solves a vector Poisson equation and turns the vector potential into B.
 * All of that needs the field register, the geometry, the time step and the
 * species that carry current - but not the simulation class itself.
 */
struct MagnetostaticConfig
{
    //! Field register that holds current_fp, vector_potential_fp_nodal and Bfield_fp.
    ablastr::fields::MultiFabRegister* fields = nullptr;
    //! Geometry of level 0, for the boundary conditions.
    const amrex::Geometry* geom = nullptr;
    //! Per-level AmrCore views the vector-Poisson solve needs.
    const amrex::Vector<amrex::Geometry>* level_geom = nullptr;
    const amrex::Vector<amrex::DistributionMapping>* level_dmap = nullptr;
    const amrex::Vector<amrex::BoxArray>* level_grids = nullptr;
    const amrex::Vector<amrex::IntVect>* ref_ratio = nullptr;
    //! Time at the end of the step (the solve uses it for the EB path).
    amrex::Real t_new = 0.0;
    //! EB factories per level (only meaningful when EB is enabled at compile time).
    std::function<amrex::FabFactory<amrex::FArrayBox> const& (int lev)> eb_factory;

    int max_level = 0;
    int finest_level = 0;
    amrex::Real dt = 0.0;
    //! Azimuthal modes; only the single-mode cylindrical case is implemented.
    int n_rz_azimuthal_modes = 1;

    //! Deposits the current of every species into current_fp at \c dt.
    std::function<void (ablastr::fields::MultiFabRegister&, amrex::Real)> deposit_current;
    //! Sums the current guard cells across levels.
    std::function<void (ablastr::fields::MultiFabRegister&, int, int)> sync_current;
    //! Applies the RZ inverse-volume scaling to the current (no-op elsewhere).
    std::function<void (ablastr::fields::MultiFabRegister&, int)> finalize_current;

    //! Field boundary types per direction, used to build the vector-potential BCs.
    amrex::Array<FieldBoundaryType, AMREX_SPACEDIM> field_boundary_lo{};
    amrex::Array<FieldBoundaryType, AMREX_SPACEDIM> field_boundary_hi{};
    //! Finite-order stencil of the field centering (EB path): orders and the
    //! device-side coefficients the kernel precomputes.
    int field_centering_nox = 0;
    int field_centering_noy = 0;
    int field_centering_noz = 0;
    amrex::Real const* field_centering_coeffs_x = nullptr;
    amrex::Real const* field_centering_coeffs_y = nullptr;
    amrex::Real const* field_centering_coeffs_z = nullptr;

    amrex::Real required_precision = 0.0;
    amrex::Real absolute_tolerance = 0.0;
    int max_iters = 0;
    int verbosity = 0;
    bool do_single_precision_comms = false;
};

/**
 * \brief The magnetostatic capability.
 *
 * Registered by a field-solver plugin; the kernel keeps one instance and calls
 * these two entry points, so a build without the plugin simply has no
 * magnetostatic solver and reports that when an input asks for one.
 */
class IMagnetostaticSolver
{
public:
    virtual ~IMagnetostaticSolver () = default;

    //! Set the vector-potential boundary conditions from the level-0 geometry.
    virtual void defineVectorPotentialBCs (const amrex::Geometry& geom) = 0;

    //! Deposit the current, solve the vector Poisson equation and add B.
    virtual void computeField (MagnetostaticConfig const& config) = 0;
};

/** \brief Creates the magnetostatic solver. */
using MagnetostaticSolverFactory = std::unique_ptr<IMagnetostaticSolver> (*) ();

/** \brief Publish the implementation (idempotent; the last registration wins). */
void register_magnetostatic_solver_factory (MagnetostaticSolverFactory factory);

//! True once a field-solver plugin published a magnetostatic solver.
[[nodiscard]] bool has_magnetostatic_solver_factory ();

//! Returns nullptr when no plugin is linked.
[[nodiscard]] std::unique_ptr<IMagnetostaticSolver> create_magnetostatic_solver ();

//! Drop the registration again (tests only).
void clear_magnetostatic_solver_factory ();

} // namespace warpx::runtime

#endif // WARPX_RUNTIME_IMAGNETOSTATICSOLVER_H_
