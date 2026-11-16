#ifndef WARPX_RUNTIME_IEBGEOMETRY_H_
#define WARPX_RUNTIME_IEBGEOMETRY_H_

#include <AMReX_Config.H>
#include <AMReX_EBFabFactory.H>
#include <AMReX_REAL.H>

/** \file
 *  Read-only access to the embedded-boundary geometry, owned by the **core**.
 *
 *  The cut-cell data (cell flags, area fractions, boundary centroid/normal) is built
 *  once by the EB module of the PicEm plugin.  A boundary-condition plugin, a field
 *  solver, or the hybrid model that needs to apply a PEC / dielectric condition on an
 *  embedded surface must read that data *without* depending on that plugin.  It asks the
 *  core for this interface instead, so the dependency graph stays
 *
 *      core  <-  plugins          (never plugin <- plugin)
 *
 *  `amrex::EBFArrayBoxFactory` already carries everything:
 *      getMultiEBCellFlagFab()  cell flags (regular / cut / covered)
 *      getAreaFrac()            face areas per direction (cut-cell areas)
 *      getBndryCent()           boundary centroid   -> the local normal for PEC tests
 *      getBndryNormal()         boundary normal
 *      getVolFrac()             volume fraction
 */
namespace warpx::runtime {

class IEBGeometry
{
public:
    virtual ~IEBGeometry () = default;

    /** True when an embedded boundary exists on level `lev`. */
    [[nodiscard]] virtual bool hasEB (int lev) const = 0;

    /** The AMReX EB factory for level `lev` (flags, area fractions, centroids). */
    [[nodiscard]] virtual amrex::EBFArrayBoxFactory const& factory (int lev) const = 0;
};

} // namespace warpx::runtime

#endif // WARPX_RUNTIME_IEBGEOMETRY_H_
