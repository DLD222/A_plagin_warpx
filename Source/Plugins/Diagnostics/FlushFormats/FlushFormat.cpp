/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "FlushFormat.H"
#include "Runtime/IParticleContainerAccess.H"
#include "Runtime/ISpeciesMetadata.H"

#include "Runtime/RuntimeContext.H"
#include "Runtime/RuntimeConfig.H"
#include "Runtime/HostServices.H"
#include "Runtime/RuntimeData.H"

#include <stdexcept>

namespace warpx::runtime
{
} // namespace warpx::runtime

const warpx::runtime::RuntimeData&
FlushFormat::data () const
{
    if (m_runtime_context == nullptr || m_runtime_context->data == nullptr) {
        throw std::runtime_error("FlushFormat: runtime context/data is not set");
    }
    return *m_runtime_context->data;
}

const warpx::runtime::RuntimeConfig&
FlushFormat::config () const
{
    if (m_runtime_context != nullptr && m_runtime_context->config != nullptr) {
        return *m_runtime_context->config;
    }
    static const warpx::runtime::RuntimeConfig legacy{};
    return legacy;
}

ablastr::fields::MultiFabRegister&
FlushFormat::fields () const
{
    return const_cast<warpx::runtime::RuntimeData&>(data()).fields();
}

warpx::runtime::HostServices&
FlushFormat::host () const
{
    if (m_runtime_context == nullptr || m_runtime_context->host == nullptr) {
        throw std::runtime_error("FlushFormat: runtime host services are not set");
    }
    return *m_runtime_context->host;
}
