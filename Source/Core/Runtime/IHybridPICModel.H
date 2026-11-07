/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_RUNTIME_IHYBRIDPICMODEL_H_
#define WARPX_RUNTIME_IHYBRIDPICMODEL_H_

#include <AMReX_Parser.H>
#include <AMReX_REAL.H>

#include <map>
#include <string>

namespace warpx::runtime
{

/** \brief Native hybrid-PIC parameters used by collision modules (A7.1). */
class IHybridPICModel
{
public:
    virtual ~IHybridPICModel () = default;

    [[nodiscard]] virtual const amrex::ParserExecutor<3>& eta () const = 0;
    [[nodiscard]] virtual const std::map<std::string, amrex::ParserExecutor<7>>&
    etaPerSpecies () const = 0;
    [[nodiscard]] virtual amrex::Real nFloor () const = 0;
};

} // namespace warpx::runtime

#endif
