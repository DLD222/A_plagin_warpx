#ifndef WARPX_RUNTIME_IBOUNDARYCONDITIONS_H_
#define WARPX_RUNTIME_IBOUNDARYCONDITIONS_H_

#include <ablastr/fields/MultiFabRegister.H>
#include <ablastr/utils/Enums.H>
#include <AMReX_MultiFab.H>
#include <AMReX_REAL.H>
#include <memory>

/** \file
 *  Core-side contract for the *boundary-condition* plugin.
 *
 *  Design rule (same as IFiniteDifferenceSolver / IMacroscopicProperties):
 *  the core only knows this interface.  PEC on embedded surfaces and dielectric
 *  jump conditions live in a plugin that implements it; neither the core nor any
 *  field solver may include the plugin's headers.  A solver that can apply the
 *  conditions asks the core for the interface pointer and calls it; a solver that
 *  cannot simply never calls it, and the plugin reports the gap via
 *  `UnsupportedSolvers()` so the input layer can refuse the combination up front.
 */
namespace warpx::runtime {

class IBoundaryConditions
{
public:
    virtual ~IBoundaryConditions () = default;

    // ---- capability discovery (used by the input checks, no solver knowledge) ----
    /** True when an embedded boundary is present and treated as a perfect conductor. */
    [[nodiscard]] virtual bool HasPecSurface () const = 0;
    /** True when a dielectric material interface is configured. */
    [[nodiscard]] virtual bool HasDielectric () const = 0;
    /** Names of solver ids this implementation cannot handle (empty = all). */
    [[nodiscard]] virtual const char* UnsupportedSolvers () const = 0;

    // ---- PEC on an embedded surface: E_tangential = 0, B_normal continuous ----
    /** Zero the tangential electric field on every cut face of the embedded boundary. */
    virtual void ApplyPecToEfield (ablastr::fields::MultiFabRegister& fields, int lev,
                                   ablastr::utils::enums::PatchType patch_type,
                                   amrex::Real time) = 0;
    /** Restore B_normal continuity on cut faces (divergence-free projection). */
    virtual void ApplyPecToBfield (ablastr::fields::MultiFabRegister& fields, int lev,
                                   ablastr::utils::enums::PatchType patch_type,
                                   amrex::Real time) = 0;

    // ---- dielectric interface: E_tangential continuous, D_normal continuous ----
    /** Face-centred permittivity seen by the E-field update (epsilon_r, per direction). */
    virtual void FillFacePermittivity (ablastr::fields::MultiFabRegister& fields, int lev,
                                       ablastr::utils::enums::PatchType patch_type) = 0;
    /** Surface charge deposited on the dielectric interface during the last step. */
    virtual void ApplyDielectricJump (ablastr::fields::MultiFabRegister& fields, int lev,
                                      ablastr::utils::enums::PatchType patch_type,
                                      amrex::Real dt) = 0;

    // ---- wall conditions for fluid / moment solvers (five-moment, ten-moment, star) ----
    // A conducting wall in a fluid model is not only E_tangential = 0: the flow cannot
    // cross it and no heat flows through it.  These hooks keep that knowledge in this
    // plugin instead of duplicating it in every fluid solver.
    /** No normal flow through the wall: v_normal = 0 on cut faces (free-slip default). */
    virtual void ApplyWallToFluidVelocity (amrex::MultiFab& fluid_state, int first_comp,
                                           int ncomp, int lev) = 0;
    /** Zero heat flux through the wall (adiabatic wall) or fix the wall temperature. */
    virtual void ApplyWallToHeatFlux (amrex::MultiFab& fluid_state, int first_comp,
                                      int ncomp, int lev, amrex::Real wall_temperature) = 0;
    /** Reflect the pressure / stress tensor at the wall (n . P . t = 0). */
    virtual void ApplyWallToStressTensor (amrex::MultiFab& fluid_state, int first_comp,
                                          int ncomp, int lev) = 0;
};

} // namespace warpx::runtime

#endif // WARPX_RUNTIME_IBOUNDARYCONDITIONS_H_
