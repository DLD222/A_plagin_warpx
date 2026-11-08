#ifndef WARPX_CORE_PARTICLES_IPARTICLEITERATOR_H_
#define WARPX_CORE_PARTICLES_IPARTICLEITERATOR_H_
#include "Modules/Particles/ParticleTileIter.H"
namespace warpx::runtime {
/** Core particle iterator contract (AMReX ParIterSoA view). */
using IParticleIterator = ParticleTileIter;
}
#endif
