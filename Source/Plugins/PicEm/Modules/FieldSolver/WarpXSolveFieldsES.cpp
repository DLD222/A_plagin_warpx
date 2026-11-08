/* Copyright 2024 The WarpX Community
 *
 * This file is part of WarpX.
 *
 * Authors: Remi Lehe, Roelof Groenewald, Arianna Formenti, Revathi Jambunathan
 *
 * License: BSD-3-Clause-LBNL
 */
#include "Runtime/IElectrostaticSolver.H"

#include "Fields.H"
#include "WarpX.H"

#include "Runtime/HostServices.H"
#include "Runtime/IParticleServices.H"
#include "Runtime/ParticleContext.H"
#include "Runtime/RuntimeContext.H"
#include "Runtime/WarpXHostServices.H"
#include "Runtime/WarpXRuntimeData.H"

#include <ablastr/profiler/ProfilerWrapper.H>

warpx::runtime::ParticleContext
WarpX::particleContext ()
{
    if (const auto* ctx = GetRuntimeContext(); ctx != nullptr && ctx->data != nullptr &&
        ctx->host != nullptr) {
        return {&ctx->data->particleAccess(), &ctx->host->particleServices()};
    }
    if (!m_legacy_host) {
        m_legacy_data = std::make_unique<warpx::runtime::WarpXRuntimeData>(*this);
        m_legacy_host = std::make_unique<warpx::runtime::WarpXHostServices>(*this);
        m_legacy_config = std::make_unique<warpx::runtime::RuntimeConfig>();
    }
    return {&m_legacy_data->particleAccess(), &m_legacy_host->particleServices()};
}

void WarpX::ComputeSpaceChargeField (bool const reset_E_field, bool const reset_B_field,
                                     bool const verbose_step)
{
    ABLASTR_PROFILE("WarpX::ComputeSpaceChargeField");
    using ablastr::fields::Direction;
    using warpx::fields::FieldType;

    // Reset E and B fields to 0, before calculating space-charge fields if requested
    for (int lev = 0; lev <= max_level; lev++) {
        for (int comp=0; comp<3; comp++) {
            if (reset_E_field) {
                m_fields.get(FieldType::Efield_fp, Direction{comp}, lev)->setVal(0);
            }
            if (reset_B_field) {
                m_fields.get(FieldType::Bfield_fp, Direction{comp}, lev)->setVal(0);
            }
        }
    }

    m_electrostatic_solver->ComputeSpaceChargeField(
        m_fields, particleContext(), myfl.get(), max_level, verbose_step);
}
