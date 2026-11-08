foreach(D IN LISTS WarpX_DIMS)
    warpx_set_suffix_dims(SD ${D})
    target_sources(warpx_picem_${SD}
      PRIVATE
        AddParticles.cpp
        AddPlasmaUtilities.cpp
        SmartUtils.cpp
    )
endforeach()
