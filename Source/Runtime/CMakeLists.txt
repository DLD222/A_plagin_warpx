foreach(D IN LISTS WarpX_DIMS)
    warpx_set_suffix_dims(SD ${D})
    target_sources(lib_${SD}
      PRIVATE
        Runtime.cpp
        CollisionPackage.cpp
        PluginLoader.cpp
        DataBus.cpp
    LevelResources.cpp
    SpeciesSpec.cpp
        WarpXHostServices.cpp
        RuntimeConfig.cpp
        FrameState.cpp
        WarpXRuntimeData.cpp
    )
    target_link_libraries(lib_${SD} PRIVATE ${CMAKE_DL_LIBS})
endforeach()
