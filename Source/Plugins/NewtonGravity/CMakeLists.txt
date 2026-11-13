# Newton-gravity plugin: solves grad^2 phi = 4 pi G rho and hands out g = -grad phi.
#
# It depends only on ablastr and AMReX - no Core, no kernel, no other plugin - so it can be
# linked into any build without touching the rest of the tree.
foreach(D IN LISTS WarpX_DIMS)
    warpx_set_suffix_dims(SD ${D})
    target_sources(warpx_newtongravity_${SD}
      PRIVATE
        NewtonGravity.cpp
    )
    target_include_directories(warpx_newtongravity_${SD} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR})
endforeach()

# Analytic acceptance test: a periodic sine density has a closed-form potential.
foreach(D IN LISTS WarpX_DIMS)
    warpx_set_suffix_dims(SD ${D})
    if(TARGET warpx_newtongravity_${SD} AND NOT TARGET test_gravity_periodic_sine_${SD})
        add_executable(test_gravity_periodic_sine_${SD} test_periodic_sine.cpp)
        target_link_libraries(test_gravity_periodic_sine_${SD}
            PRIVATE warpx_newtongravity_${SD} ablastr_${SD})
        # Not registered with ctest yet: the ablastr path asserts that the LinOp has explicit
        # periodic domain BCs, which needs a boundary handler we have not written.  The
        # executable is built so the next round can iterate on it without relinking the world.
        # add_test(NAME gravity_periodic_sine_${D}
        #          COMMAND test_gravity_periodic_sine_${SD})
    endif()
endforeach()
