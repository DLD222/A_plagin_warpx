/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_RUNTIME_SPECTRALFIELDINDEX_H_
#define WARPX_RUNTIME_SPECTRALFIELDINDEX_H_

#include "Utils/WarpXAlgorithmSelection.H"

/**
 * \brief Which component of the spectral buffer holds which field.
 *
 * The index layout is an implementation detail of the spectral solver, but the
 * kernel has to name the components when it hands real-space fields to the
 * transforms.  Keeping the plain component indices here lets the kernel read
 * them without knowing the solver that filled them.
 */
class SpectralFieldIndex
{
    public:

        /**
         * \brief Constructor of the class SpectralFieldIndex
         *
         * Set integer indices to access data in spectral space
         * and total number of fields to be stored.
         *
         * \param[in] update_with_rho whether rho is used in the field update equations
         * \param[in] time_averaging  whether the time averaging algorithm is used
         * \param[in] time_dependency_J       the PSATD-JRhom algorithm used (hence two currents
         *                            computed at the beginning and the end of the time interval
         *                            instead of one current computed at half time)
         * \param[in] time_dependency_rho     the multi-rho algorithm used (hence two densities
         *                            computed at the beginning and the end of the time interval
         *                            instead of one density computed at half time)
         * \param[in] dive_cleaning   whether to use div(E) cleaning to account for errors in
         *                            Gauss law (new field F in the update equations)
         * \param[in] divb_cleaning   whether to use div(B) cleaning to account for errors in
         *                            div(B) = 0 law (new field G in the update equations)
         * \param[in] pml             whether the indices are used to access spectral data
         *                            for the PML spectral solver
         * \param[in] pml_rz          whether the indices are used to access spectral data
         *                            for the RZ PML spectral solver
         */
        SpectralFieldIndex (bool update_with_rho,
                            bool time_averaging,
                            TimeDependencyJ time_dependency_J,
                            TimeDependencyRho time_dependency_rho,
                            bool dive_cleaning,
                            bool divb_cleaning,
                            bool pml,
                            bool pml_rz = false);

        /**
         * \brief Default constructor
         */
        SpectralFieldIndex () = default;

        /**
         * \brief Default destructor
         */
        ~SpectralFieldIndex () = default;

        /**
         * \brief Default Copy constructor
         */
        SpectralFieldIndex ( SpectralFieldIndex const &) = default;

        /**
         * \brief Default Copy operator
         */
        SpectralFieldIndex& operator= ( SpectralFieldIndex const & ) = default;

        /**
         * \brief Default Move constructor
         */
        SpectralFieldIndex ( SpectralFieldIndex&& ) = default;

        /**
         * \brief Default Move operator
         */
        SpectralFieldIndex& operator= ( SpectralFieldIndex&& ) = default;

    // Total number of fields that are actually allocated
    int n_fields;

    // Indices overwritten in the constructor, for the fields that are actually allocated
    // (index -1 will never be used, unless there is some bug in the code implementation,
    // which would result in a runtime crash due to out-of-bound accesses that can be detected
    // by running the code in DEBUG mode)

    // Always
    int Ex = -1, Ey = -1, Ez = -1;
    int Bx = -1, By = -1, Bz = -1;
    int divE = -1;

    // Time averaging
    int Ex_avg = -1, Ey_avg = -1, Ez_avg = -1;
    int Bx_avg = -1, By_avg = -1, Bz_avg = -1;

    // J
    int Jx_old = -1, Jy_old = -1, Jz_old = -1;
    int Jx_mid = -1, Jy_mid = -1, Jz_mid = -1;
    int Jx_new = -1, Jy_new = -1, Jz_new = -1;

    // rho
    int rho_old = -1, rho_mid = -1, rho_new = -1;

    // div(E) and div(B) cleaning
    int F = -1, G = -1;

    // PML
    int Exy = -1, Exz = -1, Eyx = -1, Eyz = -1, Ezx = -1, Ezy = -1;
    int Bxy = -1, Bxz = -1, Byx = -1, Byz = -1, Bzx = -1, Bzy = -1;

    // PML with div(E) and/or div(B) cleaning
    int Exx = -1, Eyy = -1, Ezz = -1, Bxx = -1, Byy = -1, Bzz = -1;
    int Fx  = -1, Fy  = -1, Fz  = -1, Gx  = -1, Gy  = -1, Gz  = -1;

    // PML RZ
    int Er_pml = -1, Et_pml = -1, Br_pml = -1, Bt_pml = -1;
};


#endif // WARPX_RUNTIME_SPECTRALFIELDINDEX_H_
