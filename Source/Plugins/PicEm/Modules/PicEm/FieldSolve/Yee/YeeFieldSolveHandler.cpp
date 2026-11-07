/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "Modules/PicEm/FieldSolve/Yee/YeeFieldSolveHandler.H"

#include "Runtime/HostServices.H"
#include "Runtime/RuntimeData.H"

#include <stdexcept>

namespace warpx::runtime
{

HandlerSpec
YeeFieldSolveHandler::spec () const
{
    return HandlerSpec{
        .name = "field_solve.yee",
        .point = {Stage::FieldSolve, 0},
        .reads_fields = {"current_fp", "rho_fp"},
        .writes_fields = {"Efield_fp", "Bfield_fp"},
        .reads_species = {},
        .writes_species = {},
        .priority = 0};
}

void
YeeFieldSolveHandler::apply (StageContext& ctx)
{
    if (ctx.host == nullptr || ctx.data == nullptr || ctx.config == nullptr) {
        throw std::runtime_error("YeeFieldSolveHandler: missing StageContext services");
    }
    ctx.host->fieldAlgebra().fieldSolve(ctx.time);
}

} // namespace warpx::runtime
