/* Copyright 2026 The WarpX Community
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause-LBNL
 */
#ifndef WARPX_RUNTIME_IPARTICLESYSTEM_H_
#define WARPX_RUNTIME_IPARTICLESYSTEM_H_

#include "Runtime/FieldGatherOrders.H"
#include "Runtime/IParticleContainerAccess.H"
#include "Runtime/RuntimeContext.H"

#include "Modules/FieldSolver/ImplicitOptions.H"
#include "Utils/WarpXAlgorithmSelection.H"

#include <ablastr/fields/MultiFabRegister.H>

#include <AMReX_IntVect.H>
#include <AMReX_MultiFab.H>
#include <AMReX_REAL.H>

#include <optional>
#include <string>

namespace warpx::runtime
{

/** The particle system as the kernel sees it.
 *
 * Everything a kernel step does to the particles goes through this interface: pushing,
 * depositing, redistributing, and the bookkeeping around mesh refinement.  A kernel
 * therefore never names a concrete particle container - the PIC implementation lives
 * behind this contract, in a plugin.
 *
 * It extends IParticleContainerAccess, which is the read-only view of the species that
 * diagnostics and boundary conditions use.
 */
class IParticleSystem : public IParticleContainerAccess
{
public:
    ~IParticleSystem () override = default;

    // ---- setup -------------------------------------------------------------
    //! Give the system the runtime context; called once before the first step.
    virtual void setRuntimeContext (const RuntimeContext* ctx) = 0;
    //! Allocate the particle data on every level.
    virtual void AllocData () = 0;
    //! Define the particle tiles for the current mesh.
    virtual void defineAllParticleTiles () = 0;
    //! Read the initial conditions and fill the species.
    virtual void InitData () = 0;

    // ---- per-step work -----------------------------------------------------
    //! Push particles and deposit rho/J on one level (the PIC step itself).
    virtual void Evolve (
        ablastr::fields::MultiFabRegister& fields,
        int lev,
        std::string const& current_fp_string,
        amrex::Real t,
        amrex::Real dt,
        SubcyclingHalf subcycling_half = SubcyclingHalf::None,
        bool skip_deposition = false,
        PositionPushType position_push_type = PositionPushType::Full,
        MomentumPushType momentum_push_type = MomentumPushType::Full,
        ImplicitOptions const* implicit_options = nullptr) = 0;

    //! Deposit the charge density of every species.
    virtual void DepositCharge (const ablastr::fields::MultiLevelScalarField& rho,
                                amrex::Real relative_time) = 0;
    //! Deposit the current density of every species.
    virtual void DepositCurrent (ablastr::fields::MultiLevelVectorField const& J,
                                 amrex::Real dt, amrex::Real relative_time,
                                 PushType push_type = PushType::Explicit,
                                 std::optional<int> particle_shape = std::nullopt) = 0;
    //! Deposit the kinetic temperature of every species.
    virtual void DepositTemperatures (ablastr::fields::MultiFabRegister& fields,
                                      amrex::Real relative_time) = 0;

    //! Push the momenta by dt without moving the positions.
    virtual void PushP (int lev, amrex::Real dt,
                        const amrex::MultiFab& Ex, const amrex::MultiFab& Ey,
                        const amrex::MultiFab& Ez,
                        const amrex::MultiFab& Bx, const amrex::MultiFab& By,
                        const amrex::MultiFab& Bz,
                        MomentumPushType momentum_push_type,
                        std::optional<FieldGatherOrders> gather_orders = std::nullopt) = 0;
    //! Push the positions by dt without changing the momenta.
    virtual void PushX (amrex::Real dt) = 0;

    //! Redistribute particles over the boxes (load balancing, regrid).
    virtual void Redistribute () = 0;
    //! Redistribute only particles that moved beyond max_cells_travelled.
    virtual void RedistributeLocal (const amrex::IntVect& max_cells_travelled) = 0;

    //! Largest dt for which the particles are still stable.
    [[nodiscard]] virtual amrex::Real maxParticleDtInv () const = 0;
};

} // namespace warpx::runtime

#endif // WARPX_RUNTIME_IPARTICLESYSTEM_H_
