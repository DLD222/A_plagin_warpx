/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_RUNTIME_PICPUSHHANDLER_H_
#define WARPX_RUNTIME_PICPUSHHANDLER_H_

#include "Runtime/HandlerSpec.H"
#include "Runtime/HostServices.H"
#include "Runtime/RuntimeData.H"

#include <stdexcept>

namespace warpx::runtime
{

/**
 * \brief Explicit PIC push/deposit handler (A3.3b3).
 *
 * It holds no WarpX pointer: only StageContext (HostServices + RuntimeData).
 */
class PicPushHandler final : public IHandler
{
public:
    [[nodiscard]] HandlerSpec spec () const override
    {
        return HandlerSpec{
            .name = "pic_push",
            .point = {Stage::Push, 0},
            .reads_fields = {},
            .writes_fields = {"current_fp", "rho_fp"},
            .reads_species = {"electrons", "ions", "photons"},
            .writes_species = {"electrons", "ions", "photons"},
            .priority = 0};
    }

    void apply (StageContext& ctx) override
    {
        if (ctx.host == nullptr || ctx.data == nullptr) {
            throw std::runtime_error("PicPushHandler: missing StageContext services");
        }
        constexpr int lev = 0;
        ctx.host->pushParticlesAndDeposit(ctx.data->t_new(lev), ctx.data->dt(lev),
                                          ctx.data->istep(lev));
    }
};

} // namespace warpx::runtime

#endif // WARPX_RUNTIME_PICPUSHHANDLER_H_
