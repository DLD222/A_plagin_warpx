# A0 API catalog — BoundaryConditions

<!-- SPDX-License-Identifier: BSD-3-Clause -->

> Generated from headers; 8 classes/structs. Signatures are parsed heuristically; check the header for exact defaults/templates.

## `PEC_Insulator`

- file: `Source/BoundaryConditions/PEC_Insulator.H`
- kind: `class`
- public methods: 10; public members: 0; private members: 40

```cpp
PEC_Insulator()
void ApplyPEC_InsulatortoEfield (std::array<amrex::MultiFab*, 3> Efield, amrex::Array<FieldBoundaryType,AMREX_SPACEDIM> const & field_boundary_lo, amrex::Array<FieldBoundaryType,AMREX_SPACEDIM> const & field_boundary_hi, amrex::IntVect const & ng_fieldgather, amrex::Geometry const & geom, int lev, PatchType patch_type, amrex::Vector<amrex::IntVect> const & ref_ratios, amrex::Real time, bool split_pml_field = false)
void ApplyPEC_InsulatortoBfield (std::array<amrex::MultiFab*, 3> Bfield, amrex::Array<FieldBoundaryType,AMREX_SPACEDIM> const & field_boundary_lo, amrex::Array<FieldBoundaryType,AMREX_SPACEDIM> const & field_boundary_hi, amrex::IntVect const & ng_fieldgather, amrex::Geometry const & geom, int lev, PatchType patch_type, amrex::Vector<amrex::IntVect> const & ref_ratios, amrex::Real time)
void ZeroParallelFieldInConductor (std::array<amrex::MultiFab*, 3> field, amrex::Array<FieldBoundaryType,AMREX_SPACEDIM> const & field_boundary_lo, amrex::Array<FieldBoundaryType,AMREX_SPACEDIM> const & field_boundary_hi, amrex::IntVect const & ng_fieldgather, amrex::Geometry const & geom, int lev, PatchType patch_type, amrex::Vector<amrex::IntVect> const & ref_ratios)
void ZeroParallelScalarInConductor (amrex::MultiFab* scalar, amrex::Array<FieldBoundaryType,AMREX_SPACEDIM> const & field_boundary_lo, amrex::Array<FieldBoundaryType,AMREX_SPACEDIM> const & field_boundary_hi, amrex::Geometry const & geom, int lev, PatchType patch_type, amrex::Vector<amrex::IntVect> const & ref_ratios)
void ApplyPEC_InsulatortoField (std::array<amrex::MultiFab*, 3> field, amrex::Array<FieldBoundaryType,AMREX_SPACEDIM> const & field_boundary_lo, amrex::Array<FieldBoundaryType,AMREX_SPACEDIM> const & field_boundary_hi, amrex::IntVect const & ng_fieldgather, amrex::Geometry const & geom, int lev, PatchType patch_type, amrex::Vector<amrex::IntVect> const & ref_ratios, amrex::Real time, bool split_pml_field, bool E_like, bool only_zero_parallel_field, amrex::Vector<int> const & set_Fx_lo, amrex::Vector<int> const & set_Fy_lo, amrex::Vector<int> const & set_Fz_lo, amrex::Vector<int> const & set_Fx_hi, amrex::Vector<int> const & set_Fy_hi, amrex::Vector<int> const & set_Fz_hi, amrex::Vector<amrex::ParserExecutor<3>> const & Fx_parsers_lo, amrex::Vector<amrex::ParserExecutor<3>> const & Fy_parsers_lo, amrex::Vector<amrex::ParserExecutor<3>> const & Fz_parsers_lo, amrex::Vector<amrex::ParserExecutor<3>> const & Fx_parsers_hi, amrex::Vector<amrex::ParserExecutor<3>> const & Fy_parsers_hi, amrex::Vector<amrex::ParserExecutor<3>> const & Fz_parsers_hi)
[[nodiscard]] int IsESet(int idim, int iside) const
[[nodiscard]] int IsBSet(int idim, int iside) const
[[nodiscard]] int IsESet(int idim, int iside, int ifield) const
[[nodiscard]] int IsBSet(int idim, int iside, int ifield) const
```

