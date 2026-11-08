/* Copyright 2019-2020 Andrew Myers, Axel Huebl, David Grote
 * Jean-Luc Vay, Junmin Gu, Luca Fedeli
 * Maxence Thevenet, Remi Lehe, Revathi Jambunathan
 * Weiqun Zhang, Yinjian Zhao
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause-LBNL
 */
#ifndef WARPX_WarpXParticleContainer_H_
#define WARPX_WarpXParticleContainer_H_

#include "Modules/Particles/ParticleAttributes.H"
#include "Modules/Particles/ParticleTileIter.H"
#include "Runtime/IParticleContainer.H"
#include "WarpXParticleContainer_fwd.H"
#include "Modules/FieldSolver/ImplicitOptions.H"

#include "Initialization/PlasmaInjector.H"
#include "Modules/Particles/Algorithms/KineticEnergy.H"
#include "Particles/ParticleBoundaries.H"
#include "Modules/Particles/SpeciesPhysicalProperties.H"

#ifdef WARPX_QED
#    include "ElementaryProcess/QEDInternals/BreitWheelerEngineWrapper_fwd.H"
#    include "ElementaryProcess/QEDInternals/QuantumSyncEngineWrapper_fwd.H"
#endif
#include "MultiParticleContainer_fwd.H"

namespace warpx::runtime { struct RuntimeContext; struct RuntimeConfig; struct FrameState; class RuntimeData; class HostServices; class WarpXHostServices; }

#include "Particles/Deposition/TemperatureDepositionTypes.H"
#include "Particles/Deposition/VarianceAccumulationBuffer.H"

#include <ablastr/fields/MultiFabRegister.H>

#include <AMReX_Array.H>
#include <AMReX_FArrayBox.H>
#include <AMReX_Geometry.H>
#include <AMReX_GpuAllocators.H>
#include <AMReX_GpuContainers.H>
#include <AMReX_INT.H>
#include <AMReX_ParIter.H>
#include <AMReX_Particles.H>
#include <AMReX_Random.H>
#include <AMReX_REAL.H>
#include <AMReX_StructOfArrays.H>
#include <AMReX_Vector.H>

#include <AMReX_BaseFwd.H>
#include <AMReX_AmrCoreFwd.H>

#include <array>
#include <iosfwd>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>

/** Real Particle Attributes stored in amrex::ParticleContainer's struct of array
 *
 *  x, y, z  : position components
 *  ux uy uz : momentum components, stored as proper velocity, i.e. gamma*v
 *  w        : weight
 *  theta    : azimuthal angle
 *  phi      : polar angle, relative to the x-y plane
 *  nattribs : number of compile-time attributes
 */



struct IntIdx {
    enum
        {
            nattribs ///< the number of attributes above (always last)
        };

    static constexpr std::initializer_list<char const *> names = {};
};

struct DiagIdx
{
    enum {
        w = 0,
        x, y, z, ux, uy, uz,
        nattribs
    };
};



/**
 * \brief Field gather orders for PushP, when they are set explicitly rather than from
 * the global WarpX::nox and WarpX::galerkin_interpolation settings.
 *
 * E is gathered with shape order e_shape and, if e_galerkin is true, one order lower
 * along each component's own direction (i.e. the Galerkin scheme on the Yee grid).
 * B is gathered with shape order b_shape and no order reduction.
 */
struct FieldGatherOrders
{
    int e_shape;
    bool e_galerkin;
    int b_shape;
};

/**
 * WarpXParticleContainer is the base polymorphic class from which all concrete
 * particle container classes (that store a collection of particles) derive. Derived
 * classes can be used for plasma particles, photon particles, or non-physical
 * particles (e.g., for the laser antenna).
 * It derives from amrex::ParticleContainerPureSoA<PIdx::nattribs>, where the
 * template arguments stand for the number of int and amrex::Real SoA
 * data in amrex::SoAParticle.
 *  - SoA amrex::Real: positions x, y, z, momentum ux, uy, uz, ... see PIdx for details;
 *                     more can be added at runtime
 *  - SoA int: 0 attributes by default, but can be added at runtime
 *  - SoA uint64_t: idcpu, a global 64bit index, with a 40bit local id and a 24bit cpu id
 *    (both set at creation)
 * the list.
 *
 * WarpXParticleContainer contains the main functions for initialization,
 * interaction with the grid (field gather and current deposition) and particle
 * push.
 *
 * Note: many functions are pure virtual (meaning they MUST be defined in
 * derived classes, e.g., Evolve) or actual functions (e.g. CurrentDeposition).
 */
