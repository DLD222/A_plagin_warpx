/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "Runtime/Scheduler.H"

#include "Runtime/DataBus.H"
#include "Runtime/StageRegistry.H"

namespace warpx::runtime
{

int
Scheduler::run (StageContext& ctx, int from, int to) const
{
    if (to <= from) { return 0; }
    int executed = 0;
    for (auto* handler : m_host.ordered()) {
        const HandlerSpec spec = handler->spec();
        if (spec.point.stage < from || spec.point.stage >= to) { continue; }
        // The bus checks every access against this declaration (S5).
        if (ctx.bus != nullptr) { ctx.bus->set_active_handler(&spec); }
        try {
            handler->apply(ctx);
        } catch (...) {
            if (ctx.bus != nullptr) { ctx.bus->clear_active_handler(); }
            throw;
        }
        if (ctx.bus != nullptr) { ctx.bus->clear_active_handler(); }
        ++executed;
    }
    return executed;
}

int
Scheduler::run_stage (StageContext& ctx, int stage) const
{
    return run(ctx, stage, stage + 1);
}

bool
Scheduler::has_handlers (int from, int to) const
{
    if (to <= from) { return false; }
    for (auto* handler : m_host.ordered()) {
        const int stage = handler->spec().point.stage;
        if (stage >= from && stage < to) { return true; }
    }
    return false;
}

} // namespace warpx::runtime
