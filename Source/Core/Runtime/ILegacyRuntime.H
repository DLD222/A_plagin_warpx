#ifndef WARPX_RUNTIME_ILEGACYRUNTIME_H_
#define WARPX_RUNTIME_ILEGACYRUNTIME_H_
#include <memory>
namespace warpx::runtime {
class RuntimeData;
class HostServices;
std::unique_ptr<RuntimeData> create_legacy_runtime_data ();
std::unique_ptr<HostServices> create_legacy_host_services ();
}
#endif
