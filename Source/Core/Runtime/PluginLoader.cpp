/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "Runtime/PluginLoader.H"

#include "Runtime/HandlerSpec.H"
#include "Runtime/PluginABI.H"

#include <dlfcn.h>

#include <stdexcept>
#include <string>

namespace warpx::runtime
{

void
PluginLoader::load (const std::string& path, PluginHost& host)
{
    void* handle = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL | RTLD_NODELETE);
    if (handle == nullptr) {
        throw std::runtime_error(
            "PluginLoader: dlopen failed for '" + path + "': " + dlerror());
    }

    auto* entry = reinterpret_cast<WarpXPluginEntry>(dlsym(handle, "warpx_plugin_entry"));
    if (entry == nullptr) {
        dlclose(handle);
        throw std::runtime_error(
            "PluginLoader: missing warpx_plugin_entry in '" + path + "'");
    }

    const WarpXPluginDesc* desc = entry();
    if (desc == nullptr || desc->name == nullptr || desc->register_handlers == nullptr) {
        dlclose(handle);
        throw std::runtime_error("PluginLoader: invalid plugin descriptor in '" + path + "'");
    }
    if (desc->struct_size != sizeof(WarpXPluginDesc)) {
        dlclose(handle);
        throw std::runtime_error(
            "PluginLoader: descriptor size mismatch in '" + path + "' (plugin " +
            std::to_string(desc->struct_size) + ", host " +
            std::to_string(sizeof(WarpXPluginDesc)) + ")");
    }
    if (desc->abi_version != warpx_plugin_abi_version) {
        dlclose(handle);
        throw std::runtime_error(
            "PluginLoader: ABI mismatch in '" + path + "' (plugin " +
            std::to_string(desc->abi_version) + ", host " +
            std::to_string(warpx_plugin_abi_version) + ")");
    }

    desc->register_handlers(&host);
    // Intentionally keep the library loaded: registered handlers live in it.
}

} // namespace warpx::runtime
