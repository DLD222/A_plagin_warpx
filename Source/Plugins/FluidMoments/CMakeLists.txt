# Five-moment (Euler) fluid plugin: multi-species conservative update with MUSCL-Hancock
# reconstruction and an HLLC Riemann solver (PHYSICS_SPEC section 2).
#
# Like the gravity plugin it depends only on ablastr and AMReX - no Core, no kernel, no other
# plugin - so it can be linked into any build without touching the rest of the tree.
foreach(D IN LISTS WarpX_DIMS)
    warpx_set_suffix_dims(SD ${D})
    target_sources(warpx_fluidmoments_${SD}
      PRIVATE
        FluidMoments.cpp
    )
    target_include_directories(warpx_fluidmoments_${SD} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR})

    # Static variant for out-of-tree runtime plugins (see the gravity plugin for the rationale).
    if(NOT TARGET warpx_fluidmoments_static_${SD})
        add_library(warpx_fluidmoments_static_${SD} STATIC FluidMoments.cpp)
        target_include_directories(warpx_fluidmoments_static_${SD} PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${PROJECT_SOURCE_DIR}/Source/Core)
        # PUBLIC so consumers get the includes.  KNOWN ISSUE: this also drags ablastr and, through
        # it, AMReX into a *shared* plugin's image, so the plugin and the executable each have a
        # copy of AMReX and their static destructors corrupt the heap at exit.  Making this PRIVATE
        # breaks the include propagation for the module target; the real fix is a separate
        # interface target for the headers (see STAR_PLUGIN_BUS.md section 3c).
        target_link_libraries(warpx_fluidmoments_static_${SD} PUBLIC ablastr_${SD})
        # PIC so a shared runtime plugin can link this archive (the "bad value" link error).
        set_target_properties(warpx_fluidmoments_static_${SD} PROPERTIES POSITION_INDEPENDENT_CODE ON)
        if(TARGET warpx_newtongravity_static_${SD})
            target_link_libraries(warpx_fluidmoments_static_${SD} PUBLIC
                warpx_newtongravity_static_${SD})
        endif()
    endif()

    # Runtime plugin for this layer.  It knows nothing about the gravity plugin: the coupling is
    # the four field names it reads from the bus (A7: plugins are independent).
    # Runtime plugin for this layer.  It knows nothing about the gravity plugin: the coupling is
    # the four field names it reads from the bus (A7: plugins are independent).  Built like the
    # gravity module - an OBJECT library for the compile flags, a MODULE that links nothing - so
    # the process keeps a single copy of AMReX.
    if(NOT TARGET star_fluid_plugin_${SD})
        add_library(star_fluid_objs_${SD} OBJECT StarFluidPlugin.cpp FluidMoments.cpp)
        target_link_libraries(star_fluid_objs_${SD} PRIVATE ablastr_${SD})
        target_include_directories(star_fluid_objs_${SD} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

        add_library(star_fluid_plugin_${SD} MODULE $<TARGET_OBJECTS:star_fluid_objs_${SD}>)
        set_target_properties(star_fluid_plugin_${SD} PROPERTIES
            PREFIX "" OUTPUT_NAME "star_fluid_plugin"
            POSITION_INDEPENDENT_CODE ON LINKER_LANGUAGE CXX)
    endif()
endforeach()

# Acceptance tests.  Both run a one-dimensional problem inside the build's dimensionality, so
# they are registered for 3D (the build the analytic work is validated in); the solver itself is
# dimension-generic.
if("3" IN_LIST WarpX_DIMS)
    warpx_set_suffix_dims(SD 3)
    foreach(case IN ITEMS sod_shock conservation smooth_wave species_advection cf88_rates network network_coupling extra_scalars ionization radiation conduction energy_budget angular_momentum composition neutrino)
        if(TARGET warpx_fluidmoments_${SD} AND NOT TARGET test_fluid_${case}_${SD})
            add_executable(test_fluid_${case}_${SD} test_${case}.cpp)
            target_link_libraries(test_fluid_${case}_${SD}
                PRIVATE warpx_fluidmoments_${SD} ablastr_${SD})
            add_test(NAME fluid_${case}_3 COMMAND test_fluid_${case}_${SD})
        endif()
    endforeach()

    # Layer-by-layer comparison against the standard solar model: a Python tool that evaluates
    # the same physics at the SSM's thermodynamic conditions (PHYSICS_SPEC section 4.3).  It is
    # registered here because it is the reaction network that it exercises.
    find_package(Python3 COMPONENTS Interpreter)
    if(Python3_Interpreter_FOUND)
        add_test(NAME fluid_ssm_epsilon
                 COMMAND ${Python3_EXECUTABLE}
                         ${CMAKE_SOURCE_DIR}/Tools/star/ssm_compare.py --selftest)
        # The structural machinery (hydrostatic integration in the mass coordinate) is validated
        # against the Lane-Emden polytropes, whose solution is known independently.
        # The OPAL opacity table (extracted, not transcribed) must reproduce the SSM's own
        # opacity column before it is trusted anywhere else.
        add_test(NAME fluid_opal_opacity
                 COMMAND ${Python3_EXECUTABLE}
                         ${CMAKE_SOURCE_DIR}/Tools/star/opal_opacity.py --selftest)
        # The Saha ionisation EOS: fully ionised centre, neutral envelope, adiabatic gradient.
        add_test(NAME fluid_saha_eos
                 COMMAND ${Python3_EXECUTABLE}
                         ${CMAKE_SOURCE_DIR}/Tools/star/eos.py --selftest)
        # Regression baseline on the solar model's integrated output.
        add_test(NAME fluid_solar_model
                 COMMAND ${Python3_EXECUTABLE}
                         ${CMAKE_SOURCE_DIR}/Tools/star/solar_compare_plot.py --check)
        add_test(NAME fluid_lane_emden
                 COMMAND ${Python3_EXECUTABLE}
                         ${CMAKE_SOURCE_DIR}/Tools/star/stellar_structure.py --selftest)
    endif()

    # The Jeans and hydrostatic tests drive the fluid with the gravity plugin's field, so they
    # link both layers.
    foreach(case IN ITEMS jeans hydrostatic)
        if(TARGET warpx_newtongravity_${SD} AND NOT TARGET test_fluid_${case}_${SD})
            add_executable(test_fluid_${case}_${SD} test_${case}.cpp)
            target_link_libraries(test_fluid_${case}_${SD}
                PRIVATE warpx_fluidmoments_${SD} warpx_newtongravity_${SD} ablastr_${SD})
            add_test(NAME fluid_${case}_3 COMMAND test_fluid_${case}_${SD})
        endif()
    endforeach()
endif()
