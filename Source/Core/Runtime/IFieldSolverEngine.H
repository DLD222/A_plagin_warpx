#ifndef WARPX_RUNTIME_IFIELDSOLVERENGINE_H_
#define WARPX_RUNTIME_IFIELDSOLVERENGINE_H_
#include <AMReX_REAL.H>
#include <memory>
namespace warpx::runtime {
class RuntimeContext;
class FieldSolverRequest;
/** FieldSolver plugin boundary: owns FD/ES/PSATD/Implicit/Hybrid/PML internally. */
class IFieldSolverEngine
{
public:
    virtual ~IFieldSolverEngine () = default;
    virtual void Initialize () = 0;
    virtual void EvolveFields (const FieldSolverRequest& request) = 0;
    virtual void SolveElectrostaticFields (const FieldSolverRequest& request) = 0;
    virtual void ImplicitStep (const FieldSolverRequest& request) = 0;
    virtual void Finalize () = 0;
};
std::unique_ptr<IFieldSolverEngine> create_field_solver_engine (const RuntimeContext* context);
}
#endif
