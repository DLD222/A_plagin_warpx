#include "Runtime/IDiagnosticsService.H"
#include "WarpX.H"
#include "Runtime/IParticleContainerAccess.H"
#include "Runtime/ISpeciesMetadata.H"
#include "Runtime/IReducedDiagsService.H"

#include "Diagnostics/MultiDiagnostics.H"
#include "Diagnostics/ReducedDiags/MultiReducedDiags.H"

namespace warpx::runtime
{
std::unique_ptr<IDiagnosticsService>
create_diagnostics_service (const RuntimeContext* ctx)
{
    return std::make_unique<MultiDiagnostics>(ctx);
}
std::unique_ptr<IReducedDiagsService>
create_reduced_diags_service (const RuntimeContext* ctx)
{
    return std::make_unique<MultiReducedDiags>(ctx);
}
} // namespace warpx::runtime
