/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "Modules/PicEm/PicEmPackage.H"

#include "Modules/PicEm/ParticlePush/ParticlePushHandler.H"
#include "Modules/PicEm/Deposition/DepositionHandler.H"
#include "Modules/PicEm/FieldSolve/FieldSolveHandler.H"
#include "Modules/PicEm/FieldSolve/Yee/YeePackage.H"

#include <memory>

namespace warpx::runtime
{

void
PicEmPackage::register_handlers (PluginHost& host,
                                 const std::string& field_solver_module,
                                 const std::set<std::string>& disabled)
{
    if (!disabled.contains("particle_push")) {
        host.add(std::make_shared<ParticlePushHandler>());
    }
    if (!disabled.contains("deposition.charge_current")) {
        host.add(std::make_shared<DepositionHandler>());
    }
    if (!disabled.contains(field_solver_module)) {
        if (field_solver_module == "field_solve.yee") {
            YeePackage::register_handlers(host);
        } else {
            host.add(std::make_shared<FieldSolveHandler>(field_solver_module));
        }
    }
}

void
PicEmPackage::register_specs (PluginHost& host, const std::string& field_solver_module)
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

} // namespace warpx::runtime
