foreach(D IN LISTS WarpX_DIMS)
    warpx_set_suffix_dims(SD ${D})
    target_sources(warpx_fieldsolver_${SD}
      PRIVATE
        MagnetostaticSolverImpl.cpp
    )
    if(WarpX_PLUGIN_FIELDSOLVER)
        target_sources(warpx_fieldsolver_${SD} PRIVATE MagnetostaticSolverRegistration.cpp)
    endif()
endforeach()
