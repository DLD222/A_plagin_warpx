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
};

/** \brief Material model factory (S6): returns nullptr when no plugin is linked. */
using MacroscopicPropertiesFactory = std::unique_ptr<IMacroscopicProperties> (*) ();

void register_macroscopic_properties_factory (MacroscopicPropertiesFactory factory);

[[nodiscard]] std::unique_ptr<IMacroscopicProperties> create_macroscopic_properties ();

[[nodiscard]] bool has_macroscopic_properties_factory ();

} // namespace warpx::runtime

#endif // WARPX_RUNTIME_IMACROSCOPICPROPERTIES_H_
