/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_RUNTIME_STAGEREGISTRY_H_
#define WARPX_RUNTIME_STAGEREGISTRY_H_

#include "Runtime/StagePoint.H"

#include <string>
#include <utility>
#include <vector>

namespace warpx::runtime
{

/** Where a newly registered stage goes relative to its anchor. */
enum class StagePosition
{
    Before,
    After
};

/**
 * \brief Name -> ordinal table for stages (S2).
 *
 * - The standard stages are pre-registered with their fixed ordinals.
 * - A plugin registers a custom stage by naming an anchor stage and a
 *   position; the registry picks a free ordinal in the gap between the
 *   standard ordinals.  Registration is idempotent by name.
 * - Ordinals never move: a registered stage keeps its integer for the whole
 *   process, so a StagePoint stored earlier stays valid.
 *
 * This is process-wide configuration, not simulation state: it is written
 * during plugin registration (before the first step) and read afterwards.
 */
class StageRegistry
{
public:
    StageRegistry ();

    static StageRegistry& instance ();

    /**
     * \brief Register (or look up) a stage by name.
     *
     * \param name      unique stage name
     * \param anchor    name of a stage already registered
     * \param position  place the new stage before or after the anchor
     * \return the ordinal; the existing ordinal when `name` is already known
     * \throws std::runtime_error on unknown anchor or when the gap is full
     */
    int register_stage (const std::string& name,
                        const std::string& anchor,
                        StagePosition position = StagePosition::After);

    //! Register/lookup without an anchor; `ordinal` must be unused.
    int register_stage_at (const std::string& name, int ordinal);

    //! Ordinal of a registered stage, or -1.
    [[nodiscard]] int find (const std::string& name) const;

    //! Stage name of an ordinal, or "<unregistered:...>" for unknown values.
    [[nodiscard]] std::string name_of (int ordinal) const;

    //! True when the ordinal belongs to a registered stage.
    [[nodiscard]] bool contains (int ordinal) const;

    //! All registered (ordinal, name) pairs, ordered by ordinal.
    [[nodiscard]] std::vector<std::pair<int, std::string>> all () const;

    //! Drop all custom registrations (tests only).
    void reset ();

private:
    void seed_standard_stages ();
    void add (int ordinal, const std::string& name);

    std::vector<std::pair<int, std::string>> m_stages; // ordered by ordinal
};

/** \brief Convenience wrapper for plugins: register a stage after an anchor. */
int register_stage (const std::string& name,
                    const std::string& anchor,
                    StagePosition position = StagePosition::After);

} // namespace warpx::runtime

#endif // WARPX_RUNTIME_STAGEREGISTRY_H_
