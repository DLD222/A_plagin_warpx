foreach(D IN LISTS WarpX_DIMS)
    warpx_set_suffix_dims(SD ${D})
    target_sources(lib_${SD}
      PRIVATE
        Diagnostics.cpp
        DiagnosticsServiceFactory.cpp
        FullDiagnostics.cpp
        MultiDiagnostics.cpp
        WarpXOpenPMD.cpp
        BTDiagnostics.cpp
        BoundaryScrapingDiagnostics.cpp
        BTD_Plotfile_Header_Impl.cpp
        OpenPMDHelpFunction.cpp
    )
endforeach()

add_subdirectory(ComputeDiagFunctors)
add_subdirectory(FlushFormats)
add_subdirectory(ParticleDiag)
add_subdirectory(ReducedDiags)
