/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "ComputeDiagFunctor.H"
#include "WarpX.H"
#include "Runtime/IParticleContainerAccess.H"
#include "Runtime/ISpeciesMetadata.H"

#include "Runtime/RuntimeConfigSnapshot.H"

#include "Runtime/RuntimeContext.H"
#include "Runtime/ILegacyRuntime.H"
#include "Runtime/RuntimeData.H"
#include "Runtime/HostServices.H"
#include "Runtime/FrameState.H"
#include "Runtime/RuntimeConfig.H"
#include "Runtime/WarpXHostServices.H"
#include "Runtime/WarpXRuntimeData.H"

ComputeDiagFunctor::ComputeDiagFunctor (int ncomp, amrex::IntVect crse_ratio)
    : m_ncomp(ncomp), m_crse_ratio(crse_ratio)
{
}

ComputeDiagFunctor::~ComputeDiagFunctor () = default;

const warpx::runtime::RuntimeData&
ComputeDiagFunctor::data () const
{
    if (m_runtime_context != nullptr && m_runtime_context->data != nullptr) {
        return *m_runtime_context->data;
    }
    if (!m_legacy_data) {
        m_legacy_data = warpx::runtime::create_legacy_runtime_data();
    }
    return *m_legacy_data;
}

warpx::runtime::HostServices&
ComputeDiagFunctor::host () const
{
    if (m_runtime_context != nullptr && m_runtime_context->host != nullptr) {
        return *m_runtime_context->host;
    }
    if (!m_legacy_host) {
        m_legacy_host = warpx::runtime::create_legacy_host_services();
    }
    return *m_legacy_host;
}

const warpx::runtime::RuntimeConfigSnapshot&
ComputeDiagFunctor::config_snapshot () const
{
    if (m_runtime_context != nullptr && m_runtime_context->config_snapshot != nullptr) {
        return *m_runtime_context->config_snapshot;
    }
    static const warpx::runtime::RuntimeConfigSnapshot legacy{};
    return legacy;
}

const warpx::runtime::RuntimeConfig&
ComputeDiagFunctor::config () const
{
    if (m_runtime_context != nullptr && m_runtime_context->config != nullptr) {
        return *m_runtime_context->config;
    }
    static const warpx::runtime::RuntimeConfig legacy{};
    return legacy;
}

const warpx::runtime::FrameState&
ComputeDiagFunctor::frame () const
{
    if (m_runtime_context != nullptr && m_runtime_context->frame != nullptr) {
        return *m_runtime_context->frame;
    }
    static const warpx::runtime::FrameState legacy = warpx::runtime::FrameState::FromLegacy();
    return legacy;
}
