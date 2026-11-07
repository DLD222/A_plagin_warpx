#include "Runtime/IFiniteDifferenceSolver.H"
#include "FieldSolver/FiniteDifference/FiniteDifferenceSolver.H"
namespace warpx::runtime {
std::unique_ptr<IFiniteDifferenceSolver> create_finite_difference_solver (
    ElectromagneticSolverAlgo algo, std::array<amrex::Real,3> cell_size,
    ablastr::utils::enums::GridType grid_type, const RuntimeContext* context)
{
    return std::make_unique<FiniteDifferenceSolver>(algo, cell_size, grid_type, context);
}
}
