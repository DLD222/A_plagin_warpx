# A0 API catalog — FieldSolver

> Generated from headers; 53 classes/structs. Signatures are parsed heuristically; check the header for exact defaults/templates.

## `EffectivePotentialES`

- file: `Source/FieldSolver/ElectrostaticSolvers/EffectivePotentialES.H`
- kind: `class`, bases: `public ElectrostaticSolver`
- public methods: 6; public members: 1; private members: 0

```cpp
explicit EffectivePotentialES (int nlevs_max) : ElectrostaticSolver (nlevs_max)
void InitData () override
void ComputeSpaceChargeField ( ablastr::fields::MultiFabRegister& fields, MultiParticleContainer& mpc, MultiFluidContainer* mfl, int max_level, bool verbose_step) override
void ComputeSigma ( ablastr::fields::MultiLevelScalarField const& rho_fp )
void computePhi ( ablastr::fields::MultiLevelScalarField const& rho, ablastr::fields::MultiLevelScalarField const& phi, ablastr::fields::MultiLevelVectorField const& efield, int verbosity )
void computePhi ( ablastr::fields::MultiLevelScalarField const& rho, ablastr::fields::MultiLevelScalarField const& phi, ablastr::fields::MultiLevelVectorField const& efield, ablastr::fields::MLMGOptions const & mlmg_options ) const
```

Public members:

```cpp
bool m_overwrite_sigma
```

## `ElectrostaticSolver`

- file: `Source/FieldSolver/ElectrostaticSolvers/ElectrostaticSolver.H`
- kind: `class`
- public methods: 14; public members: 4; private members: 0

```cpp
ElectrostaticSolver() = default
explicit ElectrostaticSolver( int nlevs_max )
virtual ~ElectrostaticSolver()
ElectrostaticSolver(const ElectrostaticSolver&) = delete
ElectrostaticSolver& operator=(const ElectrostaticSolver&) = delete
ElectrostaticSolver(ElectrostaticSolver&&) = delete
ElectrostaticSolver& operator=(ElectrostaticSolver&&) = delete
void ReadParameters ()
virtual void InitData ()
virtual void ComputeSpaceChargeField ( ablastr::fields::MultiFabRegister& fields, MultiParticleContainer& mpc, MultiFluidContainer* mfl, int max_level, bool verbose_step) = 0
void setPhiBC ( ablastr::fields::MultiLevelScalarField const& phi, amrex::Real t ) const
void computePhi ( ablastr::fields::MultiLevelScalarField const& rho, ablastr::fields::MultiLevelScalarField const& phi, std::array<amrex::Real, 3> beta, ablastr::fields::MLMGOptions const & mlmg_options, bool is_igf_2d_slices, std::optional<ablastr::fields::MultiLevelVectorField> efield = std::nullopt ) const
void computeE ( ablastr::fields::MultiLevelVectorField const& E, ablastr::fields::MultiLevelScalarField const& phi, std::array<amrex::Real, 3> beta ) const
void computeB ( ablastr::fields::MultiLevelVectorField const& B, ablastr::fields::MultiLevelScalarField const& phi, std::array<amrex::Real, 3> beta ) const
```

Public members:

```cpp
int num_levels
std::unique_ptr<PoissonBoundaryHandler> m_poisson_boundary_handler
ablastr::fields::MLMGOptions m_mlmg_options
bool is_igf_2d_slices = false
```

## `LabFrameExplicitES`

- file: `Source/FieldSolver/ElectrostaticSolvers/LabFrameExplicitES.H`
- kind: `class`, bases: `public ElectrostaticSolver`
- public methods: 5; public members: 0; private members: 0

```cpp
explicit LabFrameExplicitES (int nlevs_max) : ElectrostaticSolver (nlevs_max)
void InitData () override
void ComputeSpaceChargeField ( ablastr::fields::MultiFabRegister& fields, MultiParticleContainer& mpc, MultiFluidContainer* mfl, int max_level, bool verbose_step) override
void computePhiTriDiagonal ( const ablastr::fields::MultiLevelScalarField& rho, const ablastr::fields::MultiLevelScalarField& phi )
void computePhiTriDiagonal_periodic ( const ablastr::fields::MultiLevelScalarField& rho, const ablastr::fields::MultiLevelScalarField& phi )
```

## `EBCalcEfromPhiPerLevel`

- file: `Source/FieldSolver/ElectrostaticSolvers/PoissonBoundaryHandler.H`
- kind: `class`
- public methods: 2; public members: 0; private members: 1

```cpp
explicit EBCalcEfromPhiPerLevel(amrex::Vector<amrex::Array<amrex::MultiFab *, AMREX_SPACEDIM> > e_field) : m_e_field(std::move(e_field))
void operator()(amrex::MLMG & mlmg, int const lev)
```

## `PhiCalculatorEB`

- file: `Source/FieldSolver/ElectrostaticSolvers/PoissonBoundaryHandler.H`
- kind: `struct`
- public methods: 0; public members: 2; private members: 0

Public members:

```cpp
amrex::Real t
amrex::ParserExecutor<4> potential_eb
```

## `PoissonBoundaryHandler`

- file: `Source/FieldSolver/ElectrostaticSolvers/PoissonBoundaryHandler.H`
- kind: `class`
- public methods: 7; public members: 21; private members: 7

```cpp
PoissonBoundaryHandler ()
void ReadParameters ()
void DefinePhiBCs (const amrex::Geometry& geom)
void BuildParsers ()
void BuildParsersEB ()
void setPotentialEB(const std::string& potential)
[[nodiscard]] PhiCalculatorEB getPhiEB(amrex::Real t) const noexcept
```

Public members:

```cpp
struct PhiCalculatorEB
bool m_boundary_potential_specified = false
std::string potential_xlo_str = "0"
std::string potential_xhi_str = "0"
std::string potential_ylo_str = "0"
std::string potential_yhi_str = "0"
std::string potential_zlo_str = "0"
std::string potential_zhi_str = "0"
std::string potential_eb_str = "0"
amrex::ParserExecutor<1> potential_xlo
amrex::ParserExecutor<1> potential_xhi
amrex::ParserExecutor<1> potential_ylo
amrex::ParserExecutor<1> potential_yhi
amrex::ParserExecutor<1> potential_zlo
amrex::ParserExecutor<1> potential_zhi
amrex::ParserExecutor<1> potential_eb_t
amrex::ParserExecutor<4> potential_eb
amrex::Array<amrex::LinOpBCType, AMREX_SPACEDIM> lobc, hibc
std::array<bool, AMREX_SPACEDIM * 2> dirichlet_flag
bool has_non_periodic = false
bool phi_EB_only_t = true
```

## `RelativisticExplicitES`

- file: `Source/FieldSolver/ElectrostaticSolvers/RelativisticExplicitES.H`
- kind: `class`, bases: `public ElectrostaticSolver`
- public methods: 5; public members: 0; private members: 0

```cpp
explicit RelativisticExplicitES (int nlevs_max) : ElectrostaticSolver (nlevs_max)
void InitData () override
void ComputeSpaceChargeField ( ablastr::fields::MultiFabRegister& fields, MultiParticleContainer& mpc, MultiFluidContainer* mfl, int max_level, bool verbose_step) override
void AddSpaceChargeField ( WarpXParticleContainer& pc, ablastr::fields::MultiLevelVectorField& Efield_fp, ablastr::fields::MultiLevelVectorField& Bfield_fp, int verbosity )
void AddBoundaryField ( ablastr::fields::MultiLevelVectorField& Efield )
```

## `CartesianCKCAlgorithm`

- file: `Source/FieldSolver/FiniteDifferenceSolver/FiniteDifferenceAlgorithms/CartesianCKCAlgorithm.H`
- kind: `struct`
- public methods: 3; public members: 0; private members: 0

```cpp
static void InitializeStencilCoefficients ( std::array<amrex::Real, 3>& cell_size, amrex::Vector<amrex::Real>& stencil_coefs_x, amrex::Vector<amrex::Real>& stencil_coefs_y, amrex::Vector<amrex::Real>& stencil_coefs_z )
static amrex::Real ComputeMaxDt ( amrex::Real const * const dx )
static amrex::IntVect GetMaxGuardCell ()
```

## `CartesianNodalAlgorithm`

- file: `Source/FieldSolver/FiniteDifferenceSolver/FiniteDifferenceAlgorithms/CartesianNodalAlgorithm.H`
- kind: `struct`
- public methods: 3; public members: 0; private members: 0

```cpp
static void InitializeStencilCoefficients ( std::array<amrex::Real,3>& cell_size, amrex::Vector<amrex::Real>& stencil_coefs_x, amrex::Vector<amrex::Real>& stencil_coefs_y, amrex::Vector<amrex::Real>& stencil_coefs_z )
static amrex::Real ComputeMaxDt ( amrex::Real const * const dx )
static amrex::IntVect GetMaxGuardCell ()
```

## `CartesianYeeAlgorithm`

- file: `Source/FieldSolver/FiniteDifferenceSolver/FiniteDifferenceAlgorithms/CartesianYeeAlgorithm.H`
- kind: `struct`
- public methods: 3; public members: 0; private members: 0

```cpp
static void InitializeStencilCoefficients ( std::array<amrex::Real,3>& cell_size, amrex::Vector<amrex::Real>& stencil_coefs_x, amrex::Vector<amrex::Real>& stencil_coefs_y, amrex::Vector<amrex::Real>& stencil_coefs_z )
static amrex::Real ComputeMaxDt ( amrex::Real const * const dx )
static amrex::IntVect GetMaxGuardCell ()
```

## `CylindricalYeeAlgorithm`

- file: `Source/FieldSolver/FiniteDifferenceSolver/FiniteDifferenceAlgorithms/CylindricalYeeAlgorithm.H`
- kind: `struct`
- public methods: 3; public members: 0; private members: 0

```cpp
static void InitializeStencilCoefficients ( std::array<amrex::Real,3>& cell_size, amrex::Vector<amrex::Real>& stencil_coefs_r, amrex::Vector<amrex::Real>& stencil_coefs_z )
static amrex::Real ComputeMaxDt ( amrex::Real const * const dx, int const n_rz_azimuthal_modes )
static amrex::IntVect GetMaxGuardCell ()
```

## `FieldAccessorMacroscopic`

- file: `Source/FieldSolver/FiniteDifferenceSolver/FiniteDifferenceAlgorithms/FieldAccessorFunctors.H`
- kind: `struct`
- public methods: 0; public members: 0; private members: 2

## `SphericalYeeAlgorithm`

- file: `Source/FieldSolver/FiniteDifferenceSolver/FiniteDifferenceAlgorithms/SphericalYeeAlgorithm.H`
- kind: `struct`
- public methods: 3; public members: 0; private members: 0

