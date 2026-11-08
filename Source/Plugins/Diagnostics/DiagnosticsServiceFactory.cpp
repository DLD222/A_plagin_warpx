#include "Runtime/IDiagnosticsService.H"
#include "Runtime/ServiceFactory.H"
#include "Diagnostics/DiagnosticsDependencies.H"
#include "Runtime/IParticleContainerAccess.H"
#include "Runtime/ISpeciesMetadata.H"
#include "Runtime/IReducedDiagsService.H"

#include "Diagnostics/MultiDiagnostics.H"
#include "Diagnostics/ReducedDiags/MultiReducedDiags.H"

//! Linker anchor: keeps this registration object in the aggregate archive.
extern "C" void warpx_diagnostics_anchor (void) {}

namespace warpx::runtime
{
namespace
{
std::unique_ptr<IDiagnosticsService>
make_diagnostics (const RuntimeContext* ctx)
{
    return std::make_unique<MultiDiagnostics>(ctx);
}

std::unique_ptr<IReducedDiagsService>
make_reduced_diags (const RuntimeContext* ctx)
{
    return std::make_unique<MultiReducedDiags>(ctx);
}

// Static registration: Core looks the factory up, so the kernel does not link
// against this plugin and this plugin can be omitted from a build.
const bool diagnostics_factories_registered = [] {
    register_diagnostics_factory(&make_diagnostics);
    register_reduced_diags_factory(&make_reduced_diags);
    return true;
}();
} // namespace
} // namespace warpx::runtime
