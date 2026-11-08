/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_RUNTIME_PARTICLECONTEXT_H_
#define WARPX_RUNTIME_PARTICLECONTEXT_H_

namespace warpx::runtime
{

class IParticleContainerAccess;
class IParticleServices;

/**
 * \brief The two particle-facing services a plugin may need, without naming
 * any concrete particle container (D1-a).
 *
 * - `access`: enumerate species and query them through generic contracts
 * - `services`: operations that belong to the particle implementation
 *
 * Either side may be null when the caller does not need it.
 */
struct ParticleContext
{
    IParticleContainerAccess* access = nullptr;
    IParticleServices* services = nullptr;
};

} // namespace warpx::runtime

#endif // WARPX_RUNTIME_PARTICLECONTEXT_H_
