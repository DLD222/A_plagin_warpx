/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "example_plugin.h"

#include "Runtime/HandlerSpec.H"
#include "Runtime/PluginABI.H"
#include "Runtime/PluginCAdapter.H"

#include <iostream>

int main ()
{
    const WarpXPluginDesc* desc = warpx_example_plugin_desc();
    if (desc == nullptr || desc->register_handlers == nullptr) {
        std::cerr << "bad example plugin descriptor\n";
        return 1;
    }
    if (desc->abi_version != WARPX_PLUGIN_ABI_VERSION) {
        std::cerr << "example plugin ABI mismatch\n";
        return 1;
    }

    warpx::runtime::PluginHost host;
    desc->register_handlers(warpx::runtime::plugin_host_c_handle(host));

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
    if (warpx_example_plugin_apply_count() != 1) {
        std::cerr << "plugin apply not called\n";
        return 1;
    }
    return 0;
}
