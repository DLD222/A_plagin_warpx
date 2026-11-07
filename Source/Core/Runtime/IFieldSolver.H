#ifndef WARPX_RUNTIME_IFIELDSOLVER_H_
#define WARPX_RUNTIME_IFIELDSOLVER_H_
#include <ablastr/fields/MultiFabRegister.H>
#include <AMReX_REAL.H>
#include <memory>
namespace warpx::runtime {

class RuntimeContext;

/** Inputs shared by every field-solver unit. */
struct FieldSolverRequest
{
    ablastr::fields::MultiFabRegister* fields = nullptr;
    RuntimeContext* runtime_context = nullptr;
    amrex::Real dt = 0.0;
    amrex::Real time = 0.0;
    int step = 0;
};

/** Field-solver plugin boundary (A7.3.1). */
class IFieldSolver
{
public:
    virtual ~IFieldSolver () = default;

    virtual void Initialize () = 0;
    virtual void EvolveElectricField (FieldSolverRequest& request) = 0;
    virtual void EvolveMagneticField (FieldSolverRequest& request) = 0;
    virtual void SolveFields (FieldSolverRequest& request) = 0;
    virtual void ApplyBoundaries (FieldSolverRequest& request) = 0;
};

/** Accepts every request without changing state (disabled-plugin sentinel). */
class NullFieldSolver final : public IFieldSolver
{
public:
    void Initialize () override {}
    void EvolveElectricField (FieldSolverRequest&) override {}
    void EvolveMagneticField (FieldSolverRequest&) override {}
    void SolveFields (FieldSolverRequest&) override {}
    void ApplyBoundaries (FieldSolverRequest&) override {}
};

std::unique_ptr<IFieldSolver> create_field_solver (const RuntimeContext* context);

} // namespace warpx::runtime
#endif
