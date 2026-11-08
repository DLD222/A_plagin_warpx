#ifndef WARPX_CORE_PARTICLES_IPARTICLEITERATOR_H_
#define WARPX_CORE_PARTICLES_IPARTICLEITERATOR_H_
#include "Modules/Particles/WarpXParIter.H"
namespace warpx::runtime {
/** Core particle iterator contract (AMReX ParIterSoA view). */
using IParticleIterator = WarpXParIter;
}
#endif
