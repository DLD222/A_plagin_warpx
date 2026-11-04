# A0 API catalog — Root

> Generated from headers; 1 classes/structs. Signatures are parsed heuristically; check the header for exact defaults/templates.

## `WarpX`

- file: `Source/WarpX.H`
- kind: `class`, bases: `public amrex::AmrCore`
- public methods: 217; public members: 89; private members: 122

```cpp
static WarpX& GetInstance ()
static void ResetInstance ()
static void Finalize()
~WarpX () override
WarpX ( WarpX const &) = delete
WarpX& operator= ( WarpX const & ) = delete
WarpX ( WarpX && ) = delete
WarpX& operator= ( WarpX && ) = delete
static std::string Version ()
static std::string PicsarVersion ()
[[nodiscard]] int Verbose () const
[[nodiscard]] const amrex::Array<FieldBoundaryType,AMREX_SPACEDIM>& GetFieldBoundaryLo () const
[[nodiscard]] const amrex::Array<FieldBoundaryType,AMREX_SPACEDIM>& GetFieldBoundaryHi () const
[[nodiscard]] int GetPECInsulator_IsESet ( int bdry_dir, int bdry_side ) const
void InitData ()
void Evolve (int numsteps = -1)
void SynchronizeVelocityWithPosition ()
void SyncMassMatricesPC ()
void SyncMassMatrices ()
void SaveParticlesAtImplicitStepStart ()
void FinishImplicitParticleUpdate (amrex::Real a_time)
void ResetImplicitParticleData ()
void SetElectricFieldAndApplyBCs (const WarpXSolverVec& a_E, amrex::Real a_time)
void UpdateMagneticFieldAndApplyBCs (ablastr::fields::MultiLevelVectorField const& a_Bn, amrex::Real a_thetadt, amrex::Real start_time)
void SpectralSourceFreeFieldAdvance (amrex::Real start_time)
void FinishMagneticFieldAndApplyBCs (amrex::Real a_theta, amrex::Real a_time)
void FinishElectricFieldAndApplyBCs (amrex::Real a_theta, amrex::Real a_time)
void FinishImplicitField (const ablastr::fields::MultiLevelVectorField& Field_fp, const ablastr::fields::MultiLevelVectorField& Field_n, amrex::Real theta)
void DepositMassMatrices (amrex::Real a_dt, ImplicitOptions const& implicit_options)
void ImplicitComputeRHSE ( amrex::Real dt, WarpXSolverVec& a_Erhs_vec)
void ImplicitComputeRHSE (int lev, amrex::Real dt, WarpXSolverVec& a_Erhs_vec)
void ImplicitComputeRHSE (int lev, PatchType patch_type, amrex::Real dt, WarpXSolverVec& a_Erhs_vec)
MultiParticleContainer& GetPartContainer ()
MultiFluidContainer& GetFluidContainer ()
ElectrostaticSolver& GetElectrostaticSolver ()
[[nodiscard]] HybridPICModel * get_pointer_HybridPICModel () const
[[nodiscard]] ImplicitSolver * get_pointer_ImplicitSolver () const
MultiDiagnostics& GetMultiDiags ()
ParticleBoundaryBuffer& GetParticleBoundaryBuffer ()
amrex::Vector<std::array< std::unique_ptr<amrex::iMultiFab>,3 > >& GetEBUpdateEFlag_fp()
amrex::Vector<std::array< std::unique_ptr<amrex::iMultiFab>,3 > >& GetEBUpdateBFlag_fp()
amrex::Vector< std::unique_ptr<amrex::iMultiFab> > const & GetEBReduceParticleShapeFlag() const
[[nodiscard]] std::string GetAuthors () const
void AllocInitMultiFab ( std::unique_ptr<amrex::iMultiFab>& mf, const amrex::BoxArray& ba, const amrex::DistributionMapping& dm, int ncomp, const amrex::IntVect& ngrow, int level, const std::string& name, std::optional<const int> initial_value = {})
[[nodiscard]] const amrex::iMultiFab* getFieldDotMaskPointer (warpx::fields::FieldType field_type, int lev, ablastr::fields::Direction dir) const
[[nodiscard]] bool DoPML () const
[[nodiscard]] bool DoFluidSpecies () const
[[nodiscard]] std::vector<bool> getPMLdirections() const
static amrex::LayoutData<amrex::Real>* getCosts (int lev)
void setLoadBalanceEfficiency (int lev, amrex::Real efficiency)
amrex::Real getLoadBalanceEfficiency (int lev)
amrex::Vector<amrex::Real> m_v_galilean = amrex::Vector<amrex::Real>(3, amrex::Real(0.))
amrex::Vector<amrex::Real> m_v_comoving = amrex::Vector<amrex::Real>(3, amrex::Real(0.))
void applyMirrors(amrex::Real time)
void ComputeDt ()
amrex::Real GlobalPlasmaFrequencyMax ()
amrex::Real GlobalCyclotronFrequencyMax ()
void ApplyDtLimiters (int step)
void PrintMainPICparameters ()
void ComputeMaxStep ()
void computeMaxStepBoostAccelerator()
int MoveWindow (int step, bool move_j)
void ShiftGalileanBoundary ()
void ResetProbDomain (const amrex::RealBox& rb)
void EvolveE ( amrex::Real dt, amrex::Real start_time)
void EvolveE (int lev, amrex::Real dt, amrex::Real start_time)
void EvolveB ( amrex::Real dt, SubcyclingHalf subcycling_half, amrex::Real start_time)
void EvolveB (int lev, amrex::Real dt, SubcyclingHalf subcycling_half, amrex::Real start_time)
void EvolveF ( amrex::Real dt, int rho_comp)
void EvolveF (int lev, amrex::Real dt, int rho_comp)
void EvolveG ( amrex::Real dt)
void EvolveG (int lev, amrex::Real dt)
void EvolveB (int lev, PatchType patch_type, amrex::Real dt, SubcyclingHalf subcycling_half, amrex::Real start_time)
void EvolveE (int lev, PatchType patch_type, amrex::Real dt, amrex::Real start_time)
void EvolveF (int lev, PatchType patch_type, amrex::Real dt, int rho_comp)
void EvolveG (int lev, PatchType patch_type, amrex::Real dt)
void MacroscopicEvolveE ( amrex::Real dt, amrex::Real start_time)
void MacroscopicEvolveE (int lev, amrex::Real dt, amrex::Real start_time)
void MacroscopicEvolveE (int lev, PatchType patch_type, amrex::Real dt, amrex::Real start_time)
void HybridPICEvolveFields ()
void HybridPICInitializeRhoJandB ()
void HybridPICDepositRhoAndJ ()
void Hybrid_QED_Push ( amrex::Vector<amrex::Real> dt)
void Hybrid_QED_Push (int lev, amrex::Real dt)
void Hybrid_QED_Push (int lev, PatchType patch_type, amrex::Real dt)
void CheckLoadBalance (int step)
void LoadBalance ()
void ResetCosts ()
void RescaleCosts (int step)
[[nodiscard]] ablastr::utils::text::IntervalsParser get_load_balance_intervals () const
void DampFieldsInGuards (int lev, const ablastr::fields::VectorField& Efield, const ablastr::fields::VectorField& Bfield)
void DampFieldsInGuards (int lev, amrex::MultiFab* mf)
void ApplyInverseVolumeScalingToCurrentDensity (amrex::MultiFab* Jx, amrex::MultiFab* Jy, amrex::MultiFab* Jz, int lev) const
void ApplyInverseVolumeScalingToMassMatricesPC (amrex::MultiFab* Sxx, amrex::MultiFab* Syy, amrex::MultiFab* Szz, int lev) const
void ApplyInverseVolumeScalingToChargeDensity (amrex::MultiFab* Rho, int lev, int scomp = 0, int ncomp = -1) const
void ApplyRhofieldBoundary (int lev, amrex::MultiFab* Rho, PatchType patch_type, int scomp = 0, int ncomp = -1)
void ApplyJfieldBoundary (int lev, amrex::MultiFab* Jx, amrex::MultiFab* Jy, amrex::MultiFab* Jz, PatchType patch_type)
void ApplyEfieldBoundary (int lev, PatchType patch_type, amrex::Real cur_time)
void ApplyBfieldBoundary (int lev, PatchType patch_type, SubcyclingHalf subcycling_half, amrex::Real cur_time)
void ApplyFieldBoundaryOnAxis (amrex::MultiFab* Er, amrex::MultiFab* Et, amrex::MultiFab* Ez, int lev) const
void ApplyElectronPressureBoundary (int lev, PatchType patch_type)
void DampPML ()
void DampPML (int lev)
void DampPML (int lev, PatchType patch_type)
void DampPML_Cartesian (int lev, PatchType patch_type)
void DampJPML ()
void DampJPML (int lev)
void DampJPML (int lev, PatchType patch_type)
void CopyJPML ()
static bool isAnyParticleBoundaryThermal()
PML* GetPML (int lev)
PML_RZ* GetPML_RZ (int lev)
void doFieldIonization ()
void doQEDEvents ()
void PushParticlesandDeposit (int lev, amrex::Real cur_time, SubcyclingHalf subcycling_half=SubcyclingHalf::None, bool skip_deposition=false, PositionPushType position_push_type=PositionPushType::Full, MomentumPushType momentum_push_type=MomentumPushType::Full, amrex::Real dt_scale=static_cast<amrex::Real>(1.), ImplicitOptions const * implicit_options = nullptr)
void PushParticlesandDeposit (amrex::Real cur_time, bool skip_deposition=false, PositionPushType position_push_type=PositionPushType::Full, MomentumPushType momentum_push_type=MomentumPushType::Full, amrex::Real dt_scale=static_cast<amrex::Real>(1.), ImplicitOptions const * implicit_options = nullptr)
void HandleParticlesAtBoundaries (int step, amrex::Real cur_time, int num_moved)
void UpdateAuxiliaryData ()
void UpdateAuxiliaryDataStagToNodal ()
void UpdateAuxiliaryDataSameType ()
void FillBoundaryB (amrex::IntVect ng, std::optional<bool> nodal_sync = std::nullopt)
void FillBoundaryE (amrex::IntVect ng, std::optional<bool> nodal_sync = std::nullopt)
void FillBoundaryB_avg (amrex::IntVect ng)
void FillBoundaryE_avg (amrex::IntVect ng)
void FillBoundaryF (amrex::IntVect ng, std::optional<bool> nodal_sync = std::nullopt)
void FillBoundaryG (amrex::IntVect ng, std::optional<bool> nodal_sync = std::nullopt)
void FillBoundaryAux (amrex::IntVect ng)
void FillBoundaryE (int lev, amrex::IntVect ng, std::optional<bool> nodal_sync = std::nullopt)
void FillBoundaryB (int lev, amrex::IntVect ng, std::optional<bool> nodal_sync = std::nullopt)
void FillBoundaryE_avg (int lev, amrex::IntVect ng)
void FillBoundaryB_avg (int lev, amrex::IntVect ng)
void FillBoundaryF (int lev, amrex::IntVect ng, std::optional<bool> nodal_sync = std::nullopt)
void FillBoundaryG (int lev, amrex::IntVect ng, std::optional<bool> nodal_sync = std::nullopt)
void FillBoundaryAux (int lev, amrex::IntVect ng)
void SyncCurrentAndRho ()
void FinalizeJ ()
void FinalizeJOnLevel (int lev, amrex::MultiFab* Jx, amrex::MultiFab* Jy, amrex::MultiFab* Jz, PatchType patch_type)
void FinalizeRho ()
void FinalizeRhoOnLevel (int lev, amrex::MultiFab* rho, PatchType patch_type, int scomp = 0, int ncomp = -1)
[[nodiscard]] bool UseVerboncoeurAxisCorrection () const
void SyncCurrent (const std::string& current_fp_string)
void SyncRho ()
void SyncRho ( const ablastr::fields::MultiLevelScalarField& charge_fp, const ablastr::fields::MultiLevelScalarField& charge_cp, ablastr::fields::MultiLevelScalarField const & charge_buffer)
[[nodiscard]] amrex::Vector<int> getnsubsteps () const
[[nodiscard]] int getnsubsteps (int lev) const
[[nodiscard]] amrex::Vector<int> getistep () const
[[nodiscard]] int getistep (int lev) const
void setistep (int lev, int ii)
[[nodiscard]] amrex::Vector<amrex::Real> gett_old () const
[[nodiscard]] amrex::Real gett_old (int lev) const
[[nodiscard]] amrex::Vector<amrex::Real> gett_new () const
[[nodiscard]] amrex::Real gett_new (int lev) const
void sett_new (int lev, amrex::Real time)
[[nodiscard]] amrex::Vector<amrex::Real> getdt () const
[[nodiscard]] amrex::Real getdt (int lev) const
[[nodiscard]] int getdo_moving_window() const
[[nodiscard]] amrex::Real getmoving_window_x() const
[[nodiscard]] bool getis_synchronized() const
[[nodiscard]] int maxStep () const
void updateMaxStep (const int new_max_step)
[[nodiscard]] amrex::Real stopTime () const
void updateStopTime (const amrex::Real new_stop_time)
static std::array<amrex::Real,3> CellSize (int lev)
static amrex::XDim3 InvCellSize (int lev)
static amrex::RealBox getRealBox(const amrex::Box& bx, int lev)
static amrex::XDim3 LowerCorner (const amrex::Box& bx, int lev, amrex::Real time_shift_delta)
static amrex::XDim3 UpperCorner (const amrex::Box& bx, int lev, amrex::Real time_shift_delta)
static amrex::IntVect RefRatio (int lev)
static const amrex::iMultiFab* CurrentBufferMasks (int lev)
static const amrex::iMultiFab* GatherBufferMasks (int lev)
static int moving_window_active (int const step)
static amrex::Real MovingWindowShiftPerStep (amrex::Real dt)
static int NumCellsShifted (amrex::Real displacement, amrex::Real cell_size)
static void ComputeDivB (amrex::MultiFab& divB, int dcomp, ablastr::fields::VectorField const & B, const std::array<amrex::Real,3>& dx)
static void ComputeDivB (amrex::MultiFab& divB, int dcomp, ablastr::fields::VectorField const & B, const std::array<amrex::Real,3>& dx, amrex::IntVect ngrow)
void ComputeDivE(amrex::MultiFab& divE, int lev)
void ProjectionCleanDivB ()
void CalculateExternalCurlA ()
[[nodiscard]] amrex::IntVect getngEB() const
[[nodiscard]] amrex::IntVect getngF() const
[[nodiscard]] amrex::IntVect getngUpdateAux() const
[[nodiscard]] amrex::IntVect get_ng_depos_J() const
[[nodiscard]] amrex::IntVect get_ng_depos_rho() const
[[nodiscard]] amrex::IntVect get_ng_fieldgather () const
[[nodiscard]] amrex::IntVect get_numprocs() const
void ComputeSpaceChargeField (bool reset_E_field, bool reset_B_field, bool verbose_step)
amrex::Real magnetostatic_solver_required_precision = amrex::Real(1.e-11)
amrex::Real magnetostatic_solver_absolute_tolerance = amrex::Real(0.0)
void ComputeMagnetostaticField ()
void AddMagnetostaticFieldLabFrame ()
void computeVectorPotential (ablastr::fields::MultiLevelVectorField const& curr, ablastr::fields::MultiLevelVectorField const& A, amrex::Real required_precision=amrex::Real(1.e-11), amrex::Real absolute_tolerance=amrex::Real(0.0), int max_iters=200, int verbosity=2)
void setVectorPotentialBC (ablastr::fields::MultiLevelVectorField const& A) const
void ComputeExternalFieldOnGridUsingParser ( const std::variant<warpx::fields::FieldType, std::string>& field, amrex::ParserExecutor<4> const& fx_parser, amrex::ParserExecutor<4> const& fy_parser, amrex::ParserExecutor<4> const& fz_parser, int lev, PatchType patch_type, amrex::Vector<std::array< std::unique_ptr<amrex::iMultiFab>,3 > > const& eb_update_field, bool use_eb_flags = true)
void LoadExternalFields (int lev)
void ReadExternalFieldFromFile ( const std::string& read_fields_from_path, amrex::MultiFab* mf, const std::string& F_name, const std::string& F_component, int dest_comp = 0)
void InitializeEBGridData(int lev)
void ComputeCostsHeuristic (amrex::Vector<std::unique_ptr<amrex::LayoutData<amrex::Real> > >& costs)
void ApplyFilterandSumBoundaryRho (int lev, int glev, amrex::MultiFab& rho, int icomp, int ncomp)
void ApplyFilterMF ( const ablastr::fields::MultiLevelVectorField& mfvec, int lev, int idim)
void ApplyFilterMF ( const ablastr::fields::MultiLevelVectorField& mfvec, int lev)
amrex::IntVect ApplyVolumeWeightedFilter (amrex::MultiFab& dst, const amrex::MultiFab& src_mf, int lev, int scomp, int dcomp, int ncomp, bool is_volume_scaled = true)
void ApplyFilterJ ( const ablastr::fields::MultiLevelVectorField& current, int lev, int idim)
void ApplyFilterJ ( const ablastr::fields::MultiLevelVectorField& current, int lev)
void ErrorEst (int lev, amrex::TagBoxArray& tags, amrex::Real time, int ) final
const AcceleratorLattice& get_accelerator_lattice (int lev)
void BuildBufferMasksInBox ( amrex::Box tbx, amrex::IArrayBox &buffer_mask, const amrex::IArrayBox &guard_mask, int ng )
[[nodiscard]] amrex::EBFArrayBoxFactory const& fieldEBFactory (int lev) const noexcept
void InitEB ()
void ComputeDistanceToEB ()
void ComputeFaceExtensions()
void ComputeOneWayExtensions()
void ComputeEightWaysExtensions()
auto& get_spectral_solver_fp (int lev)
FiniteDifferenceSolver * get_pointer_fdtd_solver_fp (int lev)
ablastr::fields::MultiFabRegister& GetMultiFabRegister()
void PostProcessBaseGrids (amrex::BoxArray& ba0) const final
amrex::EB2::IndexSpace const* GetEBIndexSpace (int lev) const
```

