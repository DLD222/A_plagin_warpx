#ifndef WARPX_RUNTIME_IPARTICLEBOUNDARYBUFFER_H_
#define WARPX_RUNTIME_IPARTICLEBOUNDARYBUFFER_H_
#include <string>
namespace warpx::runtime {
class IParticleBoundaryBuffer {
public:
    virtual ~IParticleBoundaryBuffer () = default;
    virtual int getNumParticlesInContainer (const std::string&, int, bool) = 0;
    virtual std::string boundaryName (int) = 0;
    virtual void* getParticleBufferPointer (const std::string&, int) = 0;
    virtual void clearParticles (int) = 0;
};
}
#endif
