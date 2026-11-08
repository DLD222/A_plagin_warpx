foreach(D IN LISTS WarpX_DIMS)
    warpx_set_suffix_dims(SD ${D})
    target_sources(warpx_io_${SD}
      PRIVATE
        WarpXIO.cpp
        FieldIO.cpp
    )
endforeach()
