#ifndef WARPX_RUNTIME_ISIMULATIONKERNEL_H_
#define WARPX_RUNTIME_ISIMULATIONKERNEL_H_
#include <AMReX_Geometry.H>
#include <AMReX_IntVect.H>
#include <vector>
namespace warpx::runtime {
class ISimulationKernel {
public:
    virtual ~ISimulationKernel () = default;
    [[nodiscard]] virtual amrex::IntVect RefRatio (int lev) const = 0;
    [[nodiscard]] virtual std::vector<bool> pmlDirections () const = 0;
    [[nodiscard]] virtual bool computeMaxStepFromBTD () const = 0;
    [[nodiscard]] virtual const amrex::Geometry& geom (int lev) const = 0;
};
}
#endif
