foreach(D IN LISTS WarpX_DIMS)
    warpx_set_suffix_dims(SD ${D})
    target_sources(warpx_picem_${SD}
      PRIVATE
        WarpXPushFieldsEM.cpp
        WarpXImplicitOps.cpp
        WarpX_Magnetostatic.cpp
        WarpXPushFieldsHybridPIC.cpp
        WarpX_QED_Field_Pushers.cpp
        WarpXSolveFieldsES.cpp
        WarpX_ImplicitParticles.cpp
    )
    if(WarpX_PLUGIN_IMPLICIT)
        # The adapter that turns the kernel into the implicit solver's services.
        target_sources(warpx_picem_${SD} PRIVATE ImplicitKernelServicesBuilder.cpp)
    endif()
endforeach()


