/* Copyright 2026 The WarpX Community
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause-LBNL
 */
#ifndef WARPX_PETSCREINITIALIZE_H_
#define WARPX_PETSCREINITIALIZE_H_

#include <functional>

namespace warpx::petsc
{

/** Register the function that tears PETSc down and back up.
 *
 * The linear and nonlinear solver wrappers in the core need to reset PETSc between
 * solves, but the function that does it owns the MPI command line and therefore lives
 * with the simulation, not with the core.  The kernel registers it once during start-up.
 */
void set_reinitializer (std::function<void ()> a_reinitializer);

/** Reset PETSc through the registered function.
 *
 * Does nothing when no function was registered, which is the case for a core-only
 * build or when PETSc was never initialized by the simulation.
 */
void reinitialize ();

} // namespace warpx::petsc

#endif // WARPX_PETSCREINITIALIZE_H_
