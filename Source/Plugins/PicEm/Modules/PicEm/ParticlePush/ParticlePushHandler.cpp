/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "Modules/PicEm/ParticlePush/ParticlePushHandler.H"

#include "Runtime/HostServices.H"
#include "Runtime/RuntimeData.H"

#include <stdexcept>

namespace warpx::runtime
{

HandlerSpec
ParticlePushHandler::spec () const
{
    return HandlerSpec{
        .name = "particle_push",
        .point = {Stage::Push, 0},
        .reads_fields = {},
        .writes_fields = {"current_fp", "rho_fp"},
        .reads_species = {},
        .writes_species = {},
        .priority = 0};
}

void
ParticlePushHandler::apply (StageContext& ctx)
{
    if (ctx.host == nullptr || ctx.data == nullptr || ctx.config == nullptr) {
        throw std::runtime_error("ParticlePushHandler: missing StageContext services");
    }
    ctx.host->pushParticlesAndDeposit(ctx.time, ctx.dt, ctx.step);
}

} // namespace warpx::runtime
