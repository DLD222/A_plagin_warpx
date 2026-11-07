/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_RUNTIME_DIAGNOSTICSHANDLER_H_
#define WARPX_RUNTIME_DIAGNOSTICSHANDLER_H_

#include "Runtime/HandlerSpec.H"
#include "Runtime/HostServices.H"

#include <stdexcept>

namespace warpx::runtime
{

class DiagnosticsHandler final : public IHandler
{
public:
    [[nodiscard]] HandlerSpec spec () const override
    {
        return HandlerSpec{
            .name = "diagnostics",
            .point = {Stage::Diagnostics, 0},
            .reads_fields = {},
            .writes_fields = {},
            .reads_species = {},
            .writes_species = {},
            .priority = 0};
    }

    void apply (StageContext& ctx) override
    {
        if (ctx.host == nullptr) {
            throw std::runtime_error("DiagnosticsHandler: missing host services");
        }
        ctx.host->diagnosticsMetadata().computeDiagnostics(ctx.step);
    }
};

} // namespace warpx::runtime

#endif // WARPX_RUNTIME_DIAGNOSTICSHANDLER_H_
