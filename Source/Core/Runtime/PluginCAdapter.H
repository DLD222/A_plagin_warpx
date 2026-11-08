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

namespace warpx::runtime
{

class PluginHost;

/** \brief Opaque C handle for a C++ PluginHost.
 *
 * Used by the loader and by in-process tests that need to hand a host to a
 * plugin compiled against the C ABI.
 */
WarpXPluginHost* plugin_host_c_handle (PluginHost& host);

} // namespace warpx::runtime

#endif // WARPX_RUNTIME_PLUGINCADAPTER_H_