class WarpXParticleContainer : public warpx::runtime::IParticleContainer
{
public:
    friend MultiParticleContainer;

    using Base = amrex::ParticleContainerPureSoA<PIdx::nattribs, 0, amrex::PolymorphicArenaAllocator>;
    [[nodiscard]] const std::string& getName () const override { return species_name; }
    [[nodiscard]] int finestLevel () const override { return m_amr_core->finestLevel(); }
    [[nodiscard]] std::int64_t totalNumberOfParticles () const override
    { return static_cast<std::int64_t>(TotalNumberOfParticles(false)); }
    [[nodiscard]] bool isPhoton () const override { return AmIA<PhysicalSpecies::photon>(); }
    [[nodiscard]] bool isPhysicalSpecies (PhysicalSpecies species) const override
    { return physical_species == species; }
    [[nodiscard]] MomentumSum sumParticleMomentum () const override;
    [[nodiscard]] const amrex::Vector<amrex::ParticleReal>& EExternalParticle () const override
    { return m_E_external_particle; }
    [[nodiscard]] const amrex::Vector<amrex::ParticleReal>& BExternalParticle () const override
    { return m_B_external_particle; }
    [[nodiscard]] amrex::Real currentInjectionPosition () const override
    { return m_current_injection_position; }
    void setCurrentInjectionPosition (amrex::Real pos) override
    { m_current_injection_position = pos; }
    [[nodiscard]] int doNotDeposit () const override { return do_not_deposit; }

    [[nodiscard]] amrex::Real selfFieldRelativeTolerance () const override
    { return self_fields_required_precision; }
    [[nodiscard]] amrex::Real selfFieldAbsoluteTolerance () const override
    { return self_fields_absolute_tolerance; }
    [[nodiscard]] int selfFieldMaxIterations () const override
    { return self_fields_max_iters; }
    [[nodiscard]] int selfFieldVerbosity () const override
    { return self_fields_verbosity; }
    [[nodiscard]] std::array<amrex::ParticleReal, 3>
    meanVelocity (bool local = false) override
    { return meanParticleVelocity(local); }
    using warpx::runtime::IParticleContainer::DepositCurrent;
    using warpx::runtime::IParticleContainer::DepositCharge;
    using warpx::runtime::IParticleContainer::DepositTotalNGPTemperature;

    // amrex::StructOfArrays with DiagIdx::nattribs amrex::ParticleReal components
    // and 0 int components for the particle data.
    using DiagnosticParticleData = amrex::StructOfArrays<DiagIdx::nattribs, 0>;
    // DiagnosticParticles is a vector, with one element per MR level.
    // DiagnosticParticles[lev] is typically a key-value pair where the key is
    // a pair [grid_index, tile_index], and the value is the corresponding
    // DiagnosticParticleData (see above) on this tile.
    using DiagnosticParticles = amrex::Vector<std::map<std::pair<int, int>, DiagnosticParticleData> >;

    WarpXParticleContainer (amrex::AmrCore* amr_core, int ispecies, const std::string& name);

    [[nodiscard]] const warpx::runtime::RuntimeConfig& config () const override;
    [[nodiscard]] const warpx::runtime::RuntimeData& data () const;
    [[nodiscard]] const warpx::runtime::RuntimeData& particleData () const override { return data(); }
    [[nodiscard]] warpx::runtime::HostServices& host () const;
    [[nodiscard]] const warpx::runtime::FrameState& frame () const;
    [[nodiscard]] const warpx::runtime::RuntimeContext* runtime_context () const override { return m_runtime_context; }
    [[nodiscard]] amrex::AmrCore* amrCore () const { return m_amr_core; }
    ~WarpXParticleContainer() override;

