/* Copyright 2019-2020 Maxence Thevenet, Yinjian Zhao
 *
#include "Runtime/IReducedDiagsService.H"
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause-LBNL
 */

#ifndef WARPX_DIAGNOSTICS_REDUCEDDIAGS_MULTIREDUCEDDIAGS_H_
#define WARPX_DIAGNOSTICS_REDUCEDDIAGS_MULTIREDUCEDDIAGS_H_

#include "Runtime/IReducedDiagsService.H"

#include "MultiReducedDiags_fwd.H"

#include "ReducedDiags.H"

#include <memory>
#include <string>
#include <vector>

namespace warpx::runtime { struct RuntimeContext; }

/**
 * This class holds multiple instances of ReducedDiagnostics, and contains
 * general functions to initialize, compute, and write these diagnostics
 * to file.
 */
class MultiReducedDiags : public warpx::runtime::IReducedDiagsService
{
public:

    /// Bool: whether or not reduced diagnostics are activated
    int m_plot_rd = 0;

    /// names of reduced diagnostics
    std::vector<std::string> m_rd_names;

    /// m_multi_rd stores a pointer to each reduced diagnostics
    std::vector<std::unique_ptr<ReducedDiags>> m_multi_rd;

    /// non-owning runtime context (nullptr on the legacy/Python path)
    const warpx::runtime::RuntimeContext* m_ctx = nullptr;

    /// constructor
    explicit MultiReducedDiags (const warpx::runtime::RuntimeContext* ctx = nullptr);

    /** Loop over all ReducedDiags and call their InitData
     */
    void InitData () override;

    /** Redistribute parallel data structures during load balance
     */
    void LoadBalance () override;

    /** Loop over all ReducedDiags and call their ComputeDiags
     *  @param[in] step current iteration time */
    void ComputeDiags (int step) override;

    /** Loop over all ReducedDiags and call their ComputeDiagsMidStep
     *  @param[in] step current iteration time
     *  @param[in] dt time step size */
    void ComputeDiagsMidStep (int step, amrex::Real dt) override;

    /** Loop over all ReducedDiags and call their WriteToFile
     *  @param[in] step current iteration time */
    void WriteToFile (int step) override;

    /** Check if any diagnostics will be done */
    bool DoDiags(int step) override;

    /** \brief Loop over all ReducedDiags and call their WriteCheckpointData
     *  @param[in] dir checkpoint directory */
    void WriteCheckpointData (std::string const & dir) override;

    /** \brief Loop over all ReducedDiags and call their ReadCheckpointData
     *  @param[in] dir checkpoint directory */
    void ReadCheckpointData (std::string const & dir) override;
    [[nodiscard]] int plotReducedDiags () const override { return m_plot_rd; }
};

#endif
