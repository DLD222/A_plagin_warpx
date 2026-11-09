/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_RUNTIME_IMPLICITKERNELSERVICES_H_
#define WARPX_RUNTIME_IMPLICITKERNELSERVICES_H_

#include "Fields.H"
#include "Modules/FieldSolver/ImplicitOptions.H"
#include "Runtime/IParticleContainerAccess.H"
#include "Runtime/IFiniteDifferenceSolver.H"
#include "Solvers/WarpXSolverVec.H"
#include "Utils/WarpXAlgorithmSelection.H"

#include <ablastr/fields/MultiFabRegister.H>
#include <ablastr/utils/Enums.H>

#include <AMReX_Config.H>
#include <AMReX_Geometry.H>
#include <AMReX_MultiFab.H>
#include <AMReX_REAL.H>

#include <functional>
#include <string>

namespace warpx::runtime
{

/**
 * \brief The operations an implicit solver drives on the simulation.
 *
 * An implicit step is not a closed computation: it pushes particles, deposits,
 * evolves the fields and applies boundaries at points the solver chooses, then
 * asks the linear solve to invert.  Those actions belong to the simulation, so
 * the solver reaches them through this bag of callbacks instead of naming the
 * simulation class.  Nothing here is implicit-solver-specific: each entry is one
 * operation the kernel already implements.
 */
struct ImplicitKernelServices
{
    // ---- data the solver reads -------------------------------------------
    ablastr::fields::MultiFabRegister* fields = nullptr;
    //! The species (their particle containers) the solver pushes.
    warpx::runtime::IParticleContainerAccess* particles = nullptr;
    std::function<amrex::Geometry const& (int lev)> geom;
    std::function<amrex::IntVect ()> ng_eb;
    std::function<int (int dir, int side)> pec_insulator_is_e;
    std::function<amrex::Array<FieldBoundaryType, AMREX_SPACEDIM> const& ()> field_boundary_lo;
    std::function<amrex::Array<FieldBoundaryType, AMREX_SPACEDIM> const& ()> field_boundary_hi;
    std::function<bool ()> verbose;
    std::function<int ()> finest_level;
    std::function<amrex::iMultiFab const* (warpx::fields::FieldType, int lev, int dir)> field_dot_mask;
    std::function<void (ablastr::fields::MultiLevelVectorField const&, int lev)> apply_filter_mf;
    std::function<amrex::Vector<std::array<std::unique_ptr<amrex::iMultiFab>, 3>> const& ()> eb_update_e_flag;
    std::function<amrex::Vector<std::array<std::unique_ptr<amrex::iMultiFab>, 3>> const& ()> eb_update_b_flag;
    std::function<warpx::runtime::IFiniteDifferenceSolver* (int lev)> fdtd_solver;
    std::function<void (ablastr::fields::MultiFabRegister&, int lev)> apply_inverse_volume_scaling_current;
    std::function<void (ablastr::fields::MultiFabRegister&, int lev)> apply_inverse_volume_scaling_mass_matrices_pc;

    // ---- actions the solver triggers -------------------------------------
    std::function<void (amrex::Real dt, SubcyclingHalf half, amrex::Real start_time)> evolve_b;
    std::function<void (amrex::IntVect const& ng, bool nodal_sync)> fill_boundary_b;
    std::function<void (amrex::IntVect const& ng, bool nodal_sync)> fill_boundary_e;
    std::function<void (int lev, ablastr::utils::enums::PatchType patch_type, amrex::Real cur_time)> apply_efield_boundary;
    std::function<void (int lev, amrex::MultiFab* jx, amrex::MultiFab* jy, amrex::MultiFab* jz,
                        ablastr::utils::enums::PatchType patch_type)> apply_jfield_boundary;
    std::function<void ()> finalize_j;
    std::function<void ()> finalize_rho;
    std::function<void (char const* which)> sync_current;
    std::function<void ()> sync_current_and_rho;
    std::function<void ()> sync_mass_matrices;
    std::function<void ()> sync_mass_matrices_pc;
    std::function<void (amrex::Real dt, ImplicitOptions const& options)> deposit_mass_matrices;
    std::function<void (amrex::Real dt, WarpXSolverVec& rhse)> implicit_compute_rhse;
    std::function<void (amrex::Real cur_time, bool skip_deposition,
                        PositionPushType position_push, MomentumPushType momentum_push,
                        amrex::Real dt_scale, ImplicitOptions const* options)> push_particles_and_deposit;
    std::function<void (amrex::Real time)> save_particles_at_implicit_step_start;
    std::function<void ()> reset_implicit_particle_data;
    std::function<void (amrex::Real time)> finish_implicit_particle_update;
    std::function<void (int step, amrex::Real cur_time, int num_moved)> handle_particles_at_boundaries;
    std::function<void (WarpXSolverVec const& e, amrex::Real time)> set_electric_field_and_apply_bcs;
    std::function<void (amrex::Real theta, amrex::Real time)> finish_electric_field_and_apply_bcs;
    std::function<void (amrex::Real theta, amrex::Real time)> finish_magnetic_field_and_apply_bcs;
    std::function<void (amrex::Real start_time)> spectral_source_free_field_advance;
    //! Push the momentum of the sub-orbit particles on one level.
    std::function<void (int lev, amrex::Real dt,
                        ablastr::fields::VectorField const& E,
                        ablastr::fields::VectorField const& B)> push_particles_momentum;
    //! Push the positions of every species.
    std::function<void (amrex::Real dt)> push_particles_position;
    //! Attributes a species exposes (used to detect sub-orbit bookkeeping).
    std::function<bool (int ispecies, std::string const& attrib)> species_has_attrib;
    //! Number of sub-orbit attributes of a species on one level.
    std::function<void (int ispecies, int lev, amrex::Gpu::Buffer<amrex::Long>& counter)> count_suborbits;
    std::function<void (int step, amrex::Real dt)> compute_diags_mid_step;
};

} // namespace warpx::runtime

#endif // WARPX_RUNTIME_IMPLICITKERNELSERVICES_H_