```cpp
static void InitializeStencilCoefficients ( std::array<amrex::Real,3>& cell_size, amrex::Vector<amrex::Real>& stencil_coefs_r)
static amrex::Real ComputeMaxDt (amrex::Real const * const dx)
static amrex::IntVect GetMaxGuardCell ()
```

## `FiniteDifferenceSolver`

- file: `Source/FieldSolver/FiniteDifferenceSolver/FiniteDifferenceSolver.H`
- kind: `class`
- public methods: 64; public members: 0; private members: 14

```cpp
FiniteDifferenceSolver ( ElectromagneticSolverAlgo fdtd_algo, std::array<amrex::Real,3> cell_size, ablastr::utils::enums::GridType grid_type )
void EvolveB ( ablastr::fields::MultiFabRegister& fields, int lev, PatchType patch_type, std::array< std::unique_ptr<amrex::iMultiFab>, 3 >& flag_info_cell, std::array< std::unique_ptr<amrex::LayoutData<FaceInfoBox> >, 3 >& borrowing, amrex::Real dt )
void EvolveE ( ablastr::fields::MultiFabRegister & fields, int lev, PatchType patch_type, ablastr::fields::VectorField const& Efield, std::array< std::unique_ptr<amrex::iMultiFab>,3 > const& eb_update_E, amrex::Real dt )
void EvolveF ( amrex::MultiFab* Ffield, ablastr::fields::VectorField const& Efield, amrex::MultiFab* rhofield, int rho_comp, amrex::Real dt )
void EvolveG (amrex::MultiFab* Gfield, ablastr::fields::VectorField const& Bfield, amrex::Real dt)
void EvolveECTRho ( ablastr::fields::VectorField const& Efield, ablastr::fields::VectorField const& edge_lengths, ablastr::fields::VectorField const& face_areas, ablastr::fields::VectorField const& ECTRhofield, int lev )
void ApplySilverMuellerBoundary ( ablastr::fields::VectorField & Efield, ablastr::fields::VectorField & Bfield, amrex::Box domain_box, amrex::Real dt, amrex::Array<FieldBoundaryType,AMREX_SPACEDIM> field_boundary_lo, amrex::Array<FieldBoundaryType,AMREX_SPACEDIM> field_boundary_hi)
void ComputeDivE ( ablastr::fields::VectorField const & Efield, amrex::MultiFab& divE )
void MacroscopicEvolveE ( ablastr::fields::MultiFabRegister& fields, MacroscopicSolverAlgo macroscopic_solver_algo, ablastr::fields::VectorField const& Efield, ablastr::fields::VectorField const& Bfield, ablastr::fields::VectorField const& Jfield, std::array< std::unique_ptr<amrex::iMultiFab>,3 > const& eb_update_E, amrex::Real dt, std::unique_ptr<MacroscopicProperties> const& macroscopic_properties)
void EvolveBPML ( ablastr::fields::MultiFabRegister& fields, PatchType patch_type, int level, amrex::Real dt, bool dive_cleaning )
void EvolveEPML ( ablastr::fields::MultiFabRegister& fields, PatchType patch_type, int level, std::array< std::unique_ptr<amrex::iMultiFab>, 3 > const& eb_update_E, MultiSigmaBox const& sigba, amrex::Real dt, bool pml_has_particles )
void EvolveFPML ( amrex::MultiFab* Ffield, ablastr::fields::VectorField Efield, amrex::Real dt )
void HybridPICSolveE ( ablastr::fields::VectorField const& Efield, ablastr::fields::VectorField & Jfield, ablastr::fields::VectorField const& Jifield, ablastr::fields::VectorField const& Bfield, amrex::MultiFab const& rhofield, amrex::MultiFab const& Pefield, std::array< std::unique_ptr<amrex::iMultiFab>,3> const& eb_update_E, int lev, HybridPICModel const* hybrid_model, bool solve_for_Faraday )
void CalculateCurrentAmpere ( ablastr::fields::VectorField& Jfield, ablastr::fields::VectorField const& Bfield, std::array< std::unique_ptr<amrex::iMultiFab>,3> const& eb_update_E, int lev )
void ComputeCurlA ( ablastr::fields::VectorField& Bfield, ablastr::fields::VectorField const& Afield, std::array< std::unique_ptr<amrex::iMultiFab>,3> const& eb_update_B, int lev, amrex::IntVect const& ngrow = amrex::IntVect::TheZeroVector() )
void ComputeCurlB ( ablastr::fields::VectorField& Efield, ablastr::fields::VectorField const& Bfield, std::array< std::unique_ptr<amrex::iMultiFab>,3> const& eb_update_E, int lev, amrex::IntVect const& ngrow = amrex::IntVect::TheZeroVector() )
void ComputeGradient ( ablastr::fields::VectorField& out_field, ablastr::fields::ScalarField const& in_field, std::array< std::unique_ptr<amrex::iMultiFab>,3> const& eb_update, int lev )
void ComputeLaplacian ( ablastr::fields::ScalarField& out_field, ablastr::fields::ScalarField const& in_field, std::array< std::unique_ptr<amrex::iMultiFab>,3> const& eb_update, int lev )
void ComputeVectorLaplacian ( ablastr::fields::VectorField& out_field, ablastr::fields::VectorField const& in_field, std::array< std::unique_ptr<amrex::iMultiFab>,3> const& eb_update, int lev )
void ComputeVectorBiLaplacian ( ablastr::fields::VectorField& out_field, ablastr::fields::VectorField const& in_field, std::array< std::unique_ptr<amrex::iMultiFab>,3> const& eb_update, int lev )
void EvolveBCylindrical ( ablastr::fields::VectorField const& Bfield, ablastr::fields::VectorField const& Efield, int lev, amrex::Real dt )
void EvolveECylindrical ( ablastr::fields::VectorField const& Efield, ablastr::fields::VectorField const& Bfield, ablastr::fields::VectorField const& Jfield, std::array< std::unique_ptr<amrex::iMultiFab>,3 > const& eb_update_E, amrex::MultiFab const* Ffield, int lev, amrex::Real dt )
void EvolveFCylindrical ( amrex::MultiFab* Ffield, ablastr::fields::VectorField const & Efield, amrex::MultiFab* rhofield, int rho_comp, amrex::Real dt )
void ComputeDivECylindrical ( ablastr::fields::VectorField const & Efield, amrex::MultiFab& divE )
void HybridPICSolveECylindrical ( ablastr::fields::VectorField const& Efield, ablastr::fields::VectorField const& Jfield, ablastr::fields::VectorField const& Jifield, ablastr::fields::VectorField const& Bfield, amrex::MultiFab const& rhofield, amrex::MultiFab const& Pefield, std::array< std::unique_ptr<amrex::iMultiFab>,3> const& eb_update_E, int lev, HybridPICModel const* hybrid_model, bool solve_for_Faraday )
void CalculateCurrentAmpereCylindrical ( ablastr::fields::VectorField& Jfield, ablastr::fields::VectorField const& Bfield, std::array< std::unique_ptr<amrex::iMultiFab>,3> const& eb_update_E, int lev )
void ComputeCurlACylindrical ( ablastr::fields::VectorField& Bfield, ablastr::fields::VectorField const& Afield, std::array< std::unique_ptr<amrex::iMultiFab>,3> const& eb_update_B, int lev, amrex::IntVect const& ngrow )
void ComputeCurlBCylindrical ( ablastr::fields::VectorField& Efield, ablastr::fields::VectorField const& Bfield, std::array< std::unique_ptr<amrex::iMultiFab>,3> const& eb_update_E, int lev, amrex::IntVect const& ngrow )
void ComputeGradientCylindrical ( ablastr::fields::VectorField& out_field, ablastr::fields::ScalarField const& in_field, std::array< std::unique_ptr<amrex::iMultiFab>,3> const& eb_update, int lev )
void ComputeLaplacianCylindrical ( ablastr::fields::ScalarField& out_field, ablastr::fields::ScalarField const& in_field, std::array< std::unique_ptr<amrex::iMultiFab>,3> const& eb_update, int lev )
void ComputeVectorLaplacianCylindrical ( ablastr::fields::VectorField& out_field, ablastr::fields::VectorField const& in_field, std::array< std::unique_ptr<amrex::iMultiFab>,3> const& eb_update, int lev )
void ComputeVectorBiLaplacianCylindrical ( ablastr::fields::VectorField& out_field, ablastr::fields::VectorField const& in_field, std::array< std::unique_ptr<amrex::iMultiFab>,3> const& eb_update, int lev )
void EvolveBSpherical ( ablastr::fields::VectorField const& Bfield, ablastr::fields::VectorField const& Efield, int lev, amrex::Real dt )
void EvolveESpherical ( ablastr::fields::VectorField const& Efield, ablastr::fields::VectorField const& Bfield, ablastr::fields::VectorField const& Jfield, amrex::MultiFab const* Ffield, int lev, amrex::Real dt )
void EvolveFSpherical ( amrex::MultiFab* Ffield, ablastr::fields::VectorField const & Efield, amrex::MultiFab* rhofield, int rho_comp, amrex::Real dt )
void ComputeDivESpherical ( ablastr::fields::VectorField const & Efield, amrex::MultiFab& divE )
void HybridPICSolveESpherical ( ablastr::fields::VectorField const& Efield, ablastr::fields::VectorField const& Jfield, ablastr::fields::VectorField const& Jifield, ablastr::fields::VectorField const& Bfield, amrex::MultiFab const& rhofield, amrex::MultiFab const& Pefield, int lev, HybridPICModel const* hybrid_model, bool solve_for_Faraday )
void CalculateCurrentAmpereSpherical ( ablastr::fields::VectorField& Jfield, ablastr::fields::VectorField const& Bfield, int lev )
void ComputeCurlASpherical ( ablastr::fields::VectorField& Bfield, ablastr::fields::VectorField const& Afield, std::array< std::unique_ptr<amrex::iMultiFab>,3> const& eb_update_B, int lev, amrex::IntVect const& ngrow )
void ComputeCurlBSpherical ( ablastr::fields::VectorField& Efield, ablastr::fields::VectorField const& Bfield, std::array< std::unique_ptr<amrex::iMultiFab>,3> const& eb_update_E, int lev, amrex::IntVect const& ngrow )
void ComputeGradientSpherical ( ablastr::fields::VectorField& out_field, ablastr::fields::ScalarField const& in_field, std::array< std::unique_ptr<amrex::iMultiFab>,3> const& eb_update, int lev )
void ComputeLaplacianSpherical ( ablastr::fields::ScalarField& out_field, ablastr::fields::ScalarField const& in_field, std::array< std::unique_ptr<amrex::iMultiFab>,3> const& eb_update, int lev )
void ComputeVectorLaplacianSpherical ( ablastr::fields::VectorField& out_field, ablastr::fields::VectorField const& in_field, std::array< std::unique_ptr<amrex::iMultiFab>,3> const& eb_update, int lev )
void ComputeVectorBiLaplacianSpherical ( ablastr::fields::VectorField& out_field, ablastr::fields::VectorField const& in_field, std::array< std::unique_ptr<amrex::iMultiFab>,3> const& eb_update, int lev )
void EvolveBCartesian ( ablastr::fields::VectorField const& Bfield, ablastr::fields::VectorField const& Efield, amrex::MultiFab const * Gfield, int lev, amrex::Real dt )
void EvolveECartesian ( ablastr::fields::VectorField const& Efield, ablastr::fields::VectorField const& Bfield, ablastr::fields::VectorField const& Jfield, std::array< std::unique_ptr<amrex::iMultiFab>,3 > const& eb_update_E, amrex::MultiFab const* Ffield, int lev, amrex::Real dt )
void EvolveECartesianECT ( ablastr::fields::MultiFabRegister& fields, ablastr::fields::VectorField const& Efield, ablastr::fields::VectorField const& Bfield, ablastr::fields::VectorField const& Jfield, std::array< std::unique_ptr<amrex::iMultiFab>,3 > const& eb_update_E, int lev, amrex::Real dt, std::unique_ptr<MacroscopicProperties> const& macroscopic_properties = nullptr )
void EvolveFCartesian ( amrex::MultiFab* Ffield, ablastr::fields::VectorField Efield, amrex::MultiFab* rhofield, int rho_comp, amrex::Real dt )
void EvolveGCartesian ( amrex::MultiFab* Gfield, ablastr::fields::VectorField const& Bfield, amrex::Real dt)
void EvolveRhoCartesianECT ( ablastr::fields::VectorField const& Efield, ablastr::fields::VectorField const& edge_lengths, ablastr::fields::VectorField const& face_areas, ablastr::fields::VectorField const& ECTRhofield, int lev)
void EvolveBCartesianECT ( ablastr::fields::VectorField const& Bfield, ablastr::fields::VectorField const& face_areas, ablastr::fields::VectorField const& area_mod, ablastr::fields::VectorField const& ECTRhofield, ablastr::fields::VectorField const& Venl, std::array< std::unique_ptr<amrex::iMultiFab>, 3 >& flag_info_cell, std::array< std::unique_ptr<amrex::LayoutData<FaceInfoBox> >, 3 >& borrowing, int lev, amrex::Real dt )
void ComputeDivECartesian ( ablastr::fields::VectorField const & Efield, amrex::MultiFab& divE )
void MacroscopicEvolveECartesian ( ablastr::fields::VectorField const& Efield, ablastr::fields::VectorField const& Bfield, ablastr::fields::VectorField const& Jfield, std::array< std::unique_ptr<amrex::iMultiFab>,3 > const& eb_update_E, amrex::Real dt, std::unique_ptr<MacroscopicProperties> const& macroscopic_properties)
void EvolveBPMLCartesian ( std::array< amrex::MultiFab*, 3 > Bfield, ablastr::fields::VectorField Efield, amrex::Real dt, bool dive_cleaning)
void EvolveEPMLCartesian ( ablastr::fields::VectorField Efield, std::array< amrex::MultiFab*, 3 > Bfield, std::array< amrex::MultiFab*, 3 > Jfield, std::array< std::unique_ptr<amrex::iMultiFab>, 3 > const& eb_update_E, amrex::MultiFab* Ffield, MultiSigmaBox const& sigba, amrex::Real dt, bool pml_has_particles )
void EvolveFPMLCartesian ( amrex::MultiFab* Ffield, ablastr::fields::VectorField Efield, amrex::Real dt )
void HybridPICSolveECartesian ( ablastr::fields::VectorField const& Efield, ablastr::fields::VectorField const& Jfield, ablastr::fields::VectorField const& Jifield, ablastr::fields::VectorField const& Bfield, amrex::MultiFab const& rhofield, amrex::MultiFab const& Pefield, std::array< std::unique_ptr<amrex::iMultiFab>,3> const& eb_update_E, int lev, HybridPICModel const* hybrid_model, bool solve_for_Faraday )
void CalculateCurrentAmpereCartesian ( ablastr::fields::VectorField& Jfield, ablastr::fields::VectorField const& Bfield, std::array< std::unique_ptr<amrex::iMultiFab>,3> const& eb_update_E, int lev )
void ComputeCurlACartesian ( ablastr::fields::VectorField & Bfield, ablastr::fields::VectorField const& Afield, std::array< std::unique_ptr<amrex::iMultiFab>,3> const& eb_update_B, int lev, amrex::IntVect const& ngrow )
void ComputeCurlBCartesian ( ablastr::fields::VectorField & Efield, ablastr::fields::VectorField const& Bfield, std::array< std::unique_ptr<amrex::iMultiFab>,3> const& eb_update_E, int lev, amrex::IntVect const& ngrow )
void ComputeGradientCartesian ( ablastr::fields::VectorField& out_field, ablastr::fields::ScalarField const& in_field, std::array< std::unique_ptr<amrex::iMultiFab>,3> const& eb_update, int lev )
void ComputeLaplacianCartesian ( ablastr::fields::ScalarField& out_field, ablastr::fields::ScalarField const& in_field, std::array< std::unique_ptr<amrex::iMultiFab>,3> const& eb_update, int lev )
void ComputeVectorLaplacianCartesian ( ablastr::fields::VectorField& out_field, ablastr::fields::VectorField const& in_field, std::array< std::unique_ptr<amrex::iMultiFab>,3> const& eb_update, int lev )
void ComputeVectorBiLaplacianCartesian ( ablastr::fields::VectorField& out_field, ablastr::fields::VectorField const& in_field, std::array< std::unique_ptr<amrex::iMultiFab>,3> const& eb_update, int lev )
```

