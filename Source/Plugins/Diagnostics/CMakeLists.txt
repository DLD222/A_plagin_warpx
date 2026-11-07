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

# Standalone compile check: Diagnostics sources compile without PicEm/WarpX.
if(TARGET lib_2d)
    file(GLOB_RECURSE WARPX_DIAGNOSTICS_SOURCES CONFIGURE_DEPENDS
         ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
    list(FILTER WARPX_DIAGNOSTICS_SOURCES EXCLUDE REGEX "OpenPMD")
    add_library(warpx_diagnostics_standalone_2d OBJECT ${WARPX_DIAGNOSTICS_SOURCES})
    target_include_directories(warpx_diagnostics_standalone_2d PRIVATE
        $<TARGET_PROPERTY:lib_2d,INCLUDE_DIRECTORIES>)
    target_compile_definitions(warpx_diagnostics_standalone_2d PRIVATE
        $<TARGET_PROPERTY:lib_2d,COMPILE_DEFINITIONS>)
    target_compile_options(warpx_diagnostics_standalone_2d PRIVATE
        $<TARGET_PROPERTY:lib_2d,COMPILE_OPTIONS>)
    add_custom_target(diagnostics_standalone
        DEPENDS warpx_diagnostics_standalone_2d)
endif()