Public members:

```cpp
int maxlevel_extEMfield_init
static inline auto current_deposition_algo = CurrentDepositionAlgo::Default
static inline auto charge_deposition_algo = ChargeDepositionAlgo::Default
static inline auto field_gathering_algo = GatheringAlgo::Default
static inline auto particle_pusher_algo = ParticlePusherAlgo::Default
static inline auto electromagnetic_solver_id = ElectromagneticSolverAlgo::Default
static inline int ect_conformal_E = 0
static inline amrex::Real ect_conformal_E_Astab = 0.5
static inline int macroscopic_subpixel = 0
static inline int ect_te_borrowing = 1
EvolveScheme evolve_scheme = EvolveScheme::Default
static inline auto load_balance_costs_update_algo = LoadBalanceCostsUpdateAlgo::Default
static inline amrex::Array<FieldBoundaryType,AMREX_SPACEDIM> field_boundary_lo
static inline amrex::Array<FieldBoundaryType,AMREX_SPACEDIM> field_boundary_hi
static inline amrex::Array<ParticleBoundaryType,AMREX_SPACEDIM> particle_boundary_lo
static inline amrex::Array<ParticleBoundaryType,AMREX_SPACEDIM> particle_boundary_hi
static inline ParticleBoundaryType eb_particle_boundary = ParticleBoundaryType::Absorbing
static inline auto time_dependency_J = TimeDependencyJ::Default
static inline auto time_dependency_rho = TimeDependencyRho::Default
bool do_current_centering = false
bool current_correction = true
bool update_with_rho = false
static bool do_single_precision_comms
static bool do_shared_mem_charge_deposition
static bool do_shared_mem_current_deposition
static int shared_mem_current_tpb
static amrex::IntVect shared_tilesize
static amrex::IntVect m_fill_guards_fields
static amrex::IntVect m_fill_guards_current
static bool do_dive_cleaning
static bool do_divb_cleaning
static int nox
static int noy
static int noz
static int particle_max_grid_crossings
static constexpr int villasenor_mass_matrices_max_grid_crossings = 3
static int field_centering_nox
static int field_centering_noy
static int field_centering_noz
static int n_rz_azimuthal_modes
```