    // Move and copy operations
    WarpXParticleContainer(const WarpXParticleContainer&)            = delete;
    WarpXParticleContainer& operator=(const WarpXParticleContainer&) = delete;
    WarpXParticleContainer(WarpXParticleContainer&&)                 = delete;
    WarpXParticleContainer& operator=(WarpXParticleContainer&&)      = default;

    virtual void InitData () = 0;

    virtual void InitIonizationModule () {}

    /*
     * \brief Virtual function that returns a pointer to the plasma injector,
     *        for derived classes that define one (PhysicalParticleContainer).
     */
    virtual PlasmaInjector* GetPlasmaInjector (const int /*i*/) { return nullptr; }

    /**
     * \brief Pure virtual function to advance particles (typically by one time step).
     */
    virtual void Evolve (ablastr::fields::MultiFabRegister& fields,
                         int lev,
                         const std::string& current_fp_string,
                         amrex::Real t, amrex::Real dt, SubcyclingHalf subcycling_half=SubcyclingHalf::None, bool skip_deposition=false,
                         PositionPushType position_push_type=PositionPushType::Full,
                         MomentumPushType momentum_push_type=MomentumPushType::Full,
                         ImplicitOptions const * implicit_options = nullptr) = 0;

    /**
     * \brief Finish the implicit particle update at the end of a time step.
     *
     * The default implementation extrapolates position and momentum from
     * time n+1/2 to n+1, which is appropriate for physical and laser particles.
     *
     * Photon particles override this to instead perform a full explicit
     * push over the step (without deposition).
     *
     * \param fields  field register
     * \param lev     AMR level
     * \param t       time at end of the step (n+1)
     * \param dt      step size on this level
     */
    virtual void FinishImplicitParticleUpdate (
        ablastr::fields::MultiFabRegister& fields,
        int lev, amrex::Real t, amrex::Real dt);

    /** \brief Reset the particle data to the values at the start of the step
     */
    virtual void ResetImplicitParticleData (int lev);

    /**
     * \brief virtual function to deposit mass matrices for charged particles
     */
    virtual void DepositMassMatrices (ablastr::fields::MultiFabRegister& fields,
                                      int lev,
                                      amrex::Real dt,
                                      ImplicitOptions const& implicit_options)
    {
        amrex::ignore_unused(fields,lev,dt,implicit_options);
    }

    virtual void PostRestart () = 0;

    virtual void AllocData ();

    /**
     * \brief Virtual method to initialize runtime attributes. Must be overriden by each derived
     * class.
     */
    virtual void DefaultInitializeRuntimeAttributes (
        typename ContainerLike<amrex::PolymorphicArenaAllocator>::ParticleTileType& pinned_tile,
        int n_external_attr_real,
        int n_external_attr_int) = 0;

    ///
    /// This pushes the particle positions by one half time step.
    /// It is used to desynchronize the particles after initialization
    /// or when restarting from a checkpoint.
    ///
    void PushX (         amrex::Real dt);
    void PushX (int lev, amrex::Real dt);

    ///
    /// This pushes the particle momenta by dt.
    /// If gather_orders is set, E and B are gathered with these orders instead of
    /// WarpX::nox and WarpX::galerkin_interpolation (see FieldGatherOrders).
    ///
    virtual void PushP (int lev, amrex::Real dt,
                        const amrex::MultiFab& Ex,
                        const amrex::MultiFab& Ey,
                        const amrex::MultiFab& Ez,
                        const amrex::MultiFab& Bx,
                        const amrex::MultiFab& By,
                        const amrex::MultiFab& Bz,
                        MomentumPushType momentum_push_type = MomentumPushType::Full,
                        std::optional<FieldGatherOrders> gather_orders = std::nullopt) = 0;

