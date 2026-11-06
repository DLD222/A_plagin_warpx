/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_RUNTIME_PLUGINLOADER_H_
#define WARPX_RUNTIME_PLUGINLOADER_H_

#include <string>

namespace warpx::runtime
{

class PluginHost;

/** \brief dlopen-based runtime plugin loader (A5.5). */
class PluginLoader
{
public:
    static void load (const std::string& path, PluginHost& host);
};

} // namespace warpx::runtime

#endif // WARPX_RUNTIME_PLUGINLOADER_H_
