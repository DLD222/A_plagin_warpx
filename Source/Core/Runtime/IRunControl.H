#ifndef WARPX_RUNTIME_IRUNCONTROL_H_
#define WARPX_RUNTIME_IRUNCONTROL_H_
#include <AMReX_REAL.H>
namespace warpx::runtime {
class IRunControl {
public:
    virtual ~IRunControl () = default;
    virtual void updateMaxStep (int) = 0;
    virtual void updateStopTime (amrex::Real) = 0;
};
}
#endif
