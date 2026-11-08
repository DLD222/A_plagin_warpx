/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "Runtime/Callbacks.H"

#include <map>
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
    registry()[name] = std::move(callback);
}

bool has_callback (const std::string& name)
{
    return registry().count(name) == 1U;
}

void execute_callback (const std::string& name)
{
    auto& callbacks = registry();
    if (callbacks.find(name) == callbacks.end()) { return; }
    callbacks[name]();
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
