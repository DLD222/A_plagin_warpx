# Particle-core plugin (A7.4): an independent layer for particle units extracted
# from the kernel plugin.
#
# The kernel reaches these units through core contracts and the core factory;
# this layer registers its implementations at static-initialization time and is
# pulled into the link through a linker anchor (see the top-level CMakeLists).
foreach(D IN LISTS WarpX_DIMS)
    warpx_set_suffix_dims(SD ${D})
    target_sources(warpx_particlecore_${SD}
      PRIVATE
        ParticleThermalizer.cpp
        ParticleThermalizerRegistration.cpp
    )
    target_include_directories(warpx_particlecore_${SD} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR})
endforeach()
