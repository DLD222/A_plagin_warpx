/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_RUNTIME_PLUGINABI_H_
#define WARPX_RUNTIME_PLUGINABI_H_

#include <cstdint>

namespace warpx::runtime { class PluginHost; }

/** \brief C ABI descriptor exported by a runtime plugin (A5.5). */
struct WarpXPluginDesc
{
    std::uint32_t abi_version = 0;
    std::uint32_t struct_size = 0;
    const char* name = nullptr;
    const char* version = nullptr;
    void (*register_handlers)(warpx::runtime::PluginHost*) = nullptr;
};

//! Current ABI version. Bump on any layout/signature change.
inline constexpr std::uint32_t warpx_plugin_abi_version = 2;

extern "C" {
using WarpXPluginEntry = const WarpXPluginDesc* (*)();
}

#endif // WARPX_RUNTIME_PLUGINABI_H_
