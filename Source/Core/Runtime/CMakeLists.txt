foreach(D IN LISTS WarpX_DIMS)
    warpx_set_suffix_dims(SD ${D})
    target_sources(lib_${SD}
      PRIVATE
        Runtime.cpp
        ../../Plugins/PicEm/WarpXKernel.cpp
        ../../Plugins/PicEm/Modules/PicEm/FieldSolve/FieldSolveHandler.cpp
        ../../Plugins/PicEm/Modules/PicEm/FieldSolve/Yee/YeePackage.cpp
        ../../Plugins/PicEm/Modules/PicEm/FieldSolve/Yee/YeeFieldSolveHandler.cpp
        ../../Plugins/PicEm/Modules/PicEm/Deposition/DepositionHandler.cpp
        ../../Plugins/PicEm/Modules/PicEm/ParticlePush/ParticlePushHandler.cpp
        ../../Plugins/PicEm/Modules/PicEm/PicEmPackage.cpp
        PluginLoader.cpp
        DataBus.cpp
        ../../Plugins/PicEm/RuntimeBridge/Runtime/WarpXHostServices.cpp
        ../../Plugins/PicEm/RuntimeBridge/Runtime/WarpXRuntimeData.cpp
        ../../Plugins/PicEm/RuntimeBridge/Runtime/RuntimeConfig.cpp
        ../../Plugins/PicEm/RuntimeBridge/Runtime/FrameState.cpp
        ../../Plugins/PicEm/RuntimeBridge/Runtime/LevelResources.cpp
        ../../Plugins/PicEm/RuntimeBridge/Runtime/CollisionPackage.cpp
        ../../Plugins/PicEm/Modules/PicEm/RuntimeConfigSnapshotBridge.cpp
    )
    target_link_libraries(lib_${SD} PRIVATE ${CMAKE_DL_LIBS})
endforeach()
