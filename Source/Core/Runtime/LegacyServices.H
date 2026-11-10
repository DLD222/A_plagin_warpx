/* Copyright 2026 The WarpX Community
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause-LBNL
 */
#ifndef WARPX_RUNTIME_LEGACYSERVICES_H_
#define WARPX_RUNTIME_LEGACYSERVICES_H_

#include <functional>

namespace warpx::runtime
{

class RuntimeData;
class HostServices;

/** Access to the services a kernel provides outside its plugin context.
 *
 * Plugin code that runs before a runtime context exists (the particle containers are
 * built in the kernel constructor, before the context is handed over) still needs the
 * data and host services.  Rather than reaching for the simulation object - which a
 * plugin must not name - it asks here, and the kernel registers the two providers once.
 *
 * Both throw a descriptive error when nothing was registered, which is what a
 * core-only or non-PIC build should see.
 */
void set_legacy_data (std::function<RuntimeData& ()> a_provider);
void set_legacy_host (std::function<HostServices& ()> a_provider);

[[nodiscard]] RuntimeData& legacy_data ();
[[nodiscard]] HostServices& legacy_host ();

} // namespace warpx::runtime

#endif // WARPX_RUNTIME_LEGACYSERVICES_H_
