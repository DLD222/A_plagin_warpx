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

int main (int argc, char** argv)
{
    if (argc != 2) {
        std::cerr << "usage: test_plugin_loader <plugin.so>\n";
        return 2;
    }
    try {
        warpx::runtime::PluginHost host;
        warpx::runtime::PluginLoader::load(argv[1], host);
        if (host.size() != 1 || host.ordered_specs().front().name != "dynamic_example_plugin") {
            std::cerr << "plugin registration mismatch\n";
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }
    return 0;
}
