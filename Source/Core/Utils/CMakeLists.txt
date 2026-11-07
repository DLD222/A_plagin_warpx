foreach(D IN LISTS WarpX_DIMS)
    warpx_set_suffix_dims(SD ${D})
    target_sources(lib_${SD}
      PRIVATE
        ../../Plugins/PicEm/RuntimeBridge/Utils/WarpXMovingWindow.cpp
        ../../Plugins/PicEm/RuntimeBridge/Utils/WarpXUtil.cpp
        ../../Plugins/PicEm/RuntimeBridge/Utils/WarpXVersion.cpp
        Interpolate.cpp
    )
endforeach()

add_subdirectory(Logo)
add_subdirectory(Parser)
