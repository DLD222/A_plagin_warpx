# A0 API catalog — Initialization

> Generated from headers; 33 classes/structs. Signatures are parsed heuristically; check the header for exact defaults/templates.

## `ProjectionDivCleaner`

- file: `Source/Initialization/DivCleaner/ProjectionDivCleaner.H`
- kind: `class`
- public methods: 5; public members: 8; private members: 0

```cpp
explicit ProjectionDivCleaner (std::string const& a_field_name, bool a_vector_potential=false, int a_comp=0)
void ReadParameters ()
void solve ()
void setSourceFromField ()
void correctField ()
```

Public members:

```cpp
amrex::Vector< std::unique_ptr<amrex::MultiFab> > m_solution
amrex::Vector< std::unique_ptr<amrex::MultiFab> > m_source
amrex::Vector<amrex::Real> m_h_stencil_coefs_x
amrex::Vector<amrex::Real> m_h_stencil_coefs_y
amrex::Vector<amrex::Real> m_h_stencil_coefs_z
amrex::Gpu::DeviceVector<amrex::Real> m_stencil_coefs_x
amrex::Gpu::DeviceVector<amrex::Real> m_stencil_coefs_y
amrex::Gpu::DeviceVector<amrex::Real> m_stencil_coefs_z
```

## `ExternalFieldParams`

- file: `Source/Initialization/ExternalField.H`
- kind: `struct`
- public methods: 1; public members: 11; private members: 0

```cpp
explicit ExternalFieldParams(const amrex::ParmParse& pp_warpx)
```

Public members:

```cpp
amrex::GpuArray<amrex::Real,3> E_external_grid =
amrex::GpuArray<amrex::Real,3> B_external_grid =
ExternalFieldType B_ext_grid_type = ExternalFieldType::default_zero
ExternalFieldType E_ext_grid_type = ExternalFieldType::default_zero
std::unique_ptr<amrex::Parser> Bxfield_parser
std::unique_ptr<amrex::Parser> Byfield_parser
std::unique_ptr<amrex::Parser> Bzfield_parser
std::unique_ptr<amrex::Parser> Exfield_parser
std::unique_ptr<amrex::Parser> Eyfield_parser
std::unique_ptr<amrex::Parser> Ezfield_parser
std::string external_fields_path
```

## `ExternalFieldReader`

- file: `Source/Initialization/ExternalField.H`
- kind: `class`
- public methods: 6; public members: 0; private members: 16

```cpp
ExternalFieldReader (std::string read_fields_from_path, std::string F_name, std::string F_component, amrex::GpuArray<amrex::Real,AMREX_SPACEDIM> const& problo, amrex::GpuArray<amrex::Real,AMREX_SPACEDIM> const& pdx, amrex::Box const& dombox, bool distributed)
[[nodiscard]] ExternalFieldView getView (int li) const
[[nodiscard]] ExternalFieldView getView () const noexcept
void prepare (amrex::BoxArray const& grids, amrex::DistributionMapping const& dmap, amrex::IntVect const& ngrow, std::function<amrex::Real(amrex::Real)> const& get_zlab = nullptr)
void prepare (amrex::RealBox const& pbox, int moving_dir, int moving_sign, std::function<amrex::Real(amrex::Real)> const& get_zlab = nullptr)
[[nodiscard]] bool distributed () const
```

## `ExternalFieldVectorView`

- file: `Source/Initialization/ExternalField.H`
- kind: `struct`
- public methods: 2; public members: 0; private members: 3

```cpp
ExternalFieldVectorView (ExternalFieldReader const* x_reader, ExternalFieldReader const* y_reader, ExternalFieldReader const* z_reader) noexcept
[[nodiscard]] AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE amrex::XDim3 getValue (amrex::Real x, amrex::Real y, amrex::Real z) const noexcept
```

## `ExternalFieldView`

- file: `Source/Initialization/ExternalField.H`
- kind: `struct`
- public methods: 0; public members: 6; private members: 0

Public members:

