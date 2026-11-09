/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_RUNTIME_FIELDGATHERORDERS_H_
#define WARPX_RUNTIME_FIELDGATHERORDERS_H_

namespace warpx::particles
{

/**
 * \brief Particle shape orders used when gathering the fields for a push.
 *
 * A plain description of how a pusher samples E and B: the E shape order, whether
 * E uses Galerkin reduction, and the B shape order.  It travels with the push
 * request, so it lives where both the kernel and a solver can name it.
 */
struct FieldGatherOrders
{
    int e_shape;
    bool e_galerkin;
    int b_shape;
};

} // namespace warpx::particles

#endif // WARPX_RUNTIME_FIELDGATHERORDERS_H_
