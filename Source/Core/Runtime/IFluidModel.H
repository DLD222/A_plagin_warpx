/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_RUNTIME_IFLUIDMODEL_H_
#define WARPX_RUNTIME_IFLUIDMODEL_H_

#include <ablastr/fields/MultiFabRegister.H>

namespace amrex { class MultiFab; }

namespace warpx::runtime
{

/**
 * \brief Fluid-model contract (S6).
 *
 * A field solver may need charge or current from a fluid model.  It asks this
 * contract, so the solver does not depend on the fluid plugin and a different
 * fluid model can be plugged in without touching either side.
 */
class IFluidModel
{
public:
    virtual ~IFluidModel () = default;

    //! Deposit the total charge of the fluid on level `lev`.
    virtual void depositCharge (ablastr::fields::MultiFabRegister& fields,
                                amrex::MultiFab& rho, int lev) = 0;
};

} // namespace warpx::runtime

#endif // WARPX_RUNTIME_IFLUIDMODEL_H_
