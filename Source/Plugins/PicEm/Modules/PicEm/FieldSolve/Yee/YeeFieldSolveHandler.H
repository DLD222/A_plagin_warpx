/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_MODULES_PICEM_FIELDSOLVE_YEE_YEEFIELDSOLVEHANDLER_H_
#define WARPX_MODULES_PICEM_FIELDSOLVE_YEE_YEEFIELDSOLVEHANDLER_H_

#include "Runtime/HandlerSpec.H"

namespace warpx::runtime
{

/** \brief Yee Maxwell field update (one unit module). */
class YeeFieldSolveHandler final : public IHandler
{
public:
    [[nodiscard]] HandlerSpec spec () const override;
    void apply (StageContext& ctx) override;
};

} // namespace warpx::runtime

#endif
