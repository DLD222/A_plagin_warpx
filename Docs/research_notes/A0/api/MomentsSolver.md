# A0 API catalog — MomentsSolver

<!-- SPDX-License-Identifier: BSD-3-Clause -->

> Generated; 15 classes/structs. Signatures are heuristic.

## `FrictionParams`

- file: `Source/MomentsSolver/Coupling/FluidFriction.H`
- kind: `struct`
- public methods: 5; public members: 2

```cpp
Real nu = Real(0)
Real lnLambda = Real(10)
Real Z = Real(1)
Real nu0 = Real(1)
Real (*nu_func)(int s, int t, Real rho_s, Real rho_t, const Real* u_s, const Real* u_t, const FrictionParams* fp, void* ctx) = nullptr
```

Public members:

```cpp
FrictionModel model = FrictionModel::None
void* ctx = nullptr
```

## `Species5`

- file: `Source/MomentsSolver/Coupling/MomentCoupling.H`
- kind: `struct`
- public methods: 0; public members: 3

Public members:

```cpp
Real q
Real m
Real gamma
```

## `YeeDeposit`

- file: `Source/MomentsSolver/Coupling/YeeDeposition.H`
- kind: `struct`
- public methods: 0; public members: 4

Public members:

```cpp
std::vector<Real> Jlo[3]
std::vector<Real> Jhi[3]
std::vector<Real> rho_n
std::vector<Real> rho_np1
```

## `Eos`

- file: `Source/MomentsSolver/Equations/EquationOfState.H`
- kind: `struct`
- public methods: 10; public members: 1

```cpp
Real (*pressure)(const void*, Real rho, Real e) = nullptr
Real (*dp_drho_e)(const void*, Real rho, Real e) = nullptr
Real (*dp_de_rho)(const void*, Real rho, Real e) = nullptr
Real (*energy_from_p)(const void*, Real rho, Real p) = nullptr
Real (*temperature)(const void*, Real rho, Real e) = nullptr
MOMENTS_GPU_HOST_DEVICE bool valid () const noexcept
MOMENTS_GPU_HOST_DEVICE Real p (Real rho, Real e) const noexcept
MOMENTS_GPU_HOST_DEVICE Real e_from_p (Real rho, Real pv) const noexcept
MOMENTS_GPU_HOST_DEVICE Real T (Real rho, Real e) const noexcept
MOMENTS_GPU_HOST_DEVICE Real cs2 (Real rho, Real e) const noexcept
```

Public members:

```cpp
const void* ctx = nullptr
```

## `IdealGasEos`

- file: `Source/MomentsSolver/Equations/EquationOfState.H`
- kind: `struct`
- public methods: 2; public members: 0

```cpp
Real gamma = Real(5.0) / Real(3.0)
Real cv = Real(1.0)
```

## `IsothermalEos`

- file: `Source/MomentsSolver/Equations/EquationOfState.H`
- kind: `struct`
- public methods: 1; public members: 0

```cpp
Real cs2 = Real(1.0)
```

## `Context`

- file: `Source/MomentsSolver/Equations/Euler5Eqn.H`
- kind: `struct`
- public methods: 0; public members: 3

Public members:

```cpp
Real gamma = 5.0 / 3.0
Eos eos
bool use_eos = false
```

## `Euler5`

- file: `Source/MomentsSolver/Equations/Euler5Eqn.H`
- kind: `struct`
- public methods: 6; public members: 3

```cpp
MOMENTS_GPU_HOST_DEVICE static Real internal_energy (const Real* U) noexcept
MOMENTS_GPU_HOST_DEVICE static Real pressure (const Real* U, const Context& ctx) noexcept
MOMENTS_GPU_HOST_DEVICE static Real max_wave_speed (const Real* U, int dir, const Context& ctx) noexcept
MOMENTS_GPU_HOST_DEVICE static void flux (const Real* U, int dir, Real* F, const Context& ctx) noexcept
MOMENTS_GPU_HOST_DEVICE static void conserved_to_primitive (const Real* U, Real* W, const Context& ctx) noexcept
MOMENTS_GPU_HOST_DEVICE static void numerical_flux (const Real* UL, const Real* UR, int dir, Real* F, const Context& ctx) noexcept
```

Public members:

```cpp
static constexpr int num_eqs = 5
static constexpr int num_waves = 3
struct Context
```

## `Context`

- file: `Source/MomentsSolver/Equations/MaxwellEqn.H`
- kind: `struct`
- public methods: 0; public members: 1

Public members:

```cpp
Real c = 1.0
```

## `Maxwell6`

- file: `Source/MomentsSolver/Equations/MaxwellEqn.H`
- kind: `struct`
- public methods: 3; public members: 3

