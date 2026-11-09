/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_RUNTIME_SPECTRALSOLVERFACTORY_H_
#define WARPX_RUNTIME_SPECTRALSOLVERFACTORY_H_

#include "Runtime/ISpectralSolver.H"
namespace warpx::runtime { struct RuntimeContext; }
#include "Utils/WarpXAlgorithmSelection.H"

#include <ablastr/utils/Enums.H>

#include <AMReX_BoxArray.H>
#include <AMReX_Config.H>
#include <AMReX_DistributionMapping.H>
#include <AMReX_REAL.H>
#include <AMReX_RealVect.H>
#include <AMReX_Vector.H>

#include <memory>

namespace warpx::runtime
{

/**
 * \brief Everything a spectral solver needs to exist.
 *
 * The kernel used to build the concrete solver itself, which meant it named a
 * spectral implementation and carried its constructor arguments around.  This
 * flat block is the whole of that information in portable types: a field-solver
 * plugin turns it into its own solver, the kernel only gets the contract back.
 */
struct SpectralSolverConfig
{
    const amrex::BoxArray* realspace_ba = nullptr;
    const amrex::DistributionMapping* dm = nullptr;

    //! Finite-order stencil used to compute the k vectors (0 means "infinite order").
    int norder_x = 0;
    int norder_y = 0;
    int norder_z = 0;
    ablastr::utils::enums::GridType grid_type{};

    amrex::Vector<amrex::Real> v_galilean;
    amrex::Vector<amrex::Real> v_comoving;
    amrex::RealVect dx{};
    amrex::Real dt = 0.0;

    bool pml = false;
    bool periodic_single_box = false;
    bool update_with_rho = false;
    bool fft_do_time_averaging = false;
    PSATDSolutionType psatd_solution_type{};
    TimeDependencyJ time_dependency_J = TimeDependencyJ::Default;
    TimeDependencyRho time_dependency_rho = TimeDependencyRho::Default;
    bool dive_cleaning = false;
    bool divb_cleaning = false;

    //! Used by the cylindrical solvers only; ignored by the Cartesian ones.
    int lev = 0;
    int n_rz_azimuthal_modes = 1;

    //! K-space filtering the plugin should initialise on creation, if requested.
    bool use_kspace_filter = false;
    amrex::IntVect filter_npass_each_dir{};
    bool use_filter_compensation = false;

    //! Runtime context, for the solvers that need the field register or config.
    const RuntimeContext* context = nullptr;
};

/** \brief Creates a spectral solver from the block above. */
using SpectralSolverFactory = std::unique_ptr<ISpectralSolver> (*) (SpectralSolverConfig const&);

/** \brief Publish the implementation (idempotent; the last registration wins). */
void register_spectral_solver_factory (SpectralSolverFactory factory);

//! True once a field-solver plugin published a spectral solver.
[[nodiscard]] bool has_spectral_solver_factory ();

/**
 * \brief Create the spectral solver through the registered plugin.
 *
 * Returns nullptr when no field-solver plugin is linked, which is what a build
 * without spectral support should see; the kernel reports that when an input
 * asks for PSATD.
 */
[[nodiscard]] std::unique_ptr<ISpectralSolver>
create_spectral_solver (SpectralSolverConfig const& config);

//! Drop the registration again (tests only).
void clear_spectral_solver_factory ();

} // namespace warpx::runtime

#endif // WARPX_RUNTIME_SPECTRALSOLVERFACTORY_H_
