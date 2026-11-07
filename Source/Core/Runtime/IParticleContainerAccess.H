#ifndef WARPX_RUNTIME_IPARTICLECONTAINERACCESS_H_
#define WARPX_RUNTIME_IPARTICLECONTAINERACCESS_H_
#include <AMReX_MultiFab.H>
#include <AMReX_Vector.H>
#include <iosfwd>
#include <memory>
#include <string>
class WarpXParticleContainer;
#include "Runtime/IParticleContainer.H"
#include "Runtime/ISpeciesMetadata.H"
namespace warpx::runtime {
class IParticleContainerAccess : public ISpeciesMetadata {
public:
    virtual ~IParticleContainerAccess () = default;
    using SpeciesVector = amrex::Vector<std::unique_ptr<WarpXParticleContainer>>;
    using iterator = SpeciesVector::iterator;
    using const_iterator = SpeciesVector::const_iterator;
    [[nodiscard]] virtual IParticleContainer& GetParticleContainer (int index) const = 0;
    [[nodiscard]] virtual IParticleContainer& GetParticleContainerFromName (const std::string& name) const = 0;
    [[nodiscard]] virtual IParticleContainer* GetParticleContainerPtr (int index) const = 0;
    [[nodiscard]] virtual IParticleContainer& GetParticleView (int index) const = 0;
    [[nodiscard]] virtual IParticleContainer& GetParticleInterface (int index) const = 0;
    [[nodiscard]] virtual IParticleContainer& GetParticleView (const std::string& name) const = 0;
    virtual iterator begin () = 0;
    virtual iterator end () = 0;
    virtual const_iterator begin () const = 0;
    virtual const_iterator end () const = 0;
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
