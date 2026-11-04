# A0 API catalog — Utils

> Generated from headers; 3 classes/structs. Signatures are parsed heuristically; check the header for exact defaults/templates.

## `BTDIntervalsParser`

- file: `Source/Utils/Parser/BTDIntervalsParser.H`
- kind: `class`
- public methods: 6; public members: 0; private members: 5

```cpp
BTDIntervalsParser () = default
explicit BTDIntervalsParser (const std::vector<std::string>& instr_vec)
[[nodiscard]] int NumSnapshots () const
[[nodiscard]] int GetBTDIteration(int i_buffer) const
[[nodiscard]] int GetFinalIteration() const
[[nodiscard]] bool isActivated () const
```

## `HeapSortDecreasing`

- file: `Source/Utils/ParticleUtils.H`
- kind: `struct`
- public methods: 0; public members: 0; private members: 0

## `AnisotropicCoefficientTable`

- file: `Source/Utils/ScatteringUtils.H`
- kind: `struct`
- public methods: 1; public members: 4; private members: 0

```cpp
[[nodiscard]] AMREX_GPU_HOST_DEVICE AMREX_INLINE amrex::ParticleReal getCoefficient ( amrex::ParticleReal const a_energy, int const a_coefficient_index) const
```

Public members:

```cpp
amrex::ParticleReal const* m_energies = nullptr
amrex::ParticleReal const* m_coefficients = nullptr
int m_num_energies = 0
int m_num_coefficients = 0
```

