#ifndef WARPX_RUNTIME_IElectrostaticSOLVER_H_
#define WARPX_RUNTIME_IElectrostaticSOLVER_H_
#include <ablastr/fields/MultiFabRegister.H>
class MultiParticleContainer;
class MultiFluidContainer;
namespace warpx::runtime {
/** Electrostatic field-solver plugin contract (A7.3.2a). */
class IElectrostaticSolver
{
public:
    virtual ~IElectrostaticSolver () = default;
    virtual void InitData () = 0;
    virtual void ComputeSpaceChargeField (
        ablastr::fields::MultiFabRegister& fields,
        MultiParticleContainer& particles,
        MultiFluidContainer* fluids,
        int max_level, bool verbose_step) = 0;
    [[nodiscard]] virtual bool boundaryPotentialSpecified () const = 0;
};
}
#endif
