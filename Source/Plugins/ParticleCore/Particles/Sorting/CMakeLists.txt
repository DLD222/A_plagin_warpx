foreach(D IN LISTS WarpX_DIMS)
    warpx_set_suffix_dims(SD ${D})
    target_sources(warpx_particlecore_${SD}
      PRIVATE
        Partition.cpp
        SortingUtils.cpp
    )
endforeach()
