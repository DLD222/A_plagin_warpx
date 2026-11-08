foreach(D IN LISTS WarpX_DIMS)
    warpx_set_suffix_dims(SD ${D})
    target_sources(warpx_picem_${SD}
      PRIVATE
        WarpXEvolve.cpp
        WarpXComputeDt.cpp
    )
endforeach()
