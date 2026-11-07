/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "Runtime/RuntimeConfigSnapshot.H"

#include "WarpX.H"

namespace warpx::runtime
{

RuntimeConfigSnapshot
RuntimeConfigSnapshot::FromLegacy ()
{
    RuntimeConfigSnapshot snapshot;
    snapshot.current_deposition_algo = WarpX::current_deposition_algo;
    snapshot.charge_deposition_algo = WarpX::charge_deposition_algo;
    snapshot.field_gathering_algo = WarpX::field_gathering_algo;
    snapshot.particle_pusher_algo = WarpX::particle_pusher_algo;
    snapshot.electromagnetic_solver_id = WarpX::electromagnetic_solver_id;
    snapshot.electrostatic_solver_id = WarpX::electrostatic_solver_id;
    snapshot.grid_type = WarpX::grid_type;
    snapshot.field_boundary_lo = WarpX::field_boundary_lo;
    snapshot.field_boundary_hi = WarpX::field_boundary_hi;
    snapshot.particle_boundary_lo = WarpX::particle_boundary_lo;
    snapshot.particle_boundary_hi = WarpX::particle_boundary_hi;
    snapshot.nox = WarpX::nox;
    snapshot.noy = WarpX::noy;
    snapshot.noz = WarpX::noz;
    snapshot.ncomps = WarpX::ncomps;
#ifdef WARPX_DIM_RZ
    snapshot.n_rz_azimuthal_modes = WarpX::n_rz_azimuthal_modes;
#else
    snapshot.n_rz_azimuthal_modes = 1;
#endif
    snapshot.galerkin_interpolation = WarpX::galerkin_interpolation;
    snapshot.do_dive_cleaning = WarpX::do_dive_cleaning;
    snapshot.do_divb_cleaning = WarpX::do_divb_cleaning;
    snapshot.use_filter = WarpX::use_filter;
    snapshot.filter_npass_each_dir = WarpX::filter_npass_each_dir;
    snapshot.do_single_precision_comms = WarpX::do_single_precision_comms;
    snapshot.fft_do_time_averaging = WarpX::fft_do_time_averaging;
    snapshot.start_moving_window_step = WarpX::start_moving_window_step;
    snapshot.end_moving_window_step = WarpX::end_moving_window_step;
    snapshot.load_balance_costs_update_algo = WarpX::load_balance_costs_update_algo;
    return snapshot;
}

} // namespace warpx::runtime
