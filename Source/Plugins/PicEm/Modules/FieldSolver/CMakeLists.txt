foreach(D IN LISTS WarpX_DIMS)
    warpx_set_suffix_dims(SD ${D})
    target_sources(warpx_picem_${SD}
      PRIVATE
        WarpXPushFieldsEM.cpp
        WarpXImplicitOps.cpp
        WarpX_Magnetostatic.cpp
        WarpXPushFieldsHybridPIC.cpp
        WarpX_QED_Field_Pushers.cpp
        WarpXSolveFieldsES.cpp
    )
    if(WarpX_FFT)
        target_sources(warpx_picem_${SD} PRIVATE SpectralSolverRegistration.cpp)
    endif()
endforeach()

add_subdirectory(Magnetostatic)
add_subdirectory(Implicit)
if(WarpX_FFT)
    add_subdirectory(PSATD)
endif()