    /**
     * \brief Deposit current density.
     *
     * \param[in,out] J vector of current densities (one three-dimensional array of pointers
     *                to MultiFabs per mesh refinement level)
     * \param[in] dt Time step for particle level
     * \param[in] relative_time Time at which to deposit J, relative to the time of the
     *                          current positions of the particles. When different than 0,
     *                          the particle position will be temporarily modified to match
     *                          the time of the deposition.
     * \param[in] push_type Type of particle push used with the deposition. With
     *                      PushType::Implicit the time-centered velocities are used.
     * \param[in] particle_shape Order of the particle shape factors used for the
     *                           deposition (defaults to algo.particle_shape).
     */
    void DepositCurrent (ablastr::fields::MultiLevelVectorField const & J,
                         amrex::Real dt, amrex::Real relative_time,
                         PushType push_type = PushType::Explicit,
                         std::optional<int> particle_shape = std::nullopt);

    /**
     * \brief Deposit current density, sum guard values, and apply boundary conditions.
     *
     * \param[in] mf_name Name of vector field in which current density should be deposited
     * \param[in] lev Mesh refinement level for deposition
     * \param[in] dt Time step for particle level
     * \param[in] relative_time Time at which to deposit J, relative to the time of the
     *                          current positions of the particles. When different than 0,
     *                          the particle position will be temporarily modified to match
     *                          the time of the deposition.
     */
    void DepositCurrentOnLevel (const std::string& mf_name, int lev,
                                amrex::Real dt, amrex::Real relative_time);

    /**
     * \brief Deposit charge without physical boundary folding or radial volume scaling.
     *
     * \param[in,out] rho Charge deposits, one MultiFab per mesh refinement level.
     * \param[in] local If false, sum overlapping valid- and guard-cell deposits.
     * \param[in] reset If true, zero the deposited time component and its guard cells.
     * \param[in] interpolate_across_levels whether to average down from the fine patch to the coarse patch
     * \param[in] icomp component of the MultiFab where rho is deposited (old, new)
     */
    void DepositCharge (const ablastr::fields::MultiLevelScalarField& rho,
                        bool local = false, bool reset = false,
                        bool interpolate_across_levels = true,
                        int icomp = 0);

    /**
     * \brief Deposit charge on one level without physical boundary folding or radial
     *        volume scaling.
     *
     * \param[in,out] rho MultiFab receiving the charge deposits.
     * \param[in] lev Mesh refinement level for deposition.
     * \param[in] local If false, sum overlapping valid- and guard-cell deposits.
     * \param[in] reset If true, zero the deposited time component and its guard cells.
     * \param[in] icomp Time component of rho to deposit into (old or new).
     */
    void DepositChargeOnLevel (amrex::MultiFab* rho, int lev,
                        bool local = false, bool reset = false,
                        int icomp = 0);

    /**
     * \brief Deposit charge from this species on one level with optional summation and finalization.
     *
     * \param[in] lev Mesh refinement level for deposition.
     * \param[in] local If false, sum overlapping valid- and guard-cell deposits.
     * \param[in] finalize If true, apply physical boundary conditions and radial volume
     *                     scaling to return physical charge density; requires local=false.
     *                     If false, leave boundary handling and radial scaling to the caller.
     */
    std::unique_ptr<amrex::MultiFab> GetChargeDensityOnLevel (int lev, bool local, bool finalize);

    virtual void DepositCharge (ParticleTileIter& pti,
                               RealVector const & wp,
                               const int* ion_lev,
                               amrex::MultiFab* rho,
                               int icomp,
                               long offset,
                               long np_to_deposit,
                               int thread_num,
                               int lev,
                               int depos_lev);

    virtual void DepositCurrent (ParticleTileIter& pti,
                                RealVector const & wp,
                                RealVector const & uxp,
                                RealVector const & uyp,
                                RealVector const & uzp,
                                int const* ion_lev,
                                amrex::MultiFab* jx,
                                amrex::MultiFab* jy,
                                amrex::MultiFab* jz,
                                long offset,
                                long np_to_deposit,
                                int thread_num,
                                int lev,
                                int depos_lev,
                                amrex::Real dt,
                                amrex::Real relative_time,
                                PushType push_type,
                                int particle_shape);
    /**
     * \brief Accumulate velocity moments for matched temperature deposition using particle shapes
     *
     * \param[in,out] T_vf vector field of Temperatures (one three-dimensional array of pointers
     *                to MultiFabs per mesh refinement level)
     * \param[in] relative_time Time at which to deposit J, relative to the time of the
     *                          current positions of the particles. When different than 0,
     *                          the particle position will be temporarily modified to match
     *                          the time of the deposition.
     */
    virtual void AccumulateVelocitiesAndComputeTemperature (
        [[maybe_unused]]ablastr::fields::MultiLevelVectorField const & T_vf,
        [[maybe_unused]]amrex::Real relative_time)
    {}

