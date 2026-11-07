/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_RUNTIME_ISPECIESCONTAINER_H_
#define WARPX_RUNTIME_ISPECIESCONTAINER_H_

#include <AMReX_Vector.H>

#include <string>
#include <vector>

class WarpXParticleContainer;

namespace warpx::runtime
{

/** \brief Stable species-registry view for modules (A7.1 transitional). */
class ISpeciesContainer
{
public:
    virtual ~ISpeciesContainer () = default;

    [[nodiscard]] virtual int nSpecies () const = 0;
    [[nodiscard]] virtual std::vector<std::string> GetSpeciesNames () const = 0;
    [[nodiscard]] virtual int getSpeciesID (const std::string& name) const = 0;
    [[nodiscard]] virtual WarpXParticleContainer& GetParticleContainer (int index) const = 0;
    [[nodiscard]] virtual amrex::Vector<amrex::Long> NumberOfParticlesInGrid (int lev) const = 0;
};

} // namespace warpx::runtime

#endif
