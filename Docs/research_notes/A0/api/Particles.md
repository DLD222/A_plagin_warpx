# A0 API catalog — Particles

<!-- SPDX-License-Identifier: BSD-3-Clause -->

> Generated; 97 classes/structs. Signatures are heuristic.

## `BackgroundMCCCollision`

- file: `Source/Particles/Collision/BackgroundMCC/BackgroundMCCCollision.H`
- kind: `class`, bases: `public CollisionBase`
- public methods: 10; public members: 0

```cpp
explicit BackgroundMCCCollision (std::string const& collision_name)
~BackgroundMCCCollision () override = default
BackgroundMCCCollision ( BackgroundMCCCollision const &) = delete
BackgroundMCCCollision& operator= ( BackgroundMCCCollision const & ) = delete
BackgroundMCCCollision ( BackgroundMCCCollision&& ) = delete
BackgroundMCCCollision& operator= ( BackgroundMCCCollision&& ) = delete
[[nodiscard]] amrex::ParticleReal get_nu_max (amrex::Vector<ScatteringProcess> const& mcc_processes) const
void doCollisions (amrex::Real cur_time, amrex::Real dt, MultiParticleContainer* mypc) override
void doBackgroundCollisionsWithinTile ( WarpXParIter& pti, amrex::Real t)
void doBackgroundIonization ( int lev, amrex::LayoutData<amrex::Real>* cost, WarpXParticleContainer& species1, WarpXParticleContainer& species2, amrex::Real t )
```

## `ImpactIonizationFilterFunc`

- file: `Source/Particles/Collision/BackgroundMCC/ImpactIonization.H`
- kind: `class`
- public methods: 1; public members: 0

```cpp
ImpactIonizationFilterFunc( ScatteringProcess const& mcc_process, double const mass, amrex::ParticleReal const total_collision_prob, amrex::ParticleReal const nu_max, amrex::ParserExecutor<4> const& n_a_func, amrex::Real t ) : m_mcc_process(mcc_process.executor()), m_mass(mass), m_total_collision_prob(total_collision_prob), m_nu_max(nu_max), m_n_a_func(n_a_func), m_t(t)
```

## `ImpactIonizationTransformFunc`

- file: `Source/Particles/Collision/BackgroundMCC/ImpactIonization.H`
- kind: `class`
- public methods: 1; public members: 0

```cpp
ImpactIonizationTransformFunc( amrex::ParticleReal energy_cost, double mass1, amrex::ParticleReal sqrt_kb_m, amrex::ParserExecutor<4> const& T_a_func, amrex::Real t ) : m_energy_cost(energy_cost), m_mass1(mass1), m_sqrt_kb_m(sqrt_kb_m), m_T_a_func(T_a_func), m_t(t)
```

## `BackgroundStopping`

- file: `Source/Particles/Collision/BackgroundStopping/BackgroundStopping.H`
- kind: `class`, bases: `public CollisionBase`
- public methods: 9; public members: 0

```cpp
explicit BackgroundStopping (const std::string& collision_name)
~BackgroundStopping () override = default
BackgroundStopping ( BackgroundStopping const &) = delete
BackgroundStopping& operator= ( BackgroundStopping const & ) = delete
BackgroundStopping ( BackgroundStopping&& ) = delete
BackgroundStopping& operator= ( BackgroundStopping&& ) = delete
void doCollisions (amrex::Real cur_time, amrex::Real dt, MultiParticleContainer* mypc) override
void doBackgroundStoppingOnElectronsWithinTile (WarpXParIter& pti, amrex::Real dt, amrex::Real t, amrex::ParticleReal species_mass, amrex::ParticleReal species_charge)
void doBackgroundStoppingOnIonsWithinTile (WarpXParIter& pti, amrex::Real dt, amrex::Real t, amrex::ParticleReal species_mass, amrex::ParticleReal species_charge)
```

## `BinaryCollision`

- file: `Source/Particles/Collision/BinaryCollision/BinaryCollision.H`
- kind: `class`, bases: `public CollisionBase`
- public methods: 9; public members: 0

```cpp
BinaryCollision (const std::string& collision_name, MultiParticleContainer const * const mypc) : CollisionBase(collision_name)
~BinaryCollision () override = default
BinaryCollision ( BinaryCollision const &) = default
BinaryCollision& operator= ( BinaryCollision const & ) = default
BinaryCollision ( BinaryCollision&& ) = delete
BinaryCollision& operator= ( BinaryCollision&& ) = delete
void AllocData () override
void doCollisions (amrex::Real cur_time, amrex::Real dt, MultiParticleContainer* mypc) override
void doCollisionsWithinTile ( amrex::Real dt, int const lev, amrex::MFIter const& mfi, WarpXParticleContainer& species_1, WarpXParticleContainer& species_2, amrex::Vector<WarpXParticleContainer*> product_species_vector, SmartCopy* copy_species1, SmartCopy* copy_species2)
```

## `BremsstrahlungFunc`

- file: `Source/Particles/Collision/BinaryCollision/Bremsstrahlung/BremsstrahlungFunc.H`
- kind: `class`, bases: `public CollisionFuncBase`
- public methods: 4; public members: 1

```cpp
BremsstrahlungFunc () = default
BremsstrahlungFunc (std::string const& collision_name, MultiParticleContainer const * mypc, bool )
[[nodiscard]] Executor const& executor (int const , amrex::MFIter const& ) const
[[nodiscard]] bool use_global_debye_length() const
```

Public members:

```cpp
struct Executor
```

## `Executor`

- file: `Source/Particles/Collision/BinaryCollision/Bremsstrahlung/BremsstrahlungFunc.H`
- kind: `struct`
- public methods: 0; public members: 11

Public members:

```cpp
bool m_computeSpeciesDensities = true
bool m_computeSpeciesTemperatures = false
bool m_need_product_data = true
amrex::ParticleReal m_multiplier
amrex::ParticleReal m_koT1_cut_default
int nkoT1
int nKE
amrex::ParticleReal * m_koT1_grid
amrex::ParticleReal * m_KEgrid_eV
amrex::ParticleReal * m_kdsigdk
amrex::ParticleReal * m_sigma_total
```

## `PhotonCreationFunc`

- file: `Source/Particles/Collision/BinaryCollision/Bremsstrahlung/PhotonCreationFunc.H`
- kind: `class`
- public methods: 2; public members: 0

```cpp
PhotonCreationFunc () = default
PhotonCreationFunc (const std::string& collision_name, MultiParticleContainer const * mypc)
```

## `Executor`

- file: `Source/Particles/Collision/BinaryCollision/Coulomb/PairWiseCoulombCollisionFunc.H`
- kind: `struct`
- public methods: 0; public members: 6

Public members:

```cpp
amrex::ParticleReal m_CoulombLog
CumulativeScatteringModel m_scattering_model
bool m_computeSpeciesDensities = true
bool m_computeSpeciesTemperatures = false
bool m_need_product_data = false
bool m_isSameSpecies
```

## `PairWiseCoulombCollisionFunc`

- file: `Source/Particles/Collision/BinaryCollision/Coulomb/PairWiseCoulombCollisionFunc.H`
- kind: `class`, bases: `public CollisionFuncBase`
- public methods: 4; public members: 1

```cpp
PairWiseCoulombCollisionFunc () = default
PairWiseCoulombCollisionFunc (const std::string& collision_name, [[maybe_unused]] MultiParticleContainer const * const mypc, const bool isSameSpecies): m_isSameSpecies
[[nodiscard]] Executor const& executor (int const , amrex::MFIter const& ) const
[[nodiscard]] bool use_global_debye_length() const
```

Public members:

```cpp
struct Executor
```

## `DSMCFunc`

- file: `Source/Particles/Collision/BinaryCollision/DSMC/DSMCFunc.H`
- kind: `class`, bases: `public CollisionFuncBase`
- public methods: 4; public members: 1

```cpp
DSMCFunc () = default
DSMCFunc ( const std::string& collision_name, [[maybe_unused]] MultiParticleContainer const * mypc, bool isSameSpecies )
[[nodiscard]] Executor const& executor (int const , amrex::MFIter const& ) const
bool use_global_debye_length()
```

Public members:

```cpp
struct Executor
```

## `Executor`

- file: `Source/Particles/Collision/BinaryCollision/DSMC/DSMCFunc.H`
- kind: `struct`
- public methods: 0; public members: 6

Public members:

```cpp
int m_process_count
bool m_computeSpeciesDensities = false
bool m_computeSpeciesTemperatures = false
bool m_need_product_data = false
bool m_isSameSpecies = false
ScatteringProcess::Executor* m_scattering_processes_data
```

## `SplitAndScatterFunc`

- file: `Source/Particles/Collision/BinaryCollision/DSMC/SplitAndScatterFunc.H`
- kind: `class`
- public methods: 2; public members: 0

```cpp
SplitAndScatterFunc () = default
SplitAndScatterFunc (const std::string& collision_name, MultiParticleContainer const * mypc)
```

## `Executor`

- file: `Source/Particles/Collision/BinaryCollision/LinearBreitWheeler/LinearBreitWheelerCollisionFunc.H`
- kind: `struct`
- public methods: 0; public members: 7

Public members:

```cpp
amrex::ParticleReal m_event_multiplier
amrex::ParticleReal m_probability_threshold
amrex::ParticleReal m_probability_target_value
bool m_isSameSpecies
bool m_computeSpeciesDensities = false
bool m_computeSpeciesTemperatures = false
bool m_need_product_data = false
```

## `LinearBreitWheelerCollisionFunc`

- file: `Source/Particles/Collision/BinaryCollision/LinearBreitWheeler/LinearBreitWheelerCollisionFunc.H`
- kind: `class`, bases: `public CollisionFuncBase`
- public methods: 4; public members: 1

```cpp
LinearBreitWheelerCollisionFunc () = default
LinearBreitWheelerCollisionFunc ( const std::string& collision_name, MultiParticleContainer const * const , const bool isSameSpecies) : m_isSameSpecies
[[nodiscard]] Executor const& executor (int const , amrex::MFIter const& ) const
bool use_global_debye_length()
```

Public members:

```cpp
struct Executor
```

## `Executor`

- file: `Source/Particles/Collision/BinaryCollision/LinearCompton/LinearComptonCollisionFunc.H`
- kind: `struct`
- public methods: 0; public members: 7

Public members:

```cpp
amrex::ParticleReal m_event_multiplier
amrex::ParticleReal m_probability_threshold
amrex::ParticleReal m_probability_target_value
bool m_isSameSpecies
bool m_computeSpeciesDensities = false
bool m_computeSpeciesTemperatures = false
bool m_need_product_data = false
```

## `LinearComptonCollisionFunc`

- file: `Source/Particles/Collision/BinaryCollision/LinearCompton/LinearComptonCollisionFunc.H`
- kind: `class`, bases: `public CollisionFuncBase`
- public methods: 4; public members: 1

```cpp
LinearComptonCollisionFunc () = default
LinearComptonCollisionFunc ( const std::string& collision_name, MultiParticleContainer const * const , const bool isSameSpecies) : m_isSameSpecies(isSameSpecies)
[[nodiscard]] Executor const& executor (int const , amrex::MFIter const& ) const
bool use_global_debye_length()
```

Public members:

```cpp
struct Executor
```

## `Executor`

- file: `Source/Particles/Collision/BinaryCollision/NuclearFusion/NuclearFusionFunc.H`
- kind: `struct`
- public methods: 0; public members: 10

Public members:

