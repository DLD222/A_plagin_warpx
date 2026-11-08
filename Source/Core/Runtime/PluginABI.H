/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * C++ view of the plugin ABI.  The authoritative, C-compilable contract is
 * Runtime/PluginCApi.H; anything crossing a shared-library boundary must use
 * only the C types declared there.  Include this header for the C++ constant.
 */
#ifndef WARPX_RUNTIME_PLUGINABI_H_
#define WARPX_RUNTIME_PLUGINABI_H_

#include "Runtime/PluginCApi.H"

#include <cstdint>

//! Current ABI version (C constant, usable from plain C).
inline constexpr std::uint32_t warpx_plugin_abi_version = WARPX_PLUGIN_ABI_VERSION;

#endif // WARPX_RUNTIME_PLUGINABI_H_
