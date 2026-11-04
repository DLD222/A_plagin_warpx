# A0 API catalog — Filter

<!-- SPDX-License-Identifier: BSD-3-Clause -->

> Generated; 3 classes/structs. Signatures are heuristic.

## `BilinearFilter`

- file: `Source/Filter/BilinearFilter.H`
- kind: `class`, bases: `public Filter`
- public methods: 2; public members: 1

```cpp
BilinearFilter() = default
void ComputeStencils()
```

Public members:

```cpp
amrex::Array<unsigned int, AMREX_SPACEDIM> npass_each_dir
```

## `Filter`

- file: `Source/Filter/Filter.H`
- kind: `class`
- public methods: 4; public members: 1

```cpp
Filter () = default
void ApplyStencil (amrex::MultiFab& dstmf, const amrex::MultiFab& srcmf, int lev, int scomp=0, int dcomp=0, int ncomp=10000)
void ApplyStencil (amrex::FArrayBox& dstfab, const amrex::FArrayBox& srcfab, const amrex::Box& tbx, int scomp=0, int dcomp=0, int ncomp=10000)
void DoFilter (const amrex::Box& tbx, amrex::Array4<amrex::Real const> const& tmp, amrex::Array4<amrex::Real > const& dst, int scomp, int dcomp, int ncomp)
```

Public members:

```cpp
amrex::IntVect stencil_length_each_dir
```

## `NCIGodfreyFilter`

- file: `Source/Filter/NCIGodfreyFilter.H`
- kind: `class`, bases: `public Filter`
- public methods: 3; public members: 1

```cpp
NCIGodfreyFilter () = default
NCIGodfreyFilter(godfrey_coeff_set coeff_set, amrex::Real cdtodz, bool nodal_gather)
void ComputeStencils()
```

Public members:

```cpp
static constexpr int m_stencil_width = 4
```

