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
endforeach()

# Acceptance tests.  Both run a one-dimensional problem inside the build's dimensionality, so
# they are registered for 3D (the build the analytic work is validated in); the solver itself is
# dimension-generic.
if("3" IN_LIST WarpX_DIMS)
    warpx_set_suffix_dims(SD 3)
    foreach(case IN ITEMS sod_shock conservation smooth_wave species_advection cf88_rates network network_coupling)
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