```cpp
amrex::RealVect dx
amrex::RealVect offset
amrex::Table1D<double> table
amrex::Table2D<double> table
amrex::Table3D<double> table
amrex::IntVect global_size
```

## `GetTemperature`

- file: `Source/Initialization/GetTemperature.H`
- kind: `struct`
- public methods: 2; public members: 4; private members: 0

```cpp
!defined(WARPX_DIM_RCYLINDER) && !defined(WARPX_DIM_RSPHERE) ExternalFieldView m_temperature_in_eV_from_file
explicit GetTemperature (TemperatureProperties const& temp) noexcept
```

Public members:

```cpp
TemperatureInitType m_type
amrex::Real m_temperature
amrex::ParserExecutor<3> m_temperature_parser
amrex::Real m_q_e_over_mc2
```

## `GetTemperatureVector`

- file: `Source/Initialization/GetTemperature.H`
- kind: `struct`
- public methods: 2; public members: 6; private members: 0

```cpp
!defined(WARPX_DIM_RCYLINDER) && !defined(WARPX_DIM_RSPHERE) ExternalFieldVectorView m_u_std_from_file
explicit GetTemperatureVector (TemperatureProperties const& temp) noexcept
```

Public members:

```cpp
TemperatureInitType m_type
amrex::Real m_ux_std
, m_uy_std
, m_uz_std
amrex::ParserExecutor<3> m_ux_std_parser, m_uy_std_parser, m_uz_std_parser
GetTemperature m_temperature
```

## `GetVelocityVector`

- file: `Source/Initialization/GetVelocity.H`
- kind: `struct`
- public methods: 2; public members: 5; private members: 0

```cpp
!defined(WARPX_DIM_RCYLINDER) && !defined(WARPX_DIM_RSPHERE) ExternalFieldVectorView m_from_file
explicit GetVelocityVector (VelocityProperties const& vel) noexcept
```

Public members:

```cpp
VelocityInitType m_type
amrex::Real m_ux_mean
, m_uy_mean
, m_uz_mean
amrex::ParserExecutor<3> m_ux_mean_parser, m_uy_mean_parser, m_uz_mean_parser
```

## `InjectorDensity`

- file: `Source/Initialization/InjectorDensity.H`
- kind: `struct`
- public methods: 15; public members: 0; private members: 6

```cpp
InjectorDensity (InjectorDensityConstant* t, amrex::Real a_rho) : type(Type::constant), object(t,a_rho)
InjectorDensity (InjectorDensityParser* t, amrex::ParserExecutor<3> const& a_parser) : type(Type::parser), object(t,a_parser)
InjectorDensity (InjectorDensityFromFile* t, std::string const& a_file_name, std::string const& a_F_name, amrex::Geometry const& a_geom, bool a_distributed) : type(Type::fromfile), object(t,a_file_name,a_F_name,a_geom,a_distributed)
InjectorDensity (InjectorDensity const&) = delete
InjectorDensity (InjectorDensity&&) = delete
void operator= (InjectorDensity const&) = delete
void operator= (InjectorDensity &&) = delete
~InjectorDensity () = default
void clear ()
void prepare (amrex::BoxArray const& grids, amrex::DistributionMapping const& dmap, amrex::IntVect const& ngrow, std::function<amrex::Real(amrex::Real)> const& get_zlab)
void prepare (amrex::RealBox const& pbox, int moving_dir, int moving_sign, std::function<amrex::Real(amrex::Real)> const& get_zlab)
void prepare (int li, InjectorDensity** inj_rho)
[[nodiscard]] bool needPreparation () const
[[nodiscard]] bool distributed () const
[[nodiscard]] AMREX_GPU_HOST_DEVICE amrex::Real getDensity (amrex::Real x, amrex::Real y, amrex::Real z) const noexcept
```

## `InjectorDensityConstant`

- file: `Source/Initialization/InjectorDensity.H`
- kind: `struct`
- public methods: 2; public members: 0; private members: 1

```cpp
explicit InjectorDensityConstant (amrex::Real a_rho) noexcept : m_rho(a_rho)
[[nodiscard]] AMREX_GPU_HOST_DEVICE amrex::Real getDensity (amrex::Real, amrex::Real, amrex::Real) const noexcept
```

