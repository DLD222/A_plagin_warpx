#include "Runtime/IMagnetostaticSolver.H"

#include "MagnetostaticSolverImpl.H"

#include <memory>

//! Linker anchor: keeps this registration object in the aggregate archive.
extern "C" void warpx_magnetostatic_anchor (void) {}

namespace warpx::runtime
{
namespace
{
std::unique_ptr<IMagnetostaticSolver> make_magnetostatic_solver ()
{
    return std::make_unique<MagnetostaticSolverImpl>();
}

const bool magnetostatic_factory_registered = [] {
    register_magnetostatic_solver_factory(&make_magnetostatic_solver);
    return true;
}();
} // namespace
} // namespace warpx::runtime
