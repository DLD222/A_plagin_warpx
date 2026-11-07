/* Copyright 2025 The WarpX Community
 *
 * This file is part of WarpX.
 *
 * Authors: S. Eric Clark (Helion Energy)
 *
 * License: BSD-3-Clause-LBNL
 */

#include "Particles/Deposition/VarianceAccumulationBuffer.H"
#include "Particles/Deposition/TemperatureDeposition.H"
#include "Fields.H"
#include "Runtime/RuntimeConfig.H"
#include "Runtime/RuntimeContext.H"
#include "Runtime/HostServices.H"
#include "Runtime/RuntimeData.H"
#include "Runtime/WarpXHostServices.H"
#include "Runtime/WarpXRuntimeData.H"
#include "WarpX.H"

#include <ablastr/utils/Communication.H>

#include <AMReX.H>
#include <AMReX_REAL.H>

using namespace amrex::literals;
using namespace warpx::particles::deposition;

VarianceAccumulationBuffer::VarianceAccumulationBuffer (ablastr::fields::MultiLevelVectorField const& T_vf,
                                                        std::string const& species_name,
                                                        const warpx::runtime::RuntimeContext* ctx ) :
    m_species_name(species_name), m_runtime_context(ctx)
{
    using ablastr::fields::Direction;

    const int ncomps = 1;

    m_nsamples.resize(data().finestLevel() + 1);

    for (int lev = 0; lev <= data().finestLevel(); ++lev) {
        for (int idir = 0; idir < 3; ++idir) {
            amrex::BoxArray const& ba = T_vf[lev][Direction{idir}]->boxArray();
            amrex::DistributionMapping const& dm = T_vf[lev][Direction{idir}]->DistributionMap();
            amrex::IntVect const& ng = T_vf[lev][Direction{idir}]->nGrowVect();

            const_cast<ablastr::fields::MultiFabRegister&>(data().fields()).alloc_init("variance_buffer_w_" + m_species_name, Direction{idir}, lev, ba, dm, ncomps, ng, 0.0_rt);
            const_cast<ablastr::fields::MultiFabRegister&>(data().fields()).alloc_init("variance_buffer_w2_" + m_species_name, Direction{idir}, lev, ba, dm, ncomps, ng, 0.0_rt);
            const_cast<ablastr::fields::MultiFabRegister&>(data().fields()).alloc_init("variance_buffer_vbar_" + m_species_name, Direction{idir}, lev, ba, dm, ncomps, ng, 0.0_rt);

            m_nsamples[lev][idir] = std::make_unique<amrex::iMultiFab>(ba, dm, ncomps, ng);
            m_nsamples[lev][idir]->setVal(0);
        }
    }
}

void
VarianceAccumulationBuffer::reset ()
{
    using ablastr::fields::Direction;

    for (int lev = 0; lev <= data().finestLevel(); ++lev) {
        for (int idir = 0; idir < 3; ++idir) {
            const_cast<ablastr::fields::MultiFabRegister&>(data().fields()).get("variance_buffer_w_" + m_species_name, Direction{idir}, lev)->setVal(0._rt);
            const_cast<ablastr::fields::MultiFabRegister&>(data().fields()).get("variance_buffer_w2_" + m_species_name, Direction{idir}, lev)->setVal(0._rt);
            const_cast<ablastr::fields::MultiFabRegister&>(data().fields()).get("variance_buffer_vbar_" + m_species_name, Direction{idir}, lev)->setVal(0._rt);
            m_nsamples[lev][idir]->setVal(0);
        }
    }
}

amrex::MultiFab*
VarianceAccumulationBuffer::get(const std::string& arr, ablastr::fields::Direction dir, int lev)
{
    return const_cast<ablastr::fields::MultiFabRegister&>(data().fields()).get("variance_buffer_" + arr + "_" + m_species_name, dir, lev);
}

amrex::iMultiFab*
VarianceAccumulationBuffer::get_n(ablastr::fields::Direction dir, int lev)
{
    return m_nsamples[lev][dir].get();
}

void
VarianceAccumulationBuffer::ConvertVarianceToTemperatureAndFilter (
    ablastr::fields::MultiLevelVectorField const& var_vf,
    amrex::Real normalization_factor,
    bool apply_filter)
{
    using ablastr::fields::Direction;

    for (int lev = 0; lev <= data().finestLevel(); ++lev) {
        auto const& periodicity = data().geom(lev).periodicity();

        for (int idir = 0; idir < 3; ++idir) {
            // Multiplies internal cells to convert variance to temperature
            var_vf[lev][Direction{idir}]->mult(normalization_factor, 0, 1);

            amrex::Gpu::streamSynchronize();

            // Synchronize Ghost cells after normalization.
            ablastr::utils::communication::FillBoundary(
                *var_vf[lev][Direction{idir}],
                config().do_single_precision_comms(),
                periodicity,
                true);

            // If filtering, apply filter
            if (apply_filter) {
                amrex::Gpu::streamSynchronize();

                host().fieldAlgebra().applyFilterMF(var_vf, lev, idir);

                amrex::Gpu::streamSynchronize();

                // Re-synchronize MF after filtering
                ablastr::utils::communication::FillBoundary(
                    *var_vf[lev][Direction{idir}],
                    config().do_single_precision_comms(),
                    periodicity,
                    true);
            }
        }
    }
    amrex::Gpu::streamSynchronize();
}

const warpx::runtime::RuntimeConfig&
VarianceAccumulationBuffer::config () const
{
    if (m_runtime_context != nullptr && m_runtime_context->config != nullptr) { return *m_runtime_context->config; }
    static const warpx::runtime::RuntimeConfig legacy{};
    return legacy;
}

const warpx::runtime::RuntimeData&
VarianceAccumulationBuffer::data () const
{
    if (m_runtime_context != nullptr && m_runtime_context->data != nullptr) { return *m_runtime_context->data; }
    static const warpx::runtime::WarpXRuntimeData legacy(WarpX::GetInstance());
    return legacy;
}

warpx::runtime::HostServices&
VarianceAccumulationBuffer::host () const
{
    if (m_runtime_context != nullptr && m_runtime_context->host != nullptr) { return *m_runtime_context->host; }
    static warpx::runtime::WarpXHostServices legacy(WarpX::GetInstance());
    return legacy;
}
