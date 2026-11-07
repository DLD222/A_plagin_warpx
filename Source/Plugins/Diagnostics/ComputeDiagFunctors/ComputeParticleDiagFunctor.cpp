/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "ComputeParticleDiagFunctor.H"
#include "Diagnostics/DiagnosticsDependencies.H"
#include "Runtime/IParticleContainerAccess.H"
#include "Runtime/ISpeciesMetadata.H"

#include "Runtime/RuntimeContext.H"
#include "Runtime/ILegacyRuntime.H"
#include "Runtime/RuntimeData.H"

ComputeParticleDiagFunctor::ComputeParticleDiagFunctor () = default;

ComputeParticleDiagFunctor::~ComputeParticleDiagFunctor () = default;

const warpx::runtime::RuntimeData&
ComputeParticleDiagFunctor::data () const
{
    if (m_runtime_context != nullptr && m_runtime_context->data != nullptr) {
        return *m_runtime_context->data;
    }
    if (!m_legacy_data) {
        m_legacy_data = warpx::runtime::create_legacy_runtime_data();
    }
    return *m_legacy_data;
}
