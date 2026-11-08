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
#include "Runtime/ICollisionHandler.H"

#include <stdexcept>

class ICollisionHandler;

namespace warpx::runtime
{

/** \brief Non-owning adapter from PluginHost to CollisionHandler (A3.3b). */
class CollisionHandlerAdapter final : public IHandler
{
public:
    explicit CollisionHandlerAdapter (ICollisionHandler* handler) : m_handler(handler) {}

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
        if (m_handler == nullptr) { return; }
        if (m_handler == nullptr) {
            throw std::runtime_error("CollisionHandlerAdapter: null handler");
        }
        m_handler->doCollisions(ctx.step, ctx.time, ctx.dt);
    }

private:
    ICollisionHandler* m_handler;
};

/** \brief Registers the collision handler's state declarations with the core.
 *
 * The handler itself is created by the kernel, not by a plugin entry point, so
 * this thin lifecycle object gives its on_schema hook a place in the plugin
 * host: plugin state must exist before diagnostics resolve field names.
 */
class CollisionStatePlugin final : public IPluginLifecycle
{
public:
    explicit CollisionStatePlugin (ICollisionHandler* handler) : m_handler(handler) {}

    [[nodiscard]] PluginManifest manifest () const override
    {
        return PluginManifest{.name = "collision_state", .version = "1.0.0",
                              .role = PluginRole::Physics};
    }

    void on_schema (SchemaContext& ctx) override
    {
        if (m_handler != nullptr && ctx.state != nullptr) {
            m_handler->declare_state(*ctx.state);
        }
    }

private:
    ICollisionHandler* m_handler;
};

} // namespace warpx::runtime

#endif // WARPX_RUNTIME_COLLISIONHANDLERADAPTER_H_
