#ifndef WARPX_CORE_PARTICLES_IPARTICLEATTRIBUTEACCESS_H_
#define WARPX_CORE_PARTICLES_IPARTICLEATTRIBUTEACCESS_H_
#include <AMReX_Parser.H>
#include <AMReX_Vector.H>
#include <string>
#include <vector>
namespace warpx::runtime {
/** User-defined particle attribute access (A7.2.4). */
class IParticleAttributeAccess
{
public:
    virtual ~IParticleAttributeAccess () = default;
    [[nodiscard]] virtual std::vector<std::string> getUserRealAttribs () const = 0;
    [[nodiscard]] virtual std::vector<std::string> getUserIntAttribs () const = 0;
    [[nodiscard]] virtual amrex::Vector<amrex::Parser*> getUserRealAttribParser () const = 0;
    [[nodiscard]] virtual amrex::Vector<amrex::Parser*> getUserIntAttribParser () const = 0;
};
}
#endif
