/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_RUNTIME_COLLISIONHANDLERADAPTER_H_
#define WARPX_RUNTIME_COLLISIONHANDLERADAPTER_H_

#include "Runtime/HandlerSpec.H"

#include <stdexcept>

class CollisionHandler;

namespace warpx::runtime
{

/** \brief Non-owning adapter from PluginHost to CollisionHandler (A3.3b). */
class CollisionHandlerAdapter final : public IHandler
{
public:
    explicit CollisionHandlerAdapter (CollisionHandler* handler) : m_handler(handler) {}

    [[nodiscard]] HandlerSpec spec () const override
    {
        return HandlerSpec{
            .name = "collision_handler",
            .point = {Stage::Collisions, 0},
            .reads_fields = {},
            .writes_fields = {},
            .reads_species = {"electrons", "ions", "gas", "photons"},
            .writes_species = {"electrons", "ions", "gas", "photons"},
            .priority = 0};
    }

    void apply (StageContext& ctx) override
    {
        if (m_handler == nullptr) {
            throw std::runtime_error("CollisionHandlerAdapter: null handler");
        }
        m_handler->doCollisions(ctx.step, ctx.time, ctx.dt);
    }

private:
    CollisionHandler* m_handler;
};

} // namespace warpx::runtime

#endif // WARPX_RUNTIME_COLLISIONHANDLERADAPTER_H_
