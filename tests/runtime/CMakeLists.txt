# Each runtime test gets its own working directory so diags/reducedfiles
# cannot leak between tests or race under ctest -j.
function(warpx_runtime_test_workdir name outvar)
    set(dir ${CMAKE_CURRENT_BINARY_DIR}/${name})
    file(MAKE_DIRECTORY ${dir})
    set(${outvar} ${dir} PARENT_SCOPE)
endfunction()

foreach(D IN LISTS WarpX_DIMS)
    # All input files under tests/runtime/inputs_* hard-code
    # geometry.dims = 2 (Cartesian 2D).  Only register tests for the 2D build;
    # 1D/3D/RZ variants would fail to parse the input file at runtime.
    if(NOT D STREQUAL 2)
        continue()
    endif()
    warpx_set_suffix_dims(SD ${D})
    if(TARGET lib_${SD})
        add_executable(test_runtime_snapshot_${SD} test_runtime_snapshot.cpp)
        add_executable(test_field_solver_interface_${SD} test_field_solver_interface.cpp)
        target_link_libraries(test_field_solver_interface_${SD} PRIVATE lib_${SD})
        target_link_libraries(test_runtime_snapshot_${SD} PRIVATE lib_${SD})

        warpx_runtime_test_workdir(runtime_snapshot_${D} runtime_snapshot_${D}_dir)
        add_test(NAME runtime_snapshot_${D}
                 COMMAND test_runtime_snapshot_${SD} ${CMAKE_CURRENT_SOURCE_DIR}/inputs_2d)
        set_tests_properties(runtime_snapshot_${D} PROPERTIES
            WORKING_DIRECTORY ${runtime_snapshot_${D}_dir})

        warpx_runtime_test_workdir(runtime_snapshot_particles_${D} runtime_snapshot_particles_${D}_dir)
        add_test(NAME runtime_snapshot_particles_${D}
                 COMMAND test_runtime_snapshot_${SD} ${CMAKE_CURRENT_SOURCE_DIR}/inputs_2d_particles --particles)
        set_tests_properties(runtime_snapshot_particles_${D} PROPERTIES
            WORKING_DIRECTORY ${runtime_snapshot_particles_${D}_dir})

        if(WarpX_FFT AND TARGET app_${SD})
            warpx_runtime_test_workdir(runtime_psatd_${D} runtime_psatd_${D}_dir)
        add_test(NAME runtime_psatd_${D}
                     COMMAND $<TARGET_FILE:app_${SD}> ${CMAKE_CURRENT_SOURCE_DIR}/inputs_2d_psatd)
            set_tests_properties(runtime_psatd_${D} PROPERTIES
                WORKING_DIRECTORY ${runtime_psatd_${D}_dir})

            warpx_runtime_test_workdir(runtime_psatd_check_${D} runtime_psatd_check_${D}_dir)
        add_test(NAME runtime_psatd_check_${D}
                     COMMAND test_runtime_snapshot_${SD} ${CMAKE_CURRENT_SOURCE_DIR}/inputs_2d_psatd --psatd)
            set_tests_properties(runtime_psatd_check_${D} PROPERTIES
                WORKING_DIRECTORY ${runtime_psatd_check_${D}_dir})
        endif()

        if(TARGET app_${SD})
            warpx_runtime_test_workdir(runtime_btd_${D} runtime_btd_${D}_dir)
        add_test(NAME runtime_btd_${D}
                     COMMAND $<TARGET_FILE:app_${SD}> ${CMAKE_CURRENT_SOURCE_DIR}/inputs_2d_btd)
            set_tests_properties(runtime_btd_${D} PROPERTIES
                WORKING_DIRECTORY ${runtime_btd_${D}_dir})

            warpx_runtime_test_workdir(runtime_es_${D} runtime_es_${D}_dir)
        add_test(NAME runtime_es_${D}
                     COMMAND $<TARGET_FILE:app_${SD}> ${CMAKE_CURRENT_SOURCE_DIR}/inputs_2d_es)
            set_tests_properties(runtime_es_${D} PROPERTIES
                WORKING_DIRECTORY ${runtime_es_${D}_dir})

            warpx_runtime_test_workdir(runtime_lb_${D} runtime_lb_${D}_dir)
        add_test(NAME runtime_lb_${D}
                     COMMAND $<TARGET_FILE:app_${SD}> ${CMAKE_CURRENT_SOURCE_DIR}/inputs_2d_lb)
            set_tests_properties(runtime_lb_${D} PROPERTIES
                WORKING_DIRECTORY ${runtime_lb_${D}_dir})
        endif()
    endif()
endforeach()
