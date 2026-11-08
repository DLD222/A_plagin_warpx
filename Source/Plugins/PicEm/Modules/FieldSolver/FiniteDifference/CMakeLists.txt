foreach(D IN LISTS WarpX_DIMS)
    warpx_set_suffix_dims(SD ${D})
    target_sources(warpx_picem_${SD}
      PRIVATE
        ComputeDivE.cpp
        ComputeCurlA.cpp
        ComputeCurlB.cpp
        ComputeLaplacian.cpp
        ComputeGradient.cpp
        EvolveB.cpp
        EvolveBPML.cpp
        EvolveE.cpp
        EvolveEPML.cpp
        EvolveF.cpp
        EvolveFPML.cpp
        EvolveG.cpp
        EvolveECTRho.cpp
        FiniteDifferenceSolver.cpp
        FiniteDifferenceSolverFactory.cpp
        HybridPICSolveE.cpp
        MacroscopicEvolveE.cpp
        ApplySilverMuellerBoundary.cpp
    )
endforeach()

add_subdirectory(MacroscopicProperties)
