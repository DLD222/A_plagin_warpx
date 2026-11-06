/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_RUNTIME_PICFIELDSOLVEHANDLER_H_
#define WARPX_RUNTIME_PICFIELDSOLVEHANDLER_H_

#include "Runtime/HandlerSpec.H"
#include "Runtime/HostServices.H"
#include "Runtime/RuntimeData.H"

#include <stdexcept>

namespace warpx::runtime
{

/** \brief Explicit PIC E/B advance (A3.3b6). */
class PicFieldSolveHandler final : public IHandler
{
public:
    [[nodiscard]] HandlerSpec spec () const override
    {
        return HandlerSpec{
            .name = "pic_field_solve",
            .point = {Stage::FieldSolve, 0},
            .reads_fields = {"current_fp", "rho_fp"},
            .writes_fields = {"Efield_fp", "Bfield_fp"},
            .reads_species = {},
            .writes_species = {},
            .priority = 0};
    }

    void apply (StageContext& ctx) override
    {
        if (ctx.host == nullptr || ctx.data == nullptr) {
            throw std::runtime_error("PicFieldSolveHandler: missing StageContext services");
        }
        ctx.host->fieldSolve(ctx.time);
    }
};

} // namespace warpx::runtime

#endif // WARPX_RUNTIME_PICFIELDSOLVEHANDLER_H_
