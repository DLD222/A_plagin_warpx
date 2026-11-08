#include "Runtime/IFiniteDifferenceSolver.H"
#include "Runtime/ServiceFactory.H"
#include "FiniteDifference/FiniteDifferenceSolver.H"

#include <memory>

//! Linker anchor: keeps this registration object in the aggregate archive.
extern "C" void warpx_finitedifference_anchor (void) {}

namespace warpx::runtime {
namespace {
std::unique_ptr<IFiniteDifferenceSolver>
make_finite_difference_solver (ElectromagneticSolverAlgo algo,
                               std::array<amrex::Real,3> cell_size,
                               ablastr::utils::enums::GridType grid_type,
                               const RuntimeContext* context)
{
    return std::make_unique<FiniteDifferenceSolver>(algo, cell_size, grid_type, context);
}

const bool finite_difference_factory_registered = [] {
    register_finite_difference_solver_factory(&make_finite_difference_solver);
    return true;
}();
} // namespace
}
