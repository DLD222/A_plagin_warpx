# A0 API catalog — EmbeddedBoundary

> Generated from headers; 4 classes/structs. Signatures are parsed heuristically; check the header for exact defaults/templates.

## `Absorb`

- file: `Source/EmbeddedBoundary/ParticleBoundaryProcess.H`
- kind: `struct`
- public methods: 0; public members: 0; private members: 0

## `NoOp`

- file: `Source/EmbeddedBoundary/ParticleBoundaryProcess.H`
- kind: `struct`
- public methods: 0; public members: 0; private members: 0

## `ParticleBoundaryInteraction`

- file: `Source/EmbeddedBoundary/ParticleBoundaryProcess.H`
- kind: `struct`
- public methods: 1; public members: 3; private members: 0

```cpp
amrex::ParticleReal m_uth = amrex::ParticleReal(0.)
```

Public members:

```cpp
amrex::Real m_dt
amrex::ParticleReal m_mass
ParticleBoundaryType m_boundary_type
```

## `FaceInfoBox`

- file: `Source/EmbeddedBoundary/WarpXFaceInfoBox.H`
- kind: `struct`
- public methods: 0; public members: 7; private members: 0

Public members:

```cpp
enum Neighbours : uint8_t
amrex::Gpu::DeviceVector<Neighbours> neigh_faces
amrex::Gpu::DeviceVector<amrex::Real> area
amrex::Gpu::DeviceVector<int> inds
amrex::BaseFab<int> size
amrex::BaseFab<int*> inds_pointer
int vecs_size = 0
```

