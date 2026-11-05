/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_RUNTIME_WARPXHOSTSERVICES_H_
#define WARPX_RUNTIME_WARPXHOSTSERVICES_H_

#include "Runtime/HostServices.H"

class WarpX;

namespace warpx::runtime
{

/** HostServices adapter over WarpX (transitional, not for external plugins). */
class WarpXHostServices final : public HostServices
{
public:
    explicit WarpXHostServices (WarpX& warpx) : m_warpx(warpx) {}

    void prepareFieldsForDiagnostics () override;
    void computeDivB (amrex::MultiFab& divB, int dcomp,
                      ablastr::fields::VectorField const& B,
                      const std::array<amrex::Real, 3>& dx) override;
    void computeDivE (amrex::MultiFab& divE, int lev) override;

    void depositCurrent (int lev, amrex::Real dt) override;
    [[nodiscard]] std::unique_ptr<amrex::MultiFab>
    computeRho (int lev, int species_index, bool apply_rz_psatd_filter) override;
    [[nodiscard]] std::unique_ptr<amrex::MultiFab> computePhi (int lev) override;
    [[nodiscard]] amrex::Real movingWindowX () const override;
    [[nodiscard]] bool movingWindowActive (int step) const override;
    [[nodiscard]] int numCellsShifted (amrex::Real distance, amrex::Real dx) const override;
    [[nodiscard]] bool doMovingWindow () const override;
    [[nodiscard]] amrex::Real timeOfLastGalileanShift () const override;
    [[nodiscard]] bool doPML () const override;
    void checkpointPML (int lev, const std::string& prefix,
                        ablastr::fields::MultiFabRegister& fields) override;
    [[nodiscard]] amrex::AmrMesh* amrMesh () override;
    [[nodiscard]] std::vector<bool> pmlDirections () const override;
    [[nodiscard]] std::string authors () const override;
    void writeParticleHeader (std::ostream& os) override;
    void writeReducedDiagCheckpoint (const std::string& dir) override;
    [[nodiscard]] amrex::Vector<int> nsubsteps () const override;
    [[nodiscard]] amrex::Vector<amrex::Real> tNew () const override;
    [[nodiscard]] amrex::Vector<amrex::Real> tOld () const override;
    [[nodiscard]] amrex::Vector<amrex::Real> dt () const override;
    [[nodiscard]] int totalDiags () const override;
    [[nodiscard]] std::string diagName (int i) const override;
    [[nodiscard]] bool isDiagBackTransformed (int i) const override;
    [[nodiscard]] int diagNumBuffers (int i) const override;
    [[nodiscard]] amrex::Real diagTlab (int i, int buffer) const override;
    [[nodiscard]] int diagBufferKIndexHi (int i, int buffer) const override;
    [[nodiscard]] amrex::Real diagSnapshotDomainLo (int i, int buffer, int dim) const override;
    [[nodiscard]] amrex::Real diagSnapshotDomainHi (int i, int buffer, int dim) const override;
    [[nodiscard]] int diagFlushCounter (int i, int buffer) const override;
    [[nodiscard]] int diagLastValidZslice (int i, int buffer) const override;
    [[nodiscard]] int diagSnapshotFullFlag (int i, int buffer) const override;
    [[nodiscard]] int hybridPicSubsteps () const override;
    [[nodiscard]] bool hasHybridPICModel () const override;

private:
    WarpX& m_warpx;
};

} // namespace warpx::runtime

#endif // WARPX_RUNTIME_WARPXHOSTSERVICES_H_
