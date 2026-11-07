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

namespace warpx::runtime
{

/**
 * \brief Live, read-only view of the algorithm configuration.
 *
 * This replaces the former value snapshot (audit D4): every getter reads the
 * current WarpX value, so `dt`, `finestLevel`, moving-window and filter state
 * cannot go stale. It is not a plugin data store; storage remains in WarpX/
 * MultiFabRegister.
 */
class RuntimeConfig
{
public:
    RuntimeConfig () = default;

    [[nodiscard]] CurrentDepositionAlgo current_deposition_algo () const;
    [[nodiscard]] ChargeDepositionAlgo charge_deposition_algo () const;
    [[nodiscard]] GatheringAlgo field_gathering_algo () const;
    [[nodiscard]] ParticlePusherAlgo particle_pusher_algo () const;
    [[nodiscard]] ElectromagneticSolverAlgo electromagnetic_solver_id () const;
    [[nodiscard]] ElectrostaticSolverAlgo electrostatic_solver_id () const;
    [[nodiscard]] PoissonSolverAlgo poisson_solver_id () const;
    [[nodiscard]] ablastr::utils::enums::GridType grid_type () const;
    [[nodiscard]] int ect_conformal_E () const;
    [[nodiscard]] amrex::Real ect_conformal_E_Astab () const;
    [[nodiscard]] int macroscopic_subpixel () const;
    [[nodiscard]] int ect_te_borrowing () const;
    [[nodiscard]] const amrex::Array<FieldBoundaryType, AMREX_SPACEDIM>& field_boundary_lo () const;
    [[nodiscard]] const amrex::Array<FieldBoundaryType, AMREX_SPACEDIM>& field_boundary_hi () const;
    [[nodiscard]] const amrex::Array<ParticleBoundaryType, AMREX_SPACEDIM>& particle_boundary_lo () const;
    [[nodiscard]] const amrex::Array<ParticleBoundaryType, AMREX_SPACEDIM>& particle_boundary_hi () const;
    [[nodiscard]] ParticleBoundaryType eb_particle_boundary () const;
    [[nodiscard]] TimeDependencyJ time_dependency_J () const;
    [[nodiscard]] TimeDependencyRho time_dependency_rho () const;
    [[nodiscard]] bool do_single_precision_comms () const;
    [[nodiscard]] bool do_shared_mem_charge_deposition () const;
    [[nodiscard]] bool do_shared_mem_current_deposition () const;
    [[nodiscard]] bool do_dive_cleaning () const;
    [[nodiscard]] bool do_divb_cleaning () const;
    [[nodiscard]] bool use_fdtd_nci_corr () const;
    [[nodiscard]] bool galerkin_interpolation () const;
    [[nodiscard]] bool use_filter () const;
    [[nodiscard]] bool use_kspace_filter () const;
    [[nodiscard]] bool use_filter_compensation () const;
    [[nodiscard]] bool serialize_initial_conditions () const;
    [[nodiscard]] bool compute_max_step_from_btd () const;
    [[nodiscard]] bool do_dynamic_scheduling () const;
    [[nodiscard]] bool refine_plasma () const;
    [[nodiscard]] const ablastr::utils::text::IntervalsParser& sort_intervals () const;
    [[nodiscard]] const amrex::IntVect& filter_npass_each_dir () const;
    [[nodiscard]] int n_rz_azimuthal_modes () const;
    [[nodiscard]] int nox () const;
    [[nodiscard]] int noy () const;
    [[nodiscard]] int noz () const;
    [[nodiscard]] int particle_max_grid_crossings () const;
    [[nodiscard]] int villasenor_mass_matrices_max_grid_crossings () const;
    [[nodiscard]] int shared_mem_current_tpb () const;
    [[nodiscard]] const amrex::IntVect& shared_tilesize () const;
    [[nodiscard]] bool fft_do_time_averaging () const;
    [[nodiscard]] int ncomps () const;
    [[nodiscard]] bool sync_nodal_points () const;
    [[nodiscard]] LoadBalanceCostsUpdateAlgo load_balance_costs_update_algo () const;
    [[nodiscard]] int start_moving_window_step () const;
    [[nodiscard]] int end_moving_window_step () const;

    //! Compatibility factory; the object is stateless/live.
    [[nodiscard]] static RuntimeConfig FromLegacy ();
};

} // namespace warpx::runtime

#endif // WARPX_RUNTIME_RUNTIMECONFIG_H_
