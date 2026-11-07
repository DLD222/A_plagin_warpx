/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_RUNTIME_MATERIALSPEC_H_
#define WARPX_RUNTIME_MATERIALSPEC_H_

#include <AMReX_GpuContainers.H>
#include <AMReX_REAL.H>

#include <string>

namespace warpx::runtime
{

/** \brief Pure coefficient snapshot of one material (A4.3).
 *
 * It does not own the per-level MultiFabs; MacroscopicProperties keeps those.
 */
struct MaterialSpec
{
    std::string name = "default";
    amrex::Real epsilon = 0.0;
    amrex::Real sigma = 0.0;
    amrex::Real mu = 0.0;
    amrex::Real epsilon2 = 0.0;
    bool has_interface = false;
    bool has_eps_tensor = false;
    bool has_eps2_tensor = false;
    bool has_sigma_tensor = false;
    amrex::GpuArray<amrex::Real, 6> eps_tensor{};
    amrex::GpuArray<amrex::Real, 6> eps2_tensor{};
    amrex::GpuArray<amrex::Real, 6> sigma_tensor{};
};

} // namespace warpx::runtime

#endif // WARPX_RUNTIME_MATERIALSPEC_H_
