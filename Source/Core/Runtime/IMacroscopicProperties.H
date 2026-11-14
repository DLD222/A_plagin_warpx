/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_RUNTIME_IMACROSCOPICPROPERTIES_H_
#define WARPX_RUNTIME_IMACROSCOPICPROPERTIES_H_

#include <AMReX_BoxArray.H>
#include <AMReX_DistributionMapping.H>
#include <AMReX_Geometry.H>
#include <AMReX_IntVect.H>
#include <AMReX_MultiFab.H>
#include "Runtime/MaterialRegistry.H"

#include <AMReX_REAL.H>

#include <memory>
#include <cstdlib>
#include <string>

namespace amrex { class EBFArrayBoxFactory; }

namespace warpx::runtime
{

/**
 * \brief Material model contract (S6).
 *
 * The kernel and the field-solver plugin both need the medium coefficients
 * (permittivity, conductivity, permeability, conduction current).  They talk to
 * this contract, so neither side names the concrete implementation and the
 * material model can move into its own plugin.
 */
class IMacroscopicProperties
{
public:
    virtual ~IMacroscopicProperties () = default;

    virtual void AllocateLevelMFs (const amrex::BoxArray& ba,
                                   const amrex::DistributionMapping& dm,
                                   const amrex::IntVect& ng_EB_alloc,
                                   const amrex::IntVect& Ex_nodal,
                                   const amrex::IntVect& Ey_nodal,
                                   const amrex::IntVect& Ez_nodal) = 0;

    virtual void InitData (const amrex::Geometry& geom,
                           const amrex::IntVect& Ex_stag,
                           const amrex::IntVect& Ey_stag,
                           const amrex::IntVect& Ez_stag) = 0;

    virtual void InitSubpixelRows (const amrex::Geometry& geom,
                                   const amrex::EBFArrayBoxFactory* eb_fact,
                                   const amrex::IntVect& Ex_stag,
                                   const amrex::IntVect& Ey_stag,
                                   const amrex::IntVect& Ez_stag,
                                   amrex::Real dt) = 0;

    //! Conduction current deposited by the material model.
    [[nodiscard]] virtual amrex::MultiFab& get_jcond_mf (int c) = 0;
    [[nodiscard]] virtual amrex::MultiFab& get_eps_node_mf (int c) = 0;
    [[nodiscard]] virtual amrex::MultiFab& get_eps_tensor_mf () = 0;
    [[nodiscard]] virtual bool has_eps_tensor_mf () const = 0;

    [[nodiscard]] virtual amrex::MultiFab& getsigma_mf () = 0;
    [[nodiscard]] virtual amrex::MultiFab& getepsilon_mf () = 0;
    [[nodiscard]] virtual amrex::MultiFab& getmu_mf () = 0;

    [[nodiscard]] virtual bool has_subpixel_rows () const = 0;
    [[nodiscard]] virtual bool rho_medium_seeded () const = 0;
    virtual void set_rho_medium_seeded () = 0;
    //! Step interval of the Gauss-law consistency check (0 = disabled).
    [[nodiscard]] virtual int& gauss_check_step () = 0;

    //! Named material descriptions parsed from the input file.
    [[nodiscard]] virtual const MaterialRegistry& materials () const = 0;

    // ---------------------------------------------------------------------------------------
    // What the E push needs beyond the coefficient fields.  These were previously reached by a
    // dynamic_cast to the concrete implementation, which meant a third-party material model
    // satisfying this contract was still rejected at run time (CODE_REVIEW_2026-11-14, F-08).
    // ---------------------------------------------------------------------------------------

    //! Staggering of each material field, and the refinement ratio to the E components.
    [[nodiscard]] virtual amrex::GpuArray<int, 3> const& sigma_stag () const = 0;
    [[nodiscard]] virtual amrex::GpuArray<int, 3> const& mu_stag () const = 0;
    [[nodiscard]] virtual amrex::GpuArray<int, 3> const& epsilon_stag () const = 0;
    [[nodiscard]] virtual amrex::GpuArray<int, 3> const& Ex_stag () const = 0;
    [[nodiscard]] virtual amrex::GpuArray<int, 3> const& Ey_stag () const = 0;
    [[nodiscard]] virtual amrex::GpuArray<int, 3> const& Ez_stag () const = 0;
    [[nodiscard]] virtual amrex::GpuArray<int, 3> const& macro_cr () const = 0;

    /** \brief Optional capability: the exact exponential step for conductive cells.
     *
     * A material model that does not build M1/M2 returns false from has_sigma_matrices() and
     * never has any of the accessors below called; they abort with a message naming what was
     * missing rather than silently returning a stale field. */
    [[nodiscard]] virtual bool has_sigma_matrices () const { return false; }
    [[nodiscard]] virtual amrex::Real get_sigma_dt () const { return amrex::Real(0.0); }
    [[nodiscard]] virtual amrex::MultiFab& get_sigma_m1_mf () { return unavailable("get_sigma_m1_mf"); }
    [[nodiscard]] virtual amrex::MultiFab& get_sigma_m2_mf () { return unavailable("get_sigma_m2_mf"); }
    [[nodiscard]] virtual amrex::MultiFab& get_sigma_m1_node_mf (int) { return unavailable("get_sigma_m1_node_mf"); }
    [[nodiscard]] virtual amrex::MultiFab& get_sigma_m2_node_mf (int) { return unavailable("get_sigma_m2_node_mf"); }
    [[nodiscard]] virtual amrex::MultiFab& get_sigma_node_mf (int) { return unavailable("get_sigma_node_mf"); }
    [[nodiscard]] virtual amrex::MultiFab& get_j0_node_mf (int) { return unavailable("get_j0_node_mf"); }
    [[nodiscard]] virtual amrex::MultiFab& get_j1_node_mf (int) { return unavailable("get_j1_node_mf"); }
    [[nodiscard]] virtual amrex::MultiFab& get_eps_inv_node_mf (int) { return unavailable("get_eps_inv_node_mf"); }
    [[nodiscard]] virtual amrex::MultiFab& geteps_inv_x_mf () { return unavailable("geteps_inv_x_mf"); }
    [[nodiscard]] virtual amrex::MultiFab& geteps_inv_y_mf () { return unavailable("geteps_inv_y_mf"); }
    [[nodiscard]] virtual amrex::MultiFab& geteps_inv_z_mf () { return unavailable("geteps_inv_z_mf"); }

protected:
    [[noreturn]] static amrex::MultiFab& unavailable (const char* what)
    {
        amrex::Abort(std::string("material model does not provide ") + what
                     + ": it reports has_sigma_matrices() == false, so the subpixel exponential "
                       "step is not available for it (CODE_REVIEW F-08).");
        std::abort();   // amrex::Abort is not marked noreturn for every compiler
    }
};

/** \brief Material model factory (S6): returns nullptr when no plugin is linked. */
using MacroscopicPropertiesFactory = std::unique_ptr<IMacroscopicProperties> (*) ();

void register_macroscopic_properties_factory (MacroscopicPropertiesFactory factory);

[[nodiscard]] std::unique_ptr<IMacroscopicProperties> create_macroscopic_properties ();

[[nodiscard]] bool has_macroscopic_properties_factory ();

} // namespace warpx::runtime

#endif // WARPX_RUNTIME_IMACROSCOPICPROPERTIES_H_
