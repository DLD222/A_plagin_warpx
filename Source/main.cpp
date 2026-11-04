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

int
main (int argc, char* argv[]) {
    warpx::runtime::Runtime runtime(argc, argv);
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
    }
}