```cpp
amrex::ParticleReal m_fusion_multiplier
amrex::ParticleReal m_probability_threshold
amrex::ParticleReal m_probability_target_value
NuclearFusionType m_fusion_type
bool m_computeSpeciesDensities = false
bool m_computeSpeciesTemperatures = false
bool m_need_product_data = false
bool m_isSameSpecies
bool m_create_products = true
amrex::Array4<amrex::Real> m_particle_production
```

## `NuclearFusionFunc`

- file: `Source/Particles/Collision/BinaryCollision/NuclearFusion/NuclearFusionFunc.H`
- kind: `class`, bases: `public CollisionFuncBase`
- public methods: 5; public members: 5

```cpp
NuclearFusionFunc () = default
NuclearFusionFunc (const std::string& collision_name, MultiParticleContainer const * const mypc, const bool isSameSpecies): m_fusion_multiplier
void AllocData () override
[[nodiscard]] Executor executor (int const lev, amrex::MFIter const& mfi) const
bool use_global_debye_length()
```

Public members:

```cpp
, m_probability_threshold
, m_probability_target_value
, m_fusion_type
, m_isSameSpecies
struct Executor
```

## `NoParticleCreationFunc`

- file: `Source/Particles/Collision/BinaryCollision/ParticleCreationFunc.H`
- kind: `class`
- public methods: 2; public members: 0

```cpp
NoParticleCreationFunc () = default
NoParticleCreationFunc (const std::string& , MultiParticleContainer const * const )
```

## `ParticleCreationFunc`

- file: `Source/Particles/Collision/BinaryCollision/ParticleCreationFunc.H`
- kind: `class`
- public methods: 2; public members: 0

```cpp
ParticleCreationFunc () = default
ParticleCreationFunc (const std::string& collision_name, MultiParticleContainer const * mypc)
```

## `IndependentPairHelper`

- file: `Source/Particles/Collision/BinaryCollision/ParticleShufflers.H`
- kind: `struct`
- public methods: 5; public members: 6

```cpp
IndependentPairHelper (int a_n_cells, T_index const* AMREX_RESTRICT a_cell_offsets_1, T_index const* AMREX_RESTRICT a_cell_offsets_2) : m_n_cells(a_n_cells), m_cell_offsets_1(a_cell_offsets_1), m_cell_offsets_2(a_cell_offsets_2)
void Initialize ()
void shuffle (T_index* AMREX_RESTRICT a_indices_1, T_index* AMREX_RESTRICT a_indices_2)
[[nodiscard]] int numIndependentPairs () const
[[nodiscard]] const T_index* collisionOffsetsPtr () const
```

Public members:

```cpp
int m_n_cells
T_index const* AMREX_RESTRICT m_cell_offsets_1
T_index const* AMREX_RESTRICT m_cell_offsets_2
amrex::Gpu::DeviceVector<T_index> m_n_ind_pairs_in_each_cell
amrex::Gpu::DeviceVector<T_index> m_coll_offsets
int m_n_independent_pairs
```

## `CollisionBase`

- file: `Source/Particles/Collision/CollisionBase.H`
- kind: `class`
- public methods: 13; public members: 0

```cpp
explicit CollisionBase (const std::string& collision_name)
virtual void AllocData ()
virtual void doCollisions (amrex::Real , amrex::Real , MultiParticleContainer* )
CollisionBase(CollisionBase const &) = delete
CollisionBase(CollisionBase &&) = delete
CollisionBase & operator=(CollisionBase const &) = delete
CollisionBase & operator=(CollisionBase const &&) = delete
virtual ~CollisionBase() = default
void BackwardCompatibility ()
[[nodiscard]] int get_ndt() const
[[nodiscard]] int get_start_step() const
[[nodiscard]] CollisionSteppingMode get_collision_stepping_mode() const
[[nodiscard]] bool use_global_debye_length() const
```

## `CollisionFuncBase`

- file: `Source/Particles/Collision/CollisionFuncBase.H`
- kind: `class`
- public methods: 7; public members: 0

```cpp
CollisionFuncBase () = default
virtual ~CollisionFuncBase () = default
CollisionFuncBase (const CollisionFuncBase&) = default
CollisionFuncBase& operator= (const CollisionFuncBase&) = default
CollisionFuncBase (CollisionFuncBase&&) noexcept = default
CollisionFuncBase& operator= (CollisionFuncBase&&) noexcept = default
virtual void AllocData ()
```

## `CollisionHandler`

- file: `Source/Particles/Collision/CollisionHandler.H`
- kind: `class`
- public methods: 3; public members: 0

```cpp
explicit CollisionHandler (const MultiParticleContainer* mypc)
void AllocData ()
void doCollisions (int step, amrex::Real cur_time, amrex::Real dt, MultiParticleContainer* mypc)
```

## `HybridResistiveDrag`

- file: `Source/Particles/Collision/HybridResistiveDrag/HybridResistiveDrag.H`
- kind: `class`, bases: `public CollisionBase`
- public methods: 7; public members: 0

```cpp
explicit HybridResistiveDrag (const std::string& collision_name)
~HybridResistiveDrag () override = default
HybridResistiveDrag ( HybridResistiveDrag const &) = delete
HybridResistiveDrag& operator= ( HybridResistiveDrag const & ) = delete
HybridResistiveDrag ( HybridResistiveDrag&& ) = delete
HybridResistiveDrag& operator= ( HybridResistiveDrag&& ) = delete
void doCollisions (amrex::Real cur_time, amrex::Real dt, MultiParticleContainer* mypc) override
```

## `InverseBremsstrahlung`

- file: `Source/Particles/Collision/InverseBremsstrahlung/InverseBremsstrahlung.H`
- kind: `class`, bases: `public CollisionBase`
- public methods: 8; public members: 0

```cpp
InverseBremsstrahlung (std::string const& collision_name, MultiParticleContainer const* mypc)
~InverseBremsstrahlung () override = default
InverseBremsstrahlung ( InverseBremsstrahlung const &) = delete
InverseBremsstrahlung& operator= ( InverseBremsstrahlung const & ) = delete
InverseBremsstrahlung ( InverseBremsstrahlung&& ) = delete
InverseBremsstrahlung& operator= ( InverseBremsstrahlung&& ) = delete
void doCollisions (amrex::Real cur_time, amrex::Real dt, MultiParticleContainer* mypc) override
void doInverseBremsstrahlungWithinTile (amrex::Real dt, int lev, amrex::MFIter const& mfi, WarpXParticleContainer& photons, WarpXParticleContainer& electrons) const
```

## `PulsedDecay`

- file: `Source/Particles/Collision/PulsedDecay/PulsedDecay.H`
- kind: `class`, bases: `public CollisionBase`
- public methods: 7; public members: 0

```cpp
PulsedDecay (std::string const& collision_name, MultiParticleContainer const * mypc)
~PulsedDecay () override = default
PulsedDecay ( PulsedDecay const &) = delete
PulsedDecay& operator= ( PulsedDecay const & ) = delete
PulsedDecay ( PulsedDecay&& ) = delete
PulsedDecay& operator= ( PulsedDecay&& ) = delete
void doCollisions (amrex::Real cur_time, amrex::Real dt, MultiParticleContainer* mypc) override
```

## `Executor`

- file: `Source/Particles/Collision/ScatteringProcess.H`
- kind: `struct`
- public methods: 1; public members: 8

```cpp
[[nodiscard]] AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE amrex::ParticleReal getCrossSection (amrex::ParticleReal E_coll) const
```

Public members:

```cpp
amrex::ParticleReal* m_energies_data = nullptr
amrex::ParticleReal* m_sigmas_data = nullptr
int m_grid_size = 0
amrex::ParticleReal m_energy_lo, m_energy_hi, m_sigma_lo, m_sigma_hi, m_dE
amrex::ParticleReal m_energy_penalty
ScatteringProcessType m_type
ScatteringAngleModel m_scattering_angle_model
bool m_produces_products = false
```

## `ScatteringProcess`

- file: `Source/Particles/Collision/ScatteringProcess.H`
- kind: `class`
- public methods: 17; public members: 1

```cpp
ScatteringProcess ( const std::string& scattering_process, const std::string& cross_section_file, amrex::ParticleReal energy, ScatteringAngleModel scattering_angle_model )
ScatteringProcess ( const std::string& scattering_process, const InputVector&& energies, const InputVector&& sigmas, amrex::ParticleReal energy, ScatteringAngleModel scattering_angle_model )
~ScatteringProcess() = default
ScatteringProcess (ScatteringProcess const&) = delete
ScatteringProcess& operator= (ScatteringProcess const&) = delete
ScatteringProcess (ScatteringProcess &&) = default
ScatteringProcess& operator= (ScatteringProcess &&) = default
static void readCrossSectionFile ( const std::string& cross_section_file, amrex::Vector<amrex::ParticleReal>& energies, amrex::Gpu::HostVector<amrex::ParticleReal>& sigmas )
static void sanityCheckEnergyGrid ( const amrex::Vector<amrex::ParticleReal>& energies )
[[nodiscard]] Executor const& executor () const
[[nodiscard]] amrex::ParticleReal getCrossSection (amrex::ParticleReal E_coll) const
[[nodiscard]] amrex::ParticleReal getEnergyPenalty () const
[[nodiscard]] amrex::ParticleReal getMinEnergyInput () const
[[nodiscard]] amrex::ParticleReal getMaxEnergyInput () const
[[nodiscard]] amrex::ParticleReal getEnergyInputStep () const
[[nodiscard]] ScatteringProcessType type () const
static ScatteringProcessType parseProcessType(const std::string& process)
```

Public members:

```cpp
struct Executor
```

## `VarianceAccumulationBuffer`

- file: `Source/Particles/Deposition/VarianceAccumulationBuffer.H`
- kind: `class`
- public methods: 5; public members: 0

```cpp
VarianceAccumulationBuffer (ablastr::fields::MultiLevelVectorField const& T_vf, std::string const& species_name )
void reset ()
void ConvertVarianceToTemperatureAndFilter (ablastr::fields::MultiLevelVectorField const& var_vf, amrex::Real normalization_factor, bool apply_filter)
amrex::MultiFab* get(const std::string& arr, ablastr::fields::Direction dir, int lev)
amrex::iMultiFab* get_n(ablastr::fields::Direction dir, int lev)
```

## `IonizationFilterFunc`

- file: `Source/Particles/ElementaryProcess/Ionization.H`
- kind: `struct`
- public methods: 1; public members: 34

```cpp
IonizationFilterFunc (const WarpXParIter& a_pti, int lev, amrex::IntVect ngEB, amrex::FArrayBox const& exfab, amrex::FArrayBox const& eyfab, amrex::FArrayBox const& ezfab, amrex::FArrayBox const& bxfab, amrex::FArrayBox const& byfab, amrex::FArrayBox const& bzfab, amrex::Vector<amrex::ParticleReal>& E_external_particle, amrex::Vector<amrex::ParticleReal>& B_external_particle, const amrex::Real* AMREX_RESTRICT a_ionization_energies, const amrex::Real* AMREX_RESTRICT a_adk_prefactor, const amrex::Real* AMREX_RESTRICT a_adk_exp_prefactor, const amrex::Real* AMREX_RESTRICT a_adk_power, const amrex::Real* AMREX_RESTRICT a_adk_correction_factors, int a_comp, int a_atomic_number, int a_do_adk_correction, int a_offset = 0) noexcept
```

Public members:

