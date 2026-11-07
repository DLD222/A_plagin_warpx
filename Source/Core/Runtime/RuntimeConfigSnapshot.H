/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_RUNTIME_RUNTIMECONFIGSNAPSHOT_H_
#define WARPX_RUNTIME_RUNTIMECONFIGSNAPSHOT_H_

#include "Utils/WarpXAlgorithmSelection.H"

#include <ablastr/utils/Enums.H>

#include <AMReX_Array.H>
#include <AMReX_IntVect.H>
#include <AMReX_REAL.H>

namespace warpx::runtime
{

/**
 * \brief Stable value snapshot for modules.
 *
 * A7.1 transitional object: the live RuntimeConfig remains for BTD and
 * legacy consumers; new modules read this snapshot and do not depend on
 * WarpX types.
 */
struct RuntimeConfigSnapshot
{
    CurrentDepositionAlgo current_deposition_algo{CurrentDepositionAlgo::Default};
    ChargeDepositionAlgo charge_deposition_algo{ChargeDepositionAlgo::Default};
    GatheringAlgo field_gathering_algo{GatheringAlgo::Default};
    ParticlePusherAlgo particle_pusher_algo{ParticlePusherAlgo::Default};

    ElectromagneticSolverAlgo electromagnetic_solver_id{ElectromagneticSolverAlgo::Default};
    ElectrostaticSolverAlgo electrostatic_solver_id{ElectrostaticSolverAlgo::Default};
    ablastr::utils::enums::GridType grid_type{ablastr::utils::enums::GridType::Default};

    amrex::Array<FieldBoundaryType, AMREX_SPACEDIM> field_boundary_lo{};
    amrex::Array<FieldBoundaryType, AMREX_SPACEDIM> field_boundary_hi{};
    amrex::Array<ParticleBoundaryType, AMREX_SPACEDIM> particle_boundary_lo{};
    amrex::Array<ParticleBoundaryType, AMREX_SPACEDIM> particle_boundary_hi{};

    int nox{0};
    int noy{0};
    int noz{0};
    int ncomps{1};
    int n_rz_azimuthal_modes{1};
    bool galerkin_interpolation{false};
    bool do_dive_cleaning{false};
    bool do_divb_cleaning{false};
    bool use_filter{false};
    amrex::IntVect filter_npass_each_dir{amrex::IntVect::TheZeroVector()};
    bool do_single_precision_comms{false};
    bool fft_do_time_averaging{false};
    int start_moving_window_step{-1};
    int end_moving_window_step{-1};
    LoadBalanceCostsUpdateAlgo load_balance_costs_update_algo{LoadBalanceCostsUpdateAlgo::Default};

    //! Filled by the PicEm plugin adapter.
    [[nodiscard]] static RuntimeConfigSnapshot FromLegacy ();
};

} // namespace warpx::runtime

#endif
