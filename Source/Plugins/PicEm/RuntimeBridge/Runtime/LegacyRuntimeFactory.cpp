#include "Runtime/ILegacyRuntime.H"
#include "Runtime/WarpXHostServices.H"
#include "Runtime/WarpXRuntimeData.H"
#include "WarpX.H"
namespace warpx::runtime {
std::unique_ptr<RuntimeData> create_legacy_runtime_data ()
{ return std::make_unique<WarpXRuntimeData>(WarpX::GetInstance()); }
std::unique_ptr<HostServices> create_legacy_host_services ()
{ return std::make_unique<WarpXHostServices>(WarpX::GetInstance()); }
}