```cpp
const amrex::Real* AMREX_RESTRICT m_ionization_energies
const amrex::Real* AMREX_RESTRICT m_adk_prefactor
const amrex::Real* AMREX_RESTRICT m_adk_exp_prefactor
const amrex::Real* AMREX_RESTRICT m_adk_power
const amrex::Real* AMREX_RESTRICT m_adk_correction_factors
int comp
int m_atomic_number
int m_do_adk_correction = 0
GetParticlePosition<PIdx> m_get_position
GetExternalEBField m_get_externalEB
amrex::ParticleReal m_Ex_external_particle
amrex::ParticleReal m_Ey_external_particle
amrex::ParticleReal m_Ez_external_particle
amrex::ParticleReal m_Bx_external_particle
amrex::ParticleReal m_By_external_particle
amrex::ParticleReal m_Bz_external_particle
amrex::Array4<const amrex::Real> m_ex_arr
amrex::Array4<const amrex::Real> m_ey_arr
amrex::Array4<const amrex::Real> m_ez_arr
amrex::Array4<const amrex::Real> m_bx_arr
amrex::Array4<const amrex::Real> m_by_arr
amrex::Array4<const amrex::Real> m_bz_arr
amrex::IndexType m_ex_type
amrex::IndexType m_ey_type
amrex::IndexType m_ez_type
amrex::IndexType m_bx_type
amrex::IndexType m_by_type
amrex::IndexType m_bz_type
amrex::XDim3 m_dinv
amrex::XDim3 m_xyzmin
bool m_galerkin_interpolation
int m_nox
int m_n_rz_azimuthal_modes
amrex::Dim3 m_lo
```

## `IonizationTransformFunc`

- file: `Source/Particles/ElementaryProcess/Ionization.H`
- kind: `struct`
- public methods: 0; public members: 0

## `BreitWheelerEngine`

- file: `Source/Particles/ElementaryProcess/QEDInternals/BreitWheelerEngineWrapper.H`
- kind: `class`
- public methods: 11; public members: 0

```cpp
BreitWheelerEngine () = default
[[nodiscard]] BreitWheelerGetOpticalDepth build_optical_depth_functor () const
[[nodiscard]] BreitWheelerEvolveOpticalDepth build_evolve_functor () const
[[nodiscard]] BreitWheelerGeneratePairs build_pair_functor () const
[[nodiscard]] bool are_lookup_tables_initialized () const
[[nodiscard]] std::vector<char> export_lookup_tables_data () const
bool init_lookup_tables_from_raw_data ( const std::vector<char>& raw_data, amrex::ParticleReal bw_minimum_chi_phot)
void init_builtin_tables(amrex::ParticleReal bw_minimum_chi_phot)
void compute_lookup_tables (PicsarBreitWheelerCtrl ctrl, amrex::ParticleReal bw_minimum_chi_phot)
[[nodiscard]] PicsarBreitWheelerCtrl get_default_ctrl() const
[[nodiscard]] amrex::ParticleReal get_minimum_chi_phot() const
```

## `BreitWheelerEvolveOpticalDepth`

- file: `Source/Particles/ElementaryProcess/QEDInternals/BreitWheelerEngineWrapper.H`
- kind: `class`
- public methods: 2; public members: 1

```cpp
BreitWheelerEvolveOpticalDepth () = default
BreitWheelerEvolveOpticalDepth ( const BW_dndt_table_view table_view, const amrex::ParticleReal bw_minimum_chi_phot): m_table_view
```

Public members:

```cpp
, m_bw_minimum_chi_phot
```

## `BreitWheelerGeneratePairs`

- file: `Source/Particles/ElementaryProcess/QEDInternals/BreitWheelerEngineWrapper.H`
- kind: `class`
- public methods: 2; public members: 0

```cpp
BreitWheelerGeneratePairs () = default
explicit BreitWheelerGeneratePairs (const BW_pair_prod_table_view& table_view): m_table_view
```

## `BreitWheelerGetOpticalDepth`

- file: `Source/Particles/ElementaryProcess/QEDInternals/BreitWheelerEngineWrapper.H`
- kind: `class`
- public methods: 1; public members: 0

```cpp
BreitWheelerGetOpticalDepth () = default
```

## `PicsarBreitWheelerCtrl`

- file: `Source/Particles/ElementaryProcess/QEDInternals/BreitWheelerEngineWrapper.H`
- kind: `struct`
- public methods: 0; public members: 2

Public members:

```cpp
BW_dndt_table_params dndt_params
BW_pair_prod_table_params pair_prod_params
```

## `PicsarQedVector`

- file: `Source/Particles/ElementaryProcess/QEDInternals/QedWrapperCommons.H`
- kind: `class`, bases: `public std::vector<Real>`
- public methods: 3; public members: 0

```cpp
PicsarQedVector(Args&&... args) : HV(std::forward<Args>(args)...)
void pxr_sync()
const Real* data() const
```

## `PicsarQuantumSyncCtrl`

- file: `Source/Particles/ElementaryProcess/QEDInternals/QuantumSyncEngineWrapper.H`
- kind: `struct`
- public methods: 0; public members: 2

Public members:

```cpp
QS_dndt_table_params dndt_params
QS_phot_em_table_params phot_em_params
```

## `QuantumSynchrotronEngine`

- file: `Source/Particles/ElementaryProcess/QEDInternals/QuantumSyncEngineWrapper.H`
- kind: `class`
- public methods: 11; public members: 0

```cpp
QuantumSynchrotronEngine () = default
[[nodiscard]] QuantumSynchrotronGetOpticalDepth build_optical_depth_functor ()
[[nodiscard]] QuantumSynchrotronEvolveOpticalDepth build_evolve_functor ()
[[nodiscard]] QuantumSynchrotronPhotonEmission build_phot_em_functor ()
[[nodiscard]] bool are_lookup_tables_initialized () const
[[nodiscard]] std::vector<char> export_lookup_tables_data () const
bool init_lookup_tables_from_raw_data (const std::vector<char>& raw_data, amrex::ParticleReal qs_minimum_chi_part)
void init_builtin_tables(amrex::ParticleReal qs_minimum_chi_part)
void compute_lookup_tables (PicsarQuantumSyncCtrl ctrl, amrex::ParticleReal qs_minimum_chi_part)
[[nodiscard]] PicsarQuantumSyncCtrl get_default_ctrl() const
[[nodiscard]] amrex::ParticleReal get_minimum_chi_part() const
```

## `QuantumSynchrotronEvolveOpticalDepth`

- file: `Source/Particles/ElementaryProcess/QEDInternals/QuantumSyncEngineWrapper.H`
- kind: `class`
- public methods: 2; public members: 1

```cpp
QuantumSynchrotronEvolveOpticalDepth () = default
QuantumSynchrotronEvolveOpticalDepth ( const QS_dndt_table_view table_view, const amrex::ParticleReal qs_minimum_chi_part): m_table_view
```

Public members:

```cpp
, m_qs_minimum_chi_part
```

## `QuantumSynchrotronGetOpticalDepth`

- file: `Source/Particles/ElementaryProcess/QEDInternals/QuantumSyncEngineWrapper.H`
- kind: `class`
- public methods: 1; public members: 0

```cpp
QuantumSynchrotronGetOpticalDepth () = default
```

## `QuantumSynchrotronPhotonEmission`

- file: `Source/Particles/ElementaryProcess/QEDInternals/QuantumSyncEngineWrapper.H`
- kind: `class`
- public methods: 2; public members: 0

```cpp
QuantumSynchrotronPhotonEmission () = default
explicit QuantumSynchrotronPhotonEmission ( const QS_phot_em_table_view table_view): m_table_view
```

## `PairGenerationFilterFunc`

- file: `Source/Particles/ElementaryProcess/QEDPairGeneration.H`
- kind: `class`
- public methods: 1; public members: 0

```cpp
explicit PairGenerationFilterFunc(int const opt_depth_runtime_comp) : m_opt_depth_runtime_comp(opt_depth_runtime_comp)
```

## `PairGenerationTransformFunc`

- file: `Source/Particles/ElementaryProcess/QEDPairGeneration.H`
- kind: `class`
- public methods: 1; public members: 0

```cpp
PairGenerationTransformFunc(BreitWheelerGeneratePairs generate_functor, const WarpXParIter& a_pti, int lev, amrex::IntVect ngEB, amrex::FArrayBox const& exfab, amrex::FArrayBox const& eyfab, amrex::FArrayBox const& ezfab, amrex::FArrayBox const& bxfab, amrex::FArrayBox const& byfab, amrex::FArrayBox const& bzfab, amrex::Vector<amrex::ParticleReal>& E_external_particle, amrex::Vector<amrex::ParticleReal>& B_external_particle, int a_offset = 0)
```

## `PhotonEmissionFilterFunc`

- file: `Source/Particles/ElementaryProcess/QEDPhotonEmission.H`
- kind: `class`
- public methods: 1; public members: 0

```cpp
explicit PhotonEmissionFilterFunc(int const opt_depth_runtime_comp) : m_opt_depth_runtime_comp(opt_depth_runtime_comp)
```

## `PhotonEmissionTransformFunc`

- file: `Source/Particles/ElementaryProcess/QEDPhotonEmission.H`
- kind: `class`
- public methods: 1; public members: 0

```cpp
PhotonEmissionTransformFunc ( QuantumSynchrotronGetOpticalDepth opt_depth_functor, int opt_depth_runtime_comp, QuantumSynchrotronPhotonEmission emission_functor, const WarpXParIter& a_pti, int lev, amrex::IntVect ngEB, amrex::FArrayBox const& exfab, amrex::FArrayBox const& eyfab, amrex::FArrayBox const& ezfab, amrex::FArrayBox const& bxfab, amrex::FArrayBox const& byfab, amrex::FArrayBox const& bzfab, amrex::Vector<amrex::ParticleReal>& E_external_particle, amrex::Vector<amrex::ParticleReal>& B_external_particle, int a_offset = 0)
```

## `SchwingerFilterFunc`

- file: `Source/Particles/ElementaryProcess/QEDSchwingerProcess.H`
- kind: `struct`
- public methods: 0; public members: 3

Public members:

```cpp
int m_threshold_poisson_gaussian
amrex::Real m_dV
amrex::Real m_dt
```

## `SchwingerTransformFunc`

- file: `Source/Particles/ElementaryProcess/QEDSchwingerProcess.H`
- kind: `struct`
- public methods: 0; public members: 2

Public members:

```cpp
amrex::Real m_y_size
int m_weight_index
```

## `ExternalParticleFields`

- file: `Source/Particles/ExternalParticleFields.H`
- kind: `class`
- public methods: 1; public members: 4

```cpp
void ReadParameters()
```

Public members:

```cpp
std::vector<ParticleFieldMetaData> m_E_field_metadata
std::vector<ParticleFieldMetaData> m_B_field_metadata
int m_nEfields
int m_nBfields
```

## `ParticleFieldMetaData`

- file: `Source/Particles/ExternalParticleFields.H`
- kind: `struct`
- public methods: 0; public members: 4

Public members:

```cpp
std::string path
std::string time_function = "1.0"
std::unique_ptr<amrex::Parser> time_parser
amrex::ParserExecutor<1> time_executor
```

## `GeometryFilter`

- file: `Source/Particles/Filter/FilterFunctors.H`
- kind: `struct`
- public methods: 1; public members: 0

```cpp
GeometryFilter(bool a_is_active, amrex::RealBox a_domain) : m_is_active(a_is_active), m_domain(a_domain)
```

## `ParserFilter`

