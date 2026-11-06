foreach(D IN LISTS WarpX_DIMS)
    if(NOT D STREQUAL 2)
        continue()
    endif()
    warpx_set_suffix_dims(SD ${D})
    if(TARGET lib_${SD})
        add_executable(test_plugin_example_${SD}
            test_plugin_example.cpp example_plugin.cpp)
        target_link_libraries(test_plugin_example_${SD} PRIVATE lib_${SD})
        add_test(NAME plugin_example_${D} COMMAND test_plugin_example_${SD})
    endif()
endforeach()

if(TARGET lib_2d)
    add_library(warpx_dynamic_example_plugin SHARED dynamic_example_plugin.cpp)
    target_link_libraries(warpx_dynamic_example_plugin PRIVATE lib_2d)
    set_target_properties(warpx_dynamic_example_plugin PROPERTIES
        POSITION_INDEPENDENT_CODE ON)

    add_executable(test_plugin_loader_2d test_plugin_loader.cpp)
    target_link_libraries(test_plugin_loader_2d PRIVATE lib_2d ${CMAKE_DL_LIBS})
    add_test(NAME plugin_loader_2d
             COMMAND test_plugin_loader_2d $<TARGET_FILE:warpx_dynamic_example_plugin>)
endif()