    virtual void DepositTemperature (
        [[maybe_unused]]ParticleTileIter& pti,
        [[maybe_unused]]RealVector const & wp,
        [[maybe_unused]]RealVector const & uxp,
        [[maybe_unused]]RealVector const & uyp,
        [[maybe_unused]]RealVector const & uzp,
        [[maybe_unused]]amrex::MultiFab * const Tx,
        [[maybe_unused]]amrex::MultiFab * const Ty,
        [[maybe_unused]]amrex::MultiFab * const Tz,
        [[maybe_unused]]long const offset,
        [[maybe_unused]]long const np_to_deposit,
        [[maybe_unused]]int const thread_num,
        [[maybe_unused]]const int lev,
        [[maybe_unused]]int const depos_lev,
        [[maybe_unused]]amrex::Real const relative_time,
        [[maybe_unused]]PushType push_type,
        [[maybe_unused]]const warpx::particles::deposition::TemperatureDepositionType type,
        [[maybe_unused]]const warpx::particles::deposition::TemperatureDepositionPass pass)
    {}

    void DepositTotalNGPTemperature (int lev);
    void CalculateNuei(amrex::MultiFab & species_nuei,
                       warpx::runtime::IParticleContainer const & electron_species, int lev);
    std::unique_ptr<amrex::MultiFab> GetDebyeLength (int lev);

    void DepositNumberDensity (amrex::MultiFab* number_density, int lev);
    std::unique_ptr<amrex::MultiFab> GetNumberDensity (int lev);

    std::unique_ptr<amrex::MultiFab> GetPlasmaFrequency (int lev);

    void DepositMassMatrices (ParticleTileIter& pti,
                        const RealVector& wp,
                        const RealVector& uxp,
                        const RealVector& uyp,
                        const RealVector& uzp,
                              amrex::MultiFab* Sxx,
                              amrex::MultiFab* Sxy,
                              amrex::MultiFab* Sxz,
                              amrex::MultiFab* Syx,
                              amrex::MultiFab* Syy,
                              amrex::MultiFab* Syz,
                              amrex::MultiFab* Szx,
                              amrex::MultiFab* Szy,
                              amrex::MultiFab* Szz,
                        const amrex::FArrayBox* bxfab,
                        const amrex::FArrayBox* byfab,
                        const amrex::FArrayBox* bzfab,
                              long offset,
                              long np_to_deposit,
                              int thread_num,
                              int lev,
                              int depos_lev,
                              amrex::Real dt,
                              ImplicitOptions const& implicit_options);

    // If particles start outside of the domain, ContinuousInjection
    // makes sure that they are initialized when they enter the domain, and
    // NOT before. Virtual function, overriden by derived classes.
    // Current status:
    // PhysicalParticleContainer: implemented.
    // LaserParticleContainer: implemented.
    // RigidInjectedParticleContainer: not implemented.
    virtual void ContinuousInjection(const amrex::RealBox& /*injection_box*/) {}

    /**
     * \brief Update antenna position for continuous injection of lasers
     *        in a boosted frame. Empty function for containers other than lasers.
     */
    virtual void UpdateAntennaPosition(const amrex::Real /*dt*/) {}

    bool doContinuousInjection() const {return do_continuous_injection;}

    // Inject a continuous flux of particles from a defined plane
    virtual void ContinuousFluxInjection(amrex::Real /*t*/, amrex::Real /*dt*/) {}

    int getSpeciesId() const {return species_id;}