- file: `Source/Particles/Filter/FilterFunctors.H`
- kind: `struct`
- public methods: 1; public members: 7

```cpp
ParserFilter(bool a_is_active, amrex::ParserExecutor<7> const& a_filter_parser, const amrex::ParticleReal a_mass, const amrex::Real time): m_is_active
```

Public members:

```cpp
, m_function_partparser
, m_mass
, m_t
amrex::ParserExecutor<7> m_function_partparser
amrex::ParticleReal m_mass
amrex::Real m_t
InputUnits m_units
```

## `RandomFilter`

- file: `Source/Particles/Filter/FilterFunctors.H`
- kind: `struct`
- public methods: 1; public members: 0

```cpp
RandomFilter(bool a_is_active, amrex::Real a_fraction) : m_is_active(a_is_active), m_fraction(a_fraction)
```

## `UniformFilter`

- file: `Source/Particles/Filter/FilterFunctors.H`
- kind: `struct`
- public methods: 1; public members: 0

```cpp
UniformFilter(bool a_is_active, int a_stride) : m_is_active(a_is_active), m_stride(a_stride)
```

## `GetExternalEBField`

- file: `Source/Particles/Gather/GetExternalFields.H`
- kind: `struct`
- public methods: 3; public members: 24

```cpp
GetExternalEBField () = default
explicit GetExternalEBField (const WarpXParIter& a_pti, long a_offset = 0) noexcept
[[nodiscard]] AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE bool isNoOp () const
```

Public members:

```cpp
enum ExternalFieldInitType
ExternalFieldInitType m_Etype
ExternalFieldInitType m_Btype
amrex::ParticleReal m_gamma_boost
amrex::ParticleReal m_uz_boost
amrex::ParserExecutor<4> m_Exfield_partparser
amrex::ParserExecutor<4> m_Eyfield_partparser
amrex::ParserExecutor<4> m_Ezfield_partparser
amrex::ParserExecutor<4> m_Bxfield_partparser
amrex::ParserExecutor<4> m_Byfield_partparser
amrex::ParserExecutor<4> m_Bzfield_partparser
GetParticlePosition<PIdx> m_get_position
amrex::Real m_time
amrex::ParticleReal m_repeated_plasma_lens_period
const amrex::ParticleReal* AMREX_RESTRICT m_repeated_plasma_lens_starts = nullptr
const amrex::ParticleReal* AMREX_RESTRICT m_repeated_plasma_lens_lengths = nullptr
const amrex::ParticleReal* AMREX_RESTRICT m_repeated_plasma_lens_strengths_E = nullptr
const amrex::ParticleReal* AMREX_RESTRICT m_repeated_plasma_lens_strengths_B = nullptr
int m_n_lenses
amrex::Real m_dt
const amrex::ParticleReal* AMREX_RESTRICT m_ux = nullptr
const amrex::ParticleReal* AMREX_RESTRICT m_uy = nullptr
const amrex::ParticleReal* AMREX_RESTRICT m_uz = nullptr
LatticeElementFinderDevice d_lattice_element_finder
```

## `ScaleFields`

- file: `Source/Particles/Gather/ScaleFields.H`
- kind: `struct`
- public methods: 2; public members: 5

```cpp
explicit ScaleFields(bool do_scale) noexcept : m_do_scale(do_scale)
ScaleFields (bool do_scale, amrex::Real dt, amrex::Real z_plane_previous, amrex::Real vz_ave_boosted, amrex::Real v_boost) noexcept : m_do_scale(do_scale), m_dt(dt), m_z_plane_previous(z_plane_previous), m_vz_ave_boosted(vz_ave_boosted), m_v_boost(v_boost)
```

Public members:

```cpp
bool m_do_scale
amrex::Real m_dt
amrex::Real m_z_plane_previous
amrex::Real m_vz_ave_boosted
amrex::Real m_v_boost
```

## `LaserParticleContainer`

- file: `Source/Particles/LaserParticleContainer.H`
- kind: `class`, bases: `public WarpXParticleContainer`
- public methods: 15; public members: 0

```cpp
LaserParticleContainer (amrex::AmrCore* amr_core, int ispecies, const std::string& name)
~LaserParticleContainer () override = default
LaserParticleContainer ( LaserParticleContainer const &) = delete
LaserParticleContainer& operator= ( LaserParticleContainer const & ) = delete
LaserParticleContainer ( LaserParticleContainer&& ) = default
LaserParticleContainer& operator= ( LaserParticleContainer&& ) = default
void InitData () final
void DefaultInitializeRuntimeAttributes ( typename ContainerLike<amrex::PolymorphicArenaAllocator>::ParticleTileType& , int , int ) final
void ReadHeader (std::istream& is) final
void WriteHeader (std::ostream& os) const final
void Evolve (ablastr::fields::MultiFabRegister& fields, int lev, const std::string& current_fp_string, amrex::Real t, amrex::Real dt, SubcyclingHalf subcycling_half=SubcyclingHalf::None, bool skip_deposition=false, PositionPushType position_push_type=PositionPushType::Full, MomentumPushType momentum_push_type=MomentumPushType::Full, ImplicitOptions const * implicit_options = nullptr) final
void PushP (int lev, amrex::Real dt, const amrex::MultiFab& , const amrex::MultiFab& , const amrex::MultiFab& , const amrex::MultiFab& , const amrex::MultiFab& , const amrex::MultiFab& , MomentumPushType momentum_push_type = MomentumPushType::Full, std::optional<FieldGatherOrders> gather_orders = std::nullopt) final
void PostRestart () final
void calculate_laser_plane_coordinates (const WarpXParIter& pti, int np, amrex::Real * AMREX_RESTRICT pplane_Xp, amrex::Real * AMREX_RESTRICT pplane_Yp)
void update_laser_particle (WarpXParIter& pti, int np, amrex::ParticleReal * AMREX_RESTRICT puxp, amrex::ParticleReal * AMREX_RESTRICT puyp, amrex::ParticleReal * AMREX_RESTRICT puzp, amrex::ParticleReal const * AMREX_RESTRICT pwp, amrex::Real const * AMREX_RESTRICT amplitude, amrex::Real dt, PushType push_type=PushType::Explicit)
```

## `MultiParticleContainer`

- file: `Source/Particles/MultiParticleContainer.H`
- kind: `class`
- public methods: 67; public members: 18

```cpp
explicit MultiParticleContainer (amrex::AmrCore* amr_core)
~MultiParticleContainer() = default
MultiParticleContainer (MultiParticleContainer const &) = delete
MultiParticleContainer& operator= (MultiParticleContainer const & ) = delete
MultiParticleContainer(MultiParticleContainer&& ) = default
MultiParticleContainer& operator=(MultiParticleContainer&& ) = default
[[nodiscard]] WarpXParticleContainer& GetParticleContainer (int index) const
[[nodiscard]] WarpXParticleContainer* GetParticleContainerPtr (int index) const
[[nodiscard]] WarpXParticleContainer& GetParticleContainerFromName (const std::string& name) const
std::array<amrex::ParticleReal, 3> meanParticleVelocity(int index)
amrex::ParticleReal maxParticleDtInv()
void TransformMomentumToCurvilinear (bool forward)
void AllocData ()
void InitData ()
void InitMultiPhysicsModules ()
void Evolve ( ablastr::fields::MultiFabRegister& fields, int lev, std::string const& current_fp_string, amrex::Real t, amrex::Real dt, SubcyclingHalf subcycling_half=SubcyclingHalf::None, bool skip_deposition=false, PositionPushType position_push_type=PositionPushType::Full, MomentumPushType momentum_push_type=MomentumPushType::Full, ImplicitOptions const * implicit_options = nullptr )
void DepositMassMatrices (ablastr::fields::MultiFabRegister& fields, int lev, amrex::Real dt, ImplicitOptions const& implicit_options)
void PushX (amrex::Real dt)
void PushP (int lev, amrex::Real dt, const amrex::MultiFab& Ex, const amrex::MultiFab& Ey, const amrex::MultiFab& Ez, const amrex::MultiFab& Bx, const amrex::MultiFab& By, const amrex::MultiFab& Bz, MomentumPushType momentum_push_type, std::optional<FieldGatherOrders> gather_orders = std::nullopt)
std::unique_ptr<amrex::MultiFab> GetZeroChargeDensity(int lev)
void DepositCharge (const ablastr::fields::MultiLevelScalarField& rho, amrex::Real relative_time)
void DepositCurrent (ablastr::fields::MultiLevelVectorField const& J, amrex::Real dt, amrex::Real relative_time, PushType push_type = PushType::Explicit, std::optional<int> particle_shape = std::nullopt)
void DepositTemperatures (ablastr::fields::MultiFabRegister & fields, amrex::Real relative_time)
std::unique_ptr<amrex::MultiFab> GetChargeDensityOnLevel (int lev, bool local, bool finalize)
std::unique_ptr<amrex::MultiFab> GetGlobalPlasmaFrequency (int lev)
void GenerateGlobalDebyeLength ()
void CalculateNuei (std::string const & electron_species_name)
void doFieldIonization (int lev, const amrex::MultiFab& Ex, const amrex::MultiFab& Ey, const amrex::MultiFab& Ez, const amrex::MultiFab& Bx, const amrex::MultiFab& By, const amrex::MultiFab& Bz)
void doCollisions (int step, amrex::Real cur_time, amrex::Real dt)
void doResampling (const amrex::Vector<amrex::Geometry>& geom, int timestep, bool verbose)
void doQEDSchwinger ()
[[nodiscard]] amrex::Box ComputeSchwingerGlobalBox () const
void Restart (const std::string& dir)
void PostRestart ()
void ReadHeader (std::istream& is)
void WriteHeader (std::ostream& os) const
void SortParticlesByBin ( const amrex::IntVect& bin_size, bool sort_particles_for_deposition, const amrex::IntVect& sort_idx_type)
void Redistribute ()
void defineAllParticleTiles ()
void deleteInvalidParticles ()
void RedistributeLocal (const amrex::IntVect& max_cells_travelled)
void ApplyBoundaryConditions ()
[[nodiscard]] amrex::Vector<amrex::Long> GetZeroParticlesInGrid(int lev) const
[[nodiscard]] amrex::Vector<amrex::Long> NumberOfParticlesInGrid(int lev) const
void Increment (amrex::MultiFab& mf, int lev)
void SetParticleBoxArray (int lev, amrex::BoxArray& new_ba)
void SetParticleDistributionMap (int lev, amrex::DistributionMapping& new_dm)
[[nodiscard]] int nSpecies () const
[[nodiscard]] int nLasers () const
[[nodiscard]] int nContainers () const
void SetDoBackTransformedParticles (bool do_back_transformed_particles)
void SetDoBackTransformedParticles (const std::string& species_name, bool do_back_transformed_particles)
[[nodiscard]] bool getDoBackTransformedParticles () const
[[nodiscard]] int nSpeciesDepositOnMainGrid () const
[[nodiscard]] int nSpeciesGatherFromMainGrid() const
void ContinuousInjection(const amrex::RealBox& injection_box) const
void UpdateAntennaPosition(amrex::Real dt) const
[[nodiscard]] int doContinuousInjection() const
void ContinuousFluxInjection(amrex::Real t, amrex::Real dt) const
[[nodiscard]] std::vector<std::string> GetSpeciesNames() const
[[nodiscard]] std::vector<std::string> GetLasersNames() const
[[nodiscard]] std::vector<std::string> GetSpeciesAndLasersNames() const
void ScrapeParticlesAtEB (ablastr::fields::MultiLevelScalarField const& distance_to_eb)
void doQedEvents (int lev, const amrex::MultiFab& Ex, const amrex::MultiFab& Ey, const amrex::MultiFab& Ez, const amrex::MultiFab& Bx, const amrex::MultiFab& By, const amrex::MultiFab& Bz)
[[nodiscard]] int getSpeciesID (const std::string& product_str) const
amrex::Vector<std::unique_ptr<WarpXParticleContainer>>::iterator begin()
amrex::Vector<std::unique_ptr<WarpXParticleContainer>>::iterator end()
```

