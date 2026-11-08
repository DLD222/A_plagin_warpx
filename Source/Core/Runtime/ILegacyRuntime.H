#ifndef WARPX_RUNTIME_ILEGACYRUNTIME_H_
#define WARPX_RUNTIME_ILEGACYRUNTIME_H_
#include <memory>
namespace warpx::runtime {
class RuntimeData;
class HostServices;
std::unique_ptr<RuntimeData> create_legacy_runtime_data ();
std::unique_ptr<HostServices> create_legacy_host_services ();

//! Lazily created kernel-provided runtime data, kept alive by the core.
//! Solvers use this as their fallback when no runtime context is installed.
[[nodiscard]] const RuntimeData& legacy_runtime_data ();

//! Lazily created kernel-provided host services, kept alive by the core.
[[nodiscard]] HostServices& legacy_host_services ();
}
#endif
