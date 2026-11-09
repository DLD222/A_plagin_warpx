/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_RUNTIME_ISPECTRALSOLVER_H_
#define WARPX_RUNTIME_ISPECTRALSOLVER_H_

#include "Runtime/SpectralFieldIndex.H"

#include <ablastr/fields/MultiFabRegister.H>

#include <AMReX_Config.H>
#include <AMReX_MultiFab.H>

namespace warpx::runtime
{

/**
 * \brief What the kernel needs from a spectral (FFT) field solver.
 *
 * The kernel never names a concrete spectral solver: it asks for the transforms
 * it needs, the spectral field layout, and the two corrections that only a
 * spectral solver can do.  The declaration follows the per-dimension build
 * (WarpX compiles one dimension per binary), so the RZ overloads that take two
 * fields appear only in an RZ build, exactly like the helpers in the kernel.
 */
class ISpectralSolver
{
public:
    virtual ~ISpectralSolver () = default;

    //! Transform component \c i_comp of \c mf into spectral field \c field_index.
    virtual void ForwardTransform (int lev, amrex::MultiFab const& mf, int field_index,
                                   int i_comp = 0) = 0;

    /**
     * \brief Transform spectral field \c field_index back into component \c i_comp.
     *
     * \c fill_guards says which ghost cells the transform must also fill; a
     * solver that reconstructs them itself may ignore it.
     */
    virtual void BackwardTransform (
        int lev, amrex::MultiFab& mf, int field_index,
        amrex::IntVect const& fill_guards = amrex::IntVect::TheZeroVector(),
        int i_comp = 0) = 0;

    //! Advance the fields in spectral space by one time step.
    virtual void pushSpectralFields (bool doing_pml = false) = 0;

    //! Divergence of E in spectral space (used by the div(E) cleaning terms).
    virtual void ComputeSpectralDivE (int lev, ablastr::fields::VectorField const& Efield,
                                      amrex::MultiFab& divE) = 0;

    //! Rho and current corrections that only a spectral solver can perform.
    virtual void CurrentCorrection () = 0;
    virtual void VayDeposition () = 0;

    //! Component bookkeeping on the spectral buffer.
    virtual void CopySpectralDataComp (int src_comp, int dest_comp) = 0;
    virtual void ZeroOutDataComp (int icomp) = 0;
    virtual void ScaleDataComp (int icomp, amrex::Real scale_factor) = 0;

    //! Which spectral components hold which fields.
    [[nodiscard]] virtual SpectralFieldIndex const& spectralIndex () const = 0;

    //! Time step the spectral update used, in seconds.
    [[nodiscard]] virtual amrex::Real spectralDt () const = 0;
};

} // namespace warpx::runtime

#endif // WARPX_RUNTIME_ISPECTRALSOLVER_H_