Public members:

```cpp
std::string m_B_ext_particle_s = "none"
std::string m_E_ext_particle_s = "none"
std::unique_ptr<amrex::Parser> m_Bx_particle_parser
std::unique_ptr<amrex::Parser> m_By_particle_parser
std::unique_ptr<amrex::Parser> m_Bz_particle_parser
std::unique_ptr<amrex::Parser> m_Ex_particle_parser
std::unique_ptr<amrex::Parser> m_Ey_particle_parser
std::unique_ptr<amrex::Parser> m_Ez_particle_parser
ExternalParticleFields m_external_particle_fields_metadata
amrex::ParticleReal m_repeated_plasma_lens_period
amrex::Vector<amrex::ParticleReal> h_repeated_plasma_lens_starts
amrex::Vector<amrex::ParticleReal> h_repeated_plasma_lens_lengths
amrex::Vector<amrex::ParticleReal> h_repeated_plasma_lens_strengths_E
amrex::Vector<amrex::ParticleReal> h_repeated_plasma_lens_strengths_B
amrex::Gpu::DeviceVector<amrex::ParticleReal> d_repeated_plasma_lens_starts
amrex::Gpu::DeviceVector<amrex::ParticleReal> d_repeated_plasma_lens_lengths
amrex::Gpu::DeviceVector<amrex::ParticleReal> d_repeated_plasma_lens_strengths_E
amrex::Gpu::DeviceVector<amrex::ParticleReal> d_repeated_plasma_lens_strengths_B
```

## `ParticleBoundaries`

- file: `Source/Particles/ParticleBoundaries.H`
- kind: `struct`
- public methods: 9; public members: 14

```cpp
ParticleBoundaries () noexcept
void Set_reflect_all_velocities (bool flag)
void SetAll (ParticleBoundaryType bc)
void SetThermalVelocity (amrex::Real u_th)
void SetBoundsX (ParticleBoundaryType bc_lo, ParticleBoundaryType bc_hi)
void SetBoundsY (ParticleBoundaryType bc_lo, ParticleBoundaryType bc_hi)
void SetBoundsZ (ParticleBoundaryType bc_lo, ParticleBoundaryType bc_hi)
[[nodiscard]] bool CheckAll (ParticleBoundaryType bc) const
void BuildReflectionModelParsers ()
```

Public members:

```cpp
std::string reflection_model_xlo_str = "0.0"
std::string reflection_model_xhi_str = "0.0"
std::string reflection_model_ylo_str = "0.0"
std::string reflection_model_yhi_str = "0.0"
std::string reflection_model_zlo_str = "0.0"
std::string reflection_model_zhi_str = "0.0"
std::unique_ptr<amrex::Parser> reflection_model_xlo_parser
std::unique_ptr<amrex::Parser> reflection_model_xhi_parser
std::unique_ptr<amrex::Parser> reflection_model_ylo_parser
std::unique_ptr<amrex::Parser> reflection_model_yhi_parser
std::unique_ptr<amrex::Parser> reflection_model_zlo_parser
std::unique_ptr<amrex::Parser> reflection_model_zhi_parser
struct ParticleBoundariesData
ParticleBoundariesData data
```

## `ParticleBoundariesData`

- file: `Source/Particles/ParticleBoundaries.H`
- kind: `struct`
- public methods: 0; public members: 14

Public members:

```cpp
ParticleBoundaryType xmin_bc
ParticleBoundaryType xmax_bc
ParticleBoundaryType ymin_bc
ParticleBoundaryType ymax_bc
ParticleBoundaryType zmin_bc
ParticleBoundaryType zmax_bc
amrex::Real m_uth = 0.
amrex::ParserExecutor<1> reflection_model_xlo
amrex::ParserExecutor<1> reflection_model_xhi
amrex::ParserExecutor<1> reflection_model_ylo
amrex::ParserExecutor<1> reflection_model_yhi
amrex::ParserExecutor<1> reflection_model_zlo
amrex::ParserExecutor<1> reflection_model_zhi
bool reflect_all_velocities
```

## `ParticleBoundaryBuffer`

- file: `Source/Particles/ParticleBoundaryBuffer.H`
- kind: `class`
- public methods: 20; public members: 0

```cpp
ParticleBoundaryBuffer ()
~ParticleBoundaryBuffer() = default
ParticleBoundaryBuffer ( const ParticleBoundaryBuffer &) = delete
ParticleBoundaryBuffer& operator= ( const ParticleBoundaryBuffer & ) = delete
ParticleBoundaryBuffer ( ParticleBoundaryBuffer && ) = default
ParticleBoundaryBuffer& operator= ( ParticleBoundaryBuffer && ) = default
int numSpecies() const
const std::vector<std::string>& getSpeciesNames() const
void gatherParticlesFromDomainBoundaries (MultiParticleContainer& mypc, amrex::Real cur_time)
void gatherParticlesFromEmbeddedBoundaries ( MultiParticleContainer& mypc, ablastr::fields::MultiLevelScalarField const& distance_to_eb, amrex::Real cur_time )
void redistribute ()
void clearParticles ()
void clearParticles (int i)
void printNumParticles () const
int getNumParticlesInContainer(const std::string& species_name, int boundary, bool local)
WarpXParticleContainer::Base& getParticleBuffer(const std::string& species_name, int boundary)
WarpXParticleContainer::Base* getParticleBufferPointer(const std::string& species_name, int boundary)
static constexpr int numBoundaries ()
bool isDefinedForAnySpecies (int const ibuffer)
std::string boundaryName (int const ibuffer)
```

## `PlasmaParserHelper`

- file: `Source/Particles/ParticleCreation/AddPlasmaUtilities.H`
- kind: `struct`
- public methods: 5; public members: 7

```cpp
PlasmaParserHelper (SoAType& a_soa, std::size_t old_size, const std::vector<std::string>& a_user_int_attribs, const std::vector<std::string>& a_user_real_attribs, const PlasmaParserWrapper& wrapper) : m_wrapper_ptr(&wrapper)
int** getUserIntDataPtrs ()
amrex::ParticleReal** getUserRealDataPtrs ()
[[nodiscard]] amrex::ParserExecutor<7> const* getUserIntParserExecData () const
[[nodiscard]] amrex::ParserExecutor<7> const* getUserRealParserExecData () const
```

Public members:

```cpp
amrex::Gpu::PinnedVector<int*> m_pa_user_int_pinned
amrex::Gpu::PinnedVector<amrex::ParticleReal*> m_pa_user_real_pinned
amrex::Gpu::DeviceVector<int*> m_d_pa_user_int
amrex::Gpu::DeviceVector<amrex::ParticleReal*> m_d_pa_user_real
amrex::Gpu::DeviceVector< amrex::ParserExecutor<7> > m_d_user_int_attrib_parserexec
amrex::Gpu::DeviceVector< amrex::ParserExecutor<7> > m_d_user_real_attrib_parserexec
const PlasmaParserWrapper* m_wrapper_ptr
```

## `PlasmaParserWrapper`

- file: `Source/Particles/ParticleCreation/AddPlasmaUtilities.H`
- kind: `struct`
- public methods: 1; public members: 2

```cpp
PlasmaParserWrapper (std::size_t a_num_user_int_attribs, std::size_t a_num_user_real_attribs, const amrex::Vector< std::unique_ptr<amrex::Parser> >& a_user_int_attrib_parser, const amrex::Vector< std::unique_ptr<amrex::Parser> >& a_user_real_attrib_parser)
```

Public members:

```cpp
amrex::Gpu::PinnedVector< amrex::ParserExecutor<7> > m_user_int_attrib_parserexec_pinned
amrex::Gpu::PinnedVector< amrex::ParserExecutor<7> > m_user_real_attrib_parserexec_pinned
```

## `QEDHelper`

- file: `Source/Particles/ParticleCreation/AddPlasmaUtilities.H`
- kind: `struct`
- public methods: 1; public members: 6

```cpp
QEDHelper (SoAType& a_soa, std::size_t old_size, bool a_has_quantum_sync, bool a_has_breit_wheeler, const std::shared_ptr<QuantumSynchrotronEngine>& a_shr_p_qs_engine, const std::shared_ptr<BreitWheelerEngine>& a_shr_p_bw_engine) : has_quantum_sync(a_has_quantum_sync), has_breit_wheeler(a_has_breit_wheeler)
```

Public members:

```cpp
amrex::ParticleReal* p_optical_depth_QSR = nullptr
amrex::ParticleReal* p_optical_depth_BW = nullptr
bool has_quantum_sync
bool has_breit_wheeler
QuantumSynchrotronGetOpticalDepth quantum_sync_get_opt
BreitWheelerGetOpticalDepth breit_wheeler_get_opt
```

## `SmartCopy`

- file: `Source/Particles/ParticleCreation/SmartCopy.H`
- kind: `struct`
- public methods: 0; public members: 8

Public members:

```cpp
int m_num_copy_real
const int* m_src_comps_r
const int* m_dst_comps_r
int m_num_copy_int
const int* m_src_comps_i
const int* m_dst_comps_i
const InitializationPolicy* m_policy_real
const InitializationPolicy* m_policy_int
```

## `SmartCopyFactory`

- file: `Source/Particles/ParticleCreation/SmartCopy.H`
- kind: `class`
- public methods: 3; public members: 4

```cpp
SmartCopyFactory (const SrcPC& src, const DstPC& dst) noexcept : m_tag_real
[[nodiscard]] SmartCopy getSmartCopy () const noexcept
[[nodiscard]] bool isDefined () const noexcept
```

Public members:

```cpp
, m_tag_int
, m_policy_real
, m_policy_int
, m_defined
```

## `SmartCreate`

- file: `Source/Particles/ParticleCreation/SmartCreate.H`
- kind: `struct`
- public methods: 0; public members: 3

Public members:

```cpp
const InitializationPolicy* m_policy_real
const InitializationPolicy* m_policy_int
int m_weight_index = 0
```

## `SmartCreateFactory`

- file: `Source/Particles/ParticleCreation/SmartCreate.H`
- kind: `class`
- public methods: 3; public members: 2

```cpp
explicit SmartCreateFactory (const PartTileData& part) noexcept: m_policy_real
[[nodiscard]] SmartCreate getSmartCreate () const noexcept
[[nodiscard]] bool isDefined () const noexcept
```

Public members:

```cpp
, m_policy_int
, m_defined
```

## `SmartCopyTag`

- file: `Source/Particles/ParticleCreation/SmartUtils.H`
- kind: `struct`
- public methods: 1; public members: 3

```cpp
[[nodiscard]] int size () const noexcept
```

Public members:

```cpp
std::vector<std::string> common_names
amrex::Gpu::DeviceVector<int> src_comps
amrex::Gpu::DeviceVector<int> dst_comps
```

## `ParticleThermalizer`

- file: `Source/Particles/ParticleThermalizer/ParticleThermalizer.H`
- kind: `class`
- public methods: 4; public members: 0

