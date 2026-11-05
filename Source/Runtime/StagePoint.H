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

enum class Stage
{
    BeforeInit,
    Init,
    AfterInit,
    BeforePush,
    Push,
    AfterPush,
    BeforeCollisions,
    Collisions,
    AfterCollisions,
    BeforeFieldSolve,
    FieldSolve,
    AfterFieldSolve,
    Diagnostics,
    Finalize
};

struct StagePoint
{
    Stage stage = Stage::BeforeInit;
    int sub = 0;
};

inline bool operator< (const StagePoint& a, const StagePoint& b)
{
    if (a.stage != b.stage) { return static_cast<int>(a.stage) < static_cast<int>(b.stage); }
    return a.sub < b.sub;
}

inline bool operator== (const StagePoint& a, const StagePoint& b)
{
    return a.stage == b.stage && a.sub == b.sub;
}

/**
 * \brief Ordered list of named stage entries (A3.1a skeleton).
 *
 * It only orders by StagePoint; handler dependency sorting is added in A3.1c.
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
