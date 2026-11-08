/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * S3 acceptance (first slice): a plugin declares a field it wants the core to
 * own.  The core allocates it on every level, and it follows the level
 * lifecycle (dynamic refinement creates it again on the new fine level).
 * The plugin never allocates anything itself.
 */
#include "Runtime/FieldSpec.H"
#include "Runtime/Runtime.H"

#include <filesystem>
#include <iostream>
#include <string>

namespace
{

int failures = 0;

void check (bool ok, const std::string& what)
{
    if (!ok) {
        std::cerr << "FAIL: " << what << '\n';
        ++failures;
    }
}

} // namespace

int main (int argc, char* argv[])
{
    if (argc < 2) {
        std::cerr << "usage: test_plugin_field <inputs_file>\n";
        return 2;
    }

    using namespace warpx::runtime;

    char* rt_argv[3] = {argv[0], argv[1], nullptr};
    Runtime runtime(2, rt_argv);

    // A plugin asks for its own field; the core owns allocation from here on.
    FieldSpec spec;
    spec.name = "plugin_probe_field";
    spec.ncomp = 3;
    spec.nodal = amrex::IntVect(AMREX_D_DECL(0, 0, 0));
    spec.ngrow = amrex::IntVect(AMREX_D_DECL(1, 1, 1));
    spec.checkpoint = true;
    spec.unit = "V/m";
    runtime.state().request_field(spec);

    check(runtime.state().has_field("plugin_probe_field"),
          "plugin field request is recorded in the core registry");

    runtime.initialize();

    check(runtime.fields().has(std::string("plugin_probe_field"), 0),
          "core allocated the plugin field on level 0");
    if (runtime.fields().has(std::string("plugin_probe_field"), 0)) {
        const auto* mf = runtime.fields().get(std::string("plugin_probe_field"), 0);
        check(mf->nComp() == 3, "plugin field has the requested component count");
        check(mf->nGrow() >= 1, "plugin field has at least the requested guard cells");
    } else {
        check(false, "plugin field lookup returned a MultiFab");
    }

    // Dynamic refinement: level 1 is created during the run, and the field
    // must exist there too because the core owns the level lifecycle.
    runtime.evolve(2);

    check(runtime.fields().has(std::string("plugin_probe_field"), 1),
          "plugin field exists on a level created by dynamic refinement");

    // Checkpoint: the field was requested with checkpoint=true, so the
    // register's schema-driven writer must have produced its file.
    bool checkpoint_file_found = false;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(".")) {
        const std::string file_name = entry.path().filename().string();
        if (entry.is_regular_file() &&
            file_name.rfind("plugin_probe_field", 0) == 0) {
            checkpoint_file_found = true;
            break;
        }
    }
    check(checkpoint_file_found,
          "plugin field was written by the checkpoint writer");

    runtime.finalize();

    if (failures != 0) {
        std::cerr << failures << " check(s) failed\n";
        return 1;
    }
    return 0;
}
