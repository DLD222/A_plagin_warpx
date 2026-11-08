foreach(D IN LISTS WarpX_DIMS)
    warpx_set_suffix_dims(SD ${D})
    target_sources(warpx_fieldsolver_${SD}
      PRIVATE
        EffectivePotentialES.cpp
        ElectrostaticSolver.cpp
        ElectrostaticSolverFactory.cpp
        LabFrameExplicitES.cpp
        PoissonBoundaryHandler.cpp
        RelativisticExplicitES.cpp
    )
endforeach()
