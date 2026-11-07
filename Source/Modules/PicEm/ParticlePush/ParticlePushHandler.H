/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_MODULES_PICEM_PARTICLEPUSH_PARTICLEPUSHHANDLER_H_
#define WARPX_MODULES_PICEM_PARTICLEPUSH_PARTICLEPUSHHANDLER_H_

#include "Runtime/HandlerSpec.H"

namespace warpx::runtime
{

/** \brief PIC particle push and current/charge deposition (one unit module). */
class ParticlePushHandler final : public IHandler
{
public:
    [[nodiscard]] HandlerSpec spec () const override;
    void apply (StageContext& ctx) override;
};

} // namespace warpx::runtime

#endif
