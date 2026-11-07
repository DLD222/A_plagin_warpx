#ifndef WARPX_RUNTIME_IPARTICLECONTAINERACCESS_H_
#define WARPX_RUNTIME_IPARTICLECONTAINERACCESS_H_
#include <AMReX_Vector.H>
#include <memory>
#include <string>
class WarpXParticleContainer;
namespace warpx::runtime {
class IParticleContainerAccess {
public:
    virtual ~IParticleContainerAccess () = default;
    using SpeciesVector = amrex::Vector<std::unique_ptr<WarpXParticleContainer>>;
    using iterator = SpeciesVector::iterator;
    using const_iterator = SpeciesVector::const_iterator;
    [[nodiscard]] virtual WarpXParticleContainer& GetParticleContainer (int index) const = 0;
    [[nodiscard]] virtual WarpXParticleContainer& GetParticleContainerFromName (const std::string& name) const = 0;
    [[nodiscard]] virtual WarpXParticleContainer* GetParticleContainerPtr (int index) const = 0;
    virtual iterator begin () = 0;
    virtual iterator end () = 0;
    virtual const_iterator begin () const = 0;
    virtual const_iterator end () const = 0;
};
}
#endif
