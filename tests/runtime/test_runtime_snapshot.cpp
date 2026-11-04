/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "Runtime/Runtime.H"
#include "WarpX.H"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

namespace
{

int failures = 0;

void check (bool ok, const std::string& what)
{
    if (!ok) {
        std::cerr << "FAIL: " << what << '\n';
        ++failures;
    }
}

template <typename T>
void check_eq (const T& a, const T& b, const std::string& what)
{
    check(a == b, what);
}

void check_real (amrex::Real a, amrex::Real b, const std::string& what)
{
    check(std::abs(a - b) <= 1.0e-15 * std::max<amrex::Real>(1.0, std::abs(b)), what);
}

} // namespace

int main (int argc, char* argv[])
{
    warpx::runtime::Runtime runtime(argc, argv);
    runtime.initialize();

    const auto& c = runtime.config();
    check_eq(c.current_deposition_algo, WarpX::current_deposition_algo, "current_deposition_algo");
    check_eq(c.charge_deposition_algo, WarpX::charge_deposition_algo, "charge_deposition_algo");
    check_eq(c.field_gathering_algo, WarpX::field_gathering_algo, "field_gathering_algo");
    check_eq(c.particle_pusher_algo, WarpX::particle_pusher_algo, "particle_pusher_algo");
    check_eq(c.electromagnetic_solver_id, WarpX::electromagnetic_solver_id, "electromagnetic_solver_id");
    check_eq(c.electrostatic_solver_id, WarpX::electrostatic_solver_id, "electrostatic_solver_id");
    check_eq(c.poisson_solver_id, WarpX::poisson_solver_id, "poisson_solver_id");
    check_eq(c.grid_type, WarpX::grid_type, "grid_type");
    check_eq(c.ect_conformal_E, WarpX::ect_conformal_E, "ect_conformal_E");
    check_real(c.ect_conformal_E_Astab, WarpX::ect_conformal_E_Astab, "ect_conformal_E_Astab");
    check_eq(c.macroscopic_subpixel, WarpX::macroscopic_subpixel, "macroscopic_subpixel");
    check_eq(c.ect_te_borrowing, WarpX::ect_te_borrowing, "ect_te_borrowing");
    check_eq(c.field_boundary_lo, WarpX::field_boundary_lo, "field_boundary_lo");
    check_eq(c.field_boundary_hi, WarpX::field_boundary_hi, "field_boundary_hi");
    check_eq(c.particle_boundary_lo, WarpX::particle_boundary_lo, "particle_boundary_lo");
    check_eq(c.particle_boundary_hi, WarpX::particle_boundary_hi, "particle_boundary_hi");
    check_eq(c.eb_particle_boundary, WarpX::eb_particle_boundary, "eb_particle_boundary");
    check_eq(c.time_dependency_J, WarpX::time_dependency_J, "time_dependency_J");
    check_eq(c.time_dependency_rho, WarpX::time_dependency_rho, "time_dependency_rho");
    check_eq(c.do_single_precision_comms, WarpX::do_single_precision_comms, "do_single_precision_comms");
    check_eq(c.do_shared_mem_charge_deposition, WarpX::do_shared_mem_charge_deposition, "do_shared_mem_charge_deposition");
    check_eq(c.do_shared_mem_current_deposition, WarpX::do_shared_mem_current_deposition, "do_shared_mem_current_deposition");
    check_eq(c.do_dive_cleaning, WarpX::do_dive_cleaning, "do_dive_cleaning");
    check_eq(c.do_divb_cleaning, WarpX::do_divb_cleaning, "do_divb_cleaning");
    check_eq(c.use_fdtd_nci_corr, WarpX::use_fdtd_nci_corr, "use_fdtd_nci_corr");
    check_eq(c.galerkin_interpolation, WarpX::galerkin_interpolation, "galerkin_interpolation");
    check_eq(c.use_filter, WarpX::use_filter, "use_filter");
    check_eq(c.use_kspace_filter, WarpX::use_kspace_filter, "use_kspace_filter");
    check_eq(c.use_filter_compensation, WarpX::use_filter_compensation, "use_filter_compensation");
    check_eq(c.serialize_initial_conditions, WarpX::serialize_initial_conditions, "serialize_initial_conditions");
    check_eq(c.compute_max_step_from_btd, WarpX::compute_max_step_from_btd, "compute_max_step_from_btd");
    check_eq(c.do_dynamic_scheduling, WarpX::do_dynamic_scheduling, "do_dynamic_scheduling");
    check_eq(c.refine_plasma, WarpX::refine_plasma, "refine_plasma");
    check_eq(c.filter_npass_each_dir, WarpX::filter_npass_each_dir, "filter_npass_each_dir");
    check_eq(c.load_balance_costs_update_algo, WarpX::load_balance_costs_update_algo, "load_balance_costs_update_algo");
    check_eq(c.start_moving_window_step, WarpX::start_moving_window_step, "start_moving_window_step");
    check_eq(c.end_moving_window_step, WarpX::end_moving_window_step, "end_moving_window_step");

    const auto& t = runtime.time();
    check_eq(t.step, WarpX::GetInstance().getistep(0), "time.step");
    check_eq(t.finest_level, WarpX::GetInstance().finestLevel(), "time.finest_level");
    check_eq(t.istep, WarpX::GetInstance().getistep(), "time.istep");
    check_eq(t.nsubsteps, WarpX::GetInstance().getnsubsteps(), "time.nsubsteps");
    check_eq(t.dt, WarpX::GetInstance().getdt(), "time.dt");
    check_eq(t.t_new, WarpX::GetInstance().gett_new(), "time.t_new");
    check_eq(t.t_old, WarpX::GetInstance().gett_old(), "time.t_old");
    check_real(t.time, WarpX::GetInstance().gett_new(0), "time.time");

    const auto& f = runtime.frame();
    check_real(f.gamma_boost, WarpX::gamma_boost, "frame.gamma_boost");
    check_real(f.beta_boost, WarpX::beta_boost, "frame.beta_boost");
    check_eq(f.boost_direction, WarpX::boost_direction, "frame.boost_direction");
    check_eq(f.do_moving_window, WarpX::do_moving_window != 0, "frame.do_moving_window");
    check_eq(f.moving_window_dir, WarpX::moving_window_dir, "frame.moving_window_dir");
    check_real(f.moving_window_v, WarpX::moving_window_v, "frame.moving_window_v");
    check_eq(f.start_moving_window_step, WarpX::start_moving_window_step, "frame.start_moving_window_step");
    check_eq(f.end_moving_window_step, WarpX::end_moving_window_step, "frame.end_moving_window_step");

    runtime.evolve(1);
    check_eq(runtime.time().step, 1, "time.step after evolve");
    check_eq(runtime.time().istep, WarpX::GetInstance().getistep(), "time.istep after evolve");

    if (failures == 0) {
        std::cout << "PASS: runtime snapshots match legacy WarpX state\n";
    }
    return failures == 0 ? 0 : 1;
}
