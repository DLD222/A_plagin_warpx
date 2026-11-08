/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_RUNTIME_STAGEPOINT_H_
#define WARPX_RUNTIME_STAGEPOINT_H_

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

namespace warpx::runtime
{

/**
 * \brief Standard stage ordinals of the runtime.
 *
 * A stage is an integer ordinal, not an enum: plugins may register additional
 * stages between the standard ones without recompiling the core (S2).  The
 * standard stages are spaced by `StageStride` so a registration can be placed
 * in the gap with a single integer, and existing ordinals never move.
 *
 * Only the execution engine decides *when* a stage runs; declaring a handler
 * at an ordinal that the engine never reaches simply means it never runs.
 */
namespace Stage
{
inline constexpr int Stride = 1000;

inline constexpr int BeforeInit     =  0 * Stride;
inline constexpr int Init           =  1 * Stride;
inline constexpr int AfterInit      =  2 * Stride;
inline constexpr int BeforePush     =  3 * Stride;
inline constexpr int Push           =  4 * Stride;
inline constexpr int AfterPush      =  5 * Stride;
inline constexpr int BeforeCollisions =  6 * Stride;
inline constexpr int Collisions       =  7 * Stride;
inline constexpr int AfterCollisions  =  8 * Stride;
inline constexpr int BeforeFieldSolve =  9 * Stride;
inline constexpr int FieldSolve       = 10 * Stride;
inline constexpr int AfterFieldSolve  = 11 * Stride;
inline constexpr int Diagnostics      = 12 * Stride;
inline constexpr int Finalize         = 13 * Stride;

//! True for the ordinals above (used to validate plugin-provided integers).
[[nodiscard]] inline constexpr bool is_standard (int stage)
{
    return stage >= BeforeInit && stage <= Finalize && stage % Stride == 0;
}
} // namespace Stage

/** A concrete point in the step order: a stage plus a sub-stage index. */
struct StagePoint
{
    int stage = Stage::BeforeInit;
    int sub = 0;
};

inline bool operator< (const StagePoint& a, const StagePoint& b)
{
    if (a.stage != b.stage) { return a.stage < b.stage; }
    return a.sub < b.sub;
}

inline bool operator== (const StagePoint& a, const StagePoint& b)
{
    return a.stage == b.stage && a.sub == b.sub;
}

/**
 * \brief Ordered list of named stage entries (planning aid).
 *
 * It only orders by StagePoint; handler dependency sorting is done by the
 * plugin host.
 */
class StagePlan
{
public:
    void add (const StagePoint& point, std::string name)
    {
        m_entries.emplace_back(point, std::move(name));
    }

    [[nodiscard]] std::vector<std::pair<StagePoint, std::string>> sorted () const
    {
        auto out = m_entries;
        std::stable_sort(out.begin(), out.end(),
            [](const auto& a, const auto& b) { return a.first < b.first; });
        return out;
    }

    [[nodiscard]] std::size_t size () const { return m_entries.size(); }

private:
    std::vector<std::pair<StagePoint, std::string>> m_entries;
};

} // namespace warpx::runtime

#endif // WARPX_RUNTIME_STAGEPOINT_H_