## `ExternalVectorPotential`

- file: `Source/FieldSolver/FiniteDifferenceSolver/HybridPICModel/ExternalVectorPotential.H`
- kind: `class`
- public methods: 7; public members: 0; private members: 0

```cpp
ExternalVectorPotential ()
void ReadParameters ()
void AllocateLevelMFs ( ablastr::fields::MultiFabRegister & fields, int lev, const amrex::BoxArray& ba, const amrex::DistributionMapping& dm, int ncomps, const amrex::IntVect& ngEB, const amrex::IntVect& Ex_nodal_flag, const amrex::IntVect& Ey_nodal_flag, const amrex::IntVect& Ez_nodal_flag, const amrex::IntVect& Bx_nodal_flag, const amrex::IntVect& By_nodal_flag, const amrex::IntVect& Bz_nodal_flag )
void InitData ()
void CalculateExternalCurlA ()
void CalculateExternalCurlA (std::string& coil_name)
void UpdateHybridExternalFields ( amrex::Real t, amrex::Real dt )
```

## `HybridPICModel`

- file: `Source/FieldSolver/FiniteDifferenceSolver/HybridPICModel/HybridPICModel.H`
- kind: `class`
- public methods: 39; public members: 58; private members: 0

```cpp
HybridPICModel ()
~HybridPICModel ()
HybridPICModel ( HybridPICModel const & ) = delete
HybridPICModel& operator= ( HybridPICModel const & ) = delete
HybridPICModel ( HybridPICModel && ) = delete
HybridPICModel& operator= ( HybridPICModel && ) = delete
void ReadParameters ()
void AllocateLevelMFs ( ablastr::fields::MultiFabRegister & fields, int lev, const amrex::BoxArray& ba, const amrex::DistributionMapping& dm, int ncomps, const amrex::IntVect& ngJ, const amrex::IntVect& ngRho, const amrex::IntVect& ngEB, const amrex::IntVect& jx_nodal_flag, const amrex::IntVect& jy_nodal_flag, const amrex::IntVect& jz_nodal_flag, const amrex::IntVect& rho_nodal_flag, const amrex::IntVect& Ex_nodal_flag, const amrex::IntVect& Ey_nodal_flag, const amrex::IntVect& Ez_nodal_flag, const amrex::IntVect& Bx_nodal_flag, const amrex::IntVect& By_nodal_flag, const amrex::IntVect& Bz_nodal_flag ) const
void InitData (const ablastr::fields::MultiFabRegister& fields)
void GetCurrentExternal ()
void CalculatePlasmaCurrent ( ablastr::fields::MultiLevelVectorField const& Bfield, amrex::Vector<std::array< std::unique_ptr<amrex::iMultiFab>,3 > >& eb_update_E ) const
void CalculatePlasmaCurrent ( ablastr::fields::VectorField const& Bfield, std::array< std::unique_ptr<amrex::iMultiFab>,3 >& eb_update_E, int lev ) const
void HybridPICSolveE ( ablastr::fields::MultiLevelVectorField const& Efield, ablastr::fields::MultiLevelVectorField const& Jfield, ablastr::fields::MultiLevelVectorField const& Bfield, ablastr::fields::MultiLevelScalarField const& rhofield, amrex::Vector<std::array< std::unique_ptr<amrex::iMultiFab>,3 > >& eb_update_E, bool solve_for_Faraday) const
void HybridPICSolveE ( ablastr::fields::VectorField const& Efield, ablastr::fields::VectorField const& Jfield, ablastr::fields::VectorField const& Bfield, amrex::MultiFab const& rhofield, std::array< std::unique_ptr<amrex::iMultiFab>,3 >& eb_update_E, int lev, bool solve_for_Faraday) const
void HybridPICSolveE ( ablastr::fields::VectorField const& Efield, ablastr::fields::VectorField const& Jfield, ablastr::fields::VectorField const& Bfield, amrex::MultiFab const& rhofield, std::array< std::unique_ptr<amrex::iMultiFab>,3 >& eb_update_E, int lev, PatchType patch_type, bool solve_for_Faraday) const
void BfieldEvolve ( ablastr::fields::MultiLevelVectorField const& Bfield, ablastr::fields::MultiLevelVectorField const& Efield, ablastr::fields::MultiLevelVectorField const& Jfield, ablastr::fields::MultiLevelScalarField const& rhofield, amrex::Vector<std::array< std::unique_ptr<amrex::iMultiFab>,3 > >& eb_update_E, int step, amrex::Real dt_half, SubcyclingHalf subcycling_half, amrex::IntVect ng, std::optional<bool> nodal_sync)
void BfieldEvolve ( ablastr::fields::MultiLevelVectorField const& Bfield, ablastr::fields::MultiLevelVectorField const& Efield, ablastr::fields::MultiLevelVectorField const& Jfield, ablastr::fields::MultiLevelScalarField const& rhofield, amrex::Vector<std::array< std::unique_ptr<amrex::iMultiFab>,3 > >& eb_update_E, int step, amrex::Real dt_half, int lev, SubcyclingHalf subcycling_half, amrex::IntVect ng, std::optional<bool> nodal_sync)
void BfieldEvolveRK4 ( ablastr::fields::MultiLevelVectorField const& Bfield, ablastr::fields::MultiLevelVectorField const& Efield, ablastr::fields::MultiLevelVectorField const& Jfield, ablastr::fields::MultiLevelScalarField const& rhofield, amrex::Vector<std::array< std::unique_ptr<amrex::iMultiFab>,3 > >& eb_update_E, std::array<amrex::MultiFab, 3>& B_old, amrex::Real dt, int lev, SubcyclingHalf subcycling_half, amrex::IntVect ng, std::optional<bool> nodal_sync)
amrex::Real BfieldEvolveRKF45 ( ablastr::fields::MultiLevelVectorField const& Bfield, ablastr::fields::MultiLevelVectorField const& Efield, ablastr::fields::MultiLevelVectorField const& Jfield, ablastr::fields::MultiLevelScalarField const& rhofield, amrex::Vector<std::array< std::unique_ptr<amrex::iMultiFab>,3 > >& eb_update_E, std::array<amrex::MultiFab, 3>& B_old, amrex::Real dt, int lev, SubcyclingHalf subcycling_half, amrex::IntVect ng, std::optional<bool> nodal_sync)
void FieldPush ( ablastr::fields::MultiLevelVectorField const& Bfield, ablastr::fields::MultiLevelVectorField const& Efield, ablastr::fields::MultiLevelVectorField const& Jfield, ablastr::fields::MultiLevelScalarField const& rhofield, amrex::Vector<std::array< std::unique_ptr<amrex::iMultiFab>,3 > >& eb_update_E, amrex::Real dt, SubcyclingHalf subcycling_half, amrex::IntVect ng, std::optional<bool> nodal_sync)
void CalculateElectronPressure (bool floor_density = false) const
void CalculateElectronPressure (int lev, bool floor_density = false) const
void CalculateElectronFluidVelocity () const
void CalculateElectronFluidVelocity (int lev) const
void CalculateIonFluidVelocity () const
void CalculateIonFluidVelocity (int lev) const
void FillElectronPressureMF ( amrex::MultiFab& Pe_field, amrex::MultiFab& Te_field, amrex::MultiFab const& rho_field, bool floor_density = false ) const
void QDSMCInitializeUe (int lev) const
void QDSMCInitializeKe (int lev) const
void QDSMCUpdateTe (int lev) const
void QDSMCAddJouleHeating (int lev, amrex::Real dt, amrex::MultiFab * redirect_E = nullptr) const
void QDSMCAddTemperatureRelaxation (int lev, amrex::Real dt, std::map<std::string, amrex::MultiFab*> const & Ti_dep_by_species) const
void QDSMCApplyIonHeating (int lev, amrex::Real dt, amrex::MultiFab const * redirect_E = nullptr, std::map<std::string, amrex::MultiFab*> const * Ti_dep_by_species = nullptr) const
void QDSMCFillElectronPressureFromTe ( ablastr::fields::MultiLevelScalarField const& rho) const
void QDSMCFillElectronPressureFromTe (int lev, amrex::MultiFab const& rho) const
void AdvanceElectronEnergyQDSMC (amrex::Real dt) const
void ComputeResistiveOverlay () const
void ComputeResistiveOverlay (int lev) const
[[nodiscard]] bool DoRKF45(int step) const
```

