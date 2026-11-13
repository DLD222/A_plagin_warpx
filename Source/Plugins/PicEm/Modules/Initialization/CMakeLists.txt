foreach(D IN LISTS WarpX_DIMS)
    warpx_set_suffix_dims(SD ${D})
    target_sources(warpx_picem_${SD}
      PRIVATE
        WarpXAMReXInit.cpp
        WarpXInit.cpp
        WarpXInitData.cpp
    )
endforeach()

add_subdirectory(DivCleaner)
