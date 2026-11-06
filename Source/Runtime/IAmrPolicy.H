/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_RUNTIME_IAMRPOLICY_H_
#define WARPX_RUNTIME_IAMRPOLICY_H_

#include <AMReX_BoxArray.H>
#include <AMReX_DistributionMapping.H>
#include <AMReX_TagBox.H>

namespace warpx::runtime
{

/**
 * \brief Decisions for AMR tagging, load balancing and base grids.
 *
 * The policy only decides; grid creation, data transfer and regrid
 * mechanics remain in WarpX.
 */
class IAmrPolicy
{
public:
    virtual ~IAmrPolicy () = default;

    virtual void tag (int lev, amrex::TagBoxArray& tags,
                      amrex::Real time, int ngrow) = 0;

    [[nodiscard]] virtual amrex::DistributionMapping
    makeDistributionMap (int lev, amrex::BoxArray const& ba) = 0;

    virtual void postProcessBaseGrids (amrex::BoxArray& ba0) const = 0;
};

} // namespace warpx::runtime

#endif // WARPX_RUNTIME_IAMRPOLICY_H_
