#ifndef WARPX_RUNTIME_IPARTICLECONTAINER_H_
#define WARPX_RUNTIME_IPARTICLECONTAINER_H_
#include "Modules/Particles/ParticleAttributes.H"
#include <AMReX_ParticleContainer.H>
#include <AMReX_REAL.H>
#include <cstdint>
#include <string>
#include <utility>
namespace warpx::runtime {
/** \brief Project particle-container module base (AMReX-backed). */
class IParticleContainer
    : public amrex::ParticleContainerPureSoA<PIdx::nattribs, 0, amrex::PolymorphicArenaAllocator>
{
public:
    using Base = amrex::ParticleContainerPureSoA<PIdx::nattribs, 0, amrex::PolymorphicArenaAllocator>;
    using Base::Base;
    ~IParticleContainer () override = default;

    struct MomentumSum { amrex::Real Px{0}, Py{0}, Pz{0}, W{0}; };
    [[nodiscard]] virtual const std::string& getName () const = 0;
    [[nodiscard]] virtual amrex::Real getCharge () const = 0;
    [[nodiscard]] virtual amrex::Real getMass () const = 0;
    [[nodiscard]] virtual int finestLevel () const = 0;
    [[nodiscard]] virtual std::int64_t totalNumberOfParticles () const = 0;
    [[nodiscard]] virtual amrex::Real sumParticleWeight (bool local) const = 0;
    [[nodiscard]] virtual std::pair<amrex::Real, amrex::Real>
    sumParticleWeightAndEnergy (bool local) const = 0;
    [[nodiscard]] virtual bool isPhoton () const = 0;
    [[nodiscard]] virtual int DoQED () const = 0;
    [[nodiscard]] virtual MomentumSum sumParticleMomentum () const = 0;
};
}
#endif
