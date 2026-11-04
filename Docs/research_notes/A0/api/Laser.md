# A0 API catalog — Laser

<!-- SPDX-License-Identifier: BSD-3-Clause -->

> Generated; 5 classes/structs. Signatures are heuristic.

## `CommonLaserParameters`

- file: `Source/Laser/LaserProfiles.H`
- kind: `struct`
- public methods: 0; public members: 4

Public members:

```cpp
amrex::Real wavelength
amrex::Real e_max
amrex::Vector<amrex::Real> p_X
amrex::Vector<amrex::Real> nvec
```

## `FieldFunctionLaserProfile`

- file: `Source/Laser/LaserProfiles.H`
- kind: `class`, bases: `public ILaserProfile`
- public methods: 3; public members: 0

```cpp
void init ( const amrex::ParmParse& ppl, CommonLaserParameters params) final
void update (amrex::Real ) final
void fill_amplitude ( int np, amrex::Real const * AMREX_RESTRICT Xp, amrex::Real const * AMREX_RESTRICT Yp, amrex::Real t, amrex::Real * AMREX_RESTRICT amplitude) const final
```

## `FromFileLaserProfile`

- file: `Source/Laser/LaserProfiles.H`
- kind: `class`, bases: `public ILaserProfile`
- public methods: 6; public members: 0

```cpp
void init ( const amrex::ParmParse& ppl, CommonLaserParameters params) final
void update (amrex::Real t) final
void fill_amplitude ( int np, amrex::Real const * AMREX_RESTRICT Xp, amrex::Real const * AMREX_RESTRICT Yp, amrex::Real t, amrex::Real * AMREX_RESTRICT amplitude) const final
void internal_fill_amplitude_uniform_cartesian( int idx_t_left, int np, amrex::Real const * AMREX_RESTRICT Xp, amrex::Real const * AMREX_RESTRICT Yp, amrex::Real t, amrex::Real * AMREX_RESTRICT amplitude) const
void internal_fill_amplitude_uniform_cylindrical( int idx_t_left, int np, amrex::Real const * AMREX_RESTRICT Xp, amrex::Real const * AMREX_RESTRICT Yp, amrex::Real t, amrex::Real * AMREX_RESTRICT amplitude) const
void internal_fill_amplitude_uniform_binary( int idx_t_left, int np, amrex::Real const * AMREX_RESTRICT Xp, amrex::Real const * AMREX_RESTRICT Yp, amrex::Real t, amrex::Real * AMREX_RESTRICT amplitude) const
```

## `GaussianLaserProfile`

- file: `Source/Laser/LaserProfiles.H`
- kind: `class`, bases: `public ILaserProfile`
- public methods: 3; public members: 0

```cpp
void init ( const amrex::ParmParse& ppl, CommonLaserParameters params) final
void update (amrex::Real ) final
void fill_amplitude ( int np, amrex::Real const * AMREX_RESTRICT Xp, amrex::Real const * AMREX_RESTRICT Yp, amrex::Real t, amrex::Real * AMREX_RESTRICT amplitude) const final
```

## `ILaserProfile`

- file: `Source/Laser/LaserProfiles.H`
- kind: `class`
- public methods: 9; public members: 0

```cpp
virtual void init ( const amrex::ParmParse& ppl, CommonLaserParameters params) = 0
virtual void update ( amrex::Real t) = 0
virtual void fill_amplitude ( int np, amrex::Real const * AMREX_RESTRICT Xp, amrex::Real const * AMREX_RESTRICT Yp, amrex::Real t, amrex::Real* AMREX_RESTRICT amplitude) const = 0
ILaserProfile () = default
virtual ~ILaserProfile() = default
ILaserProfile ( ILaserProfile const &) = default
ILaserProfile& operator= ( ILaserProfile const & ) = default
ILaserProfile ( ILaserProfile&& ) = default
ILaserProfile& operator= ( ILaserProfile&& ) = default
```

