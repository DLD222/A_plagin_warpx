#ifndef WARPX_MULTIDIAGNOSTICS_H_
#define WARPX_MULTIDIAGNOSTICS_H_
#include "Runtime/IDiagnosticsService.H"
#include "Runtime/IParticleContainerAccess.H"

#include "Diagnostics.H"

#include "MultiDiagnostics_fwd.H"

#include <AMReX_Vector.H>

#include <memory>
#include <string>
#include <vector>

namespace warpx::runtime { struct RuntimeContext; }


/**
 * \brief This class contains a vector of all diagnostics in the simulation.
 */
class MultiDiagnostics : public warpx::runtime::IDiagnosticsService
{
public:
public:
    explicit MultiDiagnostics (const warpx::runtime::RuntimeContext* ctx = nullptr);
    /** \brief Read Input parameters. Called in constructor. */
    void ReadParameters ();

    /** \brief Loop over diags in alldiags and call their InitDiags */
    void InitData (const warpx::runtime::IParticleContainerAccess& mpc) override;
    /** Check if any diagnostics will do compute and pack. */
    bool DoComputeAndPack (int step, bool force_flush=false) override;
    /** \brief Called at each iteration. Compute diags and flush. */
    void FilterComputePackFlush (int step, bool force_flush=false, bool BackTransform=false) override;
    /** \brief Called only at the last iteration. Loop over each diag and if m_dump_last_timestep
     *         is true, compute diags and flush with force_flush=true. */
    void FilterComputePackFlushLastTimestep (int step) override;
    /** \brief Loop over diags in all diags and call their InitializeFieldFunctors.
               Called when a new partitioning is generated at level, lev.
      * \param[in] lev level at this the field functors are initialized.
      */
    void InitializeFieldFunctors (int lev) override;
    void ResizeLevels (int new_finest) override;
    void RemakeLevel (int lev) override;
    /** Start a new iteration, i.e., dump has not been done yet. */
    void NewIteration () override;
    Diagnostics& GetDiag(int idiag) {return *alldiags[idiag]; }
    [[nodiscard]] int GetTotalDiags() const override {return ndiags;}
    DiagTypes diagstypes(int idiag) {return diags_types[idiag];}
    [[nodiscard]] std::string diagName (int i) const override { return alldiags[i]->GetName(); }
    [[nodiscard]] bool isDiagBackTransformed (int i) const override { return diags_types[i] == DiagTypes::BackTransformed; }
    [[nodiscard]] int diagNumBuffers (int i) const override { return alldiags[i]->getnumbuffers(); }
    [[nodiscard]] amrex::Real diagTlab (int i, int b) const override { return alldiags[i]->gettlab(b); }
    [[nodiscard]] int diagBufferKIndexHi (int i, int b) const override { return alldiags[i]->get_buffer_k_index_hi(b); }
    [[nodiscard]] amrex::Real diagSnapshotDomainLo (int i, int b, int d) const override { return alldiags[i]->get_snapshot_domain_lo(b, d); }
    [[nodiscard]] amrex::Real diagSnapshotDomainHi (int i, int b, int d) const override { return alldiags[i]->get_snapshot_domain_hi(b, d); }
    [[nodiscard]] int diagFlushCounter (int i, int b) const override { return alldiags[i]->get_flush_counter(b); }
    [[nodiscard]] int diagLastValidZslice (int i, int b) const override { return alldiags[i]->get_last_valid_Zslice(b); }
    [[nodiscard]] int diagSnapshotFullFlag (int i, int b) const override { return alldiags[i]->get_snapshot_full_flag(b); }
private:
    /** Vector of pointers to all diagnostics */
    amrex::Vector<std::unique_ptr<Diagnostics> > alldiags;
    int ndiags = 0; /**< number of different diagnostics */
    const warpx::runtime::RuntimeContext* m_ctx = nullptr;
    std::vector<std::string> diags_names;
    /**Type of each diagnostics*/
    std::vector<DiagTypes> diags_types;
};

#endif // WARPX_MULTIDIAGNOSTICS_H_
