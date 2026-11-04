/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "Runtime/RuntimeConfig.H"

#include "WarpX.H"

namespace warpx::runtime
{

RuntimeConfig
RuntimeConfig::FromLegacy ()
{
    RuntimeConfig c;
    c.current_deposition_algo = WarpX::current_deposition_algo;
    c.charge_deposition_algo = WarpX::charge_deposition_algo;
    c.field_gathering_algo = WarpX::field_gathering_algo;
    c.particle_pusher_algo = WarpX::particle_pusher_algo;
    c.electromagnetic_solver_id = WarpX::electromagnetic_solver_id;
    c.electrostatic_solver_id = WarpX::electrostatic_solver_id;
    c.poisson_solver_id = WarpX::poisson_solver_id;
    c.grid_type = WarpX::grid_type;
    c.ect_conformal_E = WarpX::ect_conformal_E;
    c.ect_conformal_E_Astab = WarpX::ect_conformal_E_Astab;
    c.macroscopic_subpixel = WarpX::macroscopic_subpixel;
    c.ect_te_borrowing = WarpX::ect_te_borrowing;
    c.field_boundary_lo = WarpX::field_boundary_lo;
    c.field_boundary_hi = WarpX::field_boundary_hi;
    c.particle_boundary_lo = WarpX::particle_boundary_lo;
    c.particle_boundary_hi = WarpX::particle_boundary_hi;
    c.eb_particle_boundary = WarpX::eb_particle_boundary;
    c.time_dependency_J = WarpX::time_dependency_J;
    c.time_dependency_rho = WarpX::time_dependency_rho;
    c.do_single_precision_comms = WarpX::do_single_precision_comms;
    c.do_shared_mem_charge_deposition = WarpX::do_shared_mem_charge_deposition;
    c.do_shared_mem_current_deposition = WarpX::do_shared_mem_current_deposition;
    c.do_dive_cleaning = WarpX::do_dive_cleaning;
    c.do_divb_cleaning = WarpX::do_divb_cleaning;
    c.use_fdtd_nci_corr = WarpX::use_fdtd_nci_corr;
    c.galerkin_interpolation = WarpX::galerkin_interpolation;
    c.use_filter = WarpX::use_filter;
    c.use_kspace_filter = WarpX::use_kspace_filter;
    c.use_filter_compensation = WarpX::use_filter_compensation;
    c.serialize_initial_conditions = WarpX::serialize_initial_conditions;
    c.compute_max_step_from_btd = WarpX::compute_max_step_from_btd;
    c.do_dynamic_scheduling = WarpX::do_dynamic_scheduling;
    c.refine_plasma = WarpX::refine_plasma;
    c.sort_intervals = WarpX::sort_intervals;
    c.filter_npass_each_dir = WarpX::filter_npass_each_dir;
    c.n_rz_azimuthal_modes = WarpX::n_rz_azimuthal_modes;
    c.load_balance_costs_update_algo = WarpX::load_balance_costs_update_algo;
    c.start_moving_window_step = WarpX::start_moving_window_step;
    c.end_moving_window_step = WarpX::end_moving_window_step;
    return c;
}

} // namespace warpx::runtime
