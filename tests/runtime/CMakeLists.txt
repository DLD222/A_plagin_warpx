foreach(D IN LISTS WarpX_DIMS)
    warpx_set_suffix_dims(SD ${D})
    if(TARGET lib_${SD})
        add_executable(test_runtime_snapshot_${SD} test_runtime_snapshot.cpp)
        target_link_libraries(test_runtime_snapshot_${SD} PRIVATE lib_${SD})

        add_test(NAME runtime_snapshot_${D}
                 COMMAND test_runtime_snapshot_${SD} ${CMAKE_CURRENT_SOURCE_DIR}/inputs_2d)
        set_tests_properties(runtime_snapshot_${D} PROPERTIES
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

        add_test(NAME runtime_snapshot_particles_${D}
                 COMMAND test_runtime_snapshot_${SD} ${CMAKE_CURRENT_SOURCE_DIR}/inputs_2d_particles --particles)
        set_tests_properties(runtime_snapshot_particles_${D} PROPERTIES
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

        if(WarpX_FFT AND TARGET app_${SD})
            add_test(NAME runtime_psatd_${D}
                     COMMAND $<TARGET_FILE:app_${SD}> ${CMAKE_CURRENT_SOURCE_DIR}/inputs_2d_psatd)
            set_tests_properties(runtime_psatd_${D} PROPERTIES
                WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

            add_test(NAME runtime_psatd_check_${D}
                     COMMAND test_runtime_snapshot_${SD} ${CMAKE_CURRENT_SOURCE_DIR}/inputs_2d_psatd --psatd)
            set_tests_properties(runtime_psatd_check_${D} PROPERTIES
                WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
        endif()

        if(TARGET app_${SD})
            add_test(NAME runtime_es_${D}
                     COMMAND $<TARGET_FILE:app_${SD}> ${CMAKE_CURRENT_SOURCE_DIR}/inputs_2d_es)
            set_tests_properties(runtime_es_${D} PROPERTIES
                WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

            add_test(NAME runtime_lb_${D}
                     COMMAND $<TARGET_FILE:app_${SD}> ${CMAKE_CURRENT_SOURCE_DIR}/inputs_2d_lb)
            set_tests_properties(runtime_lb_${D} PROPERTIES
                WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
        endif()
    endif()
endforeach()
