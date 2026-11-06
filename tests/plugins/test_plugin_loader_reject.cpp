/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "Runtime/HandlerSpec.H"
#include "Runtime/PluginLoader.H"

#include <iostream>
#include <stdexcept>

int main (int argc, char** argv)
{
    if (argc != 2) { return 2; }
    warpx::runtime::PluginHost host;
    try {
        warpx::runtime::PluginLoader::load(argv[1], host);
    } catch (const std::runtime_error&) {
        return host.size() == 0 ? 0 : 1;
    }
    std::cerr << "expected ABI rejection\n";
    return 1;
}
