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

    std::function<HostServices& ()>& host_provider ()
    {
        static std::function<HostServices& ()> fn;
        return fn;
    }
}

void set_legacy_data (std::function<RuntimeData& ()> a_provider)
{
    data_provider() = std::move(a_provider);
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
