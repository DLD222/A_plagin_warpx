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
