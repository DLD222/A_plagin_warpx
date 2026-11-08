/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_RUNTIME_HYBRIDPICPARAMETERS_H_
#define WARPX_RUNTIME_HYBRIDPICPARAMETERS_H_

#include <AMReX_Enum.H>
#include <AMReX_Parser.H>
#include <AMReX_GpuContainers.H>
#include <AMReX_REAL.H>

namespace warpx::runtime
{

/** Sampling of the density that decides the vacuum-seam treatment of the
 *  generalized Ohm's law. */
AMREX_ENUM(VacuumSeamSwitchMode,
    Edge, /**< legacy per-component edge average */
    Node, /**< minimum over the component's edge-endpoint nodes */
    Cell  /**< minimum over the component's adjacent cells */
);

/**
 * \brief The parameter block a hybrid field update needs (S6).
 *
 * The hybrid model owns these values; a field solver that advances E for it
 * receives this plain structure, so it never needs the model class.  Keeping
 * the block flat also makes it cheap to pass to device code.
 */
struct HybridPICParameters
{
    amrex::GpuArray<int, 3> Jx_IndexType{};
    amrex::GpuArray<int, 3> Jy_IndexType{};
    amrex::GpuArray<int, 3> Jz_IndexType{};
    amrex::GpuArray<int, 3> Bx_IndexType{};
    amrex::GpuArray<int, 3> By_IndexType{};
    amrex::GpuArray<int, 3> Bz_IndexType{};
    amrex::GpuArray<int, 3> Ex_IndexType{};
    amrex::GpuArray<int, 3> Ey_IndexType{};
    amrex::GpuArray<int, 3> Ez_IndexType{};

    //! Resistivity parsers: eta(rho, |J|, t) and the hyper-resistivity
    //! eta_h(rho, |B|).  ParserExecutor is device-copyable, so this block is
    //! safe to hand to kernels.
    amrex::ParserExecutor<3> eta{};
    amrex::ParserExecutor<2> eta_h{};
    amrex::Real n_floor = 1.0;

    bool has_per_species_eta = false;
    bool has_resistive_drag = false;
    bool add_external_fields = false;
    bool holmstrom_vacuum_region = false;
    bool include_hyper_resistivity_term = false;
    bool hyper_resistivity_has_B_dependence = false;
    bool resistivity_has_J_dependence = false;
    bool solve_electron_energy_equation = false;

    VacuumSeamSwitchMode vacuum_seam_switch_mode = VacuumSeamSwitchMode::Edge;
};

} // namespace warpx::runtime

#endif // WARPX_RUNTIME_HYBRIDPICPARAMETERS_H_