Public members:

```cpp
bool m_evolve_initialized = false
int m_substeps = 10
ablastr::utils::text::IntervalsParser m_rkf45_intervals
amrex::Real m_substep_rtol = 1.e-4
amrex::Real m_substep_atol = 1.e-8
amrex::Real m_substep_safety = 0.9
amrex::Real m_substep_max_growth = 5.0
int m_max_substep_attempts = 250
bool m_holmstrom_vacuum_region = false
VacuumSeamSwitchMode m_vacuum_seam_switch_mode = VacuumSeamSwitchMode::Edge
amrex::Real m_elec_temp
amrex::Real m_n0_ref = 1.0
amrex::Real m_gamma = 5.0/3.0
bool m_solve_electron_energy_equation = false
bool m_include_joule_heating = false
bool m_joule_redirect_to_ions = false
amrex::Real m_joule_redirect_Te_eV = -1.0
bool m_include_temperature_relaxation = false
std::string m_nu_ei_expression = "0.0"
std::unique_ptr<amrex::Parser> m_nu_ei_parser
amrex::ParserExecutor<4> m_nu_ei
amrex::Real m_n_floor = 1.0
std::string m_eta_expression = "0.0"
std::unique_ptr<amrex::Parser> m_resistivity_parser
amrex::ParserExecutor<3> m_eta
bool m_resistivity_has_J_dependence = false
std::map<std::string, std::unique_ptr<amrex::Parser>> m_per_species_resistivity_parser
std::map<std::string, amrex::ParserExecutor<7>> m_eta_per_species
bool m_has_per_species_eta = false
bool m_has_resistive_drag = false
std::set<std::string> m_resistive_drag_species
mutable bool m_qdsmc_J_plasma_valid = false
bool m_need_fluid_velocities = false
bool m_need_per_species_fields = false
std::string m_eta_h_expression = "0.0"
std::unique_ptr<amrex::Parser> m_hyper_resistivity_parser
amrex::ParserExecutor<2> m_eta_h
bool m_include_hyper_resistivity_term = false
bool m_hyper_resistivity_has_B_dependence = false
std::string m_Jx_ext_grid_function = "0.0"
```

## `BackwardEulerAlgo`

- file: `Source/FieldSolver/FiniteDifferenceSolver/MacroscopicProperties/MacroscopicProperties.H`
- kind: `struct`
- public methods: 0; public members: 0; private members: 0

## `LaxWendroffAlgo`

- file: `Source/FieldSolver/FiniteDifferenceSolver/MacroscopicProperties/MacroscopicProperties.H`
- kind: `struct`
- public methods: 0; public members: 0; private members: 0

## `MacroscopicProperties`

- file: `Source/FieldSolver/FiniteDifferenceSolver/MacroscopicProperties/MacroscopicProperties.H`
- kind: `class`
- public methods: 38; public members: 17; private members: 42

```cpp
MacroscopicProperties ()
void ReadParameters ()
void AllocateLevelMFs ( const amrex::BoxArray& ba, const amrex::DistributionMapping& dm, const amrex::IntVect& ng_EB_alloc, const amrex::IntVect& Ex_nodal, const amrex::IntVect& Ey_nodal, const amrex::IntVect& Ez_nodal )
void InitData ( const amrex::Geometry& geom, const amrex::IntVect& Ex_stag, const amrex::IntVect& Ey_stag, const amrex::IntVect& Ez_stag)
amrex::MultiFab& getsigma_mf ()
amrex::MultiFab& getepsilon_mf ()
amrex::MultiFab& get_eps_node_mf (int c)
amrex::MultiFab& get_sigma_m1_node_mf (int c)
amrex::MultiFab& get_sigma_m2_node_mf (int c)
amrex::MultiFab& get_sigma_node_mf (int c)
amrex::MultiFab& get_eps_inv_node_mf (int c)
amrex::MultiFab& get_j0_node_mf (int c)
amrex::MultiFab& get_j1_node_mf (int c)
amrex::MultiFab& get_jcond_mf (int c)
amrex::MultiFab& getmu_mf ()
amrex::MultiFab& geteps_inv_x_mf ()
amrex::MultiFab& geteps_inv_y_mf ()
amrex::MultiFab& geteps_inv_z_mf ()
bool has_subpixel_rows () const
amrex::MultiFab& get_sigma_m1_mf ()
amrex::MultiFab& get_sigma_m2_mf ()
bool has_sigma_matrices () const
amrex::Real get_sigma_dt () const
bool rho_medium_seeded () const
void set_rho_medium_seeded ()
int& gauss_check_step ()
bool has_interface () const
bool has_eps_tensor () const
bool has_eps2_tensor () const
bool has_sigma_tensor () const
amrex::MultiFab& get_sigma_tensor_mf ()
bool has_sigma_tensor_mf () const
amrex::MultiFab& get_eps_tensor_mf ()
amrex::MultiFab& get_eps2_tensor_mf ()
bool has_eps_tensor_mf () const
bool has_eps2_tensor_mf () const
void InitSubpixelRows ( const amrex::Geometry& geom, const amrex::EBFArrayBoxFactory* eb_fact, const amrex::IntVect& Ex_stag, const amrex::IntVect& Ey_stag, const amrex::IntVect& Ez_stag, amrex::Real dt )
void InitializeMacroMultiFabUsingParser (amrex::MultiFab *macro_mf, amrex::ParserExecutor<3> const& macro_parser, const amrex::GpuArray<amrex::Real, AMREX_SPACEDIM>& dx_lev, const amrex::RealBox& prob_domain_lev)
```

Public members:

```cpp
amrex::Real m_epsilon2 = 0.0
bool m_has_interface = false
bool m_has_eps_tensor = false
bool m_has_eps2_tensor = false
bool m_has_sig_tensor = false
std::string m_str_interface_function
std::unique_ptr<amrex::Parser> m_interface_parser
amrex::GpuArray<amrex::Real,6> m_eps_tensor
amrex::GpuArray<amrex::Real,6> m_eps2_tensor
amrex::GpuArray<amrex::Real,6> m_sig_tensor
amrex::GpuArray<int, 3> sigma_IndexType
amrex::GpuArray<int, 3> epsilon_IndexType
amrex::GpuArray<int, 3> mu_IndexType
amrex::GpuArray<int, 3> Ex_IndexType
amrex::GpuArray<int, 3> Ey_IndexType
amrex::GpuArray<int, 3> Ez_IndexType
amrex::GpuArray<int, 3> macro_cr_ratio
```

## `Mat3`

- file: `Source/FieldSolver/FiniteDifferenceSolver/MacroscopicProperties/SubpixelTensor.H`
- kind: `struct`
- public methods: 0; public members: 1; private members: 0

Public members:

```cpp
amrex::Real m[9]
```

## `DarwinLinearFieldOperator`

- file: `Source/FieldSolver/ImplicitSolvers/DarwinLinearFieldOperator.H`
- kind: `class`, bases: `public LinearFunction<WarpXSolverVec,SemiImplicitDarwin>`
- public methods: 16; public members: 0; private members: 9

