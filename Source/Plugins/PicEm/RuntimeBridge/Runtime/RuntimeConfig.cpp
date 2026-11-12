/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "Runtime/RuntimeConfig.H"

#include "WarpX.H"
#include "Particles/MultiParticleContainer.H"

namespace warpx::runtime
{

CurrentDepositionAlgo
RuntimeConfig::current_deposition_algo () const
{
    return WarpX::current_deposition_algo;
}

ChargeDepositionAlgo
RuntimeConfig::charge_deposition_algo () const
{
    return WarpX::charge_deposition_algo;
}

GatheringAlgo
RuntimeConfig::field_gathering_algo () const
{
    return WarpX::field_gathering_algo;
}

ParticlePusherAlgo
RuntimeConfig::particle_pusher_algo () const
{
    return WarpX::particle_pusher_algo;
}

ElectromagneticSolverAlgo
RuntimeConfig::electromagnetic_solver_id () const
{
    return WarpX::electromagnetic_solver_id;
}

ElectrostaticSolverAlgo
RuntimeConfig::electrostatic_solver_id () const
{
    return WarpX::electrostatic_solver_id;
}

PoissonSolverAlgo
RuntimeConfig::poisson_solver_id () const
{
    return WarpX::poisson_solver_id;
}

ablastr::utils::enums::GridType
RuntimeConfig::grid_type () const
{
    return WarpX::grid_type;
}

int
RuntimeConfig::ect_conformal_E () const
{
    return WarpX::ect_conformal_E;
}

amrex::Real
RuntimeConfig::ect_conformal_E_Astab () const
{
    return WarpX::ect_conformal_E_Astab;
}

int
RuntimeConfig::macroscopic_subpixel () const
{
    return WarpX::macroscopic_subpixel;
}

int
RuntimeConfig::ect_te_borrowing () const
{
    return WarpX::ect_te_borrowing;
}

const amrex::Array<FieldBoundaryType, AMREX_SPACEDIM>&
RuntimeConfig::field_boundary_lo () const
{
    return WarpX::field_boundary_lo;
}

const amrex::Array<FieldBoundaryType, AMREX_SPACEDIM>&
RuntimeConfig::field_boundary_hi () const
{
    return WarpX::field_boundary_hi;
}

const amrex::Array<ParticleBoundaryType, AMREX_SPACEDIM>&
RuntimeConfig::particle_boundary_lo () const
{
    return WarpX::particle_boundary_lo;
}

const amrex::Array<ParticleBoundaryType, AMREX_SPACEDIM>&
RuntimeConfig::particle_boundary_hi () const
{
    return WarpX::particle_boundary_hi;
}

ParticleBoundaryType
RuntimeConfig::eb_particle_boundary () const
{
    return WarpX::eb_particle_boundary;
}

TimeDependencyJ
RuntimeConfig::time_dependency_J () const
{
    return WarpX::time_dependency_J;
}

TimeDependencyRho
RuntimeConfig::time_dependency_rho () const
{
    return WarpX::time_dependency_rho;
}

bool
RuntimeConfig::do_single_precision_comms () const
{
    return WarpX::do_single_precision_comms;
}

bool
RuntimeConfig::do_shared_mem_charge_deposition () const
{
    return WarpX::do_shared_mem_charge_deposition;
}

bool
RuntimeConfig::do_shared_mem_current_deposition () const
{
    return WarpX::do_shared_mem_current_deposition;
}

bool
RuntimeConfig::do_dive_cleaning () const
{
    return WarpX::do_dive_cleaning;
}

bool
RuntimeConfig::do_divb_cleaning () const
{
    return WarpX::do_divb_cleaning;
}

bool
RuntimeConfig::use_fdtd_nci_corr () const
{
    return WarpX::use_fdtd_nci_corr;
}

bool
RuntimeConfig::galerkin_interpolation () const
{
    return WarpX::galerkin_interpolation;
}

bool
RuntimeConfig::use_filter () const
{
    return WarpX::use_filter;
}

bool
RuntimeConfig::use_kspace_filter () const
{
    return WarpX::use_kspace_filter;
}

bool
RuntimeConfig::use_filter_compensation () const
{
    return WarpX::use_filter_compensation;
}

bool
RuntimeConfig::serialize_initial_conditions () const
{
    return WarpX::serialize_initial_conditions;
}

bool
RuntimeConfig::compute_max_step_from_btd () const
{
    return WarpX::compute_max_step_from_btd;
}

bool
RuntimeConfig::do_dynamic_scheduling () const
{
    return WarpX::do_dynamic_scheduling;
}

bool
RuntimeConfig::refine_plasma () const
{
    return WarpX::refine_plasma;
}

const ablastr::utils::text::IntervalsParser&
RuntimeConfig::sort_intervals () const
{
    return WarpX::sort_intervals;
}

const amrex::IntVect&
RuntimeConfig::filter_npass_each_dir () const
{
    return WarpX::filter_npass_each_dir;
}

int
RuntimeConfig::n_rz_azimuthal_modes () const
{
#ifdef WARPX_DIM_RZ
    return WarpX::n_rz_azimuthal_modes;
#else
    return 1;
#endif
}

int
RuntimeConfig::nox () const
{
    return WarpX::nox;
}

int
RuntimeConfig::noy () const
{
    return WarpX::noy;
}

int
RuntimeConfig::noz () const
{
    return WarpX::noz;
}

int
RuntimeConfig::particle_max_grid_crossings () const
{
    return WarpX::particle_max_grid_crossings;
}

int
RuntimeConfig::villasenor_mass_matrices_max_grid_crossings () const
{
    return 3; // WarpX::villasenor_mass_matrices_max_grid_crossings is constexpr
}

int
RuntimeConfig::shared_mem_current_tpb () const
{
    return WarpX::shared_mem_current_tpb;
}

const amrex::IntVect&
RuntimeConfig::shared_tilesize () const
{
    return WarpX::shared_tilesize;
}

bool
RuntimeConfig::fft_do_time_averaging () const
{
    return WarpX::fft_do_time_averaging;
}

int
RuntimeConfig::ncomps () const
{
    return WarpX::ncomps;
}

bool
RuntimeConfig::sync_nodal_points () const
{
    return true; // WarpX::sync_nodal_points is a private constexpr true
}

LoadBalanceCostsUpdateAlgo
RuntimeConfig::load_balance_costs_update_algo () const
{
    return WarpX::load_balance_costs_update_algo;
}

int
RuntimeConfig::start_moving_window_step () const
{
    return WarpX::start_moving_window_step;
}

int
RuntimeConfig::end_moving_window_step () const
{
    return WarpX::end_moving_window_step;
}

const std::string&
RuntimeConfig::E_ext_particle_s () const
{
    // Read from the input deck, not from the particle container: the container is a plugin
    // and must not be the place a kernel-side configuration value comes from.
    static const std::string value = [] {
        std::string s = "none";
        amrex::ParmParse const pp("particles");
        pp.query("E_ext_particle_init_style", s);
        std::transform(s.begin(), s.end(), s.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return s;
    }();
    return value;
}

const std::string&
RuntimeConfig::B_ext_particle_s () const
{
    static const std::string value = [] {
        std::string s = "none";
        amrex::ParmParse const pp("particles");
        pp.query("B_ext_particle_init_style", s);
        std::transform(s.begin(), s.end(), s.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return s;
    }();
    return value;
}

amrex::Real
RuntimeConfig::moving_window_v () const
{
    return WarpX::moving_window_v;
}

int
RuntimeConfig::n_field_gather_buffer () const
{
    return WarpX::n_field_gather_buffer;
}

int
RuntimeConfig::n_current_deposition_buffer () const
{
    return WarpX::n_current_deposition_buffer;
}

bool
RuntimeConfig::particle_boundary_is_thermal () const
{
    for (int idim = 0; idim < AMREX_SPACEDIM; ++idim) {
        if (WarpX::particle_boundary_lo[idim] == ParticleBoundaryType::Thermal) { return true; }
        if (WarpX::particle_boundary_hi[idim] == ParticleBoundaryType::Thermal) { return true; }
    }
    return WarpX::eb_particle_boundary == ParticleBoundaryType::Thermal;
}

RuntimeConfig
RuntimeConfig::FromLegacy ()
{
    return RuntimeConfig{};
}

} // namespace warpx::runtime
