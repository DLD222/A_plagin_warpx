/* Copyright 2026 The WarpX Community
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause-LBNL
 */
#ifndef WARPX_RUNTIME_EMPTYSPECIESMETADATA_H_
#define WARPX_RUNTIME_EMPTYSPECIESMETADATA_H_

#include "Runtime/ISpeciesMetadata.H"

namespace warpx::runtime
{

/** The species metadata of a run with no particle system.
 *
 * Diagnostics and the kernel ask "which species are there?" unconditionally.  When the particle
 * plugin is not registered the honest answer is "none", and returning this beats a null check at
 * every call site.
 */
class EmptySpeciesMetadata : public ISpeciesMetadata
{
public:
    [[nodiscard]] int nSpecies () const override { return 0; }
    [[nodiscard]] std::vector<std::string> GetSpeciesNames () const override { return {}; }
    [[nodiscard]] int getSpeciesID (const std::string&) const override { return -1; }
    [[nodiscard]] std::vector<std::string> GetSpeciesAndLasersNames () const override { return {}; }
    [[nodiscard]] amrex::Vector<amrex::Long> NumberOfParticlesInGrid (int) const override
    {
        return {};
    }
};

} // namespace warpx::runtime

#endif // WARPX_RUNTIME_EMPTYSPECIESMETADATA_H_
