/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_RUNTIME_SPECIESSPEC_H_
#define WARPX_RUNTIME_SPECIESSPEC_H_

#include <AMReX_REAL.H>

#include <cstdint>
#include <string>

class WarpXParticleContainer;

namespace warpx::runtime
{

struct SpeciesSpec
{
    std::string name;
    amrex::Real charge = 0.0;
    amrex::Real mass = 0.0;
};

/**
 * \brief Opaque species view for handlers (A3.3b2).
 *
 * The underlying WarpXParticleContainer is hidden in the .cpp; handlers only
 * see name/count and, later, views.
 */
class SpeciesHandle
{
public:
    SpeciesHandle () = default;

    [[nodiscard]] const std::string& name () const { return m_name; }
    [[nodiscard]] std::int64_t numberOfParticles () const;
    [[nodiscard]] amrex::Real charge () const;
    [[nodiscard]] amrex::Real mass () const;

private:
    friend class DataBus;
    friend class CollisionSpeciesTable;
    friend class SpeciesRegistry;
    SpeciesHandle (std::string name, WarpXParticleContainer* pc)
        : m_name(std::move(name)), m_pc(pc) {}

    std::string m_name;
    WarpXParticleContainer* m_pc = nullptr;
};

} // namespace warpx::runtime

#endif // WARPX_RUNTIME_SPECIESSPEC_H_
