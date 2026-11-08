/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "Runtime/StageRegistry.H"

#include <algorithm>
#include <stdexcept>
#include <string>

namespace warpx::runtime
{
namespace
{
struct StandardStage
{
    int ordinal;
    const char* name;
};

//! Standard stages, in execution order.  Names are the public identifiers.
constexpr StandardStage k_standard_stages[] = {
    {Stage::BeforeInit, "before_init"},
    {Stage::Init, "init"},
    {Stage::AfterInit, "after_init"},
    {Stage::BeforePush, "before_push"},
    {Stage::Push, "push"},
    {Stage::AfterPush, "after_push"},
    {Stage::BeforeCollisions, "before_collisions"},
    {Stage::Collisions, "collisions"},
    {Stage::AfterCollisions, "after_collisions"},
    {Stage::BeforeFieldSolve, "before_field_solve"},
    {Stage::FieldSolve, "field_solve"},
    {Stage::AfterFieldSolve, "after_field_solve"},
    {Stage::Diagnostics, "diagnostics"},
    {Stage::Finalize, "finalize"},
};
} // namespace

StageRegistry::StageRegistry ()
{
    seed_standard_stages();
}

StageRegistry&
StageRegistry::instance ()
{
    static StageRegistry registry;
    return registry;
}

void
StageRegistry::seed_standard_stages ()
{
    m_stages.clear();
    for (const auto& entry : k_standard_stages) {
        m_stages.emplace_back(entry.ordinal, std::string(entry.name));
    }
}

void
StageRegistry::add (int ordinal, const std::string& name)
{
    auto it = std::lower_bound(m_stages.begin(), m_stages.end(), ordinal,
        [](const auto& entry, int value) { return entry.first < value; });
    m_stages.insert(it, {ordinal, name});
}

int
StageRegistry::find (const std::string& name) const
{
    for (const auto& entry : m_stages) {
        if (entry.second == name) { return entry.first; }
    }
    return -1;
}

std::string
StageRegistry::name_of (int ordinal) const
{
    for (const auto& entry : m_stages) {
        if (entry.first == ordinal) { return entry.second; }
    }
    return "<unregistered:" + std::to_string(ordinal) + ">";
}

bool
StageRegistry::contains (int ordinal) const
{
    for (const auto& entry : m_stages) {
        if (entry.first == ordinal) { return true; }
    }
    return false;
}

std::vector<std::pair<int, std::string>>
StageRegistry::all () const
{
    return m_stages;
}

void
StageRegistry::reset ()
{
    seed_standard_stages();
}

int
StageRegistry::register_stage_at (const std::string& name, int ordinal)
{
    const int existing = find(name);
    if (existing >= 0) {
        if (existing != ordinal) {
            throw std::runtime_error("StageRegistry: stage '" + name +
                                     "' already registered at a different ordinal");
        }
        return existing;
    }
    if (contains(ordinal)) {
        throw std::runtime_error("StageRegistry: ordinal " + std::to_string(ordinal) +
                                 " already used by '" + name_of(ordinal) + "'");
    }
    add(ordinal, name);
    return ordinal;
}

int
StageRegistry::register_stage (const std::string& name,
                               const std::string& anchor,
                               StagePosition position)
{
    const int existing = find(name);
    if (existing >= 0) { return existing; }

    const int anchor_ordinal = find(anchor);
    if (anchor_ordinal < 0) {
        throw std::runtime_error("StageRegistry: unknown anchor stage '" + anchor +
                                 "' for stage '" + name + "'");
    }

    // Free slot interval on the insertion side; both bounds are exclusive of
    // the registered neighbours.  The new stage goes to the middle so that
    // later registrations still have room on either side of it.
    int lo = 0;
    int hi = 0;
    if (position == StagePosition::After) {
        lo = anchor_ordinal + 1;
        hi = anchor_ordinal + Stage::Stride - 1;
        for (const auto& entry : m_stages) {
            if (entry.first > anchor_ordinal) { hi = entry.first - 1; break; }
        }
    } else {
        lo = anchor_ordinal - Stage::Stride + 1;
        hi = anchor_ordinal - 1;
        for (auto it = m_stages.rbegin(); it != m_stages.rend(); ++it) {
            if (it->first < anchor_ordinal) { lo = it->first + 1; break; }
        }
    }

    if (hi < lo) {
        throw std::runtime_error("StageRegistry: no free ordinal between '" +
                                 anchor + "' and its neighbour for stage '" + name + "'");
    }

    add(lo + (hi - lo) / 2, name);
    return lo + (hi - lo) / 2;

}

int
register_stage (const std::string& name, const std::string& anchor, StagePosition position)
{
    return StageRegistry::instance().register_stage(name, anchor, position);
}

} // namespace warpx::runtime
