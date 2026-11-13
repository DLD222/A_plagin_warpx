/* Copyright 2026 The WarpX Community
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause-LBNL
 */
#include "LegacyServices.H"

#include <stdexcept>

namespace warpx::runtime
{

namespace
{
    std::function<RuntimeData& ()>& data_provider ()
    {
        static std::function<RuntimeData& ()> fn;
        return fn;
    }

    std::function<RuntimeConfig& ()>& config_provider ()
    {
        static std::function<RuntimeConfig& ()> fn;
        return fn;
    }

    std::function<HostServices& ()>& host_provider ()
    {
        static std::function<HostServices& ()> fn;
        return fn;
    }

    std::function<FrameState& ()>& frame_provider ()
    {
        static std::function<FrameState& ()> fn;
        return fn;
    }

    std::function<amrex::LayoutData<amrex::Real>* (int lev)>& costs_provider ()
    {
        static std::function<amrex::LayoutData<amrex::Real>* (int lev)> fn;
        return fn;
    }
}

void set_legacy_data (std::function<RuntimeData& ()> a_provider)
{
    data_provider() = std::move(a_provider);
}

void set_legacy_frame (std::function<FrameState& ()> a_provider)
{
    frame_provider() = std::move(a_provider);
}

FrameState& legacy_frame ()
{
    if (!frame_provider()) {
        throw std::logic_error(
            "No frame state is registered: the linked kernel does not expose runtime services "
            "(build with a PIC kernel plugin).");
    }
    return frame_provider()();
}

void set_legacy_costs (std::function<amrex::LayoutData<amrex::Real>* (int lev)> a_provider)
{
    costs_provider() = std::move(a_provider);
}

amrex::LayoutData<amrex::Real>* legacy_costs (int lev)
{
    if (!costs_provider()) { return nullptr; }
    return costs_provider()(lev);
}

void set_legacy_config (std::function<RuntimeConfig& ()> a_provider)
{
    config_provider() = std::move(a_provider);
}

void set_legacy_host (std::function<HostServices& ()> a_provider)
{
    host_provider() = std::move(a_provider);
}

RuntimeData& legacy_data ()
{
    if (!data_provider()) {
        throw std::logic_error(
            "No runtime data provider is registered: the linked kernel does not expose "
            "runtime services (build with a PIC kernel plugin).");
    }
    return data_provider()();
}

RuntimeConfig& legacy_config ()
{
    if (!config_provider()) {
        throw std::logic_error(
            "No runtime configuration is registered: the linked kernel does not expose "
            "runtime services (build with a PIC kernel plugin).");
    }
    return config_provider()();
}

HostServices& legacy_host ()
{
    if (!host_provider()) {
        throw std::logic_error(
            "No host services provider is registered: the linked kernel does not expose "
            "runtime services (build with a PIC kernel plugin).");
    }
    return host_provider()();
}

} // namespace warpx::runtime