    ///
    /// This returns the total energy/weight/charge for all the particles in this ParticleContainer.
    /// The total charge is needed when solving Poisson's equation with periodic boundary conditions.
    ///
    std::pair<amrex::ParticleReal, amrex::ParticleReal> sumParticleWeightAndEnergy (bool local = false) const;
    amrex::ParticleReal sumParticleCharge (bool local = false) const;
    amrex::ParticleReal sumParticleWeight (bool local = false) const;
    amrex::ParticleReal sumParticleEnergy (bool local = false) const;

    std::array<amrex::ParticleReal, 3> meanParticleVelocity(bool local = false);

    amrex::ParticleReal maxParticleDtInv(bool local = false);

    /**
     * \brief Map the momentum of the particles to and from the curvilinear frame.
     *        This is equivalent to mapping to the X-axis, i.e. theta = 0
     *        cylinderical and theta = phi = 0 for spherical. This mapping is used
     *        for binary collisions to put the colliding particles in a spatially local
     *        frame.
     *
     * \param[in] forward When true, maps to the frame, when false maps from the frame
     *            to the original location
     */
    void TransformMomentumToCurvilinear (bool forward);

    /**
     * \brief Adds n particles to the simulation
     *
     * @param[in] lev refinement level (unused)
     * @param[in] n the number of particles to add
     * @param[in] x x component of the position of particles to be added
     * @param[in] y y component of the position of particles to be added
     * @param[in] z z component of the position of particles to be added
     * @param[in] ux x component of the momentum of particles to be added
     * @param[in] uy y component of the momentum of particles to be added
     * @param[in] uz z component of the momentum of particles to be added
     * @param[in] nattr_real number of runtime real attributes to initialize with the attr_real
     * array. (particle weight is treated as a real attribute in this routine). The remaining
     * runtime real attributes are initialized in the method DefaultInitializeRuntimeAttributes.
     * @param[in] attr_real value of real attributes to initialize
     * @param[in] nattr_int number of runtime int attributes to initialize with the attr_int array.
     * The remaining runtime int attributes are initialized in the method
     * DefaultInitializeRuntimeAttributes.
     * @param[in] attr_int value of int attributes to initialize
     * @param[in] uniqueparticles if true, each MPI rank calling this function creates n
     * particles. Else, all MPI ranks work together to create n particles in total.
     * @param[in] id if different than -1, this id will be assigned to the particles (used for
     * particle tagging in some routines, e.g. SplitParticle)
     */
    void AddNParticles (int lev, long n,
                        amrex::Vector<amrex::ParticleReal> const & x,
                        amrex::Vector<amrex::ParticleReal> const & y,
                        amrex::Vector<amrex::ParticleReal> const & z,
                        amrex::Vector<amrex::ParticleReal> const & ux,
                        amrex::Vector<amrex::ParticleReal> const & uy,
                        amrex::Vector<amrex::ParticleReal> const & uz,
                        int nattr_real,
                        amrex::Vector<amrex::Vector<amrex::ParticleReal>> const & attr_real,
                        int nattr_int, amrex::Vector<amrex::Vector<int>> const & attr_int,
                        int uniqueparticles, amrex::Long id=-1);

    /** Remove particles with invalid ID
    *
    * This is a local operation (no MPI communication) and is thus preferable over `ReDistribute`
    * (which also removes invalid particles, but involves MPI communications at the same time)
    */
    void deleteInvalidParticles ();

    virtual void ReadHeader (std::istream& is) = 0;

    virtual void WriteHeader (std::ostream& os) const = 0;

    static void ReadParameters ();

    static void BackwardCompatibility ();

    /** \brief Apply particle BC.
     */
    void ApplyBoundaryConditions ();

    bool do_splitting = false;

    int do_not_deposit = 0;
    bool initialize_self_fields = false;
    amrex::Real self_fields_required_precision = amrex::Real(1.e-11);
    amrex::Real self_fields_absolute_tolerance = amrex::Real(0.0);
    int self_fields_max_iters = 200;
    int self_fields_verbosity = 2;

    // External fields added to particle fields.
    amrex::Vector<amrex::ParticleReal> m_B_external_particle;
    amrex::Vector<amrex::ParticleReal> m_E_external_particle;

    //! Current injection position
    amrex::Real m_current_injection_position = 0.;

    // split along diagonals (0) or axes (1)
    int split_type = 0;

