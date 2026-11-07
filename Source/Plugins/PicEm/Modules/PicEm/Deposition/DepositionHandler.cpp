/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "Modules/PicEm/Deposition/DepositionHandler.H"

#include "Runtime/HostServices.H"
#include "Runtime/RuntimeData.H"

#include <stdexcept>

namespace warpx::runtime
{

HandlerSpec
DepositionHandler::spec () const
{
    return HandlerSpec{
        .name = "deposition.charge_current",
        .point = {Stage::AfterPush, 0},
        .reads_fields = {"current_fp", "rho_fp"},
        .writes_fields = {"current_fp", "rho_fp"},
        .reads_species = {},
        .writes_species = {},
        .priority = 0};
}

void
DepositionHandler::apply (StageContext& ctx)
{
    if (ctx.host == nullptr || ctx.data == nullptr || ctx.config == nullptr) {
        throw std::runtime_error("DepositionHandler: missing StageContext services");
    }
    ctx.host->particleServices().finalizeDeposition();
}

} // namespace warpx::runtime