```cpp
ParticleThermalizer ()
[[nodiscard]] bool defined () const
void applyThermalizer (MultiParticleContainer &mpc) const
void applyThermalizer (WarpXParticleContainer &pcont) const
```

## `PhotonParticleContainer`

- file: `Source/Particles/PhotonParticleContainer.H`
- kind: `class`, bases: `public PhysicalParticleContainer`
- public methods: 15; public members: 0

```cpp
PhotonParticleContainer (amrex::AmrCore* amr_core, int ispecies, const std::string& name)
~PhotonParticleContainer () override = default
PhotonParticleContainer ( PhotonParticleContainer const &) = delete
PhotonParticleContainer& operator= ( PhotonParticleContainer const & ) = delete
PhotonParticleContainer ( PhotonParticleContainer&& ) = default
PhotonParticleContainer& operator= ( PhotonParticleContainer&& ) = default
void InitData() override
void Evolve (ablastr::fields::MultiFabRegister& fields, int lev, const std::string& current_fp_string, amrex::Real t, amrex::Real dt, SubcyclingHalf subcycling_half=SubcyclingHalf::None, bool skip_deposition=false, PositionPushType position_push_type=PositionPushType::Full, MomentumPushType momentum_push_type=MomentumPushType::Full, ImplicitOptions const * implicit_options = nullptr) override
void PushPX (WarpXParIter& pti, amrex::FArrayBox const * exfab, amrex::FArrayBox const * eyfab, amrex::FArrayBox const * ezfab, amrex::FArrayBox const * bxfab, amrex::FArrayBox const * byfab, amrex::FArrayBox const * bzfab, amrex::IntVect ngEB, int , long offset, long np_to_push, int lev, int gather_lev, amrex::Real dt, ScaleFields scaleFields, SubcyclingHalf subcycling_half, PositionPushType position_push_type=PositionPushType::Full, MomentumPushType momentum_push_type=MomentumPushType::Full) override
void FinishImplicitParticleUpdate ( ablastr::fields::MultiFabRegister& fields, int lev, amrex::Real t, amrex::Real dt) override
void ResetImplicitParticleData (int ) override
void PushP (int , amrex::Real , const amrex::MultiFab& , const amrex::MultiFab& , const amrex::MultiFab& , const amrex::MultiFab& , const amrex::MultiFab& , const amrex::MultiFab& , MomentumPushType , std::optional<FieldGatherOrders> ) override
void DepositCharge (WarpXParIter& , RealVector const & , const int * const , amrex::MultiFab* , int , const long , const long , int , int , int ) override
void DepositCurrent (WarpXParIter& , RealVector const & , RealVector const & , RealVector const & , RealVector const & , int const * const , amrex::MultiFab * const , amrex::MultiFab * const , amrex::MultiFab * const , long const , long const , int const , int const , int const , amrex::Real const , amrex::Real const , PushType , int ) override
void DepositMassMatrices (ablastr::fields::MultiFabRegister& , int , amrex::Real , ImplicitOptions const& ) override
```

## `PhysicalParticleContainer`

- file: `Source/Particles/PhysicalParticleContainer.H`
- kind: `class`, bases: `public WarpXParticleContainer`
- public methods: 54; public members: 0

```cpp
PhysicalParticleContainer (amrex::AmrCore* amr_core, int ispecies, const std::string& name)
explicit PhysicalParticleContainer (amrex::AmrCore* amr_core)
void BackwardCompatibility ()
~PhysicalParticleContainer () override = default
PhysicalParticleContainer (PhysicalParticleContainer const &) = delete
PhysicalParticleContainer& operator= (PhysicalParticleContainer const & ) = delete
PhysicalParticleContainer(PhysicalParticleContainer&& ) = default
PhysicalParticleContainer& operator=(PhysicalParticleContainer&& ) = default
void AllocData () override
void InitData () override
void ReadHeader (std::istream& is) override
void WriteHeader (std::ostream& os) const override
void InitIonizationModule () override
PlasmaInjector* GetPlasmaInjector (int i) override
void Evolve (ablastr::fields::MultiFabRegister& fields, int lev, const std::string& current_fp_string, amrex::Real t, amrex::Real dt, SubcyclingHalf subcycling_half=SubcyclingHalf::None, bool skip_deposition=false, PositionPushType position_push_type=PositionPushType::Full, MomentumPushType momentum_push_type=MomentumPushType::Full, ImplicitOptions const * implicit_options = nullptr) override
void DepositMassMatrices (ablastr::fields::MultiFabRegister& fields, int lev, amrex::Real dt, ImplicitOptions const& implicit_options) override
virtual void PushPX (WarpXParIter& pti, amrex::FArrayBox const * exfab, amrex::FArrayBox const * eyfab, amrex::FArrayBox const * ezfab, amrex::FArrayBox const * bxfab, amrex::FArrayBox const * byfab, amrex::FArrayBox const * bzfab, amrex::IntVect ngEB, int , long offset, long np_to_push, int lev, int gather_lev, amrex::Real dt, ScaleFields scaleFields, SubcyclingHalf subcycling_half=SubcyclingHalf::None, PositionPushType position_push_type=PositionPushType::Full, MomentumPushType momentum_push_type=MomentumPushType::Full)
void FindSuborbitParticles (WarpXParIter& pti, long offset, long np_to_push, long & num_unconverged_particles, amrex::Gpu::DeviceVector<long> & unconverged_indices, amrex::Gpu::DeviceVector<amrex::ParticleReal> & saved_weights)
void SetupSuborbitParticles (WarpXParIter& pti, long offset, long np_to_push, long num_unconverged_particles, amrex::Gpu::DeviceVector<long> & unconverged_indices, amrex::Gpu::DeviceVector<amrex::ParticleReal> & saved_weights)
void ImplicitPushXP (WarpXParIter& pti, amrex::FArrayBox const * exfab, amrex::FArrayBox const * eyfab, amrex::FArrayBox const * ezfab, amrex::FArrayBox const * bxfab, amrex::FArrayBox const * byfab, amrex::FArrayBox const * bzfab, ImplicitOptions const * implicit_options, amrex::IntVect const & ngEB, long offset, long np_to_push, int lev, int gather_lev, amrex::Real dt, long & num_unconverged_particles, amrex::Gpu::DeviceVector<long> & unconverged_indices, amrex::Gpu::DeviceVector<amrex::ParticleReal> & saved_weights)
void ImplicitPushXPSubOrbits (WarpXParIter& pti, ablastr::fields::MultiFabRegister& fields, amrex::FArrayBox const * exfab, amrex::FArrayBox const * eyfab, amrex::FArrayBox const * ezfab, amrex::FArrayBox const * bxfab, amrex::FArrayBox const * byfab, amrex::FArrayBox const * bzfab, ImplicitOptions const * implicit_options, amrex::IntVect ngEB, amrex::MultiFab* jx, amrex::MultiFab* jy, amrex::MultiFab* jz, long offset, int lev, int gather_lev, amrex::Real dt, bool skip_deposition, long num_unconverged_particles, amrex::Gpu::DeviceVector<long> & unconverged_indices, amrex::Gpu::DeviceVector<amrex::ParticleReal> & saved_weights)
void PushP (int lev, amrex::Real dt, const amrex::MultiFab& Ex, const amrex::MultiFab& Ey, const amrex::MultiFab& Ez, const amrex::MultiFab& Bx, const amrex::MultiFab& By, const amrex::MultiFab& Bz, MomentumPushType momentum_push_type, std::optional<FieldGatherOrders> gather_orders) override
void PartitionParticlesInBuffers ( long& nfine_current, long& nfine_gather, long np, WarpXParIter& pti, int lev, int n_field_gather_buffer, int n_current_deposition_buffer, amrex::iMultiFab const* current_masks, amrex::iMultiFab const* gather_masks )
void PostRestart () final
void SplitParticles (int lev)
IonizationFilterFunc getIonizationFunc (const WarpXParIter& pti, int lev, amrex::IntVect ngEB, const amrex::FArrayBox& Ex, const amrex::FArrayBox& Ey, const amrex::FArrayBox& Ez, const amrex::FArrayBox& Bx, const amrex::FArrayBox& By, const amrex::FArrayBox& Bz)
virtual void AddParticles (int lev)
void AddPlasma (PlasmaInjector& plasma_injector, int lev, amrex::RealBox part_realbox = amrex::RealBox())
void AddPlasmaFlux (PlasmaInjector const& plasma_injector, amrex::Real dt)
void MapParticletoBoostedFrame (amrex::ParticleReal& x, amrex::ParticleReal& y, amrex::ParticleReal& z, amrex::ParticleReal& ux, amrex::ParticleReal& uy, amrex::ParticleReal& uz, amrex::Real t_lab = 0.) const
void AddGaussianBeam (PlasmaInjector const& plasma_injector)
void AddPlasmaFromFile (PlasmaInjector & plasma_injector, amrex::ParticleReal q_tot, amrex::ParticleReal z_shift)
void CheckAndAddParticle ( amrex::ParticleReal x, amrex::ParticleReal y, amrex::ParticleReal z, amrex::ParticleReal ux, amrex::ParticleReal uy, amrex::ParticleReal uz, amrex::ParticleReal weight, amrex::Gpu::HostVector<amrex::ParticleReal>& particle_x, amrex::Gpu::HostVector<amrex::ParticleReal>& particle_y, amrex::Gpu::HostVector<amrex::ParticleReal>& particle_z, amrex::Gpu::HostVector<amrex::ParticleReal>& particle_ux, amrex::Gpu::HostVector<amrex::ParticleReal>& particle_uy, amrex::Gpu::HostVector<amrex::ParticleReal>& particle_uz, amrex::Gpu::HostVector<amrex::ParticleReal>& particle_w, amrex::Real t_lab= 0.) const
void DefaultInitializeRuntimeAttributes ( typename ContainerLike<amrex::PolymorphicArenaAllocator>::ParticleTileType& pinned_tile, int n_external_attr_real, int n_external_attr_int) final
void applyNCIFilter ( int lev, const amrex::Box& box, amrex::FArrayBox& filtered_Ex, amrex::FArrayBox& filtered_Ey, amrex::FArrayBox& filtered_Ez, amrex::FArrayBox& filtered_Bx, amrex::FArrayBox& filtered_By, amrex::FArrayBox& filtered_Bz, const amrex::FArrayBox& Ex, const amrex::FArrayBox& Ey, const amrex::FArrayBox& Ez, const amrex::FArrayBox& Bx, const amrex::FArrayBox& By, const amrex::FArrayBox& Bz, amrex::FArrayBox const * & ex_ptr, amrex::FArrayBox const * & ey_ptr, amrex::FArrayBox const * & ez_ptr, amrex::FArrayBox const * & bx_ptr, amrex::FArrayBox const * & by_ptr, amrex::FArrayBox const * & bz_ptr)
void resample ( const amrex::Vector<amrex::Geometry>& geom, int timestep, bool verbose=true) final
bool has_quantum_sync () const override
bool has_breit_wheeler () const override
void set_breit_wheeler_engine_ptr (const std::shared_ptr<BreitWheelerEngine>& ptr) override
void set_quantum_sync_engine_ptr (const std::shared_ptr<QuantumSynchrotronEngine>& ptr) override
BreitWheelerEngine* get_breit_wheeler_engine_ptr () const override
QuantumSynchrotronEngine* get_quantum_sync_engine_ptr () const override
PhotonEmissionFilterFunc getPhotonEmissionFilterFunc ()
PairGenerationFilterFunc getPairGenerationFilterFunc ()
bool has_virtual_photons () const override
bool has_virtual_photons_beam_size_effect () const override
int getVirtualPhotonSpeciesIndex () const override
std::vector<std::string> getUserIntAttribs () const override
std::vector<std::string> getUserRealAttribs () const override
amrex::Vector< amrex::Parser* > getUserIntAttribParser () const override
amrex::Vector< amrex::Parser* > getUserRealAttribParser () const override
bool getTemperatureDepositionFlag () const noexcept override
void AccumulateVelocitiesAndComputeTemperature ( ablastr::fields::MultiLevelVectorField const & T_vf, amrex::Real relative_time) override
void DepositTemperature ( WarpXParIter& pti, RealVector const & wp, RealVector const & uxp, RealVector const & uyp, RealVector const & uzp, amrex::MultiFab* Tx, amrex::MultiFab* Ty, amrex::MultiFab* Tz, long offset, long np_to_deposit, int thread_num, int lev, int depos_lev, amrex::Real relative_time, PushType push_type, warpx::particles::deposition::TemperatureDepositionType type, warpx::particles::deposition::TemperatureDepositionPass pass) override
```

