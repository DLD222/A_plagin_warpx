#ifndef WARPX_RUNTIME_IIMPLICITSOLVER_H_
#define WARPX_RUNTIME_IIMPLICITSOLVER_H_
#include "Modules/FieldSolver/ImplicitOptions.H"
#include <AMReX_REAL.H>
namespace warpx::runtime {
/** Implicit field-solver plugin contract (A7.3.2b).
 *  Define(WarpX*) is intentionally not in the core contract yet. */
class IImplicitSolver
{
public:
    virtual ~IImplicitSolver () = default;
    virtual void PrintParameters () const = 0;
    virtual void CreateParticleAttributes () const = 0;
    virtual void ReinitializePETScIfDue (int step, bool verbose_step) = 0;
    virtual int OneStep (amrex::Real start_time, amrex::Real dt, int step,
                         bool verbose_step) = 0;
    [[nodiscard]] virtual const ImplicitOptions& GetImplicitOptions () const = 0;
};
}
#endif
