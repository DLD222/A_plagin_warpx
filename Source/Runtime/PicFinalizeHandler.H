/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_RUNTIME_PICFINALIZEHANDLER_H_
#define WARPX_RUNTIME_PICFINALIZEHANDLER_H_

#include "Runtime/HandlerSpec.H"
#include "Runtime/HostServices.H"

#include <stdexcept>

namespace warpx::runtime
{

/** \brief Finalize current/rho after deposition (A3.3b5). */
class PicFinalizeHandler final : public IHandler
{
public:
    [[nodiscard]] HandlerSpec spec () const override
    {
        return HandlerSpec{
            .name = "pic_finalize_deposition",
            .point = {Stage::AfterPush, 0},
            .reads_fields = {"current_fp", "rho_fp"},
            .writes_fields = {"current_fp", "rho_fp"},
            .reads_species = {},
            .writes_species = {},
            .priority = 0};
    }

    void apply (StageContext& ctx) override
    {
        if (ctx.host == nullptr) {
            throw std::runtime_error("PicFinalizeHandler: missing HostServices");
        }
        ctx.host->finalizeDeposition();
    }
};

} // namespace warpx::runtime

#endif // WARPX_RUNTIME_PICFINALIZEHANDLER_H_