## `InjectorDensityDeleter`

- file: `Source/Initialization/InjectorDensity.H`
- kind: `struct`
- public methods: 1; public members: 0; private members: 0

```cpp
void operator () (InjectorDensity* p) const
```

## `InjectorDensityFromFile`

- file: `Source/Initialization/InjectorDensity.H`
- kind: `struct`
- public methods: 7; public members: 0; private members: 2

```cpp
InjectorDensityFromFile (std::string const& a_file_name, std::string const& a_F_name, amrex::Geometry const& a_geom, bool a_distributed)
void clear ()
void prepare (amrex::BoxArray const& grids, amrex::DistributionMapping const& dmap, amrex::IntVect const& ngrow, std::function<amrex::Real(amrex::Real)> const& get_zlab)
void prepare (amrex::RealBox const& pbox, int moving_dir, int moving_sign, std::function<amrex::Real(amrex::Real)> const& get_zlab)
void prepare (int li)
[[nodiscard]] bool distributed () const
[[nodiscard]] AMREX_GPU_HOST_DEVICE amrex::Real getDensity (amrex::Real x, amrex::Real y, amrex::Real z) const noexcept
```

## `InjectorDensityParser`

- file: `Source/Initialization/InjectorDensity.H`
- kind: `struct`
- public methods: 2; public members: 1; private members: 0

```cpp
explicit InjectorDensityParser (amrex::ParserExecutor<3> const& a_parser) noexcept : m_parser(a_parser)
[[nodiscard]] AMREX_GPU_HOST_DEVICE amrex::Real getDensity (amrex::Real x, amrex::Real y, amrex::Real z) const noexcept
```

Public members:

```cpp
amrex::ParserExecutor<3> m_parser
```

## `InjectorFlux`

- file: `Source/Initialization/InjectorFlux.H`
- kind: `struct`
- public methods: 9; public members: 0; private members: 4

```cpp
InjectorFlux (InjectorFluxConstant* t, amrex::Real a_flux) : type(Type::constant), object(t,a_flux)
InjectorFlux (InjectorFluxParser* t, amrex::ParserExecutor<4> const& a_parser) : type(Type::parser), object(t,a_parser)
InjectorFlux (InjectorFlux const&) = delete
InjectorFlux (InjectorFlux&&) = delete
void operator= (InjectorFlux const&) = delete
void operator= (InjectorFlux &&) = delete
~InjectorFlux () = default
void clear ()
[[nodiscard]] AMREX_GPU_HOST_DEVICE amrex::Real getFlux (amrex::Real x, amrex::Real y, amrex::Real z, amrex::Real t) const noexcept
```

## `InjectorFluxConstant`

- file: `Source/Initialization/InjectorFlux.H`
- kind: `struct`
- public methods: 2; public members: 0; private members: 1

```cpp
explicit InjectorFluxConstant (amrex::Real a_flux) noexcept : m_flux(a_flux)
[[nodiscard]] AMREX_GPU_HOST_DEVICE amrex::Real getFlux (amrex::Real, amrex::Real, amrex::Real, amrex::Real) const noexcept
```

## `InjectorFluxDeleter`

- file: `Source/Initialization/InjectorFlux.H`
- kind: `struct`
- public methods: 1; public members: 0; private members: 0

```cpp
void operator () (InjectorFlux* p) const
```

## `InjectorFluxParser`

- file: `Source/Initialization/InjectorFlux.H`
- kind: `struct`
- public methods: 2; public members: 1; private members: 0

```cpp
explicit InjectorFluxParser (amrex::ParserExecutor<4> const& a_parser) noexcept : m_parser(a_parser)
[[nodiscard]] AMREX_GPU_HOST_DEVICE amrex::Real getFlux (amrex::Real x, amrex::Real y, amrex::Real z, amrex::Real t) const noexcept
```

Public members:

```cpp
amrex::ParserExecutor<4> m_parser
```

## `InjectorMomentum`

