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
        # Compile this plugin's own translation units into an OBJECT library that borrows ablastr's
        # usage requirements (includes, AMREX_SPACEDIM, OpenMP flags) but links nothing, then wrap
        # the objects in a MODULE.  An OBJECT library does not link, so ablastr and AMReX are not
        # pulled in: the module resolves them from the loading executable, the way tests/plugins
        # does.  One AMReX per process instead of one per plugin.
        add_library(star_gravity_objs_${SD} OBJECT StarGravityPlugin.cpp NewtonGravity.cpp)
        target_link_libraries(star_gravity_objs_${SD} PRIVATE ablastr_${SD})
        target_include_directories(star_gravity_objs_${SD} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
        # These objects end up in a MODULE (.so) plugin: PIC is required even if the
        # global CMAKE_POSITION_INDEPENDENT_CODE setting is OFF.
        set_target_properties(star_gravity_objs_${SD} PROPERTIES POSITION_INDEPENDENT_CODE ON)

        # The module needs AMReX algorithms (MLMG, parallel descriptor) and WarpX builds AMReX
        # as a static archive, so those symbols are linked in here: an earlier attempt that
        # left them undefined was rejected at dlopen over amrex::ParallelDescriptor::Arecv<char>.
        # Consequence: the process holds two copies of AMReX and its static destructors run
        # twice at exit - tracked as its own defect.
        add_library(star_gravity_plugin_${SD} MODULE $<TARGET_OBJECTS:star_gravity_objs_${SD}>)
        # Deliberately no link libraries on the MODULE: the executable exports the AMReX
        # instantiations the module needs (checked: amrex::ParallelDescriptor::Arecv<char> is in
        # its dynamic symbol table), so the process keeps one copy of AMReX.
        set_target_properties(star_gravity_plugin_${SD} PROPERTIES
            PREFIX "" OUTPUT_NAME "star_gravity_plugin"
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

