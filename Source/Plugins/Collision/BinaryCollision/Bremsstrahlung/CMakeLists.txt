foreach(D IN LISTS WarpX_DIMS)
    warpx_set_suffix_dims(SD ${D})
    target_sources(warpx_collision_${SD}
      PRIVATE
        BremsstrahlungFunc.cpp
        PhotonCreationFunc.cpp
    )
endforeach()
