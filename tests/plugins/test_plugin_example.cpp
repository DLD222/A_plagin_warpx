/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "example_plugin.H"

#include "Runtime/HandlerSpec.H"

#include <iostream>

int example_plugin_apply_count ();

int main ()
{
    warpx::runtime::PluginHost host;
    host.add(make_example_plugin_handler());

    if (host.size() != 1) {
        std::cerr << "plugin handler not registered\n";
        return 1;
    }
    if (host.ordered_specs().front().name != "example_plugin") {
        std::cerr << "plugin order/name mismatch\n";
        return 1;
    }
    warpx::runtime::StageContext ctx{};
    host.ordered().front()->apply(ctx);
    if (example_plugin_apply_count() != 1) {
        std::cerr << "plugin apply not called\n";
        return 1;
    }
    return 0;
}
