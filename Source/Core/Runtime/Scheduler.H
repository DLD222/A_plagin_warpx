/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_RUNTIME_SCHEDULER_H_
#define WARPX_RUNTIME_SCHEDULER_H_

#include "Runtime/HandlerSpec.H"

namespace warpx::runtime
{

/**
 * \brief Executes registered handlers over a range of stages (S1).
 *
 * The kernel no longer asks for one stage at a time: it names an interval and
 * the scheduler runs *every* handler whose stage falls inside it, in stage
 * order.  A plugin that registers a stage between two standard stages is
 * therefore executed without any change to the core or to the kernel.
 *
 * Intervals are half-open: `[from, to)`.  Both bounds are ordinals, normally
 * taken from the Stage::* constants (or StageRegistry lookups for custom
 * stages).
 */
class Scheduler
{
public:
    explicit Scheduler (PluginHost& host) : m_host(host) {}

    //! Run every handler with from <= stage < to. Returns the number run.
    int run (StageContext& ctx, int from, int to) const;

    //! Run every handler whose stage equals `stage`. Returns the number run.
    int run_stage (StageContext& ctx, int stage) const;

    //! True when at least one handler would run in [from, to).
    [[nodiscard]] bool has_handlers (int from, int to) const;

private:
    PluginHost& m_host;
};

} // namespace warpx::runtime

#endif // WARPX_RUNTIME_SCHEDULER_H_