```cpp
DarwinLinearFieldOperator () = default
~DarwinLinearFieldOperator () override = default
DarwinLinearFieldOperator (const DarwinLinearFieldOperator&) = delete
DarwinLinearFieldOperator& operator= (const DarwinLinearFieldOperator&) = delete
DarwinLinearFieldOperator (DarwinLinearFieldOperator&&) = delete
DarwinLinearFieldOperator& operator= (DarwinLinearFieldOperator&&) = delete
void apply ( WarpXSolverVec& a_Ax, const WarpXSolverVec& a_x ) override
inline void precond ( WarpXSolverVec& a_U, const WarpXSolverVec& a_X ) override
inline void updatePreCondMat () override
inline void printParameters () const
inline void getPCMatrix ( amrex::Gpu::DeviceVector<int>& a_ridx_g, amrex::Gpu::DeviceVector<int>& a_nnz, amrex::Gpu::DeviceVector<int>& a_cidx_g, amrex::Gpu::DeviceVector<RT>& a_aij, int& a_n, int& a_ncols_max ) override
[[nodiscard]] WarpXSolverVec makeVecLHS () const override
[[nodiscard]] WarpXSolverVec makeVecRHS () const override
[[nodiscard]] inline bool isDefined () const
void define ( const WarpXSolverVec& a_U, SemiImplicitDarwin* a_ops, const PreconditionerType& a_pc_type ) override
[[nodiscard]] inline PreconditionerType pcType () const override
```

## `ImplicitOptions`

- file: `Source/FieldSolver/ImplicitSolvers/ImplicitOptions.H`
- kind: `struct`
- public methods: 1; public members: 13; private members: 0

```cpp
amrex::ParticleReal particle_tolerance = static_cast<amrex::ParticleReal>(1.0e-10)
```

Public members:

```cpp
int max_particle_iterations = 21
bool print_unconverged_particle_details = false
bool use_mass_matrices_jacobian = false
bool use_mass_matrices_pc = false
bool linear_stage_of_jfnk = false
bool evolve_suborbit_particles_only = false
amrex::IntVect ncomp_xx = amrex::IntVect
amrex::IntVect ncomp_yy = amrex::IntVect
amrex::IntVect ncomp_zz = amrex::IntVect
amrex::IntVect ncomp_pc_xx = amrex::IntVect
amrex::IntVect ncomp_pc_yy = amrex::IntVect
amrex::IntVect ncomp_pc_zz = amrex::IntVect
int particle_shape
```

## `ImplicitSolver`

- file: `Source/FieldSolver/ImplicitSolvers/ImplicitSolver.H`
- kind: `class`
- public methods: 40; public members: 0; private members: 0

```cpp
ImplicitSolver() = default
virtual ~ImplicitSolver() = default
ImplicitSolver(const ImplicitSolver&) = delete
ImplicitSolver& operator=(const ImplicitSolver&) = delete
ImplicitSolver(ImplicitSolver&&) = delete
ImplicitSolver& operator=(ImplicitSolver&&) = delete
virtual void Define (WarpX* a_WarpX, bool from_restart) = 0
[[nodiscard]] bool IsDefined () const
virtual void PrintParameters () const = 0
void CreateParticleAttributes () const
bool DoParticleSuborbits () const
virtual void SetupStep(amrex::Real )
virtual int DoSolve(amrex::Real a_start_time, int a_step, bool verbose_step) = 0
virtual void ResetStep(amrex::Real )
virtual void FinishStep(amrex::Real , int )
void ReinitializePETScIfDue (int a_step, bool verbose_step)
[[nodiscard]] ImplicitOptions const& GetImplicitOptions () const
virtual int OneStep (amrex::Real start_time, amrex::Real a_dt, int a_step, bool verbose_step)
virtual void ComputeRHS (WarpXSolverVec& a_RHS, const WarpXSolverVec& a_E, amrex::Real a_time, int a_nl_iter, bool a_from_jacobian, bool update_pc = true) = 0
void PreLinearSolve (bool update_pc = true)
[[nodiscard]] int numAMRLevels () const
[[nodiscard]] const amrex::Geometry& GetGeometry (int) const
[[nodiscard]] const amrex::Array<FieldBoundaryType,AMREX_SPACEDIM>& GetFieldBoundaryLo () const
[[nodiscard]] const amrex::Array<FieldBoundaryType,AMREX_SPACEDIM>& GetFieldBoundaryHi () const
[[nodiscard]] amrex::Array<amrex::LinOpBCType,AMREX_SPACEDIM> GetLinOpBCLo () const
[[nodiscard]] amrex::Array<amrex::LinOpBCType,AMREX_SPACEDIM> GetLinOpBCHi () const
[[nodiscard]] const amrex::Vector<amrex::Array<amrex::MultiFab*,3>>* GetMassMatricesCoeff() const
[[nodiscard]] static std::unique_ptr<MatrixSolverOps<WarpXSolverVec>> MakePCMatrixOps (ImplicitSolver* a_ops)
[[nodiscard]] virtual const amrex::MultiFab* GetCurl2BCmask (const int lev, const int dir) const
int GetMassMatricesPCnComp (const int field_dir, const int space_dir) const
[[nodiscard]] virtual amrex::Real GetThetaForPC () const = 0
void AssemblePCMatrix ()
void GetPCMatrix (amrex::Gpu::DeviceVector<int>& a_r_indices_g, amrex::Gpu::DeviceVector<int>& a_num_nz, amrex::Gpu::DeviceVector<int>& a_c_indices_g, amrex::Gpu::DeviceVector<amrex::Real>& a_a_ij, int& a_n, int& a_ncols_max)
void ApplyMassMatrices ( ablastr::fields::MultiLevelVectorField& a_out, const ablastr::fields::MultiLevelVectorField& a_in, const ablastr::fields::MultiLevelVectorField* a_in_ref = nullptr, const ablastr::fields::MultiLevelVectorField* a_baseline = nullptr, amrex::Real a_scale = 1.0, bool a_zero_out_first = false )
[[nodiscard]] amrex::IntVect MassMatricesStencilHalfWidth () const
void AssertMassMatricesStencilNotClipped ( const ablastr::fields::MultiLevelVectorField& a_out, const ablastr::fields::MultiLevelVectorField& a_in ) const
void ComputeJfromMassMatrices (bool a_J_from_MM_only)
void FinishMassMatricesDeposition ()
void SyncMassMatricesPCAndApplyBCs ()
void FinishImplicitParticleUpdate (amrex::Real time, int step)
```

## `MassMatrixSolverOps`

- file: `Source/FieldSolver/ImplicitSolvers/MassMatrixSolverOps.H`
- kind: `class`, bases: `public MatrixSolverOps<T>`
- public methods: 15; public members: 0; private members: 9

```cpp
MassMatrixSolverOps () = default
~MassMatrixSolverOps () override = default
MassMatrixSolverOps (const MassMatrixSolverOps&) = delete
MassMatrixSolverOps& operator= (const MassMatrixSolverOps&) = delete
MassMatrixSolverOps (MassMatrixSolverOps&&) noexcept = delete
MassMatrixSolverOps& operator= (MassMatrixSolverOps&&) noexcept = delete
void Define (Ops* a_ops)
void Update () override
void InitialGuess (T& a_x, const T& a_b) override
void JacobiSweep (T& a_x, const T& a_b, RT a_omega) override
[[nodiscard]] RT ResidualNorm (const T& a_x, const T& a_b) override
[[nodiscard]] RT EstimateOmega () const override
[[nodiscard]] bool HasOffDiag () const override
[[nodiscard]] bool IsDefined () const override
RT computeResidualNormInternal (const T& a_x, const T& a_b) const
```

## `SemiImplicitDarwin`

- file: `Source/FieldSolver/ImplicitSolvers/SemiImplicitDarwin.H`
- kind: `class`, bases: `public ImplicitSolver`
- public methods: 21; public members: 0; private members: 12

```cpp
SemiImplicitDarwin() = default
~SemiImplicitDarwin() override = default
SemiImplicitDarwin(const SemiImplicitDarwin&) = delete
SemiImplicitDarwin& operator=(const SemiImplicitDarwin&) = delete
SemiImplicitDarwin(SemiImplicitDarwin&&) = delete
SemiImplicitDarwin& operator=(SemiImplicitDarwin&&) = delete
void Define ( WarpX* a_WarpX, bool from_restart ) override
void PrintParameters () const override
int OneStep ( amrex::Real start_time, amrex::Real a_dt, int a_step, bool verbose_step) override
int DoSolve (amrex::Real , int , bool ) override
void ComputeRHS ( WarpXSolverVec& a_RHS, const WarpXSolverVec& a_Z, amrex::Real start_time, int a_nl_iter, bool a_from_jacobian, bool update_pc = true ) override
[[nodiscard]] WarpX* GetWarpX () const
void ApplyScaledMassMatrices ( ablastr::fields::MultiLevelVectorField& rhs, const ablastr::fields::MultiLevelVectorField& dA)
void ComputeScaledMassMatrixCC ( amrex::MultiFab& a_chi_xx_cc, amrex::MultiFab& a_chi_yy_cc, amrex::MultiFab& a_chi_zz_cc ) const
void PrepareVelocitiesForCurrentDeposition ()
void AccumulateCurrentAndMassMatrices ()
void CalculateSourceVector ()
void ComputeInductiveEfromdA( int astep )
void ClearParticleVelocities ()
void FinishVelocityUpdate ()
[[nodiscard]] amrex::Real GetThetaForPC () const override
```

## `SemiImplicitEM`

- file: `Source/FieldSolver/ImplicitSolvers/SemiImplicitEM.H`
- kind: `class`, bases: `public ImplicitSolver`
- public methods: 14; public members: 0; private members: 2

```cpp
SemiImplicitEM() = default
~SemiImplicitEM() override = default
SemiImplicitEM(const SemiImplicitEM&) = delete
SemiImplicitEM& operator=(const SemiImplicitEM&) = delete
SemiImplicitEM(SemiImplicitEM&&) = delete
SemiImplicitEM& operator=(SemiImplicitEM&&) = delete
void Define (WarpX* a_WarpX, bool a_from_restart) override
void PrintParameters () const override
void SetupStep (amrex::Real start_time) override
int DoSolve (amrex::Real start_time, int a_step, bool verbose_step) override
void ResetStep (amrex::Real start_time) override
void FinishStep (amrex::Real start_time, int a_step) override
void ComputeRHS (WarpXSolverVec& a_RHS, const WarpXSolverVec& a_E, amrex::Real start_time, int a_nl_iter, bool a_from_jacobian, bool update_pc = true) override
[[nodiscard]] amrex::Real GetThetaForPC () const override
```

## `SparseJacobianMatrix`

- file: `Source/FieldSolver/ImplicitSolvers/SparseJacobianMatrix.H`
- kind: `class`
- public methods: 14; public members: 0; private members: 11

