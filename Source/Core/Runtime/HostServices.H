/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_RUNTIME_HOSTSERVICES_H_
#define WARPX_RUNTIME_HOSTSERVICES_H_

#include "Runtime/ICheckpoint.H"
#include "Runtime/IDiagnosticsMetadata.H"
#include "Runtime/IFieldAlgebra.H"
#include "Runtime/IMovingWindow.H"
#include "Runtime/IPML.H"
#include "Runtime/IParticleBoundaryBuffer.H"
#include "Runtime/IParticleServices.H"
#include "Runtime/IRunControl.H"

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

    virtual IFieldAlgebra& fieldAlgebra () = 0;
    virtual IParticleServices& particleServices () = 0;
    virtual IMovingWindow& movingWindow () = 0;
    virtual IPML& pml () = 0;
    virtual IDiagnosticsMetadata& diagnosticsMetadata () = 0;
    virtual ICheckpoint& checkpoint () = 0;
    virtual IRunControl& runControl () = 0;
    virtual IParticleBoundaryBuffer& particleBoundaryBufferService () = 0;
};

} // namespace warpx::runtime

#endif // WARPX_RUNTIME_HOSTSERVICES_H_
