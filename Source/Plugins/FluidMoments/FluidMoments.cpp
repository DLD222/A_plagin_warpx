/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "FluidMoments.H"

namespace warpx::plugins::fluid
{

void advanceIdealGas (amrex::MultiFab& state, amrex::Geometry const& geom, int dir,
                      amrex::Real dt, amrex::Real gamma, int n_species)
{
    IdealGasEOS eos{gamma};
    StateLayout layout{n_species};
    advanceSweep(state, geom, dir, dt, eos, layout);
}

} // namespace warpx::plugins::fluid
