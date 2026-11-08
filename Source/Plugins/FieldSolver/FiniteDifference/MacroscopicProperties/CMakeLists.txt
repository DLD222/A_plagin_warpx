foreach(D IN LISTS WarpX_DIMS)
    warpx_set_suffix_dims(SD ${D})
    target_sources(warpx_fieldsolver_${SD}
      PRIVATE
        MacroscopicProperties.cpp
        MacroscopicPropertiesFactory.cpp
    )
endforeach()
