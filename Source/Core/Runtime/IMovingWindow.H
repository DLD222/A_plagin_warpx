#ifndef WARPX_RUNTIME_IMOVINGWINDOW_H_
#define WARPX_RUNTIME_IMOVINGWINDOW_H_
#include <AMReX_REAL.H>
namespace warpx::runtime {
class IMovingWindow {
public:
    virtual ~IMovingWindow () = default;
    virtual amrex::Real movingWindowX () const = 0;
    virtual bool movingWindowActive (int) const = 0;
    virtual int numCellsShifted (amrex::Real, amrex::Real) const = 0;
    virtual bool doMovingWindow () const = 0;
    virtual amrex::Real timeOfLastGalileanShift () const = 0;
};
}
#endif
