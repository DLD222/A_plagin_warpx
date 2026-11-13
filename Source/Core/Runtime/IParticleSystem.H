/* Copyright 2026 The WarpX Community
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause-LBNL
 */
#ifndef WARPX_RUNTIME_IPARTICLESYSTEM_H_
#define WARPX_RUNTIME_IPARTICLESYSTEM_H_

#include "Runtime/ExternalParticleFields.H"
#include "Runtime/FieldGatherOrders.H"
#include "Runtime/IParticleContainerAccess.H"
#include "Runtime/RuntimeContext.H"
#include "Runtime/RuntimeData.H"

#include "Modules/FieldSolver/ImplicitOptions.H"
#include "Utils/WarpXAlgorithmSelection.H"

#include <ablastr/fields/MultiFabRegister.H>

#include <AMReX_BoxArray.H>
#include <AMReX_DistributionMapping.H>
#include <AMReX_IntVect.H>
#include <AMReX_MultiFab.H>
#include <AMReX_REAL.H>

#include <optional>
#include <memory>
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

    //! The runtime data the particle code reads its geometry and time steps from.
    [[nodiscard]] virtual const RuntimeData& data () const = 0;

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
                        std::optional<warpx::particles::FieldGatherOrders> gather_orders = std::nullopt) = 0;
    //! Push the positions by dt without changing the momenta.
    virtual void PushX (amrex::Real dt) = 0;

    //! Redistribute particles over the boxes (load balancing, regrid).
    virtual void Redistribute () = 0;
    //! Redistribute only particles that moved beyond max_cells_travelled.
    virtual void RedistributeLocal (const amrex::IntVect& max_cells_travelled) = 0;

    //! Create the extra attributes the implicit push keeps (x_n, ux_n, nsuborbits, ...).
    virtual void AddImplicitParticleAttributes (bool particle_suborbits) = 0;
    //! Zero the implicit particle velocities before the solve.
    virtual void ClearImplicitParticleVelocities () = 0;
    //! Finish the implicit velocity update for every species.
    virtual void FinishImplicitVelocityUpdate () = 0;
    //! Copy the time-centered velocities into the deposition attributes.
    virtual void PrepareImplicitVelocitiesForDeposition () = 0;

    //! Save the position and momentum at the start of the step (implicit advance needs them).
    virtual void SaveParticlesAtImplicitStepStart (int finest_level) = 0;
    //! Finish the implicit update: extrapolate every species from n+1/2 to n+1.
    virtual void FinishImplicitParticleUpdate (ablastr::fields::MultiFabRegister& fields,
                                               int finest_level, amrex::Real time,
                                               const amrex::Vector<amrex::Real>& dt) = 0;
    //! Drop the saved implicit data (start of a new substepping sequence).
    virtual void ResetImplicitParticleData (int finest_level) = 0;

    //! Advance the injection plane of every continuously-injected species (moving window).
    virtual void UpdateInjectionPositions (amrex::Real gamma_boost, amrex::Real beta_boost,
                                           const amrex::Vector<int>& boost_direction,
                                           int moving_window_dir, amrex::Real dt) = 0;

    //! Largest dt for which the particles are still stable.
    [[nodiscard]] virtual amrex::Real maxParticleDtInv () const = 0;

    //! Add the particles' share to a level's load-balancing cost array.
    //! The kernel owns the cost arrays; the per-tile particle count is the system's business.
    virtual void AddParticleCosts (int lev, amrex::LayoutData<amrex::Real>& costs,
                                   amrex::Real weight) const = 0;

    //! Resample the particles (merge/split) when the input asks for it.
    virtual void doResampling (const amrex::Vector<amrex::Geometry>& geom, int timestep,
                               bool verbose) = 0;
    //! Run the QED elementary processes for this step.
    virtual void doQedEvents (int lev,
                              const amrex::MultiFab& Ex, const amrex::MultiFab& Ey,
                              const amrex::MultiFab& Ez,
                              const amrex::MultiFab& Bx, const amrex::MultiFab& By,
                              const amrex::MultiFab& Bz) = 0;

    // ---- queries the kernel makes while setting up -------------------------
    //! Number of species containers (including photon and laser ones).
    [[nodiscard]] virtual int nContainers () const = 0;
    //! Number of species whose charge is deposited on the main grid.
    [[nodiscard]] virtual int nSpeciesDepositOnMainGrid () const = 0;
    //! Number of species whose fields are gathered from the main grid.
    [[nodiscard]] virtual int nSpeciesGatherFromMainGrid () const = 0;
    //! Description of the user-declared external E/B fields acting on the particles.
    [[nodiscard]] virtual const ExternalParticleFields& externalParticleFieldsMetadata () const = 0;

    // ---- restart and mesh bookkeeping --------------------------------------
    virtual void PostRestart () = 0;
    virtual void SetParticleBoxArray (int lev, amrex::BoxArray& new_ba) = 0;
    virtual void SetParticleDistributionMap (int lev, amrex::DistributionMapping& new_dm) = 0;
    virtual void SortParticlesByBin (const amrex::IntVect& bin_size,
                                     bool sort_particles_for_deposition,
                                     const amrex::IntVect& sort_idx_type) = 0;
    virtual void deleteInvalidParticles () = 0;
    virtual void ApplyBoundaryConditions () = 0;
    virtual void ScrapeParticlesAtEB (
        ablastr::fields::MultiLevelScalarField const& distance_to_eb) = 0;
    virtual void TransformMomentumToCurvilinear (bool forward) = 0;

    // ---- optional physics the kernel drives when the input asks for it -----
    virtual void doFieldIonization (int lev,
                                    const amrex::MultiFab& Ex, const amrex::MultiFab& Ey,
                                    const amrex::MultiFab& Ez,
                                    const amrex::MultiFab& Bx, const amrex::MultiFab& By,
                                    const amrex::MultiFab& Bz) = 0;
    virtual void doQEDSchwinger () = 0;
    virtual void DepositMassMatrices (ablastr::fields::MultiFabRegister& fields, int lev,
                                      amrex::Real dt,
                                      ImplicitOptions const& implicit_options) = 0;
    virtual void ContinuousFluxInjection (amrex::Real t, amrex::Real dt) const = 0;
    virtual void UpdateAntennaPosition (amrex::Real dt) const = 0;
    virtual void CalculateNuei (std::string const& electron_species_name) = 0;
    virtual void GenerateGlobalDebyeLength () = 0;
    [[nodiscard]] virtual std::unique_ptr<amrex::MultiFab> GetGlobalPlasmaFrequency (int lev) = 0;
    /** The repeated plasma lens a lattice may declare.
     *  One query instead of five member reads, so the gather does not need the container.
     */
    struct RepeatedPlasmaLens
    {
        amrex::ParticleReal period = 0;
        int n_lenses = 0;
        amrex::ParticleReal const* starts = nullptr;
        amrex::ParticleReal const* lengths = nullptr;
        amrex::ParticleReal const* strengths_E = nullptr;
        amrex::ParticleReal const* strengths_B = nullptr;
    };
    [[nodiscard]] virtual RepeatedPlasmaLens repeatedPlasmaLens () const = 0;

    //! A user-declared external-field parser ("Ex", "By", ...), or null when none was given.
    [[nodiscard]] virtual const amrex::Parser* externalFieldParser (const std::string& name) const = 0;
    //! The accelerator lattice of one level as an opaque handle (its type is the plugin's).
    [[nodiscard]] virtual const void* acceleratorLattice (int lev) const = 0;

    //! The charge density of every species on one level, optionally without the final
    //! boundary/volume treatment (the caller decides when to finalize).
    [[nodiscard]] virtual std::unique_ptr<amrex::MultiFab>
    GetChargeDensityOnLevel (int lev, bool local, bool finalize) = 0;
};

} // namespace warpx::runtime


namespace warpx::runtime
{

/** Factory the PIC plugin registers so the kernel can build a particle system.
 *
 * The kernel constructs the system once, at startup, and only ever holds the contract.  The
 * plugin supplies the concrete class, which is why this is a registration rather than a
 * direct include: swapping or replacing the particle implementation is then a link-time
 * decision, not a kernel edit.
 */
// The owner is the kernel object (opaque here: Core must not name it).  The PIC plugin
// knows what it is and binds the concrete constructor.
using ParticleSystemFactory = std::unique_ptr<IParticleSystem> (*) (void* owner);

void register_particle_system_factory (ParticleSystemFactory factory);

//! Build the particle system the registered plugin provides; nullptr when none is linked.
[[nodiscard]] std::unique_ptr<IParticleSystem>
create_particle_system (void* owner);

} // namespace warpx::runtime

#endif
