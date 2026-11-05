/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_RUNTIME_HOSTSERVICES_H_
#define WARPX_RUNTIME_HOSTSERVICES_H_

#include <ablastr/fields/MultiFabRegister.H>
#include <ablastr/utils/Enums.H>

#include <AMReX_REAL.H>
#include <AMReX_MultiFab.H>
#include <AMReX_iMultiFab.H>
#include <AMReX_Vector.H>

#include <array>
#include <memory>
#include <iosfwd>
#include <string>
#include <vector>

namespace amrex { class AmrMesh; }

class ParticleBoundaryBuffer;

namespace warpx::runtime
{

/**
 * \brief Kernel services that diagnostics need but that are not data.
 *
 * This is audit D3's HostServices minimum: the plugin/diagnostic depends on
 * this interface only, never on WarpX. Implementations may delegate to WarpX.
 */
class HostServices
{
public:
    virtual ~HostServices () = default;

    //! Fill E/B guards, update auxiliary data and its guards (diagnostics only).
    virtual void prepareFieldsForDiagnostics () = 0;

    //! Field algebra used by diagnostics.
    virtual void computeDivB (amrex::MultiFab& divB, int dcomp,
                              ablastr::fields::VectorField const& B,
                              const std::array<amrex::Real, 3>& dx) = 0;
    virtual void computeDivE (amrex::MultiFab& divE, int lev) = 0;

    //! Deposit/finalize current on level (current_fp is updated in place).
    virtual void depositCurrent (int lev, amrex::Real dt) = 0;

    //! Deposit/finalize charge density on level.
    [[nodiscard]] virtual std::unique_ptr<amrex::MultiFab>
    computeRho (int lev, int species_index, bool apply_rz_psatd_filter) = 0;

    //! Electrostatic rho synchronization and finalization.
    virtual void syncRho (
        ablastr::fields::MultiLevelScalarField const& rho_fp,
        ablastr::fields::MultiLevelScalarField const& rho_cp,
        ablastr::fields::MultiLevelScalarField const& rho_buffer) = 0;
    virtual void finalizeRhoOnLevel (int lev, amrex::MultiFab* rho,
                                     ablastr::utils::enums::PatchType patch) = 0;
    virtual void applyFilterandSumBoundaryRho (int lev_min, int lev_max,
                                               amrex::MultiFab& rho, int scomp, int ncomp) = 0;

    //! Apply the configured filter to one direction of a multi-level vector field.
    virtual void applyFilterMF (ablastr::fields::MultiLevelVectorField const& field,
                                int lev, int idir) = 0;

    //! EB particle-shape reduction flags.
    [[nodiscard]] virtual const amrex::Vector<std::unique_ptr<amrex::iMultiFab>>&
    ebReduceParticleShapeFlag () const = 0;

    //! Synchronize current (and guard cells) for a named current field.
    virtual void syncCurrent (const std::string& current_fp_string) = 0;

    //! RZ current-density volume scaling.
    virtual void applyInverseVolumeScalingToCurrentDensity (
        amrex::MultiFab* Jx, amrex::MultiFab* Jy, amrex::MultiFab* Jz, int lev) = 0;

    //! Electrostatic potential on level (uses phi_fp when available).
    [[nodiscard]] virtual std::unique_ptr<amrex::MultiFab> computePhi (int lev) = 0;

    //! Particle boundary buffer owned by the kernel.
    virtual ParticleBoundaryBuffer& particleBoundaryBuffer () = 0;

    //! Mutable run-control (BTD may extend max_step/stop_time).
    virtual void updateMaxStep (int new_max_step) = 0;
    virtual void updateStopTime (amrex::Real new_stop_time) = 0;

    //! Moving window metadata.
    [[nodiscard]] virtual amrex::Real movingWindowX () const = 0;
    [[nodiscard]] virtual bool movingWindowActive (int step) const = 0;
    [[nodiscard]] virtual int numCellsShifted (amrex::Real distance, amrex::Real dx) const = 0;
    [[nodiscard]] virtual bool doMovingWindow () const = 0;
    [[nodiscard]] virtual amrex::Real timeOfLastGalileanShift () const = 0;

    //! PML checkpoint.
    [[nodiscard]] virtual bool doPML () const = 0;
    virtual void checkpointPML (int lev, const std::string& prefix,
                                ablastr::fields::MultiFabRegister& fields) = 0;

    //! In-situ / output metadata.
    [[nodiscard]] virtual amrex::AmrMesh* amrMesh () = 0;
    [[nodiscard]] virtual std::vector<bool> pmlDirections () const = 0;
    [[nodiscard]] virtual std::string authors () const = 0;
    virtual void writeParticleHeader (std::ostream& os) = 0;
    virtual void writeReducedDiagCheckpoint (const std::string& dir) = 0;

    //! Time vectors for plotfile headers.
    [[nodiscard]] virtual amrex::Vector<int> nsubsteps () const = 0;
    [[nodiscard]] virtual amrex::Vector<amrex::Real> tNew () const = 0;
    [[nodiscard]] virtual amrex::Vector<amrex::Real> tOld () const = 0;
    [[nodiscard]] virtual amrex::Vector<amrex::Real> dt () const = 0;

    //! Diagnostic registry (plotfile header).
    [[nodiscard]] virtual int totalDiags () const = 0;
    [[nodiscard]] virtual std::string diagName (int i) const = 0;
    [[nodiscard]] virtual bool isDiagBackTransformed (int i) const = 0;
    [[nodiscard]] virtual int diagNumBuffers (int i) const = 0;
    [[nodiscard]] virtual amrex::Real diagTlab (int i, int buffer) const = 0;
    [[nodiscard]] virtual int diagBufferKIndexHi (int i, int buffer) const = 0;
    [[nodiscard]] virtual amrex::Real diagSnapshotDomainLo (int i, int buffer, int dim) const = 0;
    [[nodiscard]] virtual amrex::Real diagSnapshotDomainHi (int i, int buffer, int dim) const = 0;
    [[nodiscard]] virtual int diagFlushCounter (int i, int buffer) const = 0;
    [[nodiscard]] virtual int diagLastValidZslice (int i, int buffer) const = 0;
    [[nodiscard]] virtual int diagSnapshotFullFlag (int i, int buffer) const = 0;
    [[nodiscard]] virtual int hybridPicSubsteps () const = 0;
    [[nodiscard]] virtual bool hasHybridPICModel () const = 0;
};

} // namespace warpx::runtime

#endif // WARPX_RUNTIME_HOSTSERVICES_H_