## `CopyParticleAttribs`

- file: `Source/Particles/Pusher/CopyParticleAttribs.H`
- kind: `struct`
- public methods: 2; public members: 10

```cpp
CopyParticleAttribs () = default
CopyParticleAttribs ( const WarpXParticleContainer& , WarpXParIter& a_pti, long a_offset = 0) noexcept
```

Public members:

```cpp
GetParticlePosition<PIdx> m_get_position
const amrex::ParticleReal* AMREX_RESTRICT uxp = nullptr
const amrex::ParticleReal* AMREX_RESTRICT uyp = nullptr
const amrex::ParticleReal* AMREX_RESTRICT uzp = nullptr
amrex::ParticleReal* AMREX_RESTRICT xpold = nullptr
amrex::ParticleReal* AMREX_RESTRICT ypold = nullptr
amrex::ParticleReal* AMREX_RESTRICT zpold = nullptr
amrex::ParticleReal* AMREX_RESTRICT uxpold = nullptr
amrex::ParticleReal* AMREX_RESTRICT uypold = nullptr
amrex::ParticleReal* AMREX_RESTRICT uzpold = nullptr
```

## `GetParticlePosition`

- file: `Source/Particles/Pusher/GetAndSetPosition.H`
- kind: `struct`
- public methods: 5; public members: 5

```cpp
static constexpr RType m_x_default = RType(0.0)
static constexpr RType m_y_default = RType(0.0)
static constexpr RType m_z_default = RType(0.0)
GetParticlePosition () = default
explicit GetParticlePosition (const ptiType& a_pti, long a_offset = 0) noexcept
```

Public members:

```cpp
const RType* AMREX_RESTRICT m_x = nullptr
const RType* AMREX_RESTRICT m_y = nullptr
const RType* AMREX_RESTRICT m_z = nullptr
const RType* AMREX_RESTRICT m_theta = nullptr
const RType* AMREX_RESTRICT m_phi = nullptr
```

## `SetParticlePosition`

- file: `Source/Particles/Pusher/GetAndSetPosition.H`
- kind: `struct`
- public methods: 1; public members: 6

```cpp
explicit SetParticlePosition (const ptiType& a_pti, long a_offset = 0) noexcept
```

Public members:

```cpp
RType* AMREX_RESTRICT m_x
RType* AMREX_RESTRICT m_y
RType* AMREX_RESTRICT m_z
RType* AMREX_RESTRICT m_x = nullptr
RType* AMREX_RESTRICT m_theta
RType* AMREX_RESTRICT m_phi
```

## `LevelingThinning`

- file: `Source/Particles/Resampling/LevelingThinning.H`
- kind: `class`, bases: `public ResamplingAlgorithm`
- public methods: 4; public members: 0

```cpp
LevelingThinning () = default
explicit LevelingThinning (const std::string& species_name)
void BackwardCompatibility (const std::string& species_name )
void operator() ( const amrex::Geometry& geom_lev, WarpXParIter& pti, int lev, WarpXParticleContainer* pc) const final
```

## `Resampling`

- file: `Source/Particles/Resampling/Resampling.H`
- kind: `class`
- public methods: 4; public members: 0

```cpp
Resampling () = default
explicit Resampling (const std::string& species_name)
bool triggered (int timestep, amrex::Real global_numparts) const
void operator() ( const amrex::Geometry& geom_lev, WarpXParIter& pti, int lev, WarpXParticleContainer* pc) const
```

## `ResamplingAlgorithm`

- file: `Source/Particles/Resampling/Resampling.H`
- kind: `struct`
- public methods: 7; public members: 0

```cpp
virtual void operator() ( const amrex::Geometry& , WarpXParIter& , int , WarpXParticleContainer* ) const = 0
virtual ~ResamplingAlgorithm () = default
ResamplingAlgorithm () = default
ResamplingAlgorithm ( ResamplingAlgorithm const &) = default
ResamplingAlgorithm& operator= ( ResamplingAlgorithm const & ) = default
ResamplingAlgorithm ( ResamplingAlgorithm&& ) = default
ResamplingAlgorithm& operator= ( ResamplingAlgorithm&& ) = default
```

## `ResamplingTrigger`

- file: `Source/Particles/Resampling/ResamplingTrigger.H`
- kind: `class`
- public methods: 4; public members: 0

```cpp
ResamplingTrigger () = default
explicit ResamplingTrigger (const std::string& species_name)
bool triggered (int timestep, amrex::Real global_numparts) const
void initialize_global_numcells () const
```

## `HeapSort`

- file: `Source/Particles/Resampling/VelocityCoincidenceThinning.H`
- kind: `struct`
- public methods: 0; public members: 0

## `VelocityBinCalculator`

- file: `Source/Particles/Resampling/VelocityCoincidenceThinning.H`
- kind: `struct`
- public methods: 0; public members: 5

Public members:

```cpp
VelocityGridType velocity_grid_type
int n1, n2
amrex::ParticleReal dur, dutheta, duphi
amrex::ParticleReal dux, duy, duz
amrex::ParticleReal ux_min, uy_min, uz_min, ux_max, uy_max
```

## `VelocityCoincidenceThinning`

- file: `Source/Particles/Resampling/VelocityCoincidenceThinning.H`
- kind: `class`, bases: `public ResamplingAlgorithm`
- public methods: 3; public members: 3

```cpp
VelocityCoincidenceThinning () = default
explicit VelocityCoincidenceThinning (const std::string& species_name)
void operator() ( const amrex::Geometry& geom_lev, WarpXParIter& pti, int lev, WarpXParticleContainer* pc) const final
```

Public members:

```cpp
enum struct VelocityGridType
struct HeapSort
struct VelocityBinCalculator
```

## `RigidInjectedParticleContainer`

- file: `Source/Particles/RigidInjectedParticleContainer.H`
- kind: `class`, bases: `public PhysicalParticleContainer`
- public methods: 13; public members: 0

```cpp
RigidInjectedParticleContainer (amrex::AmrCore* amr_core, int ispecies, const std::string& name)
~RigidInjectedParticleContainer () override = default
RigidInjectedParticleContainer ( RigidInjectedParticleContainer const &) = delete
RigidInjectedParticleContainer& operator= ( RigidInjectedParticleContainer const & ) = delete
RigidInjectedParticleContainer ( RigidInjectedParticleContainer&& ) = default
RigidInjectedParticleContainer& operator= ( RigidInjectedParticleContainer&& ) = default
void InitData() override
virtual void RemapParticles()
void Evolve (ablastr::fields::MultiFabRegister& fields, int lev, const std::string& current_fp_string, amrex::Real t, amrex::Real dt, SubcyclingHalf subcycling_half=SubcyclingHalf::None, bool skip_deposition=false, PositionPushType position_push_type=PositionPushType::Full, MomentumPushType momentum_push_type=MomentumPushType::Full, ImplicitOptions const * implicit_options = nullptr) override
void PushPX (WarpXParIter& pti, amrex::FArrayBox const * exfab, amrex::FArrayBox const * eyfab, amrex::FArrayBox const * ezfab, amrex::FArrayBox const * bxfab, amrex::FArrayBox const * byfab, amrex::FArrayBox const * bzfab, amrex::IntVect ngEB, int , long offset, long np_to_push, int lev, int gather_lev, amrex::Real dt, ScaleFields scaleFields, SubcyclingHalf subcycling_half=SubcyclingHalf::None, PositionPushType position_push_type=PositionPushType::Full, MomentumPushType momentum_push_type=MomentumPushType::Full) override
void PushP (int lev, amrex::Real dt, const amrex::MultiFab& Ex, const amrex::MultiFab& Ey, const amrex::MultiFab& Ez, const amrex::MultiFab& Bx, const amrex::MultiFab& By, const amrex::MultiFab& Bz, MomentumPushType momentum_push_type = MomentumPushType::Full, std::optional<FieldGatherOrders> gather_orders = std::nullopt) override
void ReadHeader (std::istream& is) override
void WriteHeader (std::ostream& os) const override
```

## `Compute_shape_factor`

- file: `Source/Particles/ShapeFactors.H`
- kind: `struct`
- public methods: 0; public members: 0

## `Compute_shape_factor_pair`

- file: `Source/Particles/ShapeFactors.H`
- kind: `struct`
- public methods: 0; public members: 0

## `Compute_shifted_shape_factor`

- file: `Source/Particles/ShapeFactors.H`
- kind: `struct`
- public methods: 0; public members: 0

## `copyAndReorder`

- file: `Source/Particles/Sorting/SortingUtils.H`
- kind: `class`
- public methods: 1; public members: 2

```cpp
copyAndReorder( amrex::Gpu::DeviceVector<T> const& src, amrex::Gpu::DeviceVector<T>& dst, amrex::Gpu::DeviceVector<int> const& indices ): m_src_ptr
```

Public members:

```cpp
, m_dst_ptr
, m_indices_ptr
```

## `fillBufferFlag`

- file: `Source/Particles/Sorting/SortingUtils.H`
- kind: `class`
- public methods: 1; public members: 3

```cpp
fillBufferFlag( WarpXParIter const& pti, amrex::iMultiFab const* bmasks, amrex::Gpu::DeviceVector<int>& inexflag, amrex::Geometry const& geom ): m_domain
```

Public members:

```cpp
, m_inexflag_ptr
, m_ptd
, m_buffer_mask
```

## `fillBufferFlagRemainingParticles`

- file: `Source/Particles/Sorting/SortingUtils.H`
- kind: `class`
- public methods: 1; public members: 5

```cpp
fillBufferFlagRemainingParticles( WarpXParIter const& pti, amrex::iMultiFab const* bmasks, amrex::Gpu::DeviceVector<int>& inexflag, amrex::Geometry const& geom, amrex::Gpu::DeviceVector<int> const& particle_indices, int start_index ) : m_domain
```

Public members:

```cpp
, m_inexflag_ptr
, m_ptd
, m_buffer_mask
, m_start_index
, m_indices_ptr
```

## `DiagIdx`

- file: `Source/Particles/WarpXParticleContainer.H`
- kind: `struct`
- public methods: 0; public members: 1

Public members:

```cpp
enum
```

## `FieldGatherOrders`

- file: `Source/Particles/WarpXParticleContainer.H`
- kind: `struct`
- public methods: 0; public members: 3

Public members:

```cpp
int e_shape
bool e_galerkin
int b_shape
```

## `IntIdx`

