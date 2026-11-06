/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "Runtime/SpeciesSpec.H"

#include "Particles/WarpXParticleContainer.H"

namespace warpx::runtime
{

amrex::Real
SpeciesHandle::charge () const
{
    return m_pc != nullptr ? m_pc->getCharge() : amrex::Real(0.0);
}

amrex::Real
SpeciesHandle::mass () const
{
    return m_pc != nullptr ? m_pc->getMass() : amrex::Real(0.0);
}

std::int64_t
SpeciesHandle::numberOfParticles () const
{
    return m_pc != nullptr ? static_cast<std::int64_t>(m_pc->TotalNumberOfParticles(false))
                           : 0;
}

} // namespace warpx::runtime
