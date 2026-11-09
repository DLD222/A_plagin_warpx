/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "Runtime/SpectralFieldIndex.H"

#include <AMReX_Config.H>
#include <AMReX_Utility.H>

SpectralFieldIndex::SpectralFieldIndex (const bool update_with_rho,
                                        const bool time_averaging,
                                        const TimeDependencyJ time_dependency_J,
                                        const TimeDependencyRho time_dependency_rho,
                                        const bool dive_cleaning,
                                        const bool divb_cleaning,
                                        const bool pml,
                                        const bool pml_rz)
{
    // TODO Use these to allocate rho_old, rho_new, F, and G only when needed
    amrex::ignore_unused(update_with_rho);

    int c = 0;

    if (!pml)
    {
        Ex = c++; Ey = c++; Ez = c++;
        Bx = c++; By = c++; Bz = c++;

        // TODO Allocate rho_old and rho_new only when needed

        // Reuse data corresponding to index Bx = 3 to avoid storing extra memory
        divE = 3;

        if (time_averaging)
        {
            Ex_avg = c++; Ey_avg = c++; Ez_avg = c++;
            Bx_avg = c++; By_avg = c++; Bz_avg = c++;
        }

        if (dive_cleaning) { F = c++; }

        if (divb_cleaning) { G = c++; }

        if (time_dependency_J == TimeDependencyJ::Constant)
        {
            Jx_mid = c++; Jy_mid = c++; Jz_mid = c++;
        }
        if (time_dependency_J == TimeDependencyJ::Quadratic)
        {
            Jx_old = c++; Jy_old = c++; Jz_old = c++;
            Jx_new = c++; Jy_new = c++; Jz_new = c++;
            Jx_mid = c++; Jy_mid = c++; Jz_mid = c++;
        }
        else if (time_dependency_J == TimeDependencyJ::Linear)
        {
            Jx_old = c++; Jy_old = c++; Jz_old = c++;
            Jx_new = c++; Jy_new = c++; Jz_new = c++;
        }

        if (time_dependency_rho == TimeDependencyRho::Constant)
        {
            rho_mid = c++;
        }
        if (time_dependency_rho == TimeDependencyRho::Quadratic)
        {
            rho_old = c++;
            rho_mid = c++;
            rho_new = c++;
        }
        else if (time_dependency_rho == TimeDependencyRho::Linear)
        {
            rho_old = c++;
            rho_new = c++;
        }

        if (pml_rz)
        {
            Er_pml = c++; Et_pml = c++;
            Br_pml = c++; Bt_pml = c++;
        }

    }
    else // PML
    {
        Exy = c++; Exz = c++; Eyx = c++; Eyz = c++; Ezx = c++; Ezy = c++;
        Bxy = c++; Bxz = c++; Byx = c++; Byz = c++; Bzx = c++; Bzy = c++;

        if (dive_cleaning)
        {
            Exx = c++; Eyy = c++; Ezz = c++;
            Fx  = c++; Fy  = c++; Fz  = c++;
        }

        if (divb_cleaning)
        {
            Bxx = c++; Byy = c++; Bzz = c++;
            Gx  = c++; Gy  = c++; Gz  = c++;
        }
    }

    // This is the number of arrays that will be actually allocated in spectral space
    n_fields = c;
}
