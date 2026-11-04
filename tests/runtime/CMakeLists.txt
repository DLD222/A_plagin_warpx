foreach(D IN LISTS WarpX_DIMS)
    warpx_set_suffix_dims(SD ${D})
    if(TARGET lib_${SD})
        add_executable(test_runtime_snapshot_${SD} test_runtime_snapshot.cpp)
        target_link_libraries(test_runtime_snapshot_${SD} PRIVATE lib_${SD})
        add_test(NAME runtime_snapshot_${D}
                 COMMAND test_runtime_snapshot_${SD} ${CMAKE_CURRENT_SOURCE_DIR}/inputs_2d)
    endif()
endforeach()
