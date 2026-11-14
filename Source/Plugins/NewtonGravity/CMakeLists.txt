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
    if(NOT TARGET star_runtime_plugin_${SD})
        add_library(star_runtime_plugin_${SD} MODULE StarRuntimePlugin.cpp)
        target_include_directories(star_runtime_plugin_${SD} PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${PROJECT_SOURCE_DIR}/Source/Core)
        # Only the star layer is linked in; AMReX and ablastr symbols stay undefined and are
        # resolved from the loading executable at dlopen time.  Linking a second copy of AMReX
        # into a plugin makes both copies run their static destructors at exit and corrupts the
        # heap - that was the shutdown crash this plugin had.
        target_link_libraries(star_runtime_plugin_${SD} PRIVATE
            warpx_newtongravity_static_${SD})
        set_target_properties(star_runtime_plugin_${SD} PROPERTIES
            PREFIX "" OUTPUT_NAME "star_runtime_plugin_${SD}")
    endif()

    # A static variant exists so that an out-of-tree runtime plugin (a shared library loaded
    # through WARPX_PLUGIN_PATH) can link this layer: an OBJECT library has no archive to link
    # against.  Same sources, same include directories, no extra dependency.
    if(NOT TARGET warpx_newtongravity_static_${SD})
        add_library(warpx_newtongravity_static_${SD} STATIC NewtonGravity.cpp)
        target_include_directories(warpx_newtongravity_static_${SD} PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR})
        target_link_libraries(warpx_newtongravity_static_${SD} PUBLIC ablastr_${SD})
        # PIC so a shared runtime plugin can link this archive (the "bad value" link error).
        set_target_properties(warpx_newtongravity_static_${SD} PROPERTIES POSITION_INDEPENDENT_CODE ON)
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

