#ifndef WARPX_CORE_PARTICLES_IPARTICLESPECIESACCESS_H_
#define WARPX_CORE_PARTICLES_IPARTICLESPECIESACCESS_H_
#include "Runtime/IParticleContainer.H"
#include <string>
namespace warpx::runtime {
/** Value view of species metadata used by plugin modules. */
struct ParticleSpeciesInfo
{
    std::string name;
    amrex::Real charge = 0.0;
    amrex::Real mass = 0.0;
};
inline ParticleSpeciesInfo particleSpeciesInfo (const IParticleContainer& pc)
{
    return {pc.getName(), pc.getCharge(), pc.getMass()};
}
}
#endif
