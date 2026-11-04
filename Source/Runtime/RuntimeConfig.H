/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_RUNTIME_RUNTIMECONFIG_H_
#define WARPX_RUNTIME_RUNTIMECONFIG_H_

#include "Utils/WarpXAlgorithmSelection.H"

#include <ablastr/utils/Enums.H>
#include <ablastr/utils/text/IntervalsParser.H>

#include <AMReX_Array.H>
#include <AMReX_IntVect.H>
#include <AMReX_REAL.H>

/**
 * \brief Read-only snapshot of the algorithm configuration.
 *
 * This is the A1 replacement target for the 40 config `WarpX::static` members.
 * It is populated once, after input parsing, and must not be mutated afterwards.
 * Plugins should receive `const RuntimeConfig&`, never `WarpX::`.
 */
namespace warpx::runtime
{

struct RuntimeConfig
{
    // deposition / gathering / pusher
    CurrentDepositionAlgo current_deposition_algo = CurrentDepositionAlgo::Default;
    ChargeDepositionAlgo charge_deposition_algo = ChargeDepositionAlgo::Default;
    GatheringAlgo field_gathering_algo = GatheringAlgo::Default;
    ParticlePusherAlgo particle_pusher_algo = ParticlePusherAlgo::Default;

    // field solver selection
    ElectromagneticSolverAlgo electromagnetic_solver_id = ElectromagneticSolverAlgo::Default;
    ElectrostaticSolverAlgo electrostatic_solver_id = ElectrostaticSolverAlgo::Default;
    PoissonSolverAlgo poisson_solver_id = PoissonSolverAlgo::Default;
    ablastr::utils::enums::GridType grid_type = ablastr::utils::enums::GridType::Default;

    // macroscopic / EB
    int ect_conformal_E = 0;
    amrex::Real ect_conformal_E_Astab = 0.5;
    int macroscopic_subpixel = 0;
    int ect_te_borrowing = 1;

    // boundaries
    amrex::Array<FieldBoundaryType, AMREX_SPACEDIM> field_boundary_lo{};
    amrex::Array<FieldBoundaryType, AMREX_SPACEDIM> field_boundary_hi{};
    amrex::Array<ParticleBoundaryType, AMREX_SPACEDIM> particle_boundary_lo{};
    amrex::Array<ParticleBoundaryType, AMREX_SPACEDIM> particle_boundary_hi{};
    ParticleBoundaryType eb_particle_boundary = ParticleBoundaryType::Absorbing;

    // PSATD source time dependency
    TimeDependencyJ time_dependency_J = TimeDependencyJ::Default;
    TimeDependencyRho time_dependency_rho = TimeDependencyRho::Default;

    // communication / deposition
    bool do_single_precision_comms = false;
    bool do_shared_mem_charge_deposition = false;
    bool do_shared_mem_current_deposition = false;

    // cleaning / numerics
    bool do_dive_cleaning = false;
    bool do_divb_cleaning = false;
    bool use_fdtd_nci_corr = false;
    bool galerkin_interpolation = true;
    bool use_filter = false;
    bool use_kspace_filter = false;
    bool use_filter_compensation = false;
    bool serialize_initial_conditions = false;
    bool compute_max_step_from_btd = false;
    bool do_dynamic_scheduling = false;
    bool refine_plasma = false;
    ablastr::utils::text::IntervalsParser sort_intervals{};
    amrex::IntVect filter_npass_each_dir{AMREX_D_DECL(1, 1, 1)};

    // low-level numerics (read by diagnostics)
    int n_rz_azimuthal_modes = 0;

    // load balance
    LoadBalanceCostsUpdateAlgo load_balance_costs_update_algo = LoadBalanceCostsUpdateAlgo::Default;

    // moving window (input side; runtime side lives in FrameState)
    int start_moving_window_step = -1;
    int end_moving_window_step = -1;

    //! Snapshot from the legacy WarpX statics (transitional).
    [[nodiscard]] static RuntimeConfig FromLegacy ();
};

} // namespace warpx::runtime

#endif // WARPX_RUNTIME_RUNTIMECONFIG_H_
