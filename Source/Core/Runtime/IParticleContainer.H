#ifndef WARPX_RUNTIME_IPARTICLECONTAINER_H_
#define WARPX_RUNTIME_IPARTICLECONTAINER_H_
#include <AMReX_REAL.H>
#include <cstdint>
#include <string>
#include <utility>
namespace warpx::runtime {
struct ParticleMomentumSum { amrex::Real Px{0}, Py{0}, Pz{0}, W{0}; };
/** \brief Native particle-container view for modules (A7.1). */
class IParticleContainer {
public:
    virtual ~IParticleContainer () = default;
    [[nodiscard]] virtual const std::string& getName () const = 0;
    [[nodiscard]] virtual amrex::Real getCharge () const = 0;
    [[nodiscard]] virtual amrex::Real getMass () const = 0;
    [[nodiscard]] virtual int finestLevel () const = 0;
    [[nodiscard]] virtual std::int64_t totalNumberOfParticles () const = 0;
    [[nodiscard]] virtual amrex::Real sumParticleWeight (bool local) const = 0;
    [[nodiscard]] virtual std::pair<amrex::Real, amrex::Real>
    sumParticleWeightAndEnergy (bool local) const = 0;
    [[nodiscard]] virtual bool isPhoton () const = 0;
    [[nodiscard]] virtual bool DoQED () const = 0;
    [[nodiscard]] virtual ParticleMomentumSum sumParticleMomentum () const = 0;
};
}
#endif