- file: `Source/Initialization/InjectorMomentum.H`
- kind: `struct`
- public methods: 15; public members: 2; private members: 2

```cpp
InjectorMomentum (InjectorMomentumConstant* t, amrex::Real a_ux, amrex::Real a_uy, amrex::Real a_uz) : type(Type::constant), object(t, a_ux, a_uy, a_uz)
InjectorMomentum (InjectorMomentumParser* t, GetVelocityVector const& a_velocity) : type(Type::parser), object(t, a_velocity)
InjectorMomentum (InjectorMomentumGaussian* t, amrex::Real a_ux_m, amrex::Real a_uy_m, amrex::Real a_uz_m, amrex::Real a_ux_th, amrex::Real a_uy_th, amrex::Real a_uz_th) : type(Type::gaussian), object(t,a_ux_m,a_uy_m,a_uz_m,a_ux_th,a_uy_th,a_uz_th)
InjectorMomentum (InjectorMomentumGaussianFlux* t, amrex::Real a_ux_m, amrex::Real a_uy_m, amrex::Real a_uz_m, amrex::Real a_ux_th, amrex::Real a_uy_th, amrex::Real a_uz_th, int a_flux_normal_axis, int a_flux_direction) : type(Type::gaussianflux), object(t,a_ux_m,a_uy_m,a_uz_m,a_ux_th,a_uy_th,a_uz_th,a_flux_normal_axis,a_flux_direction)
InjectorMomentum (InjectorMomentumUniform* t, amrex::Real a_ux_min, amrex::Real a_uy_min, amrex::Real a_uz_min, amrex::Real a_ux_max, amrex::Real a_uy_max, amrex::Real a_uz_max) : type(Type::uniform), object(t,a_ux_min,a_uy_min,a_uz_min,a_ux_max,a_uy_max,a_uz_max)
InjectorMomentum (InjectorMomentumMaxwellian* t, GetTemperatureVector const& temperature, GetVelocityVector const& velocity) : type(Type::maxwellian), object(t, temperature, velocity)
InjectorMomentum (InjectorMomentumJuttner* t, GetTemperature const& temperature, GetVelocityVector const& velocity) : type(Type::juttner), object(t, temperature, velocity)
InjectorMomentum (InjectorMomentum const&) = delete
InjectorMomentum (InjectorMomentum&&) = delete
void operator= (InjectorMomentum const&) = delete
void operator= (InjectorMomentum &&) = delete
~InjectorMomentum() = default
void clear ()
[[nodiscard]] AMREX_GPU_HOST_DEVICE amrex::XDim3 getMomentum (amrex::Real x, amrex::Real y, amrex::Real z, amrex::RandomEngine const& engine) const noexcept
[[nodiscard]] AMREX_GPU_HOST_DEVICE amrex::XDim3 getBulkMomentum (amrex::Real x, amrex::Real y, amrex::Real z) const noexcept
```

Public members:

```cpp
enum struct Type
Type type
```

## `InjectorMomentumConstant`

- file: `Source/Initialization/InjectorMomentum.H`
- kind: `struct`
- public methods: 3; public members: 0; private members: 1

```cpp
InjectorMomentumConstant (amrex::Real a_ux, amrex::Real a_uy, amrex::Real a_uz) noexcept : m_ux(a_ux), m_uy(a_uy), m_uz(a_uz)
[[nodiscard]] AMREX_GPU_HOST_DEVICE amrex::XDim3 getMomentum (amrex::Real, amrex::Real, amrex::Real, amrex::RandomEngine const&) const noexcept
[[nodiscard]] AMREX_GPU_HOST_DEVICE amrex::XDim3 getBulkMomentum (amrex::Real, amrex::Real, amrex::Real) const noexcept
```

## `InjectorMomentumDeleter`

- file: `Source/Initialization/InjectorMomentum.H`
- kind: `struct`
- public methods: 1; public members: 0; private members: 0

```cpp
void operator () (InjectorMomentum* p) const
```

## `InjectorMomentumGaussian`

