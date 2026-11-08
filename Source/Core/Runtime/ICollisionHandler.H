#ifndef WARPX_RUNTIME_ICOLLISIONHANDLER_H_
#define WARPX_RUNTIME_ICOLLISIONHANDLER_H_
#include <AMReX_REAL.H>
namespace warpx::runtime {
/** Collision plugin boundary (A7.2.2). */
class ICollisionHandler
{
public:
    virtual ~ICollisionHandler () = default;
    virtual void AllocData () = 0;
    virtual void doCollisions (int step, amrex::Real cur_time, amrex::Real dt) = 0;
};
}
#endif
