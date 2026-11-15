/* Copyright 2016-2020 Andrew Myers, Ann Almgren, Axel Huebl
 *                     David Grote, Jean-Luc Vay, Remi Lehe
 *                     Revathi Jambunathan, Weiqun Zhang
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause-LBNL
 */
#include "Runtime/Runtime.H"

#include <ablastr/profiler/ProfilerWrapper.H>
#include <ablastr/utils/timer/Timer.H>

#include <AMReX_Print.H>

#include <exception>
#include <iostream>

namespace
{

/** The body of main once the runtime exists (nothing here may throw during construction). */
int
run (warpx::runtime::Runtime& runtime)
{
    ABLASTR_PROFILE_VAR("main()", pmain);

    auto timer = ablastr::utils::timer::Timer{};
    timer.record_start_time();

    runtime.initialize();
    runtime.evolve();
    const auto is_warpx_verbose = runtime.verbose();
    runtime.finalize();

    timer.record_stop_time();
    if (is_warpx_verbose) {
        amrex::Print() << "Total Time                     : "
                       << timer.get_global_duration() << '\n';
    }

    ABLASTR_PROFILE_VAR_STOP(pmain);
    return 0;
}

} // namespace

int
main (int argc, char* argv[])
{
    // A throwing Runtime constructor used to escape main and hit std::terminate: the user saw
    // "terminate called after throwing an instance of 'std::runtime_error'" instead of the actual
    // reason ("no PIC kernel plugin is registered", or a failing WARPX_PLUGIN_PATH), and the
    // libraries were left un-finalized (the constructor's ExternalLibrariesGuard now finalizes
    // them on the way out).  Report the message and exit non-zero instead.
    try {
        warpx::runtime::Runtime runtime(argc, argv);
        return run(runtime);
    } catch (std::exception const& e) {
        // std::cerr rather than amrex::Print: the throw may happen inside the AMReX
        // initialisation itself, where AMReX's streams are not usable yet.
        std::cerr << "\nERROR: " << e.what() << '\n';
        return 1;
    } catch (...) {
        std::cerr << "\nERROR: unknown exception during startup\n";
        return 1;
    }
}
