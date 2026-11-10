foreach(D IN LISTS WarpX_DIMS)
    warpx_set_suffix_dims(SD ${D})
    target_sources(warpx_core_${SD}
      PRIVATE
        MatrixPCAlgMG.cpp
        PETScReinitialize.cpp
        WarpX_PETSc.cpp
    )
endforeach()