```cpp
SparseJacobianMatrix () = default
~SparseJacobianMatrix () = default
SparseJacobianMatrix (const SparseJacobianMatrix&) = delete
SparseJacobianMatrix& operator= (const SparseJacobianMatrix&) = delete
SparseJacobianMatrix (SparseJacobianMatrix&&) noexcept = delete
SparseJacobianMatrix& operator= (SparseJacobianMatrix&&) noexcept = delete
void Define (int a_ndofs_l, int a_ndofs_g)
[[nodiscard]] bool IsDefined () const
void Update (const WarpXSolverDOF* a_dofs, const amrex::Vector<amrex::Geometry>& a_geom, amrex::Real a_theta_dt, const amrex::Vector<amrex::Array<const amrex::MultiFab*,3>>& a_bc_masks, const amrex::Vector<amrex::Array<amrex::MultiFab*,3>>* a_mass_matrices, const amrex::Array<amrex::IntVect,3>& a_mm_ncomp)
void GetMatrix (amrex::Gpu::DeviceVector<int>& a_r_indices_g, amrex::Gpu::DeviceVector<int>& a_num_nz, amrex::Gpu::DeviceVector<int>& a_c_indices_g, amrex::Gpu::DeviceVector<amrex::Real>& a_a_ij, int& a_n, int& a_ncols_max) const
void printParameters () const
int Assemble (const WarpXSolverDOF* a_dofs, const amrex::Vector<amrex::Geometry>& a_geom, amrex::Real a_theta_dt, const amrex::Vector<amrex::Array<const amrex::MultiFab*,3>>& a_bc_masks, const amrex::Vector<amrex::Array<amrex::MultiFab*,3>>* a_mass_matrices, const amrex::Array<amrex::IntVect,3>& a_mm_ncomp)
[[nodiscard]] int nLocalRows () const
[[nodiscard]] int nnzMax () const
```

## `StrangImplicitSpectralEM`

- file: `Source/FieldSolver/ImplicitSolvers/StrangImplicitSpectralEM.H`
- kind: `class`, bases: `public ImplicitSolver`
- public methods: 12; public members: 0; private members: 2

```cpp
StrangImplicitSpectralEM() = default
~StrangImplicitSpectralEM() override = default
StrangImplicitSpectralEM(const StrangImplicitSpectralEM&) = delete
StrangImplicitSpectralEM& operator=(const StrangImplicitSpectralEM&) = delete
StrangImplicitSpectralEM(StrangImplicitSpectralEM&&) = delete
StrangImplicitSpectralEM& operator=(StrangImplicitSpectralEM&&) = delete
void Define (WarpX* a_WarpX, bool a_from_restart) override
void PrintParameters () const override
int DoSolve (amrex::Real , int , bool ) override
int OneStep (amrex::Real start_time, amrex::Real a_dt, int a_step, bool verbose_step) override
void ComputeRHS (WarpXSolverVec& a_RHS, const WarpXSolverVec& a_E, amrex::Real start_time, int a_nl_iter, bool a_from_jacobian, bool update_pc = true) override
[[nodiscard]] amrex::Real GetThetaForPC () const override
```

## `ThetaImplicitEM`

- file: `Source/FieldSolver/ImplicitSolvers/ThetaImplicitEM.H`
- kind: `class`, bases: `public ImplicitSolver`
- public methods: 16; public members: 0; private members: 2

```cpp
ThetaImplicitEM() = default
~ThetaImplicitEM() override = default
ThetaImplicitEM(const ThetaImplicitEM&) = delete
ThetaImplicitEM& operator=(const ThetaImplicitEM&) = delete
ThetaImplicitEM(ThetaImplicitEM&&) = delete
ThetaImplicitEM& operator=(ThetaImplicitEM&&) = delete
void Define (WarpX* a_WarpX, bool from_restart) override
void PrintParameters () const override
void SetupStep (amrex::Real start_time) override
int DoSolve (amrex::Real start_time, int a_step, bool verbose_step) override
void ResetStep (amrex::Real ) override
void FinishStep (amrex::Real start_time, int a_step) override
void ComputeRHS (WarpXSolverVec& a_RHS, const WarpXSolverVec& a_E, amrex::Real start_time, int a_nl_iter, bool a_from_jacobian, bool update_pc = true) override
[[nodiscard]] amrex::Real GetThetaForPC () const override
[[nodiscard]] const amrex::MultiFab* GetCurl2BCmask (int lev, int field_dir) const override
void InitializeCurlCurlBCMasks ()
```

## `WarpXSolverDOF`

- file: `Source/FieldSolver/ImplicitSolvers/WarpXSolverDOF.H`
- kind: `struct`
- public methods: 3; public members: 7; private members: 0

```cpp
void Define ( WarpX* , int, const std::string&, const std::string&)
void fill_local_dof (amrex::iMultiFab& dof, amrex::iMultiFab const& mask)
void fill_global_dof ()
```

Public members:

```cpp
amrex::Vector<std::array<std::unique_ptr<amrex::iMultiFab>,3>> m_array
amrex::Vector<std::unique_ptr<amrex::iMultiFab>> m_scalar
warpx::fields::FieldType m_array_type = warpx::fields::FieldType::None
warpx::fields::FieldType m_scalar_type = warpx::fields::FieldType::None
amrex::Long m_nDoFs_l = 0
amrex::Long m_nDoFs_g = 0
amrex::Vector<amrex::Long> m_row_offsets
```

## `WarpXSolverVec`

- file: `Source/FieldSolver/ImplicitSolvers/WarpXSolverVec.H`
- kind: `class`
- public methods: 38; public members: 0; private members: 11

```cpp
WarpXSolverVec() = default
WarpXSolverVec(const WarpXSolverVec&) = delete
~WarpXSolverVec()
[[nodiscard]] inline bool IsDefined () const
void Define ( WarpX* a_WarpX, const std::string& a_vector_type_name, const std::string& a_scalar_type_name = "none" )
inline void Define ( const WarpXSolverVec& a_solver_vec )
[[nodiscard]] RT dotProduct( const WarpXSolverVec& a_X ) const
void Copy ( warpx::fields::FieldType a_array_type, warpx::fields::FieldType a_scalar_type = warpx::fields::FieldType::None, bool allow_type_mismatch = false)
inline void Copy ( const WarpXSolverVec& a_solver_vec )
WarpXSolverVec& operator= ( const WarpXSolverVec& a_solver_vec ) = delete
WarpXSolverVec(WarpXSolverVec&&) noexcept = default
WarpXSolverVec& operator= ( WarpXSolverVec&& a_solver_vec ) noexcept
inline void operator+= ( const WarpXSolverVec& a_solver_vec )
inline void operator-= (const WarpXSolverVec& a_solver_vec)
inline void linComb (const RT a, const WarpXSolverVec& X, const RT b, const WarpXSolverVec& Y)
void increment (const WarpXSolverVec& X, const RT a)
inline void scale (RT a_a)
inline void zero ()
inline void setVal ( const RT a_val )
inline void assertIsDefined( const WarpXSolverVec& a_solver_vec ) const
inline void assertSameType( const WarpXSolverVec& a_solver_vec ) const
[[nodiscard]] inline RT norm2 () const
[[nodiscard]] const ablastr::fields::MultiLevelVectorField& getArrayVec() const
ablastr::fields::MultiLevelVectorField& getArrayVec()
[[nodiscard]] const ablastr::fields::MultiLevelScalarField& getScalarVec() const
ablastr::fields::MultiLevelScalarField& getScalarVec()
[[nodiscard]] warpx::fields::FieldType getArrayVecType () const
[[nodiscard]] warpx::fields::FieldType getScalarVecType () const
[[nodiscard]] std::string getVectorType () const
[[nodiscard]] std::string getScalarType () const
[[nodiscard]] amrex::Long nDOF_local () const
[[nodiscard]] amrex::Long nDOF_global () const
[[nodiscard]] const amrex::Vector<amrex::Long>& dofRowOffsets () const
void copyFrom (const amrex::Real*)
void copyTo (amrex::Real*) const
[[nodiscard]] auto getWarpX () const
[[nodiscard]] auto numAMRLevels () const
[[nodiscard]] inline static const auto& getDOFsObject ()
```

## `EBCalcBfromVectorPotentialPerLevel`

- file: `Source/FieldSolver/MagnetostaticSolver/MagnetostaticSolver.H`
- kind: `class`
- public methods: 3; public members: 0; private members: 3

```cpp
EBCalcBfromVectorPotentialPerLevel (ablastr::fields::MultiLevelVectorField const & b_field, ablastr::fields::MultiLevelVectorField const & grad_buf_e_stag, ablastr::fields::MultiLevelVectorField const & grad_buf_b_stag) : m_b_field(b_field), m_grad_buf_e_stag(grad_buf_e_stag), m_grad_buf_b_stag(grad_buf_b_stag)
void operator() (amrex::Array<std::unique_ptr<amrex::MLMG>, 3> & mlmg, int lev)
void doInterp (amrex::MultiFab & src, amrex::MultiFab & dst)
```

## `VectorPoissonBoundaryHandler`

- file: `Source/FieldSolver/MagnetostaticSolver/MagnetostaticSolver.H`
- kind: `class`
- public methods: 1; public members: 4; private members: 0

```cpp
void defineVectorPotentialBCs ()
```

Public members:

```cpp
amrex::Array<amrex::Array<amrex::LinOpBCType, AMREX_SPACEDIM>, 3> lobc, hibc
bool bcs_set = false
std::array<std::array<bool, AMREX_SPACEDIM * 2>, 3> dirichlet_flag
bool has_non_periodic = false
```

## `PsatdAlgorithmComoving`

- file: `Source/FieldSolver/SpectralSolver/SpectralAlgorithms/PsatdAlgorithmComoving.H`
- kind: `class`, bases: `public SpectralBaseAlgorithm`
- public methods: 5; public members: 0; private members: 7

```cpp
PsatdAlgorithmComoving (const SpectralKSpace& spectral_kspace, const amrex::DistributionMapping& dm, const SpectralFieldIndex& spectral_index, int norder_x, int norder_y, int norder_z, ablastr::utils::enums::GridType grid_type, const amrex::Vector<amrex::Real>& v_comoving, amrex::Real dt, bool update_with_rho)
void pushSpectralFields (SpectralFieldData& f) const final
void InitializeSpectralCoefficients (const SpectralKSpace& spectral_kspace, const amrex::DistributionMapping& dm, amrex::Real dt)
void CurrentCorrection (SpectralFieldData& field_data) final
void VayDeposition (SpectralFieldData& field_data) final
```

## `PsatdAlgorithmGalilean`

- file: `Source/FieldSolver/SpectralSolver/SpectralAlgorithms/PsatdAlgorithmGalilean.H`
- kind: `class`, bases: `public SpectralBaseAlgorithm`
- public methods: 6; public members: 0; private members: 10

