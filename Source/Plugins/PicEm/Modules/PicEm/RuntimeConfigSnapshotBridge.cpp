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
    return snapshot;
}

} // namespace warpx::runtime
