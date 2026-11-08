# Core utilities only; PicEm's RuntimeBridge/Utils sources live in the PicEm
# plugin target (Source/Plugins/PicEm/CMakeLists.txt).
foreach(D IN LISTS WarpX_DIMS)
    warpx_set_suffix_dims(SD ${D})
    target_sources(warpx_core_${SD}
      PRIVATE
        Interpolate.cpp
    )
endforeach()

add_subdirectory(Logo)
add_subdirectory(Parser)
