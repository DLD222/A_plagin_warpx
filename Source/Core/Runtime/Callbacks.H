/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_RUNTIME_CALLBACKS_H_
#define WARPX_RUNTIME_CALLBACKS_H_

#include <functional>
#include <string>

namespace warpx::runtime
{

/**
 * \brief Named callback bus (S6).
 *
 * A plugin fires a named event ("beforedeposition", "afterInitEsolve", ...)
 * without knowing who listens; another plugin (the Python bindings, a user
 * plugin, a diagnostic) installs the handler.  This is the callback half of the
 * bus: no plugin needs another plugin's header to hook into the step.
 */
using Callback = std::function<void()>;

void register_callback (const std::string& name, Callback callback);
[[nodiscard]] bool has_callback (const std::string& name);
void execute_callback (const std::string& name);
void remove_callback (const std::string& name);
void clear_callbacks ();

} // namespace warpx::runtime

#endif // WARPX_RUNTIME_CALLBACKS_H_