- file: `Source/Particles/WarpXParticleContainer.H`
- kind: `struct`
- public methods: 0; public members: 2

Public members:

```cpp
enum
static constexpr std::initializer_list<char const *> names =
```

## `PIdx`

- file: `Source/Particles/WarpXParticleContainer.H`
- kind: `struct`
- public methods: 0; public members: 2

Public members:

```cpp
enum
static constexpr auto names =
```

## `WarpXParIter`

- file: `Source/Particles/WarpXParticleContainer.H`
- kind: `class`, bases: `public amrex::ParIterSoA<PIdx::nattribs, 0, amrex::PolymorphicArenaAllocator>`
- public methods: 12; public members: 0

```cpp
WarpXParIter (ContainerType& pc, int level)
WarpXParIter (ContainerType& pc, int level, amrex::MFItInfo& info)
[[nodiscard]] const std::array<RealVector, PIdx::nattribs>& GetAttribs () const
[[nodiscard]] std::array<RealVector, PIdx::nattribs>& GetAttribs ()
[[nodiscard]] const RealVector& GetAttribs (int comp) const
[[nodiscard]] RealVector& GetAttribs (int comp)
[[nodiscard]] const IntVector& GetiAttribs (int comp) const
[[nodiscard]] IntVector& GetiAttribs (int comp)
[[nodiscard]] const RealVector& GetAttribs (const std::string& name) const
[[nodiscard]] RealVector& GetAttribs (const std::string& name)
[[nodiscard]] const IntVector& GetiAttribs (const std::string& name) const
[[nodiscard]] IntVector& GetiAttribs (const std::string& name)
```

## `WarpXParticleContainer`

- file: `Source/Particles/WarpXParticleContainer.H`
- kind: `class`, bases: `public amrex::ParticleContainerPureSoA<PIdx::nattribs, 0, amrex::PolymorphicArenaAllocator>`
- public methods: 85; public members: 9

```cpp
WarpXParticleContainer (amrex::AmrCore* amr_core, int ispecies, const std::string& name)
~WarpXParticleContainer() override = default
WarpXParticleContainer(const WarpXParticleContainer&) = delete
WarpXParticleContainer& operator=(const WarpXParticleContainer&) = delete
WarpXParticleContainer(WarpXParticleContainer&&) = default
WarpXParticleContainer& operator=(WarpXParticleContainer&&) = default
virtual void InitData () = 0
virtual void InitIonizationModule ()
virtual PlasmaInjector* GetPlasmaInjector (const int )
virtual void Evolve (ablastr::fields::MultiFabRegister& fields, int lev, const std::string& current_fp_string, amrex::Real t, amrex::Real dt, SubcyclingHalf subcycling_half=SubcyclingHalf::None, bool skip_deposition=false, PositionPushType position_push_type=PositionPushType::Full, MomentumPushType momentum_push_type=MomentumPushType::Full, ImplicitOptions const * implicit_options = nullptr) = 0
virtual void FinishImplicitParticleUpdate ( ablastr::fields::MultiFabRegister& fields, int lev, amrex::Real t, amrex::Real dt)
virtual void ResetImplicitParticleData (int lev)
virtual void DepositMassMatrices (ablastr::fields::MultiFabRegister& fields, int lev, amrex::Real dt, ImplicitOptions const& implicit_options)
virtual void PostRestart () = 0
virtual void AllocData ()
virtual void DefaultInitializeRuntimeAttributes ( typename ContainerLike<amrex::PolymorphicArenaAllocator>::ParticleTileType& pinned_tile, int n_external_attr_real, int n_external_attr_int) = 0
void PushX ( amrex::Real dt)
void PushX (int lev, amrex::Real dt)
virtual void PushP (int lev, amrex::Real dt, const amrex::MultiFab& Ex, const amrex::MultiFab& Ey, const amrex::MultiFab& Ez, const amrex::MultiFab& Bx, const amrex::MultiFab& By, const amrex::MultiFab& Bz, MomentumPushType momentum_push_type = MomentumPushType::Full, std::optional<FieldGatherOrders> gather_orders = std::nullopt) = 0
void DepositCurrent (ablastr::fields::MultiLevelVectorField const & J, amrex::Real dt, amrex::Real relative_time, PushType push_type = PushType::Explicit, std::optional<int> particle_shape = std::nullopt)
void DepositCurrentOnLevel (const std::string& mf_name, int lev, amrex::Real dt, amrex::Real relative_time)
void DepositCharge (const ablastr::fields::MultiLevelScalarField& rho, bool local = false, bool reset = false, bool interpolate_across_levels = true, int icomp = 0)
void DepositChargeOnLevel (amrex::MultiFab* rho, int lev, bool local = false, bool reset = false, int icomp = 0)
std::unique_ptr<amrex::MultiFab> GetChargeDensityOnLevel (int lev, bool local, bool finalize)
virtual void DepositCharge (WarpXParIter& pti, RealVector const & wp, const int* ion_lev, amrex::MultiFab* rho, int icomp, long offset, long np_to_deposit, int thread_num, int lev, int depos_lev)
virtual void DepositCurrent (WarpXParIter& pti, RealVector const & wp, RealVector const & uxp, RealVector const & uyp, RealVector const & uzp, int const* ion_lev, amrex::MultiFab* jx, amrex::MultiFab* jy, amrex::MultiFab* jz, long offset, long np_to_deposit, int thread_num, int lev, int depos_lev, amrex::Real dt, amrex::Real relative_time, PushType push_type, int particle_shape)
virtual void AccumulateVelocitiesAndComputeTemperature ( [[maybe_unused]]ablastr::fields::MultiLevelVectorField const & T_vf, [[maybe_unused]]amrex::Real relative_time)
virtual void DepositTemperature ( [[maybe_unused]]WarpXParIter& pti, [[maybe_unused]]RealVector const & wp, [[maybe_unused]]RealVector const & uxp, [[maybe_unused]]RealVector const & uyp, [[maybe_unused]]RealVector const & uzp, [[maybe_unused]]amrex::MultiFab * const Tx, [[maybe_unused]]amrex::MultiFab * const Ty, [[maybe_unused]]amrex::MultiFab * const Tz, [[maybe_unused]]long const offset, [[maybe_unused]]long const np_to_deposit, [[maybe_unused]]int const thread_num, [[maybe_unused]]const int lev, [[maybe_unused]]int const depos_lev, [[maybe_unused]]amrex::Real const relative_time, [[maybe_unused]]PushType push_type, [[maybe_unused]]const warpx::particles::deposition::TemperatureDepositionType type, [[maybe_unused]]const warpx::particles::deposition::TemperatureDepositionPass pass)
void DepositTotalNGPTemperature (int lev)
void CalculateNuei(amrex::MultiFab & species_nuei, WarpXParticleContainer const & electron_species, int lev)
std::unique_ptr<amrex::MultiFab> GetDebyeLength (int lev)
void DepositNumberDensity (amrex::MultiFab* number_density, int lev)
std::unique_ptr<amrex::MultiFab> GetNumberDensity (int lev)
std::unique_ptr<amrex::MultiFab> GetPlasmaFrequency (int lev)
void DepositMassMatrices (WarpXParIter& pti, const RealVector& wp, const RealVector& uxp, const RealVector& uyp, const RealVector& uzp, amrex::MultiFab* Sxx, amrex::MultiFab* Sxy, amrex::MultiFab* Sxz, amrex::MultiFab* Syx, amrex::MultiFab* Syy, amrex::MultiFab* Syz, amrex::MultiFab* Szx, amrex::MultiFab* Szy, amrex::MultiFab* Szz, const amrex::FArrayBox* bxfab, const amrex::FArrayBox* byfab, const amrex::FArrayBox* bzfab, long offset, long np_to_deposit, int thread_num, int lev, int depos_lev, amrex::Real dt, ImplicitOptions const& implicit_options)
virtual void ContinuousInjection(const amrex::RealBox& )
virtual void UpdateAntennaPosition(const amrex::Real )
bool doContinuousInjection() const
virtual void ContinuousFluxInjection(amrex::Real , amrex::Real )
int getSpeciesId() const
std::pair<amrex::ParticleReal, amrex::ParticleReal> sumParticleWeightAndEnergy (bool local = false) const
amrex::ParticleReal sumParticleCharge (bool local = false) const
amrex::ParticleReal sumParticleWeight (bool local = false) const
amrex::ParticleReal sumParticleEnergy (bool local = false) const
std::array<amrex::ParticleReal, 3> meanParticleVelocity(bool local = false)
amrex::ParticleReal maxParticleDtInv(bool local = false)
void TransformMomentumToCurvilinear (bool forward)
void AddNParticles (int lev, long n, amrex::Vector<amrex::ParticleReal> const & x, amrex::Vector<amrex::ParticleReal> const & y, amrex::Vector<amrex::ParticleReal> const & z, amrex::Vector<amrex::ParticleReal> const & ux, amrex::Vector<amrex::ParticleReal> const & uy, amrex::Vector<amrex::ParticleReal> const & uz, int nattr_real, amrex::Vector<amrex::Vector<amrex::ParticleReal>> const & attr_real, int nattr_int, amrex::Vector<amrex::Vector<int>> const & attr_int, int uniqueparticles, amrex::Long id=-1)
void deleteInvalidParticles ()
virtual void ReadHeader (std::istream& is) = 0
virtual void WriteHeader (std::ostream& os) const = 0
static void ReadParameters ()
static void BackwardCompatibility ()
void ApplyBoundaryConditions ()
amrex::Real self_fields_required_precision = amrex::Real(1.e-11)
amrex::Real self_fields_absolute_tolerance = amrex::Real(0.0)
void SetDoBackTransformedParticles (const bool do_back_transformed_particles)
bool GetDoBackTransformedParticles () const
amrex::ParticleReal getCharge () const
amrex::ParticleReal getMass () const
amrex::ParticleReal getBoundaryThermalVelocity () const
int DoFieldIonization() const
virtual bool has_quantum_sync() const
virtual bool has_breit_wheeler() const
virtual bool has_virtual_photons() const
virtual bool has_virtual_photons_beam_size_effect() const
virtual int getVirtualPhotonSpeciesIndex() const
int DoQED() const
bool AmIA () const noexcept
std::string getSpeciesTypeName () const
virtual void resample (const amrex::Vector<amrex::Geometry>& , const int , bool )
void defineAllParticleTiles () noexcept
virtual std::vector<std::string> getUserIntAttribs () const
virtual std::vector<std::string> getUserRealAttribs () const
virtual amrex::Vector< amrex::Parser* > getUserIntAttribParser () const
virtual amrex::Vector< amrex::Parser* > getUserRealAttribParser () const
virtual BreitWheelerEngine* get_breit_wheeler_engine_ptr () const
virtual QuantumSynchrotronEngine* get_quantum_sync_engine_ptr () const
void setDoNotPush (bool flag)
void setDoNotGather (int flag)
void setDoNotDeposit (int flag)
bool HasiAttrib (const std::string& name)
std::string const & getName()
int getIonizationInitialLevel () const noexcept
virtual bool getTemperatureDepositionFlag () const noexcept
```

Public members:

```cpp
bool do_splitting = false
int do_not_deposit = 0
bool initialize_self_fields = false
int self_fields_max_iters = 200
int self_fields_verbosity = 2
amrex::Vector<amrex::ParticleReal> m_B_external_particle
amrex::Vector<amrex::ParticleReal> m_E_external_particle
amrex::Real m_current_injection_position = 0.
int split_type = 0
```

