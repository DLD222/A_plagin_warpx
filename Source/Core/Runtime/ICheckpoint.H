#ifndef WARPX_RUNTIME_ICHECKPOINT_H_
#define WARPX_RUNTIME_ICHECKPOINT_H_
#include <iosfwd>
#include <string>
namespace warpx::runtime {
class ICheckpoint {
public:
    virtual ~ICheckpoint () = default;
    virtual void writeParticleHeader (std::ostream&) = 0;
    virtual void writeReducedDiagCheckpoint (const std::string&) = 0;
};
}
#endif
