#include "Modules/Particles/IParticleIterator.H"
#include "Modules/Particles/IParticleReduce.H"
#include "Modules/Particles/IParticleTile.H"
static_assert(std::is_same_v<warpx::runtime::IParticleIterator, WarpXParIter>);
int main () { return 0; }
