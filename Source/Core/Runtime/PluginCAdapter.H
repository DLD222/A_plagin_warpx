/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_RUNTIME_PLUGINCADAPTER_H_
#define WARPX_RUNTIME_PLUGINCADAPTER_H_

#include "Runtime/PluginCApi.H"

#include <memory>
#include <string>

namespace warpx::runtime
{

namespace detail
{
/** Reason the last warpx_host_add_handler() call returned non-zero (empty when it succeeded).
 *
 * The C ABI can only answer with a code; the loader quotes this string so a plugin author sees
 * "duplicate writer", "invalid stage", ... instead of a bare 4.
 */
std::string& last_registration_error ();
} // namespace detail

class IPluginLifecycle;
class PluginHost;
class StateRegistry;

/** \brief Wrap a C plugin descriptor (manifest + lifecycle) as a plugin. */
std::shared_ptr<IPluginLifecycle> make_c_api_plugin (const WarpXPluginDesc& desc);

/** \brief Opaque C handle for a C++ PluginHost.
 *
 * Used by the loader and by in-process tests that need to hand a host to a
 * plugin compiled against the C ABI.
 */
WarpXPluginHost* plugin_host_c_handle (PluginHost& host);

//! Opaque C handle for the core-owned state registry.
WarpXStateRegistry* state_registry_c_handle (StateRegistry& state);

} // namespace warpx::runtime

#endif // WARPX_RUNTIME_PLUGINCADAPTER_H_
