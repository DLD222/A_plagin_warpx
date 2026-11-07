/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "ComputeParticleDiagFunctor.H"
#include "Runtime/IParticleContainerAccess.H"
#include "Runtime/ISpeciesMetadata.H"

#include "Runtime/RuntimeContext.H"
#include "Runtime/RuntimeData.H"
#include "Runtime/WarpXRuntimeData.H"
#include "WarpX.H"

ComputeParticleDiagFunctor::ComputeParticleDiagFunctor () = default;

ComputeParticleDiagFunctor::~ComputeParticleDiagFunctor () = default;

const warpx::runtime::RuntimeData&
ComputeParticleDiagFunctor::data () const
{
    if (m_runtime_context != nullptr && m_runtime_context->data != nullptr) {
        return *m_runtime_context->data;
    }
    if (!m_legacy_data) {
        m_legacy_data = std::make_unique<warpx::runtime::WarpXRuntimeData>(WarpX::GetInstance());
    }
    return *m_legacy_data;
}
