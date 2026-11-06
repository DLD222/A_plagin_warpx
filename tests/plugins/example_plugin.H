/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_TEST_EXAMPLE_PLUGIN_H_
#define WARPX_TEST_EXAMPLE_PLUGIN_H_

#include <memory>

namespace warpx::runtime { class IHandler; }

//! Out-of-tree example module: implements IHandler without touching core.
std::shared_ptr<warpx::runtime::IHandler> make_example_plugin_handler ();

#endif
