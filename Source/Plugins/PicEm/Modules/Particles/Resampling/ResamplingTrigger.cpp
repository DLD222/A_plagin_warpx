/* Copyright 2019-2020 Neil Zaim
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause-LBNL
 */
#include "ResamplingTrigger.H"

#include "Runtime/RuntimeContext.H"
#include "Runtime/RuntimeData.H"
#include "Runtime/WarpXRuntimeData.H"
#include "Runtime/LegacyServices.H"

#include "Utils/Parser/ParserUtils.H"
#include "Runtime/LegacyServices.H"

#include <AMReX_BoxArray.H>
#include <AMReX_ParmParse.H>

#include <vector>

ResamplingTrigger::ResamplingTrigger (const std::string& species_name,
                                      const warpx::runtime::RuntimeContext* ctx)
    : m_runtime_context(ctx)
{
    const amrex::ParmParse pp_species_name(species_name);

    std::vector<std::string> resampling_trigger_int_string_vec = {"0"};
    pp_species_name.queryarr("resampling_trigger_intervals", resampling_trigger_int_string_vec);
    m_resampling_intervals =
        ablastr::utils::text::IntervalsParser(resampling_trigger_int_string_vec);

    utils::parser::queryWithParser(
        pp_species_name, "resampling_trigger_max_avg_ppc", m_max_avg_ppc);
}

bool ResamplingTrigger::triggered (const int timestep, const amrex::Real global_numparts) const
{
    if (!m_initialized) {initialize_global_numcells();};

    const amrex::Real avg_ppc = global_numparts/m_global_numcells;
    return (m_resampling_intervals.contains(timestep) ||
            avg_ppc > m_max_avg_ppc);
}

void ResamplingTrigger::initialize_global_numcells () const
{
    for (int lev = 0; lev <= data().maxLevel(); lev++)
    {
        m_global_numcells += data().boxArray(lev).numPts();
    }
    m_initialized = true;
}

const warpx::runtime::RuntimeData&
ResamplingTrigger::data () const
{
    if (m_runtime_context != nullptr && m_runtime_context->data != nullptr) { return *m_runtime_context->data; }
    return warpx::runtime::legacy_data();
}