    /** Whether back-transformed diagnostics need to be performed for a particular species.
     *
     * \param[in] do_back_transformed_particles The parameter to set if back-transformed particles are set to true/false
     */
    void SetDoBackTransformedParticles (const bool do_back_transformed_particles) {
        m_do_back_transformed_particles = do_back_transformed_particles;
    }

    bool GetDoBackTransformedParticles () const {
        return m_do_back_transformed_particles;
    }

    amrex::ParticleReal getCharge () const {return m_charge;}

    amrex::ParticleReal getMass () const {return m_mass;}

    /** Thermal velocity (normalized by c) used by thermal particle boundaries
     *  for this species, parsed from ``boundary.\<species_name\>.u_th``.
     */
    amrex::ParticleReal getBoundaryThermalVelocity () const
    {
        return static_cast<amrex::ParticleReal>(m_boundary_conditions.data.m_uth);
    }

    int DoFieldIonization() const { return do_field_ionization; }

#ifdef WARPX_QED
    //Species for which QED effects are relevant should override these methods
    virtual bool has_quantum_sync() const {return false;}
    virtual bool has_breit_wheeler() const {return false;}
    virtual bool has_virtual_photons() const {return false;}
    virtual bool has_virtual_photons_beam_size_effect() const {return false;}
    virtual int getVirtualPhotonSpeciesIndex() const { return -1; }
    int DoQED() const { return has_quantum_sync() || has_breit_wheeler(); }
#else
    int DoQED() const { return false; }
#endif

    /* \brief This function tests if the current species
    *  is of a given PhysicalSpecies (specified as a template parameter).
    * @tparam PhysSpec the PhysicalSpecies to test against
    * @return the result of the test
    */
    template<PhysicalSpecies PhysSpec>
    bool AmIA () const noexcept {return (physical_species == PhysSpec);}

    /**
    * \brief This function returns a string containing the name of the species type
    */
    std::string getSpeciesTypeName () const override {return species::get_name(physical_species);}

    /**
     * \brief Virtual method to resample the species. Overriden by PhysicalParticleContainer only.
     * Empty body is here because making the method purely virtual would mean that we need to
     * override the method for every derived class. Note that in practice this function is never
     * called because resample() is only called for PhysicalParticleContainers.
     */
    virtual void resample (const amrex::Vector<amrex::Geometry>& /*geom*/, const int /*timestep*/, bool /*verbose*/) {}

    /**
     * When using runtime components, AMReX requires to touch all tiles
     * in serial and create particles tiles with runtime components if
     * they do not exist (or if they were defined by default, i.e.,
     * without runtime component).
     */
    void defineAllParticleTiles () noexcept;

    virtual std::vector<std::string> getUserIntAttribs () const { return std::vector<std::string>{}; }

    virtual std::vector<std::string> getUserRealAttribs () const { return std::vector<std::string>{}; }

    virtual amrex::Vector< amrex::Parser* > getUserIntAttribParser () const { return amrex::Vector<amrex::Parser*>{}; }

    virtual amrex::Vector< amrex::Parser* > getUserRealAttribParser () const { return amrex::Vector<amrex::Parser*>{}; }

#ifdef WARPX_QED
    virtual BreitWheelerEngine* get_breit_wheeler_engine_ptr () const {return nullptr;}
    virtual QuantumSynchrotronEngine* get_quantum_sync_engine_ptr () const {return nullptr;}
#endif

    void setDoNotPush (bool flag) { do_not_push = flag; }
    void setDoNotGather (int flag) { do_not_gather = flag; }
    void setDoNotDeposit (int flag) { do_not_deposit = flag; }

    bool HasiAttrib (const std::string& name)
    {
        std::vector<std::string> const & int_names = GetIntSoANames();
        auto const pos = std::find(int_names.begin(), int_names.end(), name);
        return (pos != int_names.end());
    }

    std::string const & getName() { return species_name; }

protected:
    int species_id;
    std::string species_name;

    amrex::ParticleReal m_charge;
    amrex::ParticleReal m_mass;
    PhysicalSpecies physical_species = PhysicalSpecies::unspecified;