- file: `Source/Initialization/InjectorMomentum.H`
- kind: `struct`
- public methods: 3; public members: 0; private members: 2

```cpp
InjectorMomentumGaussian (amrex::Real a_ux_m, amrex::Real a_uy_m, amrex::Real a_uz_m, amrex::Real a_ux_th, amrex::Real a_uy_th, amrex::Real a_uz_th) noexcept : m_ux_m(a_ux_m), m_uy_m(a_uy_m), m_uz_m(a_uz_m), m_ux_th(a_ux_th), m_uy_th(a_uy_th), m_uz_th(a_uz_th)
[[nodiscard]] AMREX_GPU_HOST_DEVICE amrex::XDim3 getMomentum (amrex::Real , amrex::Real , amrex::Real , amrex::RandomEngine const& engine) const noexcept
[[nodiscard]] AMREX_GPU_HOST_DEVICE amrex::XDim3 getBulkMomentum (amrex::Real , amrex::Real , amrex::Real ) const noexcept
```

## `InjectorMomentumGaussianFlux`

- file: `Source/Initialization/InjectorMomentum.H`
- kind: `struct`
- public methods: 3; public members: 0; private members: 4

```cpp
InjectorMomentumGaussianFlux (amrex::Real a_ux_m, amrex::Real a_uy_m, amrex::Real a_uz_m, amrex::Real a_ux_th, amrex::Real a_uy_th, amrex::Real a_uz_th, int a_flux_normal_axis, int a_flux_direction) noexcept : m_ux_m(a_ux_m), m_uy_m(a_uy_m), m_uz_m(a_uz_m), m_ux_th(a_ux_th), m_uy_th(a_uy_th), m_uz_th(a_uz_th), m_flux_normal_axis(a_flux_normal_axis), m_flux_direction(a_flux_direction)
[[nodiscard]] AMREX_GPU_HOST_DEVICE amrex::XDim3 getMomentum (amrex::Real , amrex::Real , amrex::Real , amrex::RandomEngine const& engine) const noexcept
[[nodiscard]] AMREX_GPU_HOST_DEVICE amrex::XDim3 getBulkMomentum (amrex::Real , amrex::Real , amrex::Real ) const noexcept
```

## `InjectorMomentumJuttner`

- file: `Source/Initialization/InjectorMomentum.H`
- kind: `struct`
- public methods: 3; public members: 0; private members: 2

```cpp
InjectorMomentumJuttner(GetTemperature const& t, GetVelocityVector const& b) noexcept : velocity(b), temperature(t)
[[nodiscard]] AMREX_GPU_HOST_DEVICE amrex::XDim3 getMomentum (amrex::Real const x, amrex::Real const y, amrex::Real const z, amrex::RandomEngine const& engine) const noexcept
[[nodiscard]] AMREX_GPU_HOST_DEVICE amrex::XDim3 getBulkMomentum (amrex::Real const x, amrex::Real const y, amrex::Real const z) const noexcept
```

## `InjectorMomentumMaxwellian`

- file: `Source/Initialization/InjectorMomentum.H`
- kind: `struct`
- public methods: 3; public members: 0; private members: 2

```cpp
InjectorMomentumMaxwellian(GetTemperatureVector const& t, GetVelocityVector const& b) noexcept : velocity(b), temperature(t)
[[nodiscard]] AMREX_GPU_HOST_DEVICE amrex::XDim3 getMomentum (amrex::Real const x, amrex::Real const y, amrex::Real const z, amrex::RandomEngine const& engine) const noexcept
[[nodiscard]] AMREX_GPU_HOST_DEVICE amrex::XDim3 getBulkMomentum (amrex::Real const x, amrex::Real const y, amrex::Real const z) const noexcept
```

## `InjectorMomentumParser`

- file: `Source/Initialization/InjectorMomentum.H`
- kind: `struct`
- public methods: 3; public members: 1; private members: 0

