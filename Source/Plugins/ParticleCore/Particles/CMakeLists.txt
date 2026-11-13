foreach(D IN LISTS WarpX_DIMS)
    warpx_set_suffix_dims(SD ${D})
    target_sources(warpx_particlecore_${SD}
      PRIVATE
        MultiParticleContainer.cpp
        PhotonParticleContainer.cpp
        PhysicalParticleContainer.cpp
        RigidInjectedParticleContainer.cpp
        WarpXParticleContainer.cpp
        SpeciesUtils.cpp
        ParticleUtils.cpp
        ParticleIO.cpp
        LaserParticleContainer.cpp
        ParticleBoundaryBuffer.cpp
        SpeciesPhysicalProperties.cpp
    )
endforeach()

#add_subdirectory(Algorithms)
add_subdirectory(AcceleratorLattice)
add_subdirectory(Deposition)
add_subdirectory(ElementaryProcess)
add_subdirectory(Filter)
add_subdirectory(Gather)
add_subdirectory(Initialization)
add_subdirectory(Laser)
add_subdirectory(ParticleCreation)
add_subdirectory(Pusher)
add_subdirectory(Resampling)
add_subdirectory(Sorting)
