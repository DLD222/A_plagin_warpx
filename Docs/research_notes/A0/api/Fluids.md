# A0 API catalog — Fluids

<!-- SPDX-License-Identifier: BSD-3-Clause -->

> Generated from headers; 5 classes/structs. Signatures are parsed heuristically; check the header for exact defaults/templates.

## `MomentFluidSolver`

- file: `Source/Fluids/MomentFluidSolver.H`
- kind: `class`
- public methods: 5; public members: 0; private members: 7

```cpp
MomentFluidSolver () = default
static bool IsEnabled ()
void AdvanceAndDeposit (amrex::Real dt, amrex::MultiFab* J[3], amrex::Geometry const & geom, int lev)
amrex::Real GaussResidual (amrex::MultiFab const & Ex, amrex::MultiFab const & Ez, amrex::Geometry const & geom) const
bool Initialized () const
```

## `MultiFluidContainer`

- file: `Source/Fluids/MultiFluidContainer.H`
- kind: `class`
- public methods: 14; public members: 0; private members: 2

```cpp
MultiFluidContainer ()
~MultiFluidContainer() = default
MultiFluidContainer (MultiFluidContainer const &) = delete
MultiFluidContainer& operator= (MultiFluidContainer const & ) = delete
MultiFluidContainer(MultiFluidContainer&& ) = default
MultiFluidContainer& operator=(MultiFluidContainer&& ) = default
[[nodiscard]] WarpXFluidContainer& GetFluidContainer (int ispecies) const
std::unique_ptr<WarpXFluidContainer>& GetUniqueContainer(int ispecies)
void AllocateLevelMFs (ablastr::fields::MultiFabRegister& m_fields, const amrex::BoxArray& ba, const amrex::DistributionMapping& dm, int lev)
void InitData ( ablastr::fields::MultiFabRegister& m_fields, amrex::Box init_box, amrex::Real cur_time, int lev, const amrex::Geometry& geom_lev, amrex::Real gamma_boost, amrex::Real beta_boost)
void Evolve (ablastr::fields::MultiFabRegister& fields, int lev, std::string const& current_fp_string, amrex::Real cur_time, bool skip_deposition=false)
[[nodiscard]] int nSpecies() const
void DepositCharge (ablastr::fields::MultiFabRegister& m_fields, amrex::MultiFab &rho, int lev)
void DepositCurrent (ablastr::fields::MultiFabRegister& m_fields, amrex::MultiFab& jx, amrex::MultiFab& jy, amrex::MultiFab& jz, int lev)
```

## `QdsmcPIdx`

- file: `Source/Fluids/QdsmcParticleContainer.H`
- kind: `struct`
- public methods: 0; public members: 1; private members: 0

Public members:

```cpp
enum
```

## `QdsmcParticleContainer`

- file: `Source/Fluids/QdsmcParticleContainer.H`
- kind: `class`, bases: `public amrex::ParticleContainerPureSoA<QdsmcPIdx::nattribs, 0>`
- public methods: 13; public members: 4; private members: 0

```cpp
explicit QdsmcParticleContainer (amrex::AmrCore* amr_core)
~QdsmcParticleContainer() override = default
QdsmcParticleContainer ( QdsmcParticleContainer const &) = delete
QdsmcParticleContainer& operator= ( QdsmcParticleContainer const & ) = delete
QdsmcParticleContainer ( QdsmcParticleContainer&& ) = default
QdsmcParticleContainer& operator= ( QdsmcParticleContainer&& ) = default
void InitParticles (int lev)
void SetV (int lev, const amrex::MultiFab & Ux, const amrex::MultiFab & Uy, const amrex::MultiFab & Uz)
void SetK (int lev, const amrex::MultiFab & Kfield, const amrex::MultiFab & rhofield)
void PushX (int lev, amrex::Real dt)
void DepositK (int lev, amrex::MultiFab & Kfield)
void DepositField (int lev, amrex::MultiFab & Field)
void ResetParticles (int lev)
```

Public members:

```cpp
static constexpr int NStructReal = 0
static constexpr int NStructInt = 0
static constexpr int NReal = QdsmcPIdx::nattribs
static constexpr int NInt = 0
```

## `WarpXFluidContainer`

- file: `Source/Fluids/WarpXFluidContainer.H`
- kind: `class`
- public methods: 18; public members: 2; private members: 0

```cpp
WarpXFluidContainer (int ispecies, const std::string &name)
~WarpXFluidContainer() = default
WarpXFluidContainer (WarpXFluidContainer const &) = delete
WarpXFluidContainer& operator= (WarpXFluidContainer const & ) = delete
WarpXFluidContainer(WarpXFluidContainer&& ) = default
WarpXFluidContainer& operator=(WarpXFluidContainer&& ) = default
void AllocateLevelMFs (ablastr::fields::MultiFabRegister& m_fields, const amrex::BoxArray& ba, const amrex::DistributionMapping& dm, int lev) const
void InitData ( ablastr::fields::MultiFabRegister& m_fields, amrex::Box init_box, amrex::Real cur_time, int lev, const amrex::Geometry& geom_lev, amrex::Real gamma_boost, amrex::Real beta_boost)
void ReadParameters ()
void Evolve (ablastr::fields::MultiFabRegister& fields, int lev, const std::string& current_fp_string, amrex::Real cur_time, bool skip_deposition=false)
void AdvectivePush_Muscl (ablastr::fields::MultiFabRegister& m_fields, int lev)
void ApplyBcFluidsAndComms (ablastr::fields::MultiFabRegister& m_fields, int lev)
void centrifugal_source_rz (ablastr::fields::MultiFabRegister& m_fields, int lev)
void GatherAndPush (ablastr::fields::MultiFabRegister& m_fields, const amrex::MultiFab& Ex, const amrex::MultiFab& Ey, const amrex::MultiFab& Ez, const amrex::MultiFab& Bx, const amrex::MultiFab& By, const amrex::MultiFab& Bz, amrex::Real t, int lev)
void DepositCurrent (ablastr::fields::MultiFabRegister& m_fields, amrex::MultiFab& jx, amrex::MultiFab& jy, amrex::MultiFab& jz, int lev)
void DepositCharge (ablastr::fields::MultiFabRegister& m_fields, amrex::MultiFab &rho, int lev, int icomp = 0)
[[nodiscard]] amrex::Real getCharge () const
[[nodiscard]] amrex::Real getMass () const
```

Public members:

```cpp
std::string name_mf_N = "fluid_density_"+species_name
std::string name_mf_NU = "fluid_momentum_density_"+species_name
```

