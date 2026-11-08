#ifndef WARPX_CORE_PARTICLES_IPARTICLETILE_H_
#define WARPX_CORE_PARTICLES_IPARTICLETILE_H_
#include "Runtime/IParticleContainer.H"
namespace warpx::runtime {
/** Tile view exposed by the particle container (AMReX SoA tile). */
using IParticleTile = IParticleContainer::ParticleTileType;
using IParticleTileData = IParticleContainer::ParticleTileType::ParticleTileDataType;
}
#endif
