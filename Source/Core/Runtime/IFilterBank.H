/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_RUNTIME_IFILTERBANK_H_
#define WARPX_RUNTIME_IFILTERBANK_H_

#include <AMReX_Box.H>
#include <AMReX_FArrayBox.H>

namespace warpx::runtime
{

/** \brief Native NCI filter bank used by particle gathering (A7.1). */
class IFilterBank
{
public:
    virtual ~IFilterBank () = default;

    virtual void applyExEyBz (int lev, amrex::FArrayBox& dst,
                              const amrex::FArrayBox& src,
                              const amrex::Box& box) const = 0;
    virtual void applyBxByEz (int lev, amrex::FArrayBox& dst,
                              const amrex::FArrayBox& src,
                              const amrex::Box& box) const = 0;
};

} // namespace warpx::runtime

#endif
