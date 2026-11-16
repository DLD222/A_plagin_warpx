#ifndef WARPX_PLUGIN_PLASMABOUNDARY_H_
#define WARPX_PLUGIN_PLASMABOUNDARY_H_

#include "Runtime/IBoundaryConditions.H"
#include "Runtime/IEBGeometry.H"

#include <memory>
#include <string>

namespace warpx::boundary {

/** PEC + dielectric on embedded surfaces, as a plugin that only talks to the core.
 *
 *  Independence: this class includes `IBoundaryConditions.H` and `IEBGeometry.H`
 *  (both core headers) and never a field-solver or EB-module header.  The core hands
 *  it the geometry interface at construction; the field solver hands it the field
 *  register when it wants the condition applied.
 *
 *  Current scope (first slice): PEC on cut cells for a finite-difference solver —
 *  every E component in a non-regular cell is zeroed after the E update, which is the
 *  staircase form of `E_tangential = 0`.  Dielectric subpixel averaging is stubbed with
 *  a face-wise volume-fraction average (the `FillFacePermittivity` hook).
 */
class PlasmaBoundary final : public warpx::runtime::IBoundaryConditions
{
public:
    explicit PlasmaBoundary (warpx::runtime::IEBGeometry const& eb_geometry);

    [[nodiscard]] bool HasPecSurface () const override { return m_has_pec; }
    [[nodiscard]] bool HasDielectric () const override { return m_has_dielectric; }
    [[nodiscard]] const char* UnsupportedSolvers () const override { return ""; }

    void ApplyPecToEfield (ablastr::fields::MultiFabRegister& fields, int lev,
                           ablastr::utils::enums::PatchType patch_type,
                           amrex::Real time) override;
    void ApplyPecToBfield (ablastr::fields::MultiFabRegister& fields, int lev,
                           ablastr::utils::enums::PatchType patch_type,
                           amrex::Real time) override;
    void FillFacePermittivity (ablastr::fields::MultiFabRegister& fields, int lev,
                               ablastr::utils::enums::PatchType patch_type) override;
    void ApplyDielectricJump (ablastr::fields::MultiFabRegister& fields, int lev,
                              ablastr::utils::enums::PatchType patch_type,
                              amrex::Real dt) override;
    void ApplyWallToFluidVelocity (amrex::MultiFab& fluid_state, int first_comp,
                                   int ncomp, int lev) override;
    void ApplyWallToHeatFlux (amrex::MultiFab& fluid_state, int first_comp,
                              int ncomp, int lev, amrex::Real wall_temperature) override;
    void ApplyWallToStressTensor (amrex::MultiFab& fluid_state, int first_comp,
                                  int ncomp, int lev) override;

    /** Number of cut cells touched by the last PEC application (diagnostic). */
    [[nodiscard]] long LastCutCellCount () const { return m_last_cut_cells; }

private:
    warpx::runtime::IEBGeometry const& m_eb;
    bool m_has_pec = true;
    bool m_has_dielectric = false;
    long m_last_cut_cells = 0;
};

} // namespace warpx::boundary

#endif // WARPX_PLUGIN_PLASMABOUNDARY_H_
