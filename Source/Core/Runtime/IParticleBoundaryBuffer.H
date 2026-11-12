/* Copyright 2026 The WarpX Community
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause-LBNL
 */
#ifndef WARPX_RUNTIME_IPARTICLEBOUNDARYBUFFER_H_
#define WARPX_RUNTIME_IPARTICLEBOUNDARYBUFFER_H_

#include "Runtime/IParticleSystem.H"

#include <ablastr/fields/MultiFabRegister.H>

#include <AMReX_REAL.H>

#include <string>

namespace warpx::runtime
{

/** Particles that left the domain through a boundary.
 *
 * Diagnostics read it (how many of which species hit which boundary) and the kernel drives it
 * (collect at domain and embedded boundaries, redistribute on regrid).  Neither may know how
 * the buffers are stored, so both go through this contract; the storage lives in the particle
 * plugin.
 */
class IParticleBoundaryBuffer
{
public:
    virtual ~IParticleBoundaryBuffer () = default;

    // ---- diagnostics -------------------------------------------------------
    [[nodiscard]] virtual int getNumParticlesInContainer (const std::string& species, int boundary,
                                                          bool local) = 0;
    [[nodiscard]] virtual std::string boundaryName (int boundary) = 0;
    //! The particle data of one buffer, for a diagnostic to read (opaque on purpose: the
    //! diagnostic casts it to the core particle container type).
    virtual void* getParticleBufferPointer (const std::string& species, int boundary) = 0;
    virtual void clearParticles (int boundary) = 0;

    // ---- the kernel's step work -------------------------------------------
    //! Collect particles that reached a domain boundary at this time.
    virtual void gatherParticlesFromDomainBoundaries (IParticleSystem& particles,
                                                      amrex::Real cur_time) = 0;
    //! Collect particles absorbed by an embedded boundary.
    virtual void gatherParticlesFromEmbeddedBoundaries (
        IParticleSystem& particles,
        ablastr::fields::MultiLevelScalarField const& distance_to_eb,
        amrex::Real cur_time) = 0;
    //! Redistribute the buffered particles over the current boxes after a regrid.
    virtual void redistribute () = 0;
};

} // namespace warpx::runtime

#endif // WARPX_RUNTIME_IPARTICLEBOUNDARYBUFFER_H_
