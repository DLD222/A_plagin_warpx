/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "ComputeDiagFunctor.H"

#include "Runtime/RuntimeContext.H"
#include "Runtime/RuntimeData.H"
#include "Runtime/WarpXRuntimeData.H"
#include "WarpX.H"

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
        m_legacy_data = std::make_unique<warpx::runtime::WarpXRuntimeData>(WarpX::GetInstance());
    }
    return *m_legacy_data;
}
