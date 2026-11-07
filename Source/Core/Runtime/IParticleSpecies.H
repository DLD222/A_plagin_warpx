#ifndef WARPX_RUNTIME_IPARTICLESPECIES_H_
#define WARPX_RUNTIME_IPARTICLESPECIES_H_
#include <AMReX_REAL.H>
#include <cstdint>
#include <string>
namespace warpx::runtime {
class IParticleSpecies {
public:
    virtual ~IParticleSpecies () = default;
    [[nodiscard]] virtual const std::string& name () const = 0;
    [[nodiscard]] virtual std::int64_t numberOfParticles () const = 0;
    [[nodiscard]] virtual amrex::Real charge () const = 0;
    [[nodiscard]] virtual amrex::Real mass () const = 0;
};
}
#endif
