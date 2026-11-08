#ifndef WARPX_CORE_PARTICLES_IPARTICLEREDUCE_H_
#define WARPX_CORE_PARTICLES_IPARTICLEREDUCE_H_
#include "Runtime/IParticleContainer.H"
#include <AMReX_ParticleReduce.H>
namespace warpx::runtime {
/** Module-agnostic particle reduction (AMReX GPU-capable). */
template <typename ReduceData, typename ParticleOp, typename ReduceOp>
auto particleReduce (IParticleContainer& pc, ParticleOp&& op, ReduceOp&& reduce_op)
{
    return amrex::ParticleReduce<ReduceData>(
        pc, std::forward<ParticleOp>(op), std::forward<ReduceOp>(reduce_op));
}
template <typename ReduceData, typename ParticleOp, typename ReduceOp>
auto particleReduce (const IParticleContainer& pc, ParticleOp&& op, ReduceOp&& reduce_op)
{
    return amrex::ParticleReduce<ReduceData>(
        pc, std::forward<ParticleOp>(op), std::forward<ReduceOp>(reduce_op));
}
}
#endif
