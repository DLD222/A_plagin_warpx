# Newton-gravity plugin: solves grad^2 phi = 4 pi G rho and hands out g = -grad phi.
#
# It depends only on ablastr and AMReX - no Core, no kernel, no other plugin - so it can be
# linked into any build without touching the rest of the tree.
foreach(D IN LISTS WarpX_DIMS)
    warpx_set_suffix_dims(SD ${D})
    target_sources(warpx_newtongravity_${SD}
      PRIVATE
        NewtonGravity.cpp
    )
    # PUBLIC so another plugin (the fluid one) can link this layer and use its header.
    target_include_directories(warpx_newtongravity_${SD} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR})

    # The loadable runtime plugin: a MODULE library the host picks up through WARPX_PLUGIN_PATH.
    # Built by this project so it inherits the right AMReX configuration, but *loaded* rather than
    # linked into the executable - the core never names it (CODE_REVIEW section 15.4).
    # Runtime plugin for this layer (A7: one plugin, one job).  It links no host code - the ABI
    # entry points and AMReX resolve from the loading executable, which is the pattern
    # tests/plugins uses - and its include paths come from the per-dimension AMReX target, so it
    # compiles for the dimension it is named after.
    if(NOT TARGET star_gravity_plugin_${SD})
        add_library(star_gravity_plugin_${SD} MODULE StarGravityPlugin.cpp NewtonGravity.cpp)
        target_include_directories(star_gravity_plugin_${SD} PRIVATE
            ${WARPX_MODULE_INCLUDE_DIRS}
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_BINARY_DIR}/_deps/fetchedamrex-build
            ${fetchedamrex_SOURCE_DIR}/Src
            ${fetchedamrex_SOURCE_DIR}/Src/Base
            ${fetchedamrex_SOURCE_DIR}/Src/Base/Parser
            ${fetchedamrex_SOURCE_DIR}/Src/Boundary
            ${fetchedamrex_SOURCE_DIR}/Src/LinearSolvers/MLMG)
        # Declare this module's dimension explicitly.  AMReX's multi-dimension config header
        # picks the per-dimension settings from AMREX_SPACEDIM and falls back to 2D when nobody
        # defines it - which is exactly how a 3D plugin ends up compiled as 2D.
        target_compile_definitions(star_gravity_plugin_${SD} PRIVATE AMREX_SPACEDIM=${D})
        if(TARGET OpenMP::OpenMP_CXX)
            target_link_libraries(star_gravity_plugin_${SD} PRIVATE OpenMP::OpenMP_CXX)
        endif()
        if(TARGET MPI::MPI_CXX)
            target_link_libraries(star_gravity_plugin_${SD} PRIVATE MPI::MPI_CXX)
        endif()
        # No host libraries: like tests/plugins, the module resolves the ABI, ablastr and AMReX
        # symbols from the loading executable (it exports them), so the process keeps ONE copy of
        # AMReX instead of one per plugin.
        set_target_properties(star_gravity_plugin_${SD} PROPERTIES
            PREFIX "" OUTPUT_NAME "star_gravity_plugin_${SD}"
            POSITION_INDEPENDENT_CODE ON LINKER_LANGUAGE CXX)
    endif()
endforeach()

# Analytic acceptance test: a periodic sine density has a closed-form potential.
foreach(D IN LISTS WarpX_DIMS)
    if(D STREQUAL "RZ")
        # The plugin refuses RZ at run time: the multigrid operator it uses is Cartesian and does
        # not carry the 1/r measure, so the potential would be wrong
        # (CODE_REVIEW_2026-11-14, finding F-10).
        continue()
    endif()
    warpx_set_suffix_dims(SD ${D})
    if(TARGET warpx_newtongravity_${SD} AND NOT TARGET test_gravity_periodic_sine_${SD})
        add_executable(test_gravity_periodic_sine_${SD} test_periodic_sine.cpp)
        target_link_libraries(test_gravity_periodic_sine_${SD}
            PRIVATE warpx_newtongravity_${SD} ablastr_${SD})
        add_test(NAME gravity_periodic_sine_${D}
                 COMMAND test_gravity_periodic_sine_${SD})
    endif()
endforeach()

# Open-boundary acceptance tests: uniform sphere, point mass, two concentric shells.
# Their reference solution is the three-dimensional potential, so they are built for 3D only.
if("3" IN_LIST WarpX_DIMS)
    warpx_set_suffix_dims(SD 3)
    foreach(case IN ITEMS uniform_sphere point_mass two_shells)
        if(TARGET warpx_newtongravity_${SD} AND NOT TARGET test_gravity_${case}_${SD})
            add_executable(test_gravity_${case}_${SD} test_${case}.cpp)
            target_link_libraries(test_gravity_${case}_${SD}
                PRIVATE warpx_newtongravity_${SD} ablastr_${SD})
            add_test(NAME gravity_${case}_3 COMMAND test_gravity_${case}_${SD})
        endif()
    endforeach()
endif()

