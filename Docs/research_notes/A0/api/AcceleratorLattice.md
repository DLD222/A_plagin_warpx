# A0 API catalog — AcceleratorLattice

<!-- SPDX-License-Identifier: BSD-3-Clause -->

> Generated from headers; 9 classes/structs. Signatures are parsed heuristically; check the header for exact defaults/templates.

## `AcceleratorLattice`

- file: `Source/AcceleratorLattice/AcceleratorLattice.H`
- kind: `class`
- public methods: 5; public members: 6; private members: 0

```cpp
AcceleratorLattice ()
void ReadLattice (std::string const & root_name, amrex::ParticleReal & z_location)
void InitElementFinder ( int lev, amrex::Real gamma_boost, const amrex::Vector<amrex::Real>& time, amrex::BoxArray const & ba, amrex::DistributionMapping const & dm)
void UpdateElementFinder (int lev, const amrex::Vector<amrex::Real>& time)
[[nodiscard]] LatticeElementFinderDevice GetFinderDeviceInstance (WarpXParIter const& a_pti, int a_offset, const amrex::Vector<amrex::Real>& dts) const
```

Public members:

```cpp
int m_level
bool m_lattice_defined = false
std::unique_ptr<amrex::LayoutData<LatticeElementFinder>> m_element_finder
Drift h_drift
HardEdgedQuadrupole h_quad
HardEdgedPlasmaLens h_plasmalens
```

## `LatticeElementFinder`

- file: `Source/AcceleratorLattice/LatticeElementFinder.H`
- kind: `struct`
- public methods: 5; public members: 8; private members: 0

```cpp
void InitElementFinder (int lev, amrex::Real gamma_boost, const amrex::Vector<amrex::Real>& time, amrex::MFIter const& a_mfi, AcceleratorLattice const& accelerator_lattice)
void AllocateIndices (AcceleratorLattice const& accelerator_lattice)
void UpdateIndices (int lev, amrex::MFIter const& a_mfi, AcceleratorLattice const& accelerator_lattice, const amrex::Vector<amrex::Real>& time)
[[nodiscard]] LatticeElementFinderDevice GetFinderDeviceInstance ( WarpXParIter const& a_pti, int a_offset, AcceleratorLattice const& accelerator_lattice, const amrex::Vector<amrex::Real>& dts) const
void setup_lattice_indices (amrex::Gpu::DeviceVector<amrex::ParticleReal> const & zs, amrex::Gpu::DeviceVector<amrex::ParticleReal> const & ze, amrex::Gpu::DeviceVector<int> & indices) const
```

Public members:

```cpp
int m_nz
amrex::Real m_zmin
amrex::Real m_dz
amrex::ParticleReal m_gamma_boost
amrex::ParticleReal m_uz_boost
amrex::Real m_time
amrex::Gpu::DeviceVector<int> d_quad_indices
amrex::Gpu::DeviceVector<int> d_plasmalens_indices
```

## `LatticeElementFinderDevice`

- file: `Source/AcceleratorLattice/LatticeElementFinder.H`
- kind: `struct`
- public methods: 1; public members: 16; private members: 0

```cpp
void InitLatticeElementFinderDevice (WarpXParIter const& a_pti, int a_offset, AcceleratorLattice const& accelerator_lattice, LatticeElementFinder const & h_finder, const amrex::Vector<amrex::Real>& dts)
```

Public members:

```cpp
bool m_initialized = false
int m_nz
amrex::Real m_zmin
amrex::Real m_dz
amrex::Real m_dt
amrex::ParticleReal m_gamma_boost
amrex::ParticleReal m_uz_boost
amrex::Real m_time
GetParticlePosition<PIdx> m_get_position
const amrex::ParticleReal* AMREX_RESTRICT m_ux = nullptr
const amrex::ParticleReal* AMREX_RESTRICT m_uy = nullptr
const amrex::ParticleReal* AMREX_RESTRICT m_uz = nullptr
HardEdgedQuadrupoleDevice d_quad
HardEdgedPlasmaLensDevice d_plasmalens
int const* d_quad_indices_arr = nullptr
int const* d_plasmalens_indices_arr = nullptr
```

## `Drift`

