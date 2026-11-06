/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "example_plugin.H"

#include "Runtime/HandlerSpec.H"

namespace
{

int g_apply_count = 0;

class ExamplePluginHandler final : public warpx::runtime::IHandler
{
public:
    [[nodiscard]] warpx::runtime::HandlerSpec spec () const override
    {
        return warpx::runtime::HandlerSpec{
            .name = "example_plugin",
            .point = {warpx::runtime::Stage::Diagnostics, 0},
            .reads_fields = {},
            .writes_fields = {"example_field"},
            .reads_species = {},
            .writes_species = {},
            .priority = 0};
    }

    void apply (warpx::runtime::StageContext&) override { ++g_apply_count; }
};

} // namespace

std::shared_ptr<warpx::runtime::IHandler> make_example_plugin_handler ()
{
    return std::make_shared<ExamplePluginHandler>();
}

int example_plugin_apply_count () { return g_apply_count; }
