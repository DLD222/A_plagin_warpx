# Field-solver plugin (S6): finite-difference and electrostatic solvers.
#
# The kernel creates these through the core factories; this layer registers the
# implementations at static-init time and is pulled in through linker anchors.
foreach(D IN LISTS WarpX_DIMS)
    warpx_set_suffix_dims(SD ${D})
    target_include_directories(warpx_fieldsolver_${SD} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR})
endforeach()

add_subdirectory(FiniteDifference)
add_subdirectory(Electrostatic)
add_subdirectory(Magnetostatic)
if(WarpX_FFT)
    add_subdirectory(PSATD)
endif()
if(WarpX_PLUGIN_IMPLICIT)
    add_subdirectory(Implicit)
endif()
