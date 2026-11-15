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
#include "Runtime/PluginCAdapter.H"
#include "Runtime/PluginManifest.H"

#include <memory>

#include <dlfcn.h>

#include <stdexcept>
#include <string>

namespace warpx::runtime
{

void
PluginLoader::load (const std::string& paths, PluginHost& host)
{
    // A search-path style list, so that independent plugins can be composed without merging them
    // into one library: "a.so:b.so" loads both, each one keeping its own handlers and state.  A
    // single path behaves exactly as before.
    if (paths.find(':') != std::string::npos) {
        std::string::size_type begin = 0;
        while (begin <= paths.size()) {
            std::string::size_type const end = paths.find(':', begin);
            std::string const one = paths.substr(begin, end - begin);
            if (!one.empty()) { PluginLoader::load(one, host); }
            if (end == std::string::npos) { break; }
            begin = end + 1;
        }
        return;
    }
    std::string const& path = paths;
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

    // C ABI: the plugin only ever sees opaque handles, never host classes.
    // Watch what it actually registered: `register_handlers` returns void, so a plugin whose
    // every handler was rejected (duplicate writer, invalid stage, ...) used to be loaded as if
    // it had succeeded, and the run continued with that plugin's physics silently absent.
    detail::last_registration_error().clear();
    std::size_t const before = host.size();
    desc->register_handlers(plugin_host_c_handle(host));
    std::size_t const registered = host.size() - before;
    std::string const& failure = detail::last_registration_error();
    if (!failure.empty()) {
        throw std::runtime_error("PluginLoader: '" + std::string(desc->name != nullptr ? desc->name : path)
                                 + "' failed to register a handler: " + failure);
    }
    if (registered == 0 && desc->name != nullptr) {
        throw std::runtime_error("PluginLoader: '" + std::string(desc->name)
                                 + "' registered no handler; the plugin would have no effect");
    }
    if (desc->name != nullptr) {
        host.add_plugin(make_c_api_plugin(*desc));
    }
    // Intentionally keep the library loaded: registered handlers live in it.
}

} // namespace warpx::runtime