```cpp
PsatdAlgorithmGalilean ( const SpectralKSpace& spectral_kspace, const amrex::DistributionMapping& dm, const SpectralFieldIndex& spectral_index, int norder_x, int norder_y, int norder_z, ablastr::utils::enums::GridType grid_type, const amrex::Vector<amrex::Real>& v_galilean, amrex::Real dt, bool update_with_rho, bool time_averaging, bool dive_cleaning, bool divb_cleaning)
void pushSpectralFields (SpectralFieldData& f) const final
void InitializeSpectralCoefficients ( const SpectralKSpace& spectral_kspace, const amrex::DistributionMapping& dm, amrex::Real dt)
void InitializeSpectralCoefficientsAveraging ( const SpectralKSpace& spectral_kspace, const amrex::DistributionMapping& dm, amrex::Real dt)
void CurrentCorrection (SpectralFieldData& field_data) final
void VayDeposition (SpectralFieldData& field_data) final
```

## `PsatdAlgorithmGalileanRZ`

- file: `Source/FieldSolver/SpectralSolver/SpectralAlgorithms/PsatdAlgorithmGalileanRZ.H`
- kind: `class`, bases: `public SpectralBaseAlgorithmRZ`
- public methods: 5; public members: 0; private members: 6

```cpp
PsatdAlgorithmGalileanRZ (SpectralKSpaceRZ const & spectral_kspace, amrex::DistributionMapping const & dm, const SpectralFieldIndex& spectral_index, int n_rz_azimuthal_modes, int norder_z, ablastr::utils::enums::GridType grid_type, const amrex::Vector<amrex::Real>& v_galilean, amrex::Real dt_step, bool update_with_rho)
void pushSpectralFields (SpectralFieldDataRZ & f) final
void InitializeSpectralCoefficients (SpectralFieldDataRZ const & f)
void CurrentCorrection (SpectralFieldDataRZ& field_data) final
void VayDeposition (SpectralFieldDataRZ& field_data) final
```

## `PsatdAlgorithmJRhomFirstOrder`

- file: `Source/FieldSolver/SpectralSolver/SpectralAlgorithms/PsatdAlgorithmJRhomFirstOrder.H`
- kind: `class`, bases: `public SpectralBaseAlgorithm`
- public methods: 4; public members: 0; private members: 4

```cpp
PsatdAlgorithmJRhomFirstOrder ( const SpectralKSpace& spectral_kspace, const amrex::DistributionMapping& dm, const SpectralFieldIndex& spectral_index, int norder_x, int norder_y, int norder_z, ablastr::utils::enums::GridType grid_type, amrex::Real dt, bool div_cleaning, TimeDependencyJ time_dependency_J, TimeDependencyRho time_dependency_rho)
void pushSpectralFields (SpectralFieldData& f) const final
void CurrentCorrection (SpectralFieldData& field_data) final
void VayDeposition (SpectralFieldData& field_data) final
```

## `PsatdAlgorithmJRhomSecondOrder`

- file: `Source/FieldSolver/SpectralSolver/SpectralAlgorithms/PsatdAlgorithmJRhomSecondOrder.H`
- kind: `class`, bases: `public SpectralBaseAlgorithm`
- public methods: 6; public members: 0; private members: 9

```cpp
PsatdAlgorithmJRhomSecondOrder ( const SpectralKSpace& spectral_kspace, const amrex::DistributionMapping& dm, const SpectralFieldIndex& spectral_index, int norder_x, int norder_y, int norder_z, ablastr::utils::enums::GridType grid_type, amrex::Real dt, bool update_with_rho, bool time_averaging, bool dive_cleaning, bool divb_cleaning, TimeDependencyJ time_dependency_J, TimeDependencyRho time_dependency_rho )
void pushSpectralFields (SpectralFieldData& f) const final
void InitializeSpectralCoefficients ( const SpectralKSpace& spectral_kspace, const amrex::DistributionMapping& dm, amrex::Real dt)
void InitializeSpectralCoefficientsAveraging ( const SpectralKSpace& spectral_kspace, const amrex::DistributionMapping& dm, amrex::Real dt)
void CurrentCorrection (SpectralFieldData& field_data) final
void VayDeposition (SpectralFieldData& field_data) final
```

## `PsatdAlgorithmPml`

- file: `Source/FieldSolver/SpectralSolver/SpectralAlgorithms/PsatdAlgorithmPml.H`
- kind: `class`, bases: `public SpectralBaseAlgorithm`
- public methods: 5; public members: 0; private members: 10

```cpp
PsatdAlgorithmPml( const SpectralKSpace& spectral_kspace, const amrex::DistributionMapping& dm, const SpectralFieldIndex& spectral_index, int norder_x, int norder_y, int norder_z, ablastr::utils::enums::GridType grid_type, const amrex::Vector<amrex::Real>& v_galilean, amrex::Real dt, bool dive_cleaning, bool divb_cleaning)
void InitializeSpectralCoefficients( const SpectralKSpace& spectral_kspace, const amrex::DistributionMapping& dm)
void pushSpectralFields(SpectralFieldData& f) const final
void CurrentCorrection (SpectralFieldData& field_data) final
void VayDeposition (SpectralFieldData& field_data) final
```

## `PsatdAlgorithmPmlRZ`

- file: `Source/FieldSolver/SpectralSolver/SpectralAlgorithms/PsatdAlgorithmPmlRZ.H`
- kind: `class`, bases: `public SpectralBaseAlgorithmRZ`
- public methods: 5; public members: 0; private members: 3

```cpp
PsatdAlgorithmPmlRZ (SpectralKSpaceRZ const & spectral_kspace, amrex::DistributionMapping const & dm, const SpectralFieldIndex& spectral_index, int n_rz_azimuthal_modes, int norder_z, ablastr::utils::enums::GridType grid_type, amrex::Real dt_step)
void pushSpectralFields (SpectralFieldDataRZ & f) final
void InitializeSpectralCoefficients (SpectralFieldDataRZ const & f)
void CurrentCorrection (SpectralFieldDataRZ& field_data) final
void VayDeposition (SpectralFieldDataRZ& field_data) final
```

## `PsatdAlgorithmRZ`

- file: `Source/FieldSolver/SpectralSolver/SpectralAlgorithms/PsatdAlgorithmRZ.H`
- kind: `class`, bases: `public SpectralBaseAlgorithmRZ`
- public methods: 5; public members: 0; private members: 9

```cpp
PsatdAlgorithmRZ(SpectralKSpaceRZ const & spectral_kspace, amrex::DistributionMapping const & dm, const SpectralFieldIndex& spectral_index, int n_rz_azimuthal_modes, int norder_z, ablastr::utils::enums::GridType grid_type, amrex::Real dt_step, bool update_with_rho, bool time_averaging, TimeDependencyJ time_dependency_J, TimeDependencyRho time_dependency_rho, bool dive_cleaning, bool divb_cleaning)
void pushSpectralFields(SpectralFieldDataRZ & f) final
void InitializeSpectralCoefficients(SpectralFieldDataRZ const & f)
void CurrentCorrection (SpectralFieldDataRZ& field_data) final
void VayDeposition (SpectralFieldDataRZ& field_data) final
```

## `SpectralBaseAlgorithm`

- file: `Source/FieldSolver/SpectralSolver/SpectralAlgorithms/SpectralBaseAlgorithm.H`
- kind: `class`
- public methods: 9; public members: 0; private members: 0

```cpp
virtual void pushSpectralFields(SpectralFieldData& f) const = 0
virtual ~SpectralBaseAlgorithm() = default
SpectralBaseAlgorithm(const SpectralBaseAlgorithm&) = default
SpectralBaseAlgorithm& operator=(const SpectralBaseAlgorithm&) = default
SpectralBaseAlgorithm(SpectralBaseAlgorithm&&) = default
SpectralBaseAlgorithm& operator=(SpectralBaseAlgorithm&&) = default
virtual void CurrentCorrection (SpectralFieldData& field_data) = 0
virtual void VayDeposition (SpectralFieldData& field_data) = 0
void ComputeSpectralDivE ( int lev, SpectralFieldData& field_data, ablastr::fields::VectorField const& Efield, amrex::MultiFab& divE )
```

## `SpectralBaseAlgorithmRZ`

- file: `Source/FieldSolver/SpectralSolver/SpectralAlgorithms/SpectralBaseAlgorithmRZ.H`
- kind: `class`
- public methods: 9; public members: 0; private members: 0

```cpp
virtual void pushSpectralFields(SpectralFieldDataRZ & f) = 0
virtual ~SpectralBaseAlgorithmRZ() = default
SpectralBaseAlgorithmRZ ( SpectralBaseAlgorithmRZ const &) = default
SpectralBaseAlgorithmRZ& operator= ( SpectralBaseAlgorithmRZ const & ) = default
SpectralBaseAlgorithmRZ ( SpectralBaseAlgorithmRZ&& ) = default
SpectralBaseAlgorithmRZ& operator= ( SpectralBaseAlgorithmRZ&& ) = default
virtual void CurrentCorrection (SpectralFieldDataRZ& field_data) = 0
void ComputeSpectralDivE ( int lev, SpectralFieldDataRZ& field_data, ablastr::fields::VectorField const & Efield, amrex::MultiFab& divE )
virtual void VayDeposition (SpectralFieldDataRZ& field_data) = 0
```

## `SpectralBinomialFilter`

- file: `Source/FieldSolver/SpectralSolver/SpectralBinomialFilter.H`
- kind: `class`
- public methods: 5; public members: 0; private members: 0

```cpp
SpectralBinomialFilter () = default
void InitFilterArray (RealKVector const & kvec, amrex::Real dels, int npasses, bool compensation, KFilterArray & filter)
void InitFilterArray (RealKVector const & kr, RealKVector const & kz, amrex::RealVect dx, amrex::IntVect filter_npass_each_dir, bool compensation)
KFilterArray const & getFilterArrayR ()
KFilterArray const & getFilterArrayZ ()
```

## `SpectralFieldData`

- file: `Source/FieldSolver/SpectralSolver/SpectralFieldData.H`
- kind: `class`
- public methods: 9; public members: 1; private members: 5

```cpp
SpectralFieldData( const amrex::BoxArray& realspace_ba, const SpectralKSpace& k_space, const amrex::DistributionMapping& dm, int n_field_required, bool periodic_single_box)
SpectralFieldData() = default
~SpectralFieldData()
SpectralFieldData(const SpectralFieldData&) = delete
SpectralFieldData& operator=(const SpectralFieldData&) = delete
SpectralFieldData(SpectralFieldData&&) = default
SpectralFieldData& operator=(SpectralFieldData&& field_data) = default
void ForwardTransform (int lev, const amrex::MultiFab& mf, int field_index, int i_comp)
void BackwardTransform (int lev, amrex::MultiFab& mf, int field_index, const amrex::IntVect& fill_guards, int i_comp)
```

Public members:

```cpp
SpectralField fields
```

## `SpectralFieldIndex`

- file: `Source/FieldSolver/SpectralSolver/SpectralFieldData.H`
- kind: `class`
- public methods: 7; public members: 16; private members: 0

