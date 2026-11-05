/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_RUNTIME_FIELDSPEC_H_
#define WARPX_RUNTIME_FIELDSPEC_H_

#include <AMReX_IntVect.H>

#include <string>

namespace warpx::runtime
{

/**
 * \brief Schema declaration for one field entry in the MultiFabRegister.
 *
 * A spec never owns storage. It only describes how the existing register entry
 * is created, aliased, checkpointed and validated. This is the A0.6 replacement
 * for a second DataBus storage layer (audit D1).
 */
struct FieldSpec
{
    std::string name;
    int ncomp = 1;
    amrex::IntVect nodal{AMREX_D_DECL(0, 0, 0)};
    amrex::IntVect ngrow{AMREX_D_DECL(0, 0, 0)};
    bool checkpoint = false;
    bool moving_window_shift = false;
    std::string unit;
    //! Empty = owned field; otherwise the register entry is an alias of this name.
    std::string alias_of;
};

} // namespace warpx::runtime

#endif // WARPX_RUNTIME_FIELDSPEC_H_
