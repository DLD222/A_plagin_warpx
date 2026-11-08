# Core runtime only.  Plugin sources (PicEm adapters, WarpXKernel, handlers,
# RuntimeBridge) are compiled by their own plugin target, see
# Source/Plugins/PicEm/CMakeLists.txt.  Keep this list free of ../../Plugins/*
# (checked by Tools/Modularization/check_build_boundaries.py).
foreach(D IN LISTS WarpX_DIMS)
    warpx_set_suffix_dims(SD ${D})
    target_sources(warpx_core_${SD}
      PRIVATE
        Callbacks.cpp
Runtime.cpp
        DataBus.cpp
        PluginLoader.cpp
        PluginCAdapter.cpp
        KernelFactory.cpp
        StageRegistry.cpp
        Scheduler.cpp
        StateRegistry.cpp
        ScalarRegistry.cpp
        ServiceFactory.cpp
    )
    target_link_libraries(warpx_core_${SD} PRIVATE ${CMAKE_DL_LIBS})
endforeach()