## `MultiSigmaBox`

- file: `Source/BoundaryConditions/PML.H`
- kind: `class`, bases: `public amrex::FabArray<SigmaBox>`
- public methods: 3; public members: 0; private members: 2

```cpp
MultiSigmaBox(const amrex::BoxArray& ba, const amrex::DistributionMapping& dm, const amrex::BoxArray* grid_ba, const amrex::Real* dx, const amrex::IntVect& ncell, const amrex::IntVect& delta, const amrex::Box& regular_domain, amrex::Real v_sigma_sb)
void ComputePMLFactorsB (const amrex::Real* dx, amrex::Real dt)
void ComputePMLFactorsE (const amrex::Real* dx, amrex::Real dt)
```

## `PML`

- file: `Source/BoundaryConditions/PML.H`
- kind: `class`
- public methods: 17; public members: 0; private members: 14

```cpp
PML (int lev, const amrex::BoxArray& ba, const amrex::DistributionMapping& dm, bool do_similar_dm_pml, const amrex::Geometry* geom, const amrex::Geometry* cgeom, int ncell, int delta, amrex::IntVect ref_ratio, amrex::Real dt, int nox_fft, int noy_fft, int noz_fft, ablastr::utils::enums::GridType grid_type, int do_moving_window, int pml_has_particles, int do_pml_in_domain, PSATDSolutionType psatd_solution_type, TimeDependencyJ time_dependency_J, TimeDependencyRho time_dependency_rho, bool do_pml_dive_cleaning, bool do_pml_divb_cleaning, const amrex::IntVect& fill_guards_fields, const amrex::IntVect& fill_guards_current, bool eb_enabled, int max_guard_EB, amrex::Real v_sigma_sb, ablastr::fields::MultiFabRegister& fields, amrex::IntVect do_pml_Lo = amrex::IntVect::TheUnitVector(), amrex::IntVect do_pml_Hi = amrex::IntVect::TheUnitVector())
void ComputePMLFactors (amrex::Real dt)
[[nodiscard]] const MultiSigmaBox& GetMultiSigmaBox_fp () const
[[nodiscard]] const MultiSigmaBox& GetMultiSigmaBox_cp () const
[[nodiscard]] std::array< std::unique_ptr<amrex::iMultiFab>, 3 > const & GetEBUpdateEFlag_fp () const
[[nodiscard]] std::array< std::unique_ptr<amrex::iMultiFab>, 3 > const & GetEBUpdateEFlag_cp () const
void PushPSATD (ablastr::fields::MultiFabRegister& fields, int lev)
void CopyJtoPMLs (ablastr::fields::MultiFabRegister& fields, int lev)
void Exchange (ablastr::fields::VectorField mf_pml, ablastr::fields::VectorField mf, const PatchType& patch_type, int do_pml_in_domain)
void Exchange (amrex::MultiFab* mf_pml, amrex::MultiFab* mf, const PatchType& patch_type, int do_pml_in_domain)
void CopyJtoPMLs ( ablastr::fields::MultiFabRegister& fields, PatchType patch_type, int lev )
void FillBoundary (ablastr::fields::VectorField mf_pml, PatchType patch_type, std::optional<bool> nodal_sync=std::nullopt)
void FillBoundary (amrex::MultiFab & mf_pml, PatchType patch_type, std::optional<bool> nodal_sync=std::nullopt)
[[nodiscard]] bool ok () const
void CheckPoint (ablastr::fields::MultiFabRegister& fields, const std::string& dir) const
void Restart (ablastr::fields::MultiFabRegister& fields, const std::string& dir)
static void Exchange (amrex::MultiFab& pml, amrex::MultiFab& reg, const amrex::Geometry& geom, int do_pml_in_domain)
```

## `Sigma`

- file: `Source/BoundaryConditions/PML.H`
- kind: `struct`, bases: `amrex::Gpu::DeviceVector<amrex::Real>`
- public methods: 2; public members: 1; private members: 0

```cpp
[[nodiscard]] int lo() const
[[nodiscard]] int hi() const
```

Public members:

```cpp
int m_lo, m_hi
```

## `SigmaBox`

