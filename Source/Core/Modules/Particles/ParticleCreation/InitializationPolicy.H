#ifndef WARPX_CORE_PARTICLES_INITIALIZATIONPOLICY_H_
#define WARPX_CORE_PARTICLES_INITIALIZATIONPOLICY_H_
#include <AMReX.H>
#include <AMReX_Random.H>
#include <AMReX_REAL.H>
#include <cmath>
enum struct InitializationPolicy { Zero = 0, One, RandomExp };
AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE
amrex::ParticleReal initializeRealValue (InitializationPolicy policy, amrex::RandomEngine const& engine) noexcept
{
    switch (policy) {
        case InitializationPolicy::Zero: return 0.0;
        case InitializationPolicy::One: return 1.0;
        case InitializationPolicy::RandomExp: return -std::log(amrex::Random(engine));
        default: amrex::Abort("Initialization Policy not recognized"); return 1.0;
    }
}
AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE
int initializeIntValue (InitializationPolicy policy) noexcept
{
    switch (policy) {
        case InitializationPolicy::Zero: return 0;
        case InitializationPolicy::One: return 1;
        default: amrex::Abort("Initialization Policy not recognized"); return 1;
    }
}
#endif
