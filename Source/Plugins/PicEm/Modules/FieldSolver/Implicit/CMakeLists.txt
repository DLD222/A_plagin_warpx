# The implicit solver is separable: with WarpX_PLUGIN_IMPLICIT=OFF it is not
# compiled at all, and an input that asks for an implicit evolve scheme reports
# that the capability is not built.
if(NOT WarpX_PLUGIN_IMPLICIT)
    return()
endif()

foreach(D IN LISTS WarpX_DIMS)
    warpx_set_suffix_dims(SD ${D})
    target_sources(warpx_picem_${SD}
      PRIVATE
        DarwinLinearFieldOperator.cpp
        ImplicitKernelServicesBuilder.cpp
        ImplicitSolver.cpp
        SemiImplicitEM.cpp
        SemiImplicitDarwin.cpp
        SparseJacobianMatrix.cpp
        ThetaImplicitEM.cpp
        StrangImplicitSpectralEM.cpp
    )
endforeach()
