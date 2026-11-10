# The implicit solver is separable: with WarpX_PLUGIN_IMPLICIT=OFF it is not
# compiled at all, and an input that asks for an implicit evolve scheme reports
# that the capability is not built.
if(NOT WarpX_PLUGIN_IMPLICIT)
    return()
endif()

foreach(D IN LISTS WarpX_DIMS)
    warpx_set_suffix_dims(SD ${D})
    target_sources(warpx_fieldsolver_${SD}
      PRIVATE
        DarwinLinearFieldOperator.cpp
        PETScImplicitBridge.cpp
        ImplicitSolver.cpp
        SemiImplicitEM.cpp
        SemiImplicitDarwin.cpp
        SparseJacobianMatrix.cpp
        ThetaImplicitEM.cpp
        StrangImplicitSpectralEM.cpp
        NonlinearSolvers/MatrixPCAlgMG.cpp
        NonlinearSolvers/WarpX_PETSc.cpp
    )
    # The solver cluster moved here with the implicit solver: these are its internals,
    # not generic core linear algebra.
    target_include_directories(warpx_fieldsolver_${SD} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR})
endforeach()
