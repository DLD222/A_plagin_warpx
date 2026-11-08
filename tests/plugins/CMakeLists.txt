# Plugin ABI tests.  All plugins are plain C (v3 ABI): they include only
# Runtime/PluginCApi.H and resolve host symbols from the executable at load
# time, so no host object code is linked into them.
foreach(D IN LISTS WarpX_DIMS)
    if(NOT D STREQUAL 2)
        continue()
    endif()
    warpx_set_suffix_dims(SD ${D})
    if(TARGET lib_${SD})
        add_executable(test_plugin_example_${SD}
            test_plugin_example.cpp example_plugin.c)
        target_link_libraries(test_plugin_example_${SD} PRIVATE lib_${SD})
        add_test(NAME plugin_example_${D} COMMAND test_plugin_example_${SD})
    endif()
endforeach()

if(TARGET lib_2d)
    # ---------------------------------------------------------------- C ABI --
    # Pure-C compile/link/run check of the ABI header and its accessors.
    add_executable(test_c_abi_header_2d test_c_abi_header.c)
    target_include_directories(test_c_abi_header_2d PRIVATE
        ${WarpX_SOURCE_DIR}/Source/Core)
    target_link_libraries(test_c_abi_header_2d PRIVATE lib_2d)
    set_target_properties(test_c_abi_header_2d PROPERTIES LINKER_LANGUAGE CXX)
    add_test(NAME plugin_c_abi_header_2d COMMAND test_c_abi_header_2d)

    # ------------------------------------------------- dynamic loading path --
    # The plugin links no host code: warpx_host_add_handler must be resolved
    # from the loading executable, which is what ENABLE_EXPORTS provides.
    add_library(warpx_dynamic_example_plugin SHARED dynamic_example_plugin.c)
    target_include_directories(warpx_dynamic_example_plugin PRIVATE
        ${WarpX_SOURCE_DIR}/Source/Core)
    set_target_properties(warpx_dynamic_example_plugin PROPERTIES
        POSITION_INDEPENDENT_CODE ON
        LINKER_LANGUAGE CXX)

    add_executable(test_plugin_loader_2d test_plugin_loader.cpp)
    target_link_libraries(test_plugin_loader_2d PRIVATE lib_2d ${CMAKE_DL_LIBS})
    set_target_properties(test_plugin_loader_2d PROPERTIES ENABLE_EXPORTS ON)
    add_test(NAME plugin_loader_2d
             COMMAND test_plugin_loader_2d $<TARGET_FILE:warpx_dynamic_example_plugin>)
endif()

if(TARGET lib_2d)
    add_library(warpx_bad_version_plugin SHARED bad_version_plugin.c)
    target_include_directories(warpx_bad_version_plugin PRIVATE
        ${WarpX_SOURCE_DIR}/Source/Core)
    set_target_properties(warpx_bad_version_plugin PROPERTIES
        POSITION_INDEPENDENT_CODE ON LINKER_LANGUAGE CXX)

    add_executable(test_plugin_loader_reject_2d test_plugin_loader_reject.cpp)
    target_link_libraries(test_plugin_loader_reject_2d PRIVATE lib_2d ${CMAKE_DL_LIBS})
    add_test(NAME plugin_loader_reject_2d
             COMMAND test_plugin_loader_reject_2d $<TARGET_FILE:warpx_bad_version_plugin>)
endif()

if(TARGET lib_2d)
    add_library(warpx_no_entry_plugin SHARED no_entry_plugin.c)
    set_target_properties(warpx_no_entry_plugin PROPERTIES
        POSITION_INDEPENDENT_CODE ON LINKER_LANGUAGE CXX)

    add_test(NAME plugin_loader_missing_entry_2d
             COMMAND test_plugin_loader_reject_2d $<TARGET_FILE:warpx_no_entry_plugin>)
endif()

if(TARGET lib_2d)
    add_library(warpx_invalid_desc_plugin SHARED invalid_desc_plugin.c)
    target_include_directories(warpx_invalid_desc_plugin PRIVATE
        ${WarpX_SOURCE_DIR}/Source/Core)
    set_target_properties(warpx_invalid_desc_plugin PROPERTIES
        POSITION_INDEPENDENT_CODE ON LINKER_LANGUAGE CXX)

    add_test(NAME plugin_loader_invalid_desc_2d
             COMMAND test_plugin_loader_reject_2d $<TARGET_FILE:warpx_invalid_desc_plugin>)
endif()

if(TARGET lib_2d)
    add_executable(test_plugin_loader_missing_file_2d test_plugin_loader_missing_file.cpp)
    target_link_libraries(test_plugin_loader_missing_file_2d PRIVATE lib_2d ${CMAKE_DL_LIBS})
    add_test(NAME plugin_loader_missing_file_2d COMMAND test_plugin_loader_missing_file_2d)
endif()
