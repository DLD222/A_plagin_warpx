foreach(D IN LISTS WarpX_DIMS)
    warpx_set_suffix_dims(SD ${D})
    target_sources(warpx_diagnostics_${SD}
      PRIVATE
        FlushFormat.cpp
        FlushFormatInSitu.cpp
        FlushFormatCatalyst.cpp
        FlushFormatAscent.cpp
        FlushFormatCheckpoint.cpp
        FlushFormatPlotfile.cpp
        FlushFormatSensei.cpp
    )

    if(WarpX_HAVE_OPENPMD)
        target_sources(warpx_diagnostics_${SD}
          PRIVATE
            FlushFormatOpenPMD.cpp
        )
    endif()
endforeach()