    // Controls boundaries for particles exiting the domain
    ParticleBoundaries m_boundary_conditions;

    // Controls whether particle trajectoris are cropped when particles
    // cross a PEC boundary
    bool m_crop_on_PEC_boundary = false;

    //! instead of depositing (current, charge) on the finest patch level, deposit to the coarsest grid
    bool m_deposit_on_main_grid = false;

    //! instead of gathering fields from the finest patch level, gather from the coarsest
    bool m_gather_from_main_grid = false;

    bool do_not_push = false;
    int do_not_gather = 0;

    // Whether to allow particles outside of the simulation domain to be
    // initialized when they enter the domain.
    // This is currently required because continuous injection does not
    // support all features allowed by direct injection.
    int do_continuous_injection = 0;

    int do_field_ionization = 0;
    int do_adk_correction = 0;
    int ionization_product;
    std::string ionization_product_name;
    int ion_atomic_number;
    int ionization_initial_level = 0;
    amrex::Gpu::DeviceVector<amrex::Real> ionization_energies;
    amrex::Gpu::DeviceVector<amrex::Real> adk_power;
    amrex::Gpu::DeviceVector<amrex::Real> adk_prefactor;
    amrex::Gpu::DeviceVector<amrex::Real> adk_exp_prefactor;
    /** for correction in Zhang et al., PRA 90, 043410 (2014). a1, a2, a3, Ecrit. */
    amrex::Gpu::DeviceVector<amrex::Real> adk_correction_factors;
    std::string physical_element;

    int do_resampling = 0;

    /** Whether back-transformed diagnostics is turned on for the corresponding species.*/
    bool m_do_back_transformed_particles = false;

#ifdef WARPX_QED
    //Species can receive a shared pointer to a QED engine (species for
    //which this is relevant should override these functions)
    virtual void
    set_breit_wheeler_engine_ptr(const std::shared_ptr<BreitWheelerEngine>&){}
    virtual void
    set_quantum_sync_engine_ptr(const std::shared_ptr<QuantumSynchrotronEngine>&){}

    int m_qed_breit_wheeler_ele_product;
    std::string m_qed_breit_wheeler_ele_product_name;
    int m_qed_breit_wheeler_pos_product;
    std::string m_qed_breit_wheeler_pos_product_name;
    int m_qed_quantum_sync_phot_product;
    std::string m_qed_quantum_sync_phot_product_name;
    int m_qed_virtual_photon_species;
    std::string m_qed_virtual_photon_species_name;
    amrex::Real m_qed_virtual_photon_min_energy;
#endif
    amrex::Vector<amrex::FArrayBox> local_rho;
    amrex::Vector<amrex::FArrayBox> local_jx;
    amrex::Vector<amrex::FArrayBox> local_jy;
    amrex::Vector<amrex::FArrayBox> local_jz;

    // local containers for mass matrix deposit used by implicit solvers
    amrex::Vector<amrex::FArrayBox> local_Sxx;
    amrex::Vector<amrex::FArrayBox> local_Sxy;
    amrex::Vector<amrex::FArrayBox> local_Sxz;
    amrex::Vector<amrex::FArrayBox> local_Syx;
    amrex::Vector<amrex::FArrayBox> local_Syy;
    amrex::Vector<amrex::FArrayBox> local_Syz;
    amrex::Vector<amrex::FArrayBox> local_Szx;
    amrex::Vector<amrex::FArrayBox> local_Szy;
    amrex::Vector<amrex::FArrayBox> local_Szz;

public:

    int getIonizationInitialLevel () const noexcept override {return ionization_initial_level;}

    // By default this returns false, can be overridden in PysicalParticleContainer
    virtual bool getTemperatureDepositionFlag () const noexcept {return false;}

private:
    const warpx::runtime::RuntimeContext* m_runtime_context = nullptr;
    amrex::AmrCore* m_amr_core = nullptr;
    mutable std::unique_ptr<warpx::runtime::WarpXHostServices> m_legacy_host;
    void particlePostLocate(ParticleType& p, const amrex::ParticleLocData& pld, int lev) override;

};

#endif