```cpp
explicit InjectorMomentumParser (GetVelocityVector const& a_velocity) noexcept : m_velocity(a_velocity)
[[nodiscard]] AMREX_GPU_HOST_DEVICE amrex::XDim3 getMomentum (amrex::Real x, amrex::Real y, amrex::Real z, amrex::RandomEngine const&) const noexcept
[[nodiscard]] AMREX_GPU_HOST_DEVICE amrex::XDim3 getBulkMomentum (amrex::Real x, amrex::Real y, amrex::Real z) const noexcept
```

Public members:

```cpp
GetVelocityVector m_velocity
```

## `InjectorMomentumUniform`

- file: `Source/Initialization/InjectorMomentum.H`
- kind: `struct`
- public methods: 3; public members: 0; private members: 4

```cpp
InjectorMomentumUniform (amrex::Real a_ux_min, amrex::Real a_uy_min, amrex::Real a_uz_min, amrex::Real a_ux_max, amrex::Real a_uy_max, amrex::Real a_uz_max) noexcept : m_ux_min(a_ux_min), m_uy_min(a_uy_min), m_uz_min(a_uz_min), m_ux_max(a_ux_max), m_uy_max(a_uy_max), m_uz_max(a_uz_max), m_Dux(m_ux_max - m_ux_min), m_Duy(m_uy_max - m_uy_min), m_Duz(m_uz_max - m_uz_min), m_ux_h(amrex::Real(0.5) * (m_ux_max + m_ux_min)), m_uy_h(amrex::Real(0.5) * (m_uy_max + m_uy_min)), m_uz_h(amrex::Real(0.5) * (m_uz_max + m_uz_min))
[[nodiscard]] AMREX_GPU_HOST_DEVICE amrex::XDim3 getMomentum (amrex::Real , amrex::Real , amrex::Real , amrex::RandomEngine const& engine) const noexcept
[[nodiscard]] AMREX_GPU_HOST_DEVICE amrex::XDim3 getBulkMomentum (amrex::Real , amrex::Real , amrex::Real ) const noexcept
```

## `InjectorPosition`

- file: `Source/Initialization/InjectorPosition.H`
- kind: `struct`
- public methods: 12; public members: 0; private members: 7

```cpp
InjectorPosition (InjectorPositionRandom* t, amrex::Real a_xmin, amrex::Real a_xmax, amrex::Real a_ymin, amrex::Real a_ymax, amrex::Real a_zmin, amrex::Real a_zmax) : type(Type::random), object(t), xmin(a_xmin), xmax(a_xmax), ymin(a_ymin), ymax(a_ymax), zmin(a_zmin), zmax(a_zmax)
InjectorPosition (InjectorPositionRandomPlane* t, amrex::Real a_xmin, amrex::Real a_xmax, amrex::Real a_ymin, amrex::Real a_ymax, amrex::Real a_zmin, amrex::Real a_zmax, int const& a_dir) : type(Type::randomplane), object(t, a_dir), xmin(a_xmin), xmax(a_xmax), ymin(a_ymin), ymax(a_ymax), zmin(a_zmin), zmax(a_zmax)
InjectorPosition (InjectorPositionRegular* t, amrex::Real a_xmin, amrex::Real a_xmax, amrex::Real a_ymin, amrex::Real a_ymax, amrex::Real a_zmin, amrex::Real a_zmax, amrex::Dim3 const& a_ppc) : type(Type::regular), object(t, a_ppc), xmin(a_xmin), xmax(a_xmax), ymin(a_ymin), ymax(a_ymax), zmin(a_zmin), zmax(a_zmax)
~InjectorPosition () = default
InjectorPosition (InjectorPosition const&) = delete
InjectorPosition (InjectorPosition&&) = delete
void operator= (InjectorPosition const&) = delete
void operator= (InjectorPosition &&) = delete
[[nodiscard]] AMREX_GPU_HOST_DEVICE amrex::XDim3 getPositionUnitBox (int const i_part, amrex::IntVect const ref_fac, amrex::RandomEngine const& engine) const noexcept
[[nodiscard]] AMREX_GPU_HOST_DEVICE bool insideBounds (amrex::Real x, amrex::Real y, amrex::Real z) const noexcept
[[nodiscard]] AMREX_GPU_HOST_DEVICE bool insideBoundsInclusive (amrex::Real x, amrex::Real y, amrex::Real z) const noexcept
[[nodiscard]] AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE bool overlapsWith (const amrex::XDim3& lo, const amrex::XDim3& hi) const noexcept
```

