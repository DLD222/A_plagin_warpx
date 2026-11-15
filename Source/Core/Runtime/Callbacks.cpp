/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "Runtime/Callbacks.H"

#include <cstdio>
#include <map>
#include <set>
#include <stdexcept>
#include <utility>

namespace warpx::runtime
{
namespace
{
std::map<std::string, Callback>& registry ()
{
    static std::map<std::string, Callback> callbacks;
    return callbacks;
}
} // namespace

void register_callback (const std::string& name, Callback callback)
{
    // `registry()[name] = ...` silently replaced an existing callback: the first plugin to
    // register a name was dropped without a word (`has_callback` still returned true), and two
    // plugins picking the same event name is exactly what this bus invites.  Refuse instead.
    if (registry().count(name) == 1U) {
        throw std::runtime_error(
            "register_callback: '" + name + "' is already registered; event names must be unique "
            "(namespace yours, e.g. \"<plugin>:<event>\")");
    }
    registry().emplace(name, std::move(callback));
}

bool has_callback (const std::string& name)
{
    return registry().count(name) == 1U;
}

void execute_callback (const std::string& name)
{
    auto& callbacks = registry();
    auto const it = callbacks.find(name);
    if (it == callbacks.end()) {
        // A misspelt event name used to be a silent no-op, so a plugin that never ran looked
        // exactly like one with nothing to do.  Warn once per name (a missing callback is not
        // fatal: a kernel may legitimately not register an optional event).
        static std::set<std::string> warned;
        if (warned.insert(name).second) {
            std::printf("Warning: execute_callback: no callback registered under '%s'; "
                        "use has_callback() to query before relying on it\n", name.c_str());
        }
        return;
    }
    it->second();
}

void remove_callback (const std::string& name)
{
    registry().erase(name);
}

void clear_callbacks ()
{
    registry().clear();
}
} // namespace warpx::runtime