```cpp
MOMENTS_GPU_HOST_DEVICE static Real max_wave_speed (const Real* U, int dir, const Context& ctx) noexcept
MOMENTS_GPU_HOST_DEVICE static void flux (const Real* U, int dir, Real* F, const Context& ctx) noexcept
MOMENTS_GPU_HOST_DEVICE static void numerical_flux (const Real* UL, const Real* UR, int dir, Real* F, const Context& ctx) noexcept
```

Public members:

```cpp
static constexpr int num_eqs = 6
static constexpr int num_waves = 2
struct Context
```

## `EquationVTable`

- file: `Source/MomentsSolver/Equations/MomentEquation.H`
- kind: `struct`
- public methods: 3; public members: 2

```cpp
MOMENTS_GPU_HOST_DEVICE Real (*max_wave_speed)(const void* ctx, const Real* U, int dir) noexcept = nullptr
MOMENTS_GPU_HOST_DEVICE void (*flux)(const void* ctx, const Real* U, int dir, Real* F) noexcept = nullptr
MOMENTS_GPU_HOST_DEVICE void (*conserved_to_primitive)(const void* ctx, const Real* U, Real* W) noexcept = nullptr
```

Public members:

```cpp
int num_eqs = 0
int num_waves = 0
```

## `Context`

- file: `Source/MomentsSolver/Equations/TenMomentEqn.H`
- kind: `struct`
- public methods: 0; public members: 1

Public members:

```cpp
Real k0 = 1.0
```

## `TenMoment`

- file: `Source/MomentsSolver/Equations/TenMomentEqn.H`
- kind: `struct`
- public methods: 5; public members: 3

```cpp
MOMENTS_GPU_HOST_DEVICE static int p_index (int i, int j) noexcept
MOMENTS_GPU_HOST_DEVICE static void conserved_to_primitive (const Real* U, Real* W, const Context& ctx) noexcept
MOMENTS_GPU_HOST_DEVICE static Real max_wave_speed (const Real* U, int dir, const Context& ctx) noexcept
MOMENTS_GPU_HOST_DEVICE static void flux (const Real* U, int dir, Real* F, const Context& ctx) noexcept
MOMENTS_GPU_HOST_DEVICE static void numerical_flux (const Real* UL, const Real* UR, int dir, Real* F, const Context& ctx) noexcept
```

Public members:

```cpp
static constexpr int num_eqs = 10
static constexpr int num_waves = 5
struct Context
```

## `FV1D`

- file: `Source/MomentsSolver/FV/MomentFVDriver.H`
- kind: `struct`
- public methods: 5; public members: 1

```cpp
static Real minmod (Real a, Real b)
static Real mc_limiter (Real a, Real b)
static void get_cell (const Real* U, int nx, int i, BC bc, Real* out)
static void rhs (const Real* U, int nx, Real dx, const Ctx& ctx, BC bc, Real* out)
static void advance (Real* U, int nx, Real dx, Real dt, const Ctx& ctx, BC bc)
```

Public members:

```cpp
static constexpr int N = Eq::num_eqs
```

## `FVND`

- file: `Source/MomentsSolver/FV/MomentFVNDDriver.H`
- kind: `struct`
- public methods: 14; public members: 3

```cpp
FVND (int a_nx, int a_ny, int a_nz, Real a_dx, Real a_dy, Real a_dz) : nx(a_nx), ny(a_ny), nz((DIM == 2) ? 1 : a_nz), dx(a_dx), dy(a_dy), dz((DIM == 2) ? Real(1) : a_dz)
std::size_t ncell_total () const
std::size_t size () const
int ncell (int dir) const
Real hcell (int dir) const
std::size_t cidx (int i, int j, int k) const
std::size_t idx (int i, int j, int k, int v) const
void coords (std::size_t c, int& i, int& j, int& k) const
static int wrap (int q, int n, BC bc)
void get_cell (const Real* U, int i, int j, int k, const BC bc[3], Real* out) const
Real max_dt (const Real* U, const Ctx& ctx, Real cfl) const
void rhs (const Real* U, const Ctx& ctx, const BC bc[3], Real* out, Real* Flo[3] = nullptr, Real* Fhi[3] = nullptr) const
void advance (Real* U, Real dt, const Ctx& ctx, const BC bc[3]) const
void advance_with_mass_flux (Real* U, Real dt, const Ctx& ctx, const BC bc[3], std::vector<Real> Flo_out[3], std::vector<Real> Fhi_out[3]) const
```

Public members:

```cpp
static constexpr int N = Eq::num_eqs
int nx, ny, nz
Real dx, dy, dz
```

