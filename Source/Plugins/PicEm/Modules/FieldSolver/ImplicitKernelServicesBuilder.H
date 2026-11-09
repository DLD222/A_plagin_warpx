/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_IMPLICITKERNELSERVICESBUILDER_H_
#define WARPX_IMPLICITKERNELSERVICESBUILDER_H_

#include "Runtime/ImplicitKernelServices.H"

class WarpX;

/**
 * \brief Fill the implicit solver's service bag from the simulation.
 *
 * This is the only place that knows both sides: the solver sees the bag, the
 * kernel fills it.  The callbacks capture \c warpx by reference, so the bag must
 * not outlive the simulation (it is owned by the solver, which the simulation
 * owns).
 */
warpx::runtime::ImplicitKernelServices BuildImplicitKernelServices (WarpX& warpx);

#endif // WARPX_IMPLICITKERNELSERVICESBUILDER_H_
