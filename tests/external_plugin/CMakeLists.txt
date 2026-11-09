# Acceptance test for "a user's own plugin, written outside this repository,
# works without touching the core" (roadmap R2/R4).
#
# The plugin is compiled here with the plain C compiler, includes only the public
# C ABI header and links no WarpX object code.  The host resolves the ABI entry
# points when the simulation loads the library through WARPX_PLUGIN_PATH, so the
# test fails if the core needs any change to accept a third-party plugin.
if(NOT CMAKE_C_COMPILER)
    return()
endif()

foreach(D IN LISTS WarpX_DIMS)
    # The input file is Cartesian 2D, like the runtime contract tests.
    if(NOT D STREQUAL 2)
        continue()
    endif()
    warpx_set_suffix_dims(SD ${D})
    if(NOT TARGET app_${SD})
        continue()
    endif()

    set(ep_dir ${CMAKE_CURRENT_BINARY_DIR})
    set(ep_lib ${ep_dir}/libthird_party_plugin.so)

    # Deliberately a bare compiler invocation: no CMake target, no include path
    # into the build tree, no link line against the project libraries.
    add_custom_command(
        OUTPUT ${ep_lib}
        COMMAND ${CMAKE_C_COMPILER} -shared -fPIC -std=c11
                -I${WarpX_SOURCE_DIR}/Source/Core
                ${CMAKE_CURRENT_SOURCE_DIR}/third_party_plugin.c
                -o ${ep_lib}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/third_party_plugin.c
        COMMENT "Compiling the out-of-repository plugin with the plain C compiler"
        VERBATIM
    )
    add_custom_target(third_party_plugin_so_${SD} DEPENDS ${ep_lib})
    add_dependencies(app_${SD} third_party_plugin_so_${SD})

    add_test(
        NAME external_plugin_${D}
        COMMAND
            ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 1 ${MPIEXEC_POSTFLAGS}
            $<TARGET_FILE:app_${SD}>
            ${CMAKE_CURRENT_SOURCE_DIR}/inputs_external_plugin
            "amrex.abort_on_unused_inputs = 1"
            "amrex.throw_exception = 1"
            "warpx.do_dynamic_scheduling = 0"
    )
    set_tests_properties(external_plugin_${D} PROPERTIES
        WORKING_DIRECTORY ${ep_dir}
        ENVIRONMENT "WARPX_PLUGIN_PATH=${ep_lib}"
        # The plugin's own printf output is the evidence: the core scheduled a
        # handler it has never heard of, at a stage the plugin chose.
        PASS_REGULAR_EXPRESSION "\\[third_party\\] handler call"
    )
endforeach()
