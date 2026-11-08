#ifndef WARPX_CORE_PARTICLES_IPARTICLEPOSITIONACCESS_H_
#define WARPX_CORE_PARTICLES_IPARTICLEPOSITIONACCESS_H_
#include "Modules/Particles/Pusher/GetAndSetPosition.H"
namespace warpx::runtime {
/** Position access used by modules without including WarpX particle classes. */
using IParticlePositionAccess = GetParticlePosition<PIdx>;
template <typename Particle>
AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE
void particlePosition (const Particle& p, amrex::ParticleReal& x,
                       amrex::ParticleReal& y, amrex::ParticleReal& z) noexcept
{
    get_particle_position<PIdx>(p, x, y, z);
}
}
#endif