## `InjectorPositionRandom`

- file: `Source/Initialization/InjectorPosition.H`
- kind: `struct`
- public methods: 1; public members: 0; private members: 0

```cpp
[[nodiscard]] AMREX_GPU_HOST_DEVICE amrex::XDim3 getPositionUnitBox (int , amrex::IntVect const , amrex::RandomEngine const& engine) const noexcept
```

## `InjectorPositionRandomPlane`

- file: `Source/Initialization/InjectorPosition.H`
- kind: `struct`
- public methods: 2; public members: 0; private members: 1

```cpp
explicit InjectorPositionRandomPlane (int const& a_dir) noexcept : dir(a_dir)
[[nodiscard]] AMREX_GPU_HOST_DEVICE amrex::XDim3 getPositionUnitBox (int , amrex::IntVect const , amrex::RandomEngine const& engine) const noexcept
```

## `InjectorPositionRegular`

- file: `Source/Initialization/InjectorPosition.H`
- kind: `struct`
- public methods: 2; public members: 0; private members: 1

```cpp
explicit InjectorPositionRegular (amrex::Dim3 const& a_ppc) noexcept : ppc(a_ppc)
[[nodiscard]] AMREX_GPU_HOST_DEVICE amrex::XDim3 getPositionUnitBox (int const i_part, amrex::IntVect const ref_fac, amrex::RandomEngine const&) const noexcept
```

## `PlasmaInjector`

- file: `Source/Initialization/PlasmaInjector.H`
- kind: `class`
- public methods: 28; public members: 47; private members: 0

```cpp
PlasmaInjector () = default
PlasmaInjector (int ispecies, const std::string& name, const amrex::Geometry& geom, const std::string& src_name="")
PlasmaInjector(const PlasmaInjector&) = delete
PlasmaInjector& operator=(const PlasmaInjector&) = delete
PlasmaInjector(PlasmaInjector&&) = default
PlasmaInjector& operator=(PlasmaInjector&&) = default
~PlasmaInjector ()
[[nodiscard]] bool insideBounds ( amrex::Real x, amrex::Real y, amrex::Real z) const noexcept
[[nodiscard]] bool overlapsWith ( const amrex::XDim3& lo, const amrex::XDim3& hi) const noexcept
[[nodiscard]] amrex::XDim3 getMomentum ( amrex::Real x, amrex::Real y, amrex::Real z) const noexcept
bool queryCharge (amrex::ParticleReal& a_charge) const
bool queryMass (amrex::ParticleReal& a_mass) const
[[nodiscard]] bool doInjection () const noexcept
[[nodiscard]] bool doFluxInjection () const noexcept
amrex::Real x_cut = std::numeric_limits<amrex::Real>::max()
amrex::Real y_cut = std::numeric_limits<amrex::Real>::max()
amrex::Real z_cut = std::numeric_limits<amrex::Real>::max()
amrex::Real density_min = std::numeric_limits<amrex::Real>::epsilon()
amrex::Real density_max = std::numeric_limits<amrex::Real>::max()
[[nodiscard]] InjectorPosition* getInjectorPosition () const
[[nodiscard]] InjectorPosition* getInjectorFluxPosition () const
[[nodiscard]] InjectorDensity* getInjectorDensity (int li) const
[[nodiscard]] InjectorFlux* getInjectorFlux () const
[[nodiscard]] InjectorMomentum* getInjectorMomentumDevice () const
[[nodiscard]] InjectorMomentum* getInjectorMomentumHost () const
void prepare (amrex::BoxArray const& grids, amrex::DistributionMapping const& dmap, amrex::IntVect const& ngrow, std::function<amrex::Real(amrex::Real)> const& get_zlab)
void prepare (amrex::RealBox const& pbox, int moving_dir, int moving_sign, std::function<amrex::Real(amrex::Real)> const& get_zlab)
[[nodiscard]] bool distributedInjectorDensity () const
```

