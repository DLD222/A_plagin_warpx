# PicEm plugin: the legacy WarpX physical kernel plus its runtime adapters.
# Everything here may include Core headers; Core never includes this directory.
foreach(D IN LISTS WarpX_DIMS)
    warpx_set_suffix_dims(SD ${D})
    target_sources(warpx_picem_${SD}
      PRIVATE
        WarpX.cpp
        WarpXKernel.cpp
        RuntimeBridge/IO/WarpXCheckpoint.cpp
        RuntimeBridge/Utils/WarpXMovingWindow.cpp
        RuntimeBridge/Utils/WarpXUtil.cpp
        RuntimeBridge/Utils/WarpXVersion.cpp
        RuntimeBridge/Runtime/WarpXHostServices.cpp
        RuntimeBridge/Runtime/WarpXRuntimeData.cpp
        RuntimeBridge/Runtime/RuntimeConfig.cpp
        RuntimeBridge/Runtime/FrameState.cpp
        RuntimeBridge/Runtime/LegacyRuntimeFactory.cpp
        RuntimeBridge/Runtime/LevelResources.cpp
        RuntimeBridge/Runtime/CollisionPackage.cpp
        Modules/PicEm/PicEmPackage.cpp
        Modules/PicEm/RuntimeConfigSnapshotBridge.cpp
        Modules/PicEm/FieldSolve/FieldSolveHandler.cpp
        Modules/PicEm/FieldSolve/Yee/YeePackage.cpp
        Modules/PicEm/FieldSolve/Yee/YeeFieldSolveHandler.cpp
        Modules/PicEm/Deposition/DepositionHandler.cpp
        Modules/PicEm/ParticlePush/ParticlePushHandler.cpp
    )
endforeach()