```cpp
SpectralFieldIndex (bool update_with_rho, bool time_averaging, TimeDependencyJ time_dependency_J, TimeDependencyRho time_dependency_rho, bool dive_cleaning, bool divb_cleaning, bool pml, bool pml_rz = false)
SpectralFieldIndex () = default
~SpectralFieldIndex () = default
SpectralFieldIndex ( SpectralFieldIndex const &) = default
SpectralFieldIndex& operator= ( SpectralFieldIndex const & ) = default
SpectralFieldIndex ( SpectralFieldIndex&& ) = default
SpectralFieldIndex& operator= ( SpectralFieldIndex&& ) = default
```

Public members:

```cpp
int n_fields
int Ex = -1, Ey = -1, Ez = -1
int Bx = -1, By = -1, Bz = -1
int divE = -1
int Ex_avg = -1, Ey_avg = -1, Ez_avg = -1
int Bx_avg = -1, By_avg = -1, Bz_avg = -1
int Jx_old = -1, Jy_old = -1, Jz_old = -1
int Jx_mid = -1, Jy_mid = -1, Jz_mid = -1
int Jx_new = -1, Jy_new = -1, Jz_new = -1
int rho_old = -1, rho_mid = -1, rho_new = -1
int F = -1, G = -1
int Exy = -1, Exz = -1, Eyx = -1, Eyz = -1, Ezx = -1, Ezy = -1
int Bxy = -1, Bxz = -1, Byx = -1, Byz = -1, Bzx = -1, Bzy = -1
int Exx = -1, Eyy = -1, Ezz = -1, Bxx = -1, Byy = -1, Bzz = -1
int Fx = -1, Fy = -1, Fz = -1, Gx = -1, Gy = -1, Gz = -1
int Er_pml = -1, Et_pml = -1, Br_pml = -1, Bt_pml = -1
```

## `SpectralFieldDataRZ`

- file: `Source/FieldSolver/SpectralSolver/SpectralFieldDataRZ.H`
- kind: `class`
- public methods: 20; public members: 3; private members: 8

```cpp
SpectralFieldDataRZ (int lev, const amrex::BoxArray& realspace_ba, const SpectralKSpaceRZ& k_space, const amrex::DistributionMapping& dm, int n_field_required, int n_modes)
SpectralFieldDataRZ () = default
~SpectralFieldDataRZ ()
SpectralFieldDataRZ ( SpectralFieldDataRZ const &) = delete
SpectralFieldDataRZ& operator= ( SpectralFieldDataRZ const & ) = delete
SpectralFieldDataRZ ( SpectralFieldDataRZ&& ) = default
SpectralFieldDataRZ& operator=(SpectralFieldDataRZ&& field_data) = default
void ForwardTransform (int lev, const amrex::MultiFab& mf, int field_index, int i_comp=0)
void ForwardTransform (int lev, const amrex::MultiFab& mf_r, int field_index_r, const amrex::MultiFab& mf_t, int field_index_t)
void BackwardTransform (int lev, amrex::MultiFab& mf, int field_index, int i_comp=0)
void BackwardTransform (int lev, amrex::MultiFab& mf_r, int field_index_r, amrex::MultiFab& mf_t, int field_index_t)
void FABZForwardTransform (amrex::MFIter const & mfi, amrex::Box const & realspace_bx, amrex::MultiFab const & tempHTransformedSplit, int field_index, bool is_nodal_z)
void FABZBackwardTransform (amrex::MFIter const & mfi, amrex::Box const & realspace_bx, int field_index, amrex::MultiFab & tempHTransformedSplit, bool is_nodal_z)
void CopySpectralDataComp (int src_comp, int dest_comp)
void ZeroOutDataComp(int icomp)
void ScaleDataComp(int icomp, amrex::Real scale_factor)
void InitFilter (amrex::IntVect const & filter_npass_each_dir, bool compensation, SpectralKSpaceRZ const & k_space)
void ApplyFilter (int lev, int field_index)
void ApplyFilter (int lev, int field_index1, int field_index2, int field_index3)
HankelTransform::RealVector const & getKrArray () const
```

Public members:

```cpp
SpectralField fields
int n_rz_azimuthal_modes
int m_ncomps
```

## `HankelTransform`

- file: `Source/FieldSolver/SpectralSolver/SpectralHankelTransform/HankelTransform.H`
- kind: `class`
- public methods: 4; public members: 0; private members: 5

```cpp
HankelTransform(int hankel_order, int azimuthal_mode, int nr, amrex::Real rmax)
const RealVector & getSpectralWavenumbers()
void HankelForwardTransform(amrex::FArrayBox const& F, int F_icomp, amrex::FArrayBox & G, int G_icomp)
void HankelInverseTransform(amrex::FArrayBox const& G, int G_icomp, amrex::FArrayBox & F, int F_icomp)
```

## `SpectralHankelTransformer`

- file: `Source/FieldSolver/SpectralSolver/SpectralHankelTransform/SpectralHankelTransformer.H`
- kind: `class`
- public methods: 8; public members: 0; private members: 6

```cpp
SpectralHankelTransformer () = default
SpectralHankelTransformer (int nr, int n_rz_azimuthal_modes, amrex::Real rmax)
void ExtractKrArray ()
[[nodiscard]] HankelTransform::RealVector const & getKrArray () const
void PhysicalToSpectral_Scalar (amrex::FArrayBox const & F_physical, amrex::FArrayBox & G_spectral)
void PhysicalToSpectral_Vector (amrex::Box const & box, amrex::FArrayBox & F_r_physical, amrex::FArrayBox & F_t_physical, amrex::FArrayBox & G_p_spectral, amrex::FArrayBox & G_m_spectral)
void SpectralToPhysical_Scalar (amrex::FArrayBox const & G_spectral, amrex::FArrayBox & F_physical)
void SpectralToPhysical_Vector (amrex::Box const & box, amrex::FArrayBox const & G_p_spectral, amrex::FArrayBox const & G_m_spectral, amrex::FArrayBox & F_r_physical, amrex::FArrayBox & F_t_physical)
```

## `SpectralKSpace`

- file: `Source/FieldSolver/SpectralSolver/SpectralKSpace.H`
- kind: `class`
- public methods: 5; public members: 1; private members: 0

```cpp
SpectralKSpace() : dx(amrex::RealVect::Zero)
SpectralKSpace( const amrex::BoxArray& realspace_ba, const amrex::DistributionMapping& dm, amrex::RealVect realspace_dx )
KVectorComponent getKComponent( const amrex::DistributionMapping& dm, const amrex::BoxArray& realspace_ba, int i_dim, bool only_positive_k ) const
KVectorComponent getModifiedKComponent ( const amrex::DistributionMapping& dm, int i_dim, int n_order, ablastr::utils::enums::GridType grid_type) const
SpectralShiftFactor getSpectralShiftFactor( const amrex::DistributionMapping& dm, int i_dim, ShiftType shift_type ) const
```

Public members:

```cpp
amrex::BoxArray spectralspace_ba
```

## `SpectralKSpaceRZ`

- file: `Source/FieldSolver/SpectralSolver/SpectralKSpaceRZ.H`
- kind: `class`, bases: `public SpectralKSpace`
- public methods: 3; public members: 0; private members: 0

```cpp
SpectralKSpaceRZ(const amrex::BoxArray& realspace_ba, const amrex::DistributionMapping& dm, amrex::RealVect realspace_dx)
KVectorComponent const & getKzArray () const
amrex::RealVect const & getCellSize () const
```

## `SpectralSolver`

- file: `Source/FieldSolver/SpectralSolver/SpectralSolver.H`
- kind: `class`
- public methods: 10; public members: 2; private members: 2

```cpp
SpectralSolver (const amrex::BoxArray& realspace_ba, const amrex::DistributionMapping& dm, int norder_x, int norder_y, int norder_z, ablastr::utils::enums::GridType grid_type, const amrex::Vector<amrex::Real>& v_galilean, const amrex::Vector<amrex::Real>& v_comoving, amrex::RealVect dx, amrex::Real dt, bool pml, bool periodic_single_box, bool update_with_rho, bool fft_do_time_averaging, PSATDSolutionType psatd_solution_type, TimeDependencyJ time_dependency_J, TimeDependencyRho time_dependency_rho, bool dive_cleaning, bool divb_cleaning)
void ForwardTransform (int lev, const amrex::MultiFab& mf, int field_index, int i_comp = 0)
void BackwardTransform( int lev, amrex::MultiFab& mf, int field_index, const amrex::IntVect& fill_guards, int i_comp=0 )
void pushSpectralFields()
void ComputeSpectralDivE ( int lev, ablastr::fields::VectorField const & Efield, amrex::MultiFab& divE)
void CurrentCorrection ()
void VayDeposition ()
void CopySpectralDataComp (int src_comp, int dest_comp)
void ZeroOutDataComp (int icomp)
void ScaleDataComp (int icomp, amrex::Real scale_factor)
```

Public members:

```cpp
SpectralFieldIndex m_spectral_index
amrex::Real m_dt
```

## `SpectralSolverRZ`

- file: `Source/FieldSolver/SpectralSolver/SpectralSolverRZ.H`
- kind: `class`
- public methods: 15; public members: 2; private members: 4

```cpp
SpectralSolverRZ (int lev, amrex::BoxArray const & realspace_ba, amrex::DistributionMapping const & dm, int n_rz_azimuthal_modes, int norder_z, ablastr::utils::enums::GridType grid_type, const amrex::Vector<amrex::Real>& v_galilean, amrex::RealVect dx, amrex::Real dt, bool with_pml, bool update_with_rho, bool fft_do_time_averaging, TimeDependencyJ time_dependency_J, TimeDependencyRho time_dependency_rho, bool dive_cleaning, bool divb_cleaning)
void ForwardTransform (int lev, amrex::MultiFab const & field_mf, int field_index, int i_comp=0)
void ForwardTransform (int lev, amrex::MultiFab const & field_mf1, int field_index1, amrex::MultiFab const & field_mf2, int field_index2)
void BackwardTransform (int lev, amrex::MultiFab& field_mf, int field_index, int i_comp=0)
void BackwardTransform (int lev, amrex::MultiFab& field_mf1, int field_index1, amrex::MultiFab& field_mf2, int field_index2)
void pushSpectralFields (bool doing_pml=false)
void InitFilter (amrex::IntVect const & filter_npass_each_dir, bool compensation)
void ApplyFilter (int lev, int field_index)
void ApplyFilter (int lev, int field_index1, int field_index2, int field_index3)
void ComputeSpectralDivE (int lev, ablastr::fields::VectorField const & Efield, amrex::MultiFab& divE)
void CurrentCorrection ()
void VayDeposition ()
void CopySpectralDataComp (int src_comp, int dest_comp)
void ZeroOutDataComp (int icomp)
void ScaleDataComp (int icomp, amrex::Real scale_factor)
```

Public members:

```cpp
SpectralFieldIndex m_spectral_index
amrex::Real m_dt
```