- file: `Source/BoundaryConditions/PML.H`
- kind: `struct`
- public methods: 5; public members: 9; private members: 0

```cpp
SigmaBox (const amrex::Box& box, const amrex::BoxArray& grids, const amrex::Real* dx, const amrex::IntVect& ncell, const amrex::IntVect& delta, const amrex::Box& regdomain, amrex::Real v_sigma)
void define_single (const amrex::Box& regdomain, const amrex::IntVect& ncell, const amrex::Array<amrex::Real,AMREX_SPACEDIM>& fac, amrex::Real v_sigma)
void define_multiple (const amrex::Box& box, const amrex::BoxArray& grids, const amrex::IntVect& ncell, const amrex::Array<amrex::Real,AMREX_SPACEDIM>& fac, amrex::Real v_sigma)
void ComputePMLFactorsB (const amrex::Real* dx, amrex::Real dt)
void ComputePMLFactorsE (const amrex::Real* dx, amrex::Real dt)
```

Public members:

```cpp
SigmaVect sigma
SigmaVect sigma_cumsum
SigmaVect sigma_star
SigmaVect sigma_star_cumsum
SigmaVect sigma_fac
SigmaVect sigma_cumsum_fac
SigmaVect sigma_star_fac
SigmaVect sigma_star_cumsum_fac
amrex::Real v_sigma
```

## `SigmaBoxFactory`

- file: `Source/BoundaryConditions/PML.H`
- kind: `class`, bases: `public amrex::FabFactory<SigmaBox>`
- public methods: 11; public members: 0; private members: 6

```cpp
SigmaBoxFactory (const amrex::BoxArray* grid_ba, const amrex::Real* dx, const amrex::IntVect& ncell, const amrex::IntVect& delta, const amrex::Box& regular_domain, const amrex::Real v_sigma_sb) : m_grids
, m_dx(dx), m_ncell(ncell), m_delta(delta), m_regdomain(regular_domain), m_v_sigma_sb(v_sigma_sb)
~SigmaBoxFactory () override = default
SigmaBoxFactory (const SigmaBoxFactory&) = default
SigmaBoxFactory (SigmaBoxFactory&&) noexcept = default
SigmaBoxFactory () = delete
SigmaBoxFactory& operator= (const SigmaBoxFactory&) = delete
SigmaBoxFactory& operator= (SigmaBoxFactory&&) = delete
[[nodiscard]] SigmaBox* create (const amrex::Box& box, int , const amrex::FabInfo& , int ) const final
void destroy (SigmaBox* fab) const final
[[nodiscard]] SigmaBoxFactory* clone () const final
```

## `PMLComp`

- file: `Source/BoundaryConditions/PMLComponent.H`
- kind: `struct`
- public methods: 0; public members: 1; private members: 0

Public members:

```cpp
enum
```

## `PML_RZ`

- file: `Source/BoundaryConditions/PML_RZ.H`
- kind: `class`
- public methods: 7; public members: 0; private members: 3

```cpp
PML_RZ (int lev, amrex::BoxArray const& grid_ba, amrex::DistributionMapping const& grid_dm, amrex::Geometry const* geom, ablastr::fields::MultiFabRegister& fields, int ncell, int do_pml_in_domain)
void ApplyDamping(amrex::MultiFab* Et_fp, amrex::MultiFab* Ez_fp, amrex::MultiFab* Bt_fp, amrex::MultiFab* Bz_fp, amrex::Real dt, ablastr::fields::MultiFabRegister& fields)
void PushPSATD (int lev, ablastr::fields::MultiFabRegister& fields, SpectralSolverRZ& spec_solver)
void FillBoundaryE (ablastr::fields::MultiFabRegister& fields, PatchType patch_type, bool do_single_precision_comms, std::optional<bool> nodal_sync=std::nullopt)
void FillBoundaryB (ablastr::fields::MultiFabRegister& fields, PatchType patch_type, bool do_single_precision_comms, std::optional<bool> nodal_sync=std::nullopt)
void CheckPoint (ablastr::fields::MultiFabRegister& fields, std::string const& dir) const
void Restart (ablastr::fields::MultiFabRegister& fields, std::string const& dir)
```