- file: `Source/AcceleratorLattice/LatticeElements/Drift.H`
- kind: `struct`, bases: `LatticeElementBase`
- public methods: 2; public members: 0; private members: 0

```cpp
Drift ()
void AddElement (amrex::ParmParse & pp_element, amrex::ParticleReal & z_location)
```

## `HardEdgedPlasmaLens`

- file: `Source/AcceleratorLattice/LatticeElements/HardEdgedPlasmaLens.H`
- kind: `struct`, bases: `LatticeElementBase`
- public methods: 4; public members: 4; private members: 0

```cpp
HardEdgedPlasmaLens ()
void AddElement (amrex::ParmParse & pp_element, amrex::ParticleReal & z_location)
void WriteToDevice ()
[[nodiscard]] HardEdgedPlasmaLensDevice GetDeviceInstance () const
```

Public members:

```cpp
std::vector<amrex::ParticleReal> h_dEdx
std::vector<amrex::ParticleReal> h_dBdx
amrex::Gpu::DeviceVector<amrex::ParticleReal> d_dEdx
amrex::Gpu::DeviceVector<amrex::ParticleReal> d_dBdx
```

## `HardEdgedPlasmaLensDevice`

- file: `Source/AcceleratorLattice/LatticeElements/HardEdgedPlasmaLens.H`
- kind: `struct`
- public methods: 1; public members: 5; private members: 0

```cpp
void InitHardEdgedPlasmaLensDevice (HardEdgedPlasmaLens const& h_plasmalens)
```

Public members:

```cpp
int nelements = 0
const amrex::ParticleReal* AMREX_RESTRICT d_zs_arr
const amrex::ParticleReal* AMREX_RESTRICT d_ze_arr
const amrex::ParticleReal* AMREX_RESTRICT d_dEdx_arr
const amrex::ParticleReal* AMREX_RESTRICT d_dBdx_arr
```

## `HardEdgedQuadrupole`

- file: `Source/AcceleratorLattice/LatticeElements/HardEdgedQuadrupole.H`
- kind: `struct`, bases: `LatticeElementBase`
- public methods: 4; public members: 4; private members: 0

```cpp
HardEdgedQuadrupole ()
void AddElement (amrex::ParmParse & pp_element, amrex::ParticleReal & z_location)
void WriteToDevice ()
[[nodiscard]] HardEdgedQuadrupoleDevice GetDeviceInstance () const
```

Public members:

```cpp
std::vector<amrex::ParticleReal> h_dEdx
std::vector<amrex::ParticleReal> h_dBdx
amrex::Gpu::DeviceVector<amrex::ParticleReal> d_dEdx
amrex::Gpu::DeviceVector<amrex::ParticleReal> d_dBdx
```

## `HardEdgedQuadrupoleDevice`

- file: `Source/AcceleratorLattice/LatticeElements/HardEdgedQuadrupole.H`
- kind: `struct`
- public methods: 1; public members: 5; private members: 0

```cpp
void InitHardEdgedQuadrupoleDevice (HardEdgedQuadrupole const& h_quad)
```

Public members:

```cpp
int nelements = 0
const amrex::ParticleReal* AMREX_RESTRICT d_zs_arr
const amrex::ParticleReal* AMREX_RESTRICT d_ze_arr
const amrex::ParticleReal* AMREX_RESTRICT d_dEdx_arr
const amrex::ParticleReal* AMREX_RESTRICT d_dBdx_arr
```

## `LatticeElementBase`

- file: `Source/AcceleratorLattice/LatticeElements/LatticeElementBase.H`
- kind: `struct`
- public methods: 3; public members: 6; private members: 0

```cpp
explicit LatticeElementBase (std::string const& element_name)
void AddElementBase(amrex::ParmParse & pp_element, amrex::ParticleReal & z_location)
void WriteToDeviceBase ()
```

Public members:

```cpp
std::string m_element_name
int nelements = 0
std::vector<amrex::ParticleReal> h_zs
std::vector<amrex::ParticleReal> h_ze
amrex::Gpu::DeviceVector<amrex::ParticleReal> d_zs
amrex::Gpu::DeviceVector<amrex::ParticleReal> d_ze
```

