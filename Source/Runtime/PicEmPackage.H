/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_RUNTIME_PICEMPACKAGE_H_
#define WARPX_RUNTIME_PICEMPACKAGE_H_

#include "Runtime/HandlerSpec.H"
#include "Runtime/PicPushHandler.H"
#include "Runtime/PicFinalizeHandler.H"
#include "Runtime/PicFieldSolveHandler.H"

namespace warpx::runtime
{

/**
 * \brief Built-in PIC/EM package schema (A3.3b1).
 *
 * This stage intentionally declares only HandlerSpecs. It does not own a
 * WarpX pointer and does not execute physics yet: execution handlers are added
 * once the PIC phase state lives in StageContext/RuntimeData.
 */
class PicEmPackage
{
public:
    static void register_handlers (PluginHost& host, const std::string& field_solver_module)
    {
        host.add(std::make_shared<PicPushHandler>());
        host.add(std::make_shared<PicFinalizeHandler>());
        host.add(std::make_shared<PicFieldSolveHandler>(field_solver_module));
    }

    static void register_specs (PluginHost& host,
                                const std::string& field_solver_module = "field_solve.yee")
    {
        host.declare(HandlerSpec{
            .name = "particle_push",
            .point = {Stage::Push, 0},
            .reads_fields = {},
            .writes_fields = {"current_fp", "rho_fp"},
            .reads_species = {},
            .writes_species = {},
            .priority = 0});

        host.declare(HandlerSpec{
            .name = "deposition.charge_current",
            .point = {Stage::AfterPush, 0},
            .reads_fields = {"current_fp", "rho_fp"},
            .writes_fields = {"current_fp", "rho_fp"},
            .reads_species = {},
            .writes_species = {},
            .priority = 0});

        host.declare(HandlerSpec{
            .name = field_solver_module,
            .point = {Stage::FieldSolve, 0},
            .reads_fields = {"current_fp", "rho_fp"},
            .writes_fields = {"Efield_fp", "Bfield_fp"},
            .reads_species = {},
            .writes_species = {},
            .priority = 0});
    }
};

} // namespace warpx::runtime

#endif // WARPX_RUNTIME_PICEMPACKAGE_H_
