#ifndef WARPX_RUNTIME_ISPECIESMETADATA_H_
#define WARPX_RUNTIME_ISPECIESMETADATA_H_
#include <AMReX_Vector.H>
#include <string>
#include <vector>
namespace warpx::runtime {
class ISpeciesMetadata {
public:
    virtual ~ISpeciesMetadata () = default;
    [[nodiscard]] virtual int nSpecies () const = 0;
    [[nodiscard]] virtual std::vector<std::string> GetSpeciesNames () const = 0;
    [[nodiscard]] virtual int getSpeciesID (const std::string& name) const = 0;
    [[nodiscard]] virtual amrex::Vector<amrex::Long> NumberOfParticlesInGrid (int lev) const = 0;
};
}
#endif
