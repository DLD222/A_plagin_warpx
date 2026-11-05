foreach(D IN LISTS WarpX_DIMS)
    warpx_set_suffix_dims(SD ${D})
    target_sources(lib_${SD}
      PRIVATE
        Runtime.cpp
        DataBus.cpp
        WarpXHostServices.cpp
        RuntimeConfig.cpp
        FrameState.cpp
        WarpXRuntimeData.cpp
    )
endforeach()
