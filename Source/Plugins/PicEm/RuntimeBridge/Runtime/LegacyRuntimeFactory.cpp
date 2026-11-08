#include "Runtime/ILegacyRuntime.H"
#include "Runtime/ServiceFactory.H"
#include "Runtime/WarpXHostServices.H"
#include "Runtime/WarpXRuntimeData.H"
#include "WarpX.H"
//! Linker anchor: keeps this registration object in the aggregate archive.
extern "C" void warpx_picem_legacy_anchor (void) {}

namespace warpx::runtime {
namespace {
std::unique_ptr<RuntimeData> make_legacy_runtime_data ()
{ return std::make_unique<WarpXRuntimeData>(WarpX::GetInstance()); }
std::unique_ptr<HostServices> make_legacy_host_services ()
{ return std::make_unique<WarpXHostServices>(WarpX::GetInstance()); }

// Static registration: Core looks the factory up, so the diagnostics plugin
// does not link against the kernel.
const bool legacy_factories_registered = [] {
    register_legacy_runtime_factories(&make_legacy_runtime_data, &make_legacy_host_services);
    return true;
}();
} // namespace
}
