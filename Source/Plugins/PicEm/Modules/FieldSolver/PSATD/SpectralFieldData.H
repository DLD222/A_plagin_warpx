/* Copyright 2019 David Grote, Maxence Thevenet, Remi Lehe
 * Revathi Jambunathan
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause-LBNL
 */
#ifndef WARPX_SPECTRAL_FIELD_DATA_H_
#define WARPX_SPECTRAL_FIELD_DATA_H_

#include "SpectralFieldData_fwd.H"

#include "SpectralKSpace.H"

#include "Runtime/SpectralFieldIndex.H"
#include "Utils/WarpXAlgorithmSelection.H"
#include "Utils/WarpX_Complex.H"

#include <ablastr/math/fft/AnyFFT.H>

#include <AMReX_BaseFab.H>
#include <AMReX_Config.H>
#include <AMReX_Extension.H>
#include <AMReX_FabArray.H>
#include <AMReX_IndexType.H>
#include <AMReX_MultiFab.H>

namespace warpx::runtime { struct RuntimeContext; class RuntimeData; }

namespace warpx::runtime { struct RuntimeContext; class RuntimeData; }
#include <AMReX_Vector.H>

#include <AMReX_BaseFwd.H>

#include <vector>

// Declare type for spectral fields
using SpectralField = amrex::FabArray< amrex::BaseFab <Complex> >;


/** \brief Class that stores the fields in spectral space, and performs the
 *  Fourier transforms between real space and spectral space
 */
class SpectralFieldData
{

    public:
        SpectralFieldData( const amrex::BoxArray& realspace_ba,
                           const SpectralKSpace& k_space,
                           const amrex::DistributionMapping& dm,
                           int n_field_required,
                           bool periodic_single_box,
                           const warpx::runtime::RuntimeContext* ctx = nullptr);
        SpectralFieldData() = default; // Default constructor
        ~SpectralFieldData();

        // default move and copy operations
        SpectralFieldData(const SpectralFieldData&) = delete;
        SpectralFieldData& operator=(const SpectralFieldData&) = delete;
        SpectralFieldData(SpectralFieldData&&) = default;
        SpectralFieldData& operator=(SpectralFieldData&& field_data) = default;

        void ForwardTransform (int lev,
                               const amrex::MultiFab& mf, int field_index,
                               int i_comp);

        void BackwardTransform (int lev, amrex::MultiFab& mf, int field_index,
                                const amrex::IntVect& fill_guards, int i_comp);

        // `fields` stores fields in spectral space, as multicomponent FabArray
        SpectralField fields;

    private:
        // tmpRealField and tmpSpectralField store fields
        // right before/after the Fourier transform
        SpectralField tmpSpectralField; // contains Complexs
        amrex::MultiFab tmpRealField; // contains Reals
        ablastr::math::anyfft::FFTplans forward_plan, backward_plan;
        // Correcting "shift" factors when performing FFT from/to
        // a cell-centered grid in real space, instead of a nodal grid
        // (0,1,2) is the dimension number
        SpectralShiftFactor shift0_FFTfromCell, shift0_FFTtoCell,
                            shift1_FFTfromCell, shift1_FFTtoCell,
                            shift2_FFTfromCell, shift2_FFTtoCell;

        bool m_periodic_single_box;

        [[nodiscard]] const warpx::runtime::RuntimeData& data () const;
        const warpx::runtime::RuntimeContext* m_runtime_context = nullptr;
};

#endif // WARPX_SPECTRAL_FIELD_DATA_H_
