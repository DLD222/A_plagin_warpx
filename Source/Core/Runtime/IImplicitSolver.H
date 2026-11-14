#ifndef WARPX_RUNTIME_IIMPLICITSOLVER_H_
#define WARPX_RUNTIME_IIMPLICITSOLVER_H_
#include "Modules/FieldSolver/ImplicitOptions.H"

#include <ablastr/fields/MultiFabRegister.H>

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

    //! Fill the second half of the symmetric diagonal mass matrices.
    //! These two are solver-level operations the Python bindings expose; they belong on the
    //! contract so that a binding does not have to name the concrete solver.
    virtual void FinishMassMatricesDeposition () = 0;
    //! Apply the mass matrices to a vector field.
    virtual void ApplyMassMatrices (
        ablastr::fields::MultiLevelVectorField& a_out,
        const ablastr::fields::MultiLevelVectorField& a_in,
        const ablastr::fields::MultiLevelVectorField* a_in_ref = nullptr,
        const ablastr::fields::MultiLevelVectorField* a_baseline = nullptr,
        amrex::Real a_scale = 1.0,
        bool a_zero_out_first = false) = 0;
};
}
#endif
