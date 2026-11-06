/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "Runtime/HandlerSpec.H"
#include "Runtime/PluginABI.H"

namespace
{

int g_apply_count = 0;

class DynamicExampleHandler final : public warpx::runtime::IHandler
{
public:
    [[nodiscard]] warpx::runtime::HandlerSpec spec () const override
    {
        return warpx::runtime::HandlerSpec{
            .name = "dynamic_example_plugin",
            .point = {warpx::runtime::Stage::Finalize, 0},
            .reads_fields = {},
            .writes_fields = {},
            .reads_species = {},
            .writes_species = {},
            .priority = 0};
    }
    void apply (warpx::runtime::StageContext&) override { ++g_apply_count; }
};

void register_handlers (warpx::runtime::PluginHost* host)
{
    host->add(std::make_shared<DynamicExampleHandler>());
}

const WarpXPluginDesc desc{
    warpx_plugin_abi_version,
    "dynamic_example_plugin",
    "1.0",
    &register_handlers
};

} // namespace

extern "C" const WarpXPluginDesc* warpx_plugin_entry ()
{
    return &desc;
}

int dynamic_example_apply_count () { return g_apply_count; }
