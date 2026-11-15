/* Copyright 2025 The WarpX Community
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause-LBNL
 */
#ifndef WARPX_MODULES_PARTICLES_MASSMATRIXLIMITS_H_
#define WARPX_MODULES_PARTICLES_MASSMATRIXLIMITS_H_

namespace warpx::mass_matrices
{
    /** Maximum number of grid crossings the Villasenor mass-matrix kernels are
     *  compiled for.
     *
     *  This is a compile-time constant by construction: the kernels hold
     *  fixed-size arrays of that size. It is the single source of truth for both
     *  the kernel capacity (the template arguments) and the runtime check
     *  `particles.max_grid_crossings <= ...` in the implicit solver, so the two
     *  cannot drift apart and silently overflow the kernels' arrays.
     */
    inline constexpr int max_grid_crossings = 3;
}

#endif // WARPX_MODULES_PARTICLES_MASSMATRIXLIMITS_H_
