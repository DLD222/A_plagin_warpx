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
    static void register_specs (PluginHost& host)
    {
        host.declare(HandlerSpec{
            .name = "pic_push",
            .point = {Stage::Push, 0},
            .reads_fields = {},
            .writes_fields = {"current_fp", "rho_fp"},
            .reads_species = {"electrons", "ions", "photons"},
            .writes_species = {"electrons", "ions", "photons"},
            .priority = 0});

        host.declare(HandlerSpec{
            .name = "pic_finalize_deposition",
            .point = {Stage::AfterPush, 0},
            .reads_fields = {"current_fp", "rho_fp"},
            .writes_fields = {"current_fp", "rho_fp"},
            .reads_species = {},
            .writes_species = {},
            .priority = 0});

        host.declare(HandlerSpec{
            .name = "pic_field_solve",
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
