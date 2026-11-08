/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_RUNTIME_KEYMATCH_H_
#define WARPX_RUNTIME_KEYMATCH_H_

#include <string>
#include <vector>

namespace warpx::runtime
{

/**
 * \brief Whether two state keys denote the same entry.
 *
 * A handler may declare an aggregate name ("Efield_fp") while the register
 * stores components ("Efield_fp_x"), so an exact match is not enough.  This is
 * the single definition used by the plugin host (ordering/conflict detection)
 * and by the data bus (declared-access checks): if the two ever disagreed, a
 * handler could be ordered against one key and rejected on another.
 */
[[nodiscard]] inline bool key_matches (const std::string& declared,
                                       const std::string& requested)
{
    if (declared == requested) { return true; }
    if (requested.size() > declared.size() &&
        requested.compare(0, declared.size(), declared) == 0 &&
        requested[declared.size()] == '_') {
        return true;
    }
    if (declared.size() > requested.size() &&
        declared.compare(0, requested.size(), requested) == 0 &&
        declared[requested.size()] == '_') {
        return true;
    }
    return false;
}

//! True when `key` matches any entry of `declared`.
[[nodiscard]] inline bool key_declared (const std::string& key,
                                        const std::vector<std::string>& declared)
{
    for (const auto& candidate : declared) {
        if (key_matches(candidate, key)) { return true; }
    }
    return false;
}

//! True when any entry of `a` matches any entry of `b`.
[[nodiscard]] inline bool key_intersects (const std::vector<std::string>& a,
                                          const std::vector<std::string>& b)
{
    for (const auto& x : a) {
        for (const auto& y : b) {
            if (key_matches(x, y)) { return true; }
        }
    }
    return false;
}

} // namespace warpx::runtime

#endif // WARPX_RUNTIME_KEYMATCH_H_
