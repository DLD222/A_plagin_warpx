#ifndef WARPX_RUNTIME_IPARTICLECONTAINERACCESS_H_
#define WARPX_RUNTIME_IPARTICLECONTAINERACCESS_H_
#include <AMReX_MultiFab.H>
#include <AMReX_Vector.H>
#include <iosfwd>
#include <memory>
#include <string>
#include <vector>
#include "Runtime/IParticleContainer.H"
#include "Runtime/ISpeciesMetadata.H"
namespace warpx::runtime {
class IParticleContainerAccess : public ISpeciesMetadata {
public:
    virtual ~IParticleContainerAccess () = default;
    [[nodiscard]] virtual IParticleContainer& GetParticleContainer (int index) const = 0;
    [[nodiscard]] virtual IParticleContainer& GetParticleContainerFromName (const std::string& name) const = 0;
    [[nodiscard]] virtual IParticleContainer* GetParticleContainerPtr (int index) const = 0;
    [[nodiscard]] virtual IParticleContainer& GetParticleView (int index) const = 0;
    [[nodiscard]] virtual IParticleContainer& GetParticleInterface (int index) const = 0;
    [[nodiscard]] virtual IParticleContainer& GetParticleView (const std::string& name) const = 0;
    //! Whether species `index` asked for its self-fields to be initialized.
    //! (Generic query: the contract must not expose a concrete container.)
    [[nodiscard]] virtual bool initializesSelfFields (int index) const = 0;
    [[nodiscard]] virtual int nSpecies () const = 0;
    [[nodiscard]] virtual std::vector<std::string> GetSpeciesNames () const = 0;
    virtual void Increment (amrex::MultiFab& mf, int lev) = 0;
    virtual void AllocData () = 0;
    virtual void ReadHeader (std::istream&) = 0;
    virtual void WriteHeader (std::ostream&) const = 0;
    virtual void Restart (const std::string&) = 0;
    virtual void SetDoBackTransformedParticles (bool) = 0;
    virtual void SetDoBackTransformedParticles (const std::string&, bool) = 0;
};
}
#endif
