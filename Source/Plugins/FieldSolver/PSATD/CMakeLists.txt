foreach(D IN LISTS WarpX_DIMS)
    warpx_set_suffix_dims(SD ${D})
    target_sources(warpx_fieldsolver_${SD}
      PRIVATE
        SpectralFieldData.cpp
        SpectralKSpace.cpp
        SpectralSolver.cpp
    )
    if(WarpX_PLUGIN_FIELDSOLVER)
        target_sources(warpx_fieldsolver_${SD} PRIVATE SpectralSolverRegistration.cpp)
    endif()

    if(D STREQUAL "RZ")
        target_sources(warpx_fieldsolver_${SD}
          PRIVATE
            SpectralSolverRZ.cpp
            SpectralFieldDataRZ.cpp
            SpectralKSpaceRZ.cpp
            SpectralBinomialFilter.cpp
        )
    endif()
endforeach()

if(RZ IN_LIST WarpX_DIMS)
    add_subdirectory(SpectralHankelTransform)
endif()
add_subdirectory(SpectralAlgorithms)
