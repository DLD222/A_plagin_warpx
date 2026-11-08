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
        add_executable(test_field_solver_engine_interface_${SD} test_field_solver_engine_interface.cpp)
        add_executable(test_particle_contracts_${SD} test_particle_contracts.cpp)
        add_executable(test_particle_access_contracts_${SD} test_particle_access_contracts.cpp)
        add_executable(test_collision_contracts_${SD} test_collision_contracts.cpp)
        add_executable(test_stage_registry_${SD} test_stage_registry.cpp)
        target_link_libraries(test_stage_registry_${SD} PRIVATE lib_${SD})
        add_executable(test_stage_dispatch_${SD} test_stage_dispatch.cpp)
        target_link_libraries(test_stage_dispatch_${SD} PRIVATE lib_${SD})
        add_executable(test_plugin_field_${SD} test_plugin_field.cpp)
        target_link_libraries(test_plugin_field_${SD} PRIVATE lib_${SD})
        add_executable(test_bus_access_${SD} test_bus_access.cpp)
        target_link_libraries(test_bus_access_${SD} PRIVATE lib_${SD})
        add_executable(test_scalar_state_${SD} test_scalar_state.cpp)
        target_link_libraries(test_scalar_state_${SD} PRIVATE lib_${SD})
        add_executable(test_plugin_lifecycle_${SD} test_plugin_lifecycle.cpp)
        target_link_libraries(test_plugin_lifecycle_${SD} PRIVATE lib_${SD})
        add_executable(test_service_registry_${SD} test_service_registry.cpp)
        target_link_libraries(test_service_registry_${SD} PRIVATE lib_${SD})
        target_link_libraries(test_collision_contracts_${SD} PRIVATE lib_${SD})
        target_link_libraries(test_particle_access_contracts_${SD} PRIVATE lib_${SD})
        target_link_libraries(test_particle_contracts_${SD} PRIVATE lib_${SD})
        target_link_libraries(test_field_solver_engine_interface_${SD} PRIVATE lib_${SD})
        target_link_libraries(test_field_solver_interface_${SD} PRIVATE lib_${SD})
        target_link_libraries(test_runtime_snapshot_${SD} PRIVATE lib_${SD}
            $<TARGET_OBJECTS:warpx_picem_${SD}>
            $<TARGET_OBJECTS:warpx_diagnostics_${SD}>
            $<TARGET_OBJECTS:warpx_io_${SD}>)

        add_test(NAME stage_registry_${D} COMMAND test_stage_registry_${SD})

        add_test(NAME service_registry_${D} COMMAND test_service_registry_${SD})

        warpx_runtime_test_workdir(plugin_lifecycle_${D} plugin_lifecycle_${D}_dir)
        add_test(NAME plugin_lifecycle_${D}
                 COMMAND test_plugin_lifecycle_${SD} ${CMAKE_CURRENT_SOURCE_DIR}/inputs_2d)
        set_tests_properties(plugin_lifecycle_${D} PROPERTIES
            WORKING_DIRECTORY ${plugin_lifecycle_${D}_dir})

        warpx_runtime_test_workdir(scalar_state_${D} scalar_state_${D}_dir)
        add_test(NAME scalar_state_${D}
                 COMMAND test_scalar_state_${SD} ${CMAKE_CURRENT_SOURCE_DIR}/inputs_2d)
        set_tests_properties(scalar_state_${D} PROPERTIES
            WORKING_DIRECTORY ${scalar_state_${D}_dir})

        warpx_runtime_test_workdir(bus_access_${D} bus_access_${D}_dir)
        add_test(NAME bus_access_${D}
                 COMMAND test_bus_access_${SD} ${CMAKE_CURRENT_SOURCE_DIR}/inputs_2d)
        set_tests_properties(bus_access_${D} PROPERTIES
            WORKING_DIRECTORY ${bus_access_${D}_dir})

        warpx_runtime_test_workdir(plugin_field_${D} plugin_field_${D}_dir)
        add_test(NAME plugin_field_${D}
                 COMMAND test_plugin_field_${SD}
                         ${CMAKE_CURRENT_SOURCE_DIR}/inputs_2d_plugin_field)
        set_tests_properties(plugin_field_${D} PROPERTIES
            WORKING_DIRECTORY ${plugin_field_${D}_dir})

        warpx_runtime_test_workdir(stage_dispatch_${D} stage_dispatch_${D}_dir)
        add_test(NAME stage_dispatch_${D}
                 COMMAND test_stage_dispatch_${SD} ${CMAKE_CURRENT_SOURCE_DIR}/inputs_2d)
        set_tests_properties(stage_dispatch_${D} PROPERTIES
            WORKING_DIRECTORY ${stage_dispatch_${D}_dir})

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
