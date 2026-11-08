foreach(D IN LISTS WarpX_DIMS)
    warpx_set_suffix_dims(SD ${D})
    target_sources(warpx_picem_${SD}
      PRIVATE
        DarwinLinearFieldOperator.cpp
        ImplicitSolver.cpp
        SemiImplicitEM.cpp
        SemiImplicitDarwin.cpp
        SparseJacobianMatrix.cpp
        ThetaImplicitEM.cpp
        StrangImplicitSpectralEM.cpp
        WarpXSolverDOF.cpp
        WarpXSolverVec.cpp
    )
endforeach()
