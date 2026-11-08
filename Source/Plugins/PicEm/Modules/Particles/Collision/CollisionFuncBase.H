/* Copyright 2026 The WarpX Community
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause-LBNL
 */

#ifndef WARPX_COLLISION_FUNC_BASE_H_
#define WARPX_COLLISION_FUNC_BASE_H_

namespace warpx::runtime { class RuntimeData; }

class CollisionFuncBase
{
public:

    CollisionFuncBase () = default;
    virtual ~CollisionFuncBase () = default;

    CollisionFuncBase (const CollisionFuncBase&) = default;
    CollisionFuncBase& operator= (const CollisionFuncBase&) = default;

    CollisionFuncBase (CollisionFuncBase&&) noexcept = default;
    CollisionFuncBase& operator= (CollisionFuncBase&&) noexcept = default;

    //! Optional hook to allocate additional data structures.
    virtual void AllocData (const warpx::runtime::RuntimeData& /*data*/) {}

};

#endif /* WARPX_COLLISION_FUNC_BASE_H_ */
