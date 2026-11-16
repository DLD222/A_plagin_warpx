#include "PlasmaBoundary.H"
#include "Runtime/IBoundaryConditions.H"
#include "Runtime/IEBGeometry.H"

namespace {
/** Self-registration: the plugin announces its factory to the core at load time.
 *
 *  Nothing in the core or in PicEm mentions `PlasmaBoundary`; a build without this
 *  plugin leaves `boundary_conditions_factory()` empty and every boundary call is a
 *  no-op.  This is the "plugin plugs into the core through an interface" rule.
 */
struct Registrar {
    Registrar ()
    {
        warpx::runtime::RegisterBoundaryConditions(
            [](warpx::runtime::IEBGeometry const& geometry)
                -> std::unique_ptr<warpx::runtime::IBoundaryConditions> {
                return std::make_unique<warpx::boundary::PlasmaBoundary>(geometry);
            });
    }
};
Registrar const g_registrar;
} // namespace