Public members:

```cpp
int num_particles_per_cell
amrex::Real num_particles_per_cell_real
amrex::Vector<int> num_particles_per_cell_each_dim
bool add_single_particle = false
amrex::Vector<amrex::ParticleReal> single_particle_pos
amrex::Vector<amrex::ParticleReal> single_particle_u
amrex::ParticleReal single_particle_weight
bool add_multiple_particles = false
amrex::Vector<amrex::ParticleReal> multiple_particles_pos_x
amrex::Vector<amrex::ParticleReal> multiple_particles_pos_y
amrex::Vector<amrex::ParticleReal> multiple_particles_pos_z
amrex::Vector<amrex::ParticleReal> multiple_particles_ux
amrex::Vector<amrex::ParticleReal> multiple_particles_uy
amrex::Vector<amrex::ParticleReal> multiple_particles_uz
amrex::Vector<amrex::ParticleReal> multiple_particles_weight
bool gaussian_beam = false
amrex::Real x_m
amrex::Real y_m
amrex::Real z_m
amrex::Real x_rms
amrex::Real y_rms
amrex::Real z_rms
amrex::Real q_tot = 0.0
amrex::Real N_tot = 0.0
long npart
int do_symmetrize = 0
int symmetrization_order = 4
bool do_focusing = false
amrex::Real focal_distance
bool do_rotation = false
bool do_rotation_momenta = false
amrex::Real rotation_angle
amrex::Vector<amrex::Real> rotation_axis
bool external_file = false
amrex::Real z_shift = 0.0
std::any m_openpmd_input_series
amrex::Real surface_flux_pos
amrex::Real flux_tmin = -1.
amrex::Real flux_tmax = -1.
int flux_normal_axis
```

## `TemperatureProperties`

- file: `Source/Initialization/TemperatureProperties.H`
- kind: `struct`
- public methods: 2; public members: 13; private members: 0

```cpp
TemperatureProperties (const amrex::ParmParse& pp, std::string const& source_name, amrex::Geometry const& geom)
!defined(WARPX_DIM_RCYLINDER) && !defined(WARPX_DIM_RSPHERE) std::unique_ptr<ExternalFieldReader> m_u_std_x_reader
```

Public members:

```cpp
TemperatureInitType m_type
amrex::Real m_temperature
std::unique_ptr<amrex::Parser> m_ptr_temperature_parser
amrex::Real m_ux_std
, m_uy_std
, m_uz_std
std::unique_ptr<amrex::Parser> m_ptr_ux_std_parser, m_ptr_uy_std_parser, m_ptr_uz_std_parser
std::string m_read_u_std_path
std::string m_read_temperature_in_eV_path
amrex::Real m_q_e_over_mc2
std::unique_ptr<ExternalFieldReader> m_u_std_y_reader
std::unique_ptr<ExternalFieldReader> m_u_std_z_reader
std::unique_ptr<ExternalFieldReader> m_temperature_in_eV_reader
```

## `VelocityProperties`

- file: `Source/Initialization/VelocityProperties.H`
- kind: `struct`
- public methods: 2; public members: 8; private members: 0

```cpp
VelocityProperties (const amrex::ParmParse& pp, std::string const& source_name, amrex::Geometry const& geom)
!defined(WARPX_DIM_RCYLINDER) && !defined(WARPX_DIM_RSPHERE) std::unique_ptr<ExternalFieldReader> m_u_mean_x_reader
```

Public members:

```cpp
VelocityInitType m_type
amrex::Real m_ux_mean
, m_uy_mean
, m_uz_mean
std::unique_ptr<amrex::Parser> m_ptr_ux_mean_parser, m_ptr_uy_mean_parser, m_ptr_uz_mean_parser
std::string m_read_u_mean_path
std::unique_ptr<ExternalFieldReader> m_u_mean_y_reader
std::unique_ptr<ExternalFieldReader> m_u_mean_z_reader
```

