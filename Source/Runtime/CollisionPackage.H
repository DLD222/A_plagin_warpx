/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_RUNTIME_COLLISIONPACKAGE_H_
#define WARPX_RUNTIME_COLLISIONPACKAGE_H_

#include "Runtime/HandlerSpec.H"

namespace warpx::runtime
{

/**
 * \brief Collision package schema (A3.3b).
 *
 * Declares collision handlers at Stage::Collisions. Execution handlers are
 * intentionally not registered yet: CollisionHandler still owns a
 * MultiParticleContainer pointer, so wrapping it would hide the old coupling.
 */
class CollisionPackage
{
public:
    static void register_specs (PluginHost& host)
    {
        host.declare(HandlerSpec{
            .name = "binary_collision",
            .point = {Stage::Collisions, 0},
            .reads_fields = {},
            .writes_fields = {},
            .reads_species = {"electrons", "ions"},
            .writes_species = {"electrons", "ions", "products"},
            .priority = 0});

        host.declare(HandlerSpec{
            .name = "mcc_collision",
            .point = {Stage::Collisions, 1},
            .reads_fields = {},
            .writes_fields = {},
            .reads_species = {"electrons", "gas"},
            .writes_species = {"electrons", "gas"},
            .priority = 0});

        host.declare(HandlerSpec{
            .name = "dsmc_collision",
            .point = {Stage::Collisions, 2},
            .reads_fields = {},
            .writes_fields = {},
            .reads_species = {"gas"},
            .writes_species = {"gas"},
            .priority = 0});
    }
};

} // namespace warpx::runtime

#endif // WARPX_RUNTIME_COLLISIONPACKAGE_H_
