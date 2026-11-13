/* Copyright 2026 The WarpX Community
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause-LBNL
 */
#ifndef WARPX_RUNTIME_LEGACYSERVICES_H_
#define WARPX_RUNTIME_LEGACYSERVICES_H_

#include "Runtime/FrameState.H"

#include <AMReX_LayoutData.H>

#include <functional>

namespace warpx::runtime
{

class RuntimeConfig;
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
void set_legacy_config (std::function<RuntimeConfig& ()> a_provider);

[[nodiscard]] RuntimeData& legacy_data ();
[[nodiscard]] HostServices& legacy_host ();
[[nodiscard]] RuntimeConfig& legacy_config ();

//! The kernel's load-balancing cost array for a level, or null when it does not keep one.
void set_legacy_costs (std::function<amrex::LayoutData<amrex::Real>* (int lev)> a_provider);
[[nodiscard]] amrex::LayoutData<amrex::Real>* legacy_costs (int lev);

//! The frame state (boost velocity and the like) the kernel registered.
void set_legacy_frame (std::function<FrameState& ()> a_provider);
[[nodiscard]] FrameState& legacy_frame ();

} // namespace warpx::runtime

#endif // WARPX_RUNTIME_LEGACYSERVICES_H_
