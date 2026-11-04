# A0 API catalog — Diagnostics

<!-- SPDX-License-Identifier: BSD-3-Clause -->

> Generated from headers; 71 classes/structs. Signatures are parsed heuristically; check the header for exact defaults/templates.

## `BTDMultiFabHeaderImpl`

- file: `Source/Diagnostics/BTD_Plotfile_Header_Impl.H`
- kind: `class`
- public methods: 16; public members: 0; private members: 12

```cpp
explicit BTDMultiFabHeaderImpl (std::string const& Headerfile_path)
void ReadMultiFabHeader ()
void WriteMultiFabHeader ()
[[nodiscard]] int ba_size () const
[[nodiscard]] amrex::Box ba_box (int ibox) const
std::string fodPrefix (int ifab)
std::string FabName (int ifab)
int FabHead (int ifab)
amrex::Vector<amrex::Real> minval(int ifab)
amrex::Vector<amrex::Real> maxval(int ifab)
void ResizeFabData ()
void IncreaseMultiFabSize (int add_mf_size)
void SetBox (int ibox, amrex::Box ba_box)
void SetFabName (int ifab, const std::string& fodPrefix, const std::string& FabName, int FabHead)
void SetMinVal (int ifab, const amrex::Vector<amrex::Real>& minval)
void SetMaxVal (int ifab, const amrex::Vector<amrex::Real>& maxval)
```

## `BTDParticleDataHeaderImpl`

- file: `Source/Diagnostics/BTD_Plotfile_Header_Impl.H`
- kind: `class`
- public methods: 8; public members: 3; private members: 0

```cpp
explicit BTDParticleDataHeaderImpl (std::string const& Headerfile_path)
void ReadHeader ()
void WriteHeader () const
[[nodiscard]] int ba_size () const
void IncreaseBoxArraySize ( const int add_size)
[[nodiscard]] amrex::Box ba_box (int ibox) const
void ResizeBoxArray ()
void SetBox (int ibox, amrex::Box ba_box)
```

Public members:

```cpp
int m_ba_size
amrex::BoxArray m_ba
std::string m_Header_path
```

## `BTDPlotfileHeaderImpl`

- file: `Source/Diagnostics/BTD_Plotfile_Header_Impl.H`
- kind: `class`
- public methods: 27; public members: 0; private members: 19

```cpp
explicit BTDPlotfileHeaderImpl (std::string const& Headerfile_path)
[[nodiscard]] std::string fileVersion () const noexcept
[[nodiscard]] int ncomp () const noexcept
[[nodiscard]] const amrex::Vector<std::string>& varnames () const noexcept
[[nodiscard]] int spaceDim () const noexcept
[[nodiscard]] amrex::Real time () const noexcept
[[nodiscard]] int finestLevel () const noexcept
[[nodiscard]] amrex::Real problo (int dim) const noexcept
[[nodiscard]] amrex::Real probhi (int dim) const noexcept
[[nodiscard]] amrex::Box probDomain () const noexcept
[[nodiscard]] int timestep () const noexcept
[[nodiscard]] int numFabs () const noexcept
[[nodiscard]] amrex::Array<amrex::Real, AMREX_SPACEDIM> FabLo (int iFab) const noexcept
[[nodiscard]] amrex::Array<amrex::Real, AMREX_SPACEDIM> FabHi (int iFab) const noexcept
[[nodiscard]] std::string CellPath () const noexcept
void ReadHeaderData ()
void WriteHeader ()
void set_time ( amrex::Real new_time)
void set_timestep (int new_timestep)
void set_problo (int dim, amrex::Real lo)
void set_probhi (int dim, amrex::Real hi)
void set_probDomain (amrex::Box domainBox)
void IncrementNumFabs ()
void ResizeFabLo ()
void ResizeFabHi ()
void AppendNewFabLo (amrex::Array<amrex::Real, AMREX_SPACEDIM> newFabLo)
void AppendNewFabHi (amrex::Array<amrex::Real, AMREX_SPACEDIM> newFabHi)
```

## `BTDSpeciesHeaderImpl`

- file: `Source/Diagnostics/BTD_Plotfile_Header_Impl.H`
- kind: `class`
- public methods: 7; public members: 7; private members: 9

```cpp
BTDSpeciesHeaderImpl (std::string const& Headerfile_path, std::string const& species_name)
void ReadHeader ()
void WriteHeader ()
void set_DataIndex (int lev, int box_id, int data_index)
void AddTotalParticles (const int new_particles)
void IncrementParticleBoxArraySize ()
void AppendParticleInfoForNewBox (int data_index, int particles_per_box, int offset)
```

Public members:

```cpp
amrex::Vector< amrex::Vector<int> > m_which_data
amrex::Vector< amrex::Vector<int> > m_particles_per_box
amrex::Vector< amrex::Vector<int> > m_offset_per_box
amrex::Vector<int> m_particleBoxArray_size
int m_total_particles
int m_nextid
int m_finestLevel
```

## `BTDiagnostics`

- file: `Source/Diagnostics/BTDiagnostics.H`
- kind: `class`, bases: `public Diagnostics`
- public methods: 1; public members: 0; private members: 34

```cpp
BTDiagnostics (int i, const std::string& name, DiagTypes diag_type)
```

## `BoundaryScrapingDiagnostics`

- file: `Source/Diagnostics/BoundaryScrapingDiagnostics.H`
- kind: `class`, bases: `public Diagnostics`
- public methods: 1; public members: 0; private members: 1

```cpp
BoundaryScrapingDiagnostics (int i, const std::string& name, DiagTypes diag_type)
```

## `BackTransformFunctor`

- file: `Source/Diagnostics/ComputeDiagFunctors/BackTransformFunctor.H`
- kind: `class`, bases: `public ComputeDiagFunctor`
- public methods: 5; public members: 0; private members: 10

```cpp
BackTransformFunctor ( const amrex::MultiFab * mf_src, int lev, int ncomp, int num_buffers, amrex::Vector< std::string > varnames, amrex::Vector< std::string > varnames_fields, amrex::IntVect crse_ratio= amrex::IntVect(1))
void operator ()(amrex::MultiFab& mf_dst, int dcomp, int i_buffer) const override
void PrepareFunctorData ( int i_buffer, bool z_slice_in_domain, amrex::Real current_z_boost, amrex::Box buffer_box, int k_index_zlab, int snapshot_full ) override
void InitData () override
void LorentzTransformZ (amrex::MultiFab& data, amrex::Real gamma_boost, amrex::Real beta_boost) const
```

## `BackTransformParticleFunctor`

- file: `Source/Diagnostics/ComputeDiagFunctors/BackTransformParticleFunctor.H`
- kind: `class`, bases: `public ComputeParticleDiagFunctor`
- public methods: 4; public members: 0; private members: 7

```cpp
BackTransformParticleFunctor(WarpXParticleContainer *pc_src, std::string species_name, int num_buffers)
void operator () (WarpXParticleContainer::Base& pc_dst, int &TotalParticleCounter, int i_buffer) const override
void InitData() override
void PrepareFunctorData ( int i_buffer, bool z_slice_in_domain, amrex::Real old_z_boost, amrex::Real current_z_boost, amrex::Real t_lab, int snapshot_full) override
```

## `LorentzTransformParticles`

- file: `Source/Diagnostics/ComputeDiagFunctors/BackTransformParticleFunctor.H`
- kind: `struct`
- public methods: 1; public members: 19; private members: 0

```cpp
LorentzTransformParticles (const WarpXParticleContainer& pc, WarpXParIter& a_pti, amrex::Real t_boost, amrex::Real dt, amrex::Real t_lab, int a_offset = 0)
```

Public members:

```cpp
amrex::Real m_t_boost
amrex::Real m_dt
amrex::Real m_t_lab
amrex::Real m_gammaboost
amrex::Real m_betaboost
amrex::Real m_Phys_c
amrex::Real m_inv_c2
amrex::Real m_uzfrm
GetParticlePosition<PIdx> m_get_position
amrex::ParticleReal* AMREX_RESTRICT m_xpold = nullptr
amrex::ParticleReal* AMREX_RESTRICT m_ypold = nullptr
amrex::ParticleReal* AMREX_RESTRICT m_zpold = nullptr
amrex::ParticleReal* AMREX_RESTRICT m_uxpold = nullptr
amrex::ParticleReal* AMREX_RESTRICT m_uypold = nullptr
amrex::ParticleReal* AMREX_RESTRICT m_uzpold = nullptr
const amrex::ParticleReal* AMREX_RESTRICT m_uxpnew = nullptr
const amrex::ParticleReal* AMREX_RESTRICT m_uypnew = nullptr
const amrex::ParticleReal* AMREX_RESTRICT m_uzpnew = nullptr
const amrex::ParticleReal* AMREX_RESTRICT m_wpnew = nullptr
```

## `SelectParticles`

- file: `Source/Diagnostics/ComputeDiagFunctors/BackTransformParticleFunctor.H`
- kind: `struct`
- public methods: 1; public members: 4; private members: 0

```cpp
SelectParticles (const WarpXParticleContainer& pc, WarpXParIter& a_pti, amrex::Real current_z_boost, amrex::Real old_z_boost, int a_offset = 0)
```

Public members:

```cpp
GetParticlePosition<PIdx> m_get_position
amrex::Real m_current_z_boost
amrex::Real m_old_z_boost
amrex::ParticleReal* AMREX_RESTRICT zpold = nullptr
```

## `CellCenterFunctor`

- file: `Source/Diagnostics/ComputeDiagFunctors/CellCenterFunctor.H`
- kind: `class`, bases: `public ComputeDiagFunctor`
- public methods: 2; public members: 0; private members: 3

```cpp
CellCenterFunctor(const amrex::MultiFab * mf_src, int lev, amrex::IntVect crse_ratio, bool convertRZmodes2cartesian=true, int ncomp=1)
void operator()(amrex::MultiFab& mf_dst, int dcomp, int ) const override
```

## `ComputeDiagFunctor`

- file: `Source/Diagnostics/ComputeDiagFunctors/ComputeDiagFunctor.H`
- kind: `class`
- public methods: 11; public members: 0; private members: 1

```cpp
ComputeDiagFunctor( int ncomp, amrex::IntVect crse_ratio) : m_ncomp(ncomp), m_crse_ratio(crse_ratio)
/ virtual ~ComputeDiagFunctor() = default
ComputeDiagFunctor(const ComputeDiagFunctor&) = default
ComputeDiagFunctor& operator=(const ComputeDiagFunctor&) = default
ComputeDiagFunctor(ComputeDiagFunctor&&) = default
ComputeDiagFunctor& operator=(ComputeDiagFunctor&&) = default
virtual void operator() (amrex::MultiFab& mf_dst, int dcomp, int i_buffer = 0) const = 0
[[nodiscard]] int nComp () const
virtual void PrepareFunctorData ( int i_buffer, bool z_slice_in_domain, amrex::Real current_z_boost, amrex::Box buffer_box, const int k_index_zlab, const int snapshot_full)
virtual void InitData()
void InterpolateMFForDiag ( amrex::MultiFab& mf_dst, const amrex::MultiFab& mf_src, int dcomp, const amrex::DistributionMapping& dm, bool convertRZmodes2cartesian ) const
```

## `ComputeParticleDiagFunctor`

- file: `Source/Diagnostics/ComputeDiagFunctors/ComputeParticleDiagFunctor.H`
- kind: `class`
- public methods: 9; public members: 0; private members: 0

```cpp
ComputeParticleDiagFunctor( ) = default
virtual ~ComputeParticleDiagFunctor() = default
ComputeParticleDiagFunctor(const ComputeParticleDiagFunctor&) = default
ComputeParticleDiagFunctor& operator=(const ComputeParticleDiagFunctor&) = default
ComputeParticleDiagFunctor(ComputeParticleDiagFunctor&&) = default
ComputeParticleDiagFunctor& operator=(ComputeParticleDiagFunctor&&) = default
virtual void PrepareFunctorData ( int i_buffer, bool ZSliceInDomain, amrex::Real old_z_boost, amrex::Real current_z_boost, amrex::Real t_lab, int snapshot_full)
virtual void operator () (WarpXParticleContainer::Base& pc_dst, int &totalParticlesInBuffer, int i_buffer = 0) const = 0
virtual void InitData ()
```

## `DarwinEfieldFunctor`

- file: `Source/Diagnostics/ComputeDiagFunctors/DarwinEfieldFunctor.H`
- kind: `class`, bases: `public ComputeDiagFunctor`
- public methods: 2; public members: 0; private members: 3

```cpp
DarwinEfieldFunctor ( const amrex::MultiFab* mf_Efield, const amrex::MultiFab* mf_dA, int lev, amrex::IntVect crse_ratio, int ncomp=1 )
void operator()(amrex::MultiFab& mf_dst, int dcomp, int ) const override
```

## `DivBFunctor`

- file: `Source/Diagnostics/ComputeDiagFunctors/DivBFunctor.H`
- kind: `class`, bases: `public ComputeDiagFunctor`
- public methods: 2; public members: 0; private members: 3

```cpp
DivBFunctor ( ablastr::fields::VectorField const & arr_mf_src, int lev, amrex::IntVect crse_ratio, bool convertRZmodes2cartesian=true, int ncomp=1 )
void operator()(amrex::MultiFab& mf_dst, int dcomp, int ) const override
```

## `DivEFunctor`

- file: `Source/Diagnostics/ComputeDiagFunctors/DivEFunctor.H`
- kind: `class`, bases: `public ComputeDiagFunctor`
- public methods: 2; public members: 0; private members: 3

```cpp
DivEFunctor ( ablastr::fields::VectorField const & arr_mf_src, int lev, amrex::IntVect crse_ratio, bool convertRZmodes2cartesian=true, int ncomp=1 )
void operator()(amrex::MultiFab& mf_dst, int dcomp, int ) const override
```

## `EBCoveredFunctor`

- file: `Source/Diagnostics/ComputeDiagFunctors/EBCoveredFunctor.H`
- kind: `class`, bases: `public ComputeDiagFunctor`
- public methods: 2; public members: 0; private members: 1

```cpp
EBCoveredFunctor ( int lev, amrex::IntVect crse_ratio, int ncomp=1 )
void operator()(amrex::MultiFab& mf_dst, int dcomp, int ) const override
```

## `JFunctor`

- file: `Source/Diagnostics/ComputeDiagFunctors/JFunctor.H`
- kind: `class`, bases: `public ComputeDiagFunctor`
- public methods: 2; public members: 0; private members: 4

```cpp
JFunctor (int dir, int lev, amrex::IntVect crse_ratio, bool convertRZmodes2cartesian=true, bool deposit_current=false, int ncomp=1)
void operator()(amrex::MultiFab& mf_dst, int dcomp, int ) const override
```

## `JdispFunctor`

- file: `Source/Diagnostics/ComputeDiagFunctors/JdispFunctor.H`
- kind: `class`, bases: `public ComputeDiagFunctor`
- public methods: 2; public members: 0; private members: 3

```cpp
JdispFunctor (int dir, int lev, amrex::IntVect crse_ratio, bool convertRZmodes2cartesian=true, int ncomp=1)
void operator()(amrex::MultiFab& mf_dst, int dcomp, int ) const override
```

## `PartPerCellFunctor`

- file: `Source/Diagnostics/ComputeDiagFunctors/PartPerCellFunctor.H`
- kind: `class`, bases: `public ComputeDiagFunctor`
- public methods: 2; public members: 0; private members: 2

```cpp
PartPerCellFunctor(const amrex::MultiFab* mf_src, int lev, amrex::IntVect crse_ratio, int species_index=-1, int ncomp=1)
void operator()(amrex::MultiFab& mf_dst, int dcomp, int ) const override
```

## `PartPerGridFunctor`

- file: `Source/Diagnostics/ComputeDiagFunctors/PartPerGridFunctor.H`
- kind: `class`, bases: `public ComputeDiagFunctor`
- public methods: 2; public members: 0; private members: 1

```cpp
PartPerGridFunctor(const amrex::MultiFab * mf_src, int lev, amrex::IntVect crse_ratio, int ncomp=1)
void operator()(amrex::MultiFab& mf_dst, int dcomp, int ) const override
```

## `ParticleReductionFunctor`

- file: `Source/Diagnostics/ComputeDiagFunctors/ParticleReductionFunctor.H`
- kind: `class`, bases: `public ComputeDiagFunctor`
- public methods: 2; public members: 0; private members: 8

```cpp
ParticleReductionFunctor(const amrex::MultiFab * mf_src, int lev, amrex::IntVect crse_ratio, const std::string& fn_str, int ispec, bool do_average, bool do_filter, const std::string& filter_str, int ncomp=1)
void operator()(amrex::MultiFab& mf_dst, int dcomp, int ) const override
```

## `PhiFunctor`

- file: `Source/Diagnostics/ComputeDiagFunctors/PhiFunctor.H`
- kind: `class`, bases: `public ComputeDiagFunctor`
- public methods: 2; public members: 0; private members: 2

```cpp
PhiFunctor (int lev, amrex::IntVect crse_ratio, bool convertRZmodes2cartesian = true, int ncomp = 1)
void operator() ( amrex::MultiFab& mf_dst, int dcomp, int ) const override
```

## `ProcessNumberFunctor`

- file: `Source/Diagnostics/ComputeDiagFunctors/ProcessNumberFunctor.H`
- kind: `class`, bases: `public ComputeDiagFunctor`
- public methods: 2; public members: 0; private members: 1

```cpp
ProcessNumberFunctor(const amrex::MultiFab * mf_src, int lev, amrex::IntVect crse_ratio, int ncomp=1)
void operator()(amrex::MultiFab& mf_dst, int dcomp, int ) const override
```

## `RhoFunctor`

- file: `Source/Diagnostics/ComputeDiagFunctors/RhoFunctor.H`
- kind: `class`, bases: `public ComputeDiagFunctor`
- public methods: 2; public members: 0; private members: 4

```cpp
RhoFunctor (int lev, amrex::IntVect crse_ratio, bool apply_rz_psatd_filter = false, int species_index = -1, bool convertRZmodes2cartesian = true, int ncomp = 1)
void operator() ( amrex::MultiFab& mf_dst, int dcomp, int ) const override
```

## `TemperatureFunctor`

- file: `Source/Diagnostics/ComputeDiagFunctors/TemperatureFunctor.H`
- kind: `class`, bases: `public ComputeDiagFunctor`
- public methods: 2; public members: 0; private members: 2

```cpp
TemperatureFunctor(int lev, amrex::IntVect crse_ratio, int ispec, int ncomp=1)
void operator()(amrex::MultiFab& mf_dst, int dcomp, int ) const override
```

## `Diagnostics`

- file: `Source/Diagnostics/Diagnostics.H`
- kind: `class`
- public methods: 34; public members: 1; private members: 0

```cpp
Diagnostics (int i, std::string name, DiagTypes diag_type)
virtual ~Diagnostics ()
Diagnostics (Diagnostics const &) = delete
Diagnostics& operator= (Diagnostics const & ) = delete
Diagnostics(Diagnostics&& ) = default
Diagnostics& operator=(Diagnostics&& ) = default
void ComputeAndPack ()
virtual void Flush (int i_buffer, bool force_flush) = 0
void InitData (const MultiParticleContainer& mpc)
void InitDataBeforeRestart ()
void InitDataAfterRestart (const MultiParticleContainer& mpc)
virtual void InitializeFieldFunctors (int lev) = 0
virtual void InitializeFieldFunctorsRZopenPMD ([[maybe_unused]] int lev)
virtual void InitializeParticleFunctors ()
virtual bool DoComputeAndPack(int step, bool force_flush=false) = 0
virtual bool DoDump (int step, int i_buffer, bool force_flush=false) = 0
void NewIteration ()
void FilterComputePackFlush (int step, bool force_flush=false)
[[nodiscard]] bool DoDumpLastTimestep () const
[[nodiscard]] int getnumbuffers() const
virtual amrex::Real gettlab ( [[maybe_unused]] int i_buffer)
virtual void settlab ( [[maybe_unused]] int i_buffer, [[maybe_unused]] amrex::Real tlab)
virtual int get_buffer_k_index_hi ( [[maybe_unused]] int i_buffer)
virtual void set_buffer_k_index_hi ( [[maybe_unused]] int i_buffer, [[maybe_unused]] int kindex)
virtual amrex::Real get_snapshot_domain_lo ([[maybe_unused]] int i_buffer, [[maybe_unused]] int idim)
virtual amrex::Real get_snapshot_domain_hi ([[maybe_unused]] int i_buffer, [[maybe_unused]] int idim)
void setSnapshotDomainLo(int i_buffer, int idim, amrex::Real domain_lab_lo)
void setSnapshotDomainHi(int i_buffer, int idim, amrex::Real domain_lab_hi)
virtual int get_flush_counter ( [[maybe_unused]] int i_buffer)
virtual void set_flush_counter ( [[maybe_unused]] int i_buffer, [[maybe_unused]] int flush_counter)
virtual int get_last_valid_Zslice ( [[maybe_unused]] int i_buffer)
virtual void set_last_valid_Zslice ( [[maybe_unused]] int i_buffer, [[maybe_unused]] int last_valid_Zslice)
virtual int get_snapshot_full_flag ( [[maybe_unused]] int i_buffer)
virtual void set_snapshot_full ( [[maybe_unused]] int i_buffer, [[maybe_unused]] int snapshot_full)
```

Public members:

```cpp
DiagTypes m_diag_type
```

## `FlushFormat`

- file: `Source/Diagnostics/FlushFormats/FlushFormat.H`
- kind: `class`
- public methods: 7; public members: 0; private members: 0

```cpp
virtual void WriteToFile ( const amrex::Vector<std::string>& varnames, const amrex::Vector<amrex::MultiFab>& mf, amrex::Vector<amrex::Geometry>& geom, amrex::Vector<int> iteration, double time, const amrex::Vector<ParticleDiag>& particle_diags, int nlev, std::string prefix, int file_min_digits, bool plot_raw_fields, bool plot_raw_fields_guards, int verbose = 2, bool use_pinned_pc = false, bool isBTD = false, int snapshotID = -1, int bufferID = 1, int numBuffers = 1, const amrex::Geometry& full_BTD_snapshot = amrex::Geometry(), bool isLastBTDFlush = false) const = 0
FlushFormat () = default
virtual ~FlushFormat() = default
FlushFormat ( FlushFormat const &) = default
FlushFormat& operator= ( FlushFormat const & ) = default
FlushFormat ( FlushFormat&& ) = default
FlushFormat& operator= ( FlushFormat&& ) = default
```

## `FlushFormatAscent`

- file: `Source/Diagnostics/FlushFormats/FlushFormatAscent.H`
- kind: `class`, bases: `public FlushFormatInSitu`
- public methods: 7; public members: 0; private members: 0

```cpp
void WriteToFile ( const amrex::Vector<std::string>& varnames, const amrex::Vector<amrex::MultiFab>& mf, amrex::Vector<amrex::Geometry>& geom, amrex::Vector<int> iteration, double time, const amrex::Vector<ParticleDiag>& particle_diags, int nlev, std::string prefix, int file_min_digits, bool plot_raw_fields, bool plot_raw_fields_guards, int verbose = 2, bool use_pinned_pc = false, bool isBTD = false, int snapshotID = -1, int bufferID = 1, int numBuffers = 1, const amrex::Geometry& full_BTD_snapshot = amrex::Geometry(), bool isLastBTDFlush = false) const override
FlushFormatAscent () = default
~FlushFormatAscent() override = default
FlushFormatAscent ( FlushFormatAscent const &) = default
FlushFormatAscent& operator= ( FlushFormatAscent const & ) = default
FlushFormatAscent ( FlushFormatAscent&& ) = default
FlushFormatAscent& operator= ( FlushFormatAscent&& ) = default
```

## `FlushFormatCatalyst`

- file: `Source/Diagnostics/FlushFormats/FlushFormatCatalyst.H`
- kind: `class`, bases: `public FlushFormatInSitu`
- public methods: 8; public members: 0; private members: 0

```cpp
FlushFormatCatalyst()
void WriteToFile ( const amrex::Vector<std::string>& varnames, const amrex::Vector<amrex::MultiFab>& mf, amrex::Vector<amrex::Geometry>& geom, amrex::Vector<int> iteration, double time, const amrex::Vector<ParticleDiag>& particle_diags, int nlev, std::string prefix, int file_min_digits, bool plot_raw_fields, bool plot_raw_fields_guards, int verbose = 2, bool use_pinned_pc = false, bool isBTD = false, int snapshotID = -1, int bufferID = 1, int numBuffers = 1, const amrex::Geometry& full_BTD_snapshot = amrex::Geometry(), bool isLastBTDFlush = false) const override
~FlushFormatCatalyst() override
~FlushFormatCatalyst() override = default
FlushFormatCatalyst ( FlushFormatCatalyst const &) = default
FlushFormatCatalyst& operator= ( FlushFormatCatalyst const & ) = default
FlushFormatCatalyst ( FlushFormatCatalyst&& ) = default
FlushFormatCatalyst& operator= ( FlushFormatCatalyst&& ) = default
```

## `FlushFormatCheckpoint`

- file: `Source/Diagnostics/FlushFormats/FlushFormatCheckpoint.H`
- kind: `class`, bases: `public FlushFormatPlotfile`
- public methods: 0; public members: 0; private members: 0

## `FlushFormatInSitu`

- file: `Source/Diagnostics/FlushFormats/FlushFormatInSitu.H`
- kind: `class`, bases: `public FlushFormat`
- public methods: 7; public members: 0; private members: 0

```cpp
void WriteParticles(const amrex::Vector<ParticleDiag>& particle_diags, conduit::Node& a_bp_mesh) const
FlushFormatInSitu () = default
~FlushFormatInSitu() override = default
FlushFormatInSitu ( FlushFormatInSitu const &) = default
FlushFormatInSitu& operator= ( FlushFormatInSitu const & ) = default
FlushFormatInSitu ( FlushFormatInSitu&& ) = default
FlushFormatInSitu& operator= ( FlushFormatInSitu&& ) = default
```

## `FlushFormatOpenPMD`

- file: `Source/Diagnostics/FlushFormats/FlushFormatOpenPMD.H`
- kind: `class`, bases: `public FlushFormat`
- public methods: 7; public members: 0; private members: 2

```cpp
explicit FlushFormatOpenPMD (const std::string& diag_name)
void WriteToFile ( const amrex::Vector<std::string>& varnames, const amrex::Vector<amrex::MultiFab>& mf, amrex::Vector<amrex::Geometry>& geom, amrex::Vector<int> iteration, double time, const amrex::Vector<ParticleDiag>& particle_diags, int output_levels, std::string prefix, int file_min_digits, bool plot_raw_fields, bool plot_raw_fields_guards, int verbose, bool use_pinned_pc = false, bool isBTD = false, int snapshotID = -1, int bufferID = 1, int numBuffers = 1, const amrex::Geometry& full_BTD_snapshot = amrex::Geometry(), bool isLastBTDFlush = false) const override
~FlushFormatOpenPMD () override = default
FlushFormatOpenPMD ( FlushFormatOpenPMD const &) = delete
FlushFormatOpenPMD& operator= ( FlushFormatOpenPMD const & ) = delete
FlushFormatOpenPMD ( FlushFormatOpenPMD&& ) = default
FlushFormatOpenPMD& operator= ( FlushFormatOpenPMD&& ) = default
```

## `FlushFormatPlotfile`

- file: `Source/Diagnostics/FlushFormats/FlushFormatPlotfile.H`
- kind: `class`, bases: `public FlushFormat`
- public methods: 11; public members: 0; private members: 0

```cpp
void WriteToFile ( const amrex::Vector<std::string>& varnames, const amrex::Vector<amrex::MultiFab>& mf, amrex::Vector<amrex::Geometry>& geom, amrex::Vector<int> iteration, double time, const amrex::Vector<ParticleDiag>& particle_diags, int nlev, std::string prefix, int file_min_digits, bool plot_raw_fields, bool plot_raw_fields_guards, int verbose = 2, bool use_pinned_pc = false, bool isBTD = false, int snapshotID = -1, int bufferID = 1, int numBuffers = 1, const amrex::Geometry& full_BTD_snapshot = amrex::Geometry(), bool isLastBTDFlush = false) const override
void WriteJobInfo(const std::string& dir) const
void WriteWarpXHeader(const std::string& name, amrex::Vector<amrex::Geometry>& geom, bool is_checkpoint = false) const
void WriteAllRawFields (bool plot_raw_fields, int nlevels, const std::string& plotfilename, bool plot_raw_fields_guards) const
void WriteParticles(const std::string& dir, const amrex::Vector<ParticleDiag>& particle_diags, amrex::Real time, bool isBTD = false) const
FlushFormatPlotfile () = default
~FlushFormatPlotfile() override = default
FlushFormatPlotfile ( FlushFormatPlotfile const &) = default
FlushFormatPlotfile& operator= ( FlushFormatPlotfile const & ) = default
FlushFormatPlotfile ( FlushFormatPlotfile&& ) = default
FlushFormatPlotfile& operator= ( FlushFormatPlotfile&& ) = default
```

## `FlushFormatSensei`

- file: `Source/Diagnostics/FlushFormats/FlushFormatSensei.H`
- kind: `class`, bases: `public FlushFormat`
- public methods: 9; public members: 0; private members: 4

```cpp
FlushFormatSensei()
~FlushFormatSensei() override
FlushFormatSensei(const FlushFormatSensei &) = delete
FlushFormatSensei& operator=(const FlushFormatSensei &) = delete
FlushFormatSensei(FlushFormatSensei&&) = default
FlushFormatSensei& operator=(FlushFormatSensei&& ) = default
FlushFormatSensei (amrex::AmrMesh *amr_mesh, const std::string& diag_name)
void WriteToFile ( const amrex::Vector<std::string>& varnames, const amrex::Vector<amrex::MultiFab>& mf, amrex::Vector<amrex::Geometry>& geom, amrex::Vector<int> iteration, double time, const amrex::Vector<ParticleDiag>& particle_diags, int nlev, std::string prefix, int file_min_digits, bool plot_raw_fields, bool plot_raw_fields_guards, int verbose = 2, bool use_pinned_pc = false, bool isBTD = false, int snapshotID = -1, int bufferID = 1, int numBuffers = 1, const amrex::Geometry& full_BTD_snapshot = amrex::Geometry(), bool isLastBTDFlush = false) const override
void WriteParticles(const amrex::Vector<ParticleDiag>& particle_diags) const
```

## `FullDiagnostics`

- file: `Source/Diagnostics/FullDiagnostics.H`
- kind: `class`, bases: `public Diagnostics`
- public methods: 1; public members: 1; private members: 9

```cpp
FullDiagnostics (int i, const std::string& name, DiagTypes diag_type)
```

Public members:

```cpp
enum struct TimeAverageType
```

## `MultiDiagnostics`

- file: `Source/Diagnostics/MultiDiagnostics.H`
- kind: `class`
- public methods: 11; public members: 0; private members: 4

```cpp
MultiDiagnostics ()
void ReadParameters ()
void InitData (const MultiParticleContainer& mpc)
bool DoComputeAndPack (int step, bool force_flush=false)
void FilterComputePackFlush (int step, bool force_flush=false, bool BackTransform=false)
void FilterComputePackFlushLastTimestep (int step)
void InitializeFieldFunctors (int lev)
void NewIteration ()
Diagnostics& GetDiag(int idiag)
[[nodiscard]] int GetTotalDiags() const
DiagTypes diagstypes(int idiag)
```

## `ParticleDiag`

- file: `Source/Diagnostics/ParticleDiag/ParticleDiag.H`
- kind: `class`
- public methods: 4; public members: 17; private members: 4

```cpp
ParticleDiag(const std::string& diag_name, const std::string& name, WarpXParticleContainer* pc, WarpXParticleContainer::Base *pinned_pc = nullptr)
[[nodiscard]] WarpXParticleContainer* getParticleContainer() const
[[nodiscard]] WarpXParticleContainer::Base* getPinnedParticleContainer() const
[[nodiscard]] std::string getSpeciesName() const
```

Public members:

```cpp
amrex::Vector<int> m_plot_flags
bool m_plot_phi = false
bool m_plot_Ex = false
bool m_plot_Ey = false
bool m_plot_Ez = false
bool m_plot_Bx = false
bool m_plot_By = false
bool m_plot_Bz = false
bool m_do_random_filter = false
bool m_do_uniform_filter = false
bool m_do_parser_filter = false
bool m_do_geom_filter = false
amrex::Real m_random_fraction = 1.0
int m_uniform_stride = 1
static constexpr int m_nvars = 7
std::unique_ptr<amrex::Parser> m_particle_filter_parser
amrex::RealBox m_diag_domain
```

## `BeamRelevant`

- file: `Source/Diagnostics/ReducedDiags/BeamRelevant.H`
- kind: `class`, bases: `public ReducedDiags`
- public methods: 2; public members: 1; private members: 0

```cpp
explicit BeamRelevant(const std::string& rd_name)
void ComputeDiags(int step) final
```

Public members:

```cpp
std::string m_beam_name
```

## `ChargeOnEB`

- file: `Source/Diagnostics/ReducedDiags/ChargeOnEB.H`
- kind: `class`, bases: `public ReducedDiags`
- public methods: 2; public members: 0; private members: 2

```cpp
explicit ChargeOnEB (const std::string& rd_name)
void ComputeDiags (int step) final
```

## `ColliderRelevant`

- file: `Source/Diagnostics/ReducedDiags/ColliderRelevant.H`
- kind: `class`, bases: `public ReducedDiags`
- public methods: 2; public members: 1; private members: 2

```cpp
explicit ColliderRelevant(const std::string& rd_name)
void ComputeDiags(int step) final
```

Public members:

```cpp
std::vector<std::string> m_beam_name
```

## `aux_header_index`

- file: `Source/Diagnostics/ReducedDiags/ColliderRelevant.H`
- kind: `struct`
- public methods: 0; public members: 2; private members: 0

Public members:

```cpp
std::string header
int idx
```

## `DifferentialLuminosity`

- file: `Source/Diagnostics/ReducedDiags/DifferentialLuminosity.H`
- kind: `class`, bases: `public ReducedDiags`
- public methods: 2; public members: 5; private members: 3

```cpp
explicit DifferentialLuminosity(const std::string& rd_name)
void ComputeDiags(int step) final
```

Public members:

```cpp
std::vector<std::string> m_beam_name
int m_bin_num
amrex::Real m_bin_max
amrex::Real m_bin_min
amrex::Real m_bin_size
```

## `aux_header_index`

- file: `Source/Diagnostics/ReducedDiags/DifferentialLuminosity.H`
- kind: `struct`
- public methods: 0; public members: 2; private members: 0

Public members:

```cpp
std::string header
int idx
```

## `DifferentialLuminosity2D`

- file: `Source/Diagnostics/ReducedDiags/DifferentialLuminosity2D.H`
- kind: `class`, bases: `public ReducedDiags`
- public methods: 3; public members: 12; private members: 1

```cpp
explicit DifferentialLuminosity2D(const std::string& rd_name)
void ComputeDiags(int step) final
void WriteToFile (int step) const final
```

Public members:

```cpp
std::string m_openpmd_backend
int m_file_min_digits = 6
std::vector<std::string> m_beam_name
int m_bin_num_1
int m_bin_num_2
amrex::Real m_bin_max_1
amrex::Real m_bin_min_1
amrex::Real m_bin_max_2
amrex::Real m_bin_min_2
amrex::Real m_bin_size_1
amrex::Real m_bin_size_2
amrex::TableData<amrex::Real,2> m_h_data_2D
```

## `FieldEnergy`

- file: `Source/Diagnostics/ReducedDiags/FieldEnergy.H`
- kind: `class`, bases: `public ReducedDiags`
- public methods: 3; public members: 0; private members: 0

```cpp
explicit FieldEnergy(const std::string& rd_name)
void ComputeDiags(int step) final
amrex::Real ComputeNorm2(const amrex::MultiFab& field, int lev)
```

## `FieldMaximum`

- file: `Source/Diagnostics/ReducedDiags/FieldMaximum.H`
- kind: `class`, bases: `public ReducedDiags`
- public methods: 2; public members: 0; private members: 0

```cpp
explicit FieldMaximum(const std::string& rd_name)
void ComputeDiags(int step) final
```

## `FieldMomentum`

- file: `Source/Diagnostics/ReducedDiags/FieldMomentum.H`
- kind: `class`, bases: `public ReducedDiags`
- public methods: 2; public members: 0; private members: 0

```cpp
explicit FieldMomentum(const std::string& rd_name)
void ComputeDiags(int step) final
```

## `FieldPoyntingFlux`

- file: `Source/Diagnostics/ReducedDiags/FieldPoyntingFlux.H`
- kind: `class`, bases: `public ReducedDiags`
- public methods: 6; public members: 0; private members: 1

```cpp
explicit FieldPoyntingFlux (const std::string& rd_name)
void ComputeDiags (int step) final
void ComputeDiagsMidStep (int step, amrex::Real dt) final
void ComputePoyntingFlux (amrex::Real dt)
void WriteCheckpointData (std::string const & dir) final
void ReadCheckpointData (std::string const & dir) final
```

## `PoyntingCellCentered`

- file: `Source/Diagnostics/ReducedDiags/FieldPoyntingFlux.H`
- kind: `struct`
- public methods: 0; public members: 0; private members: 0

## `PoyntingNodal`

- file: `Source/Diagnostics/ReducedDiags/FieldPoyntingFlux.H`
- kind: `struct`
- public methods: 0; public members: 0; private members: 0

## `PoyntingStaggered`

- file: `Source/Diagnostics/ReducedDiags/FieldPoyntingFlux.H`
- kind: `struct`
- public methods: 0; public members: 0; private members: 0

## `FieldProbe`

- file: `Source/Diagnostics/ReducedDiags/FieldProbe.H`
- kind: `class`, bases: `public ReducedDiags`
- public methods: 4; public members: 1; private members: 22

```cpp
explicit FieldProbe (const std::string& rd_name)
void InitData () final
void LoadBalance () final
void ComputeDiags (int step) final
```

Public members:

```cpp
static const int noutputs = 11
```

## `FieldProbePIdx`

- file: `Source/Diagnostics/ReducedDiags/FieldProbeParticleContainer.H`
- kind: `struct`
- public methods: 0; public members: 1; private members: 0

Public members:

```cpp
enum
```

## `FieldProbeParticleContainer`

- file: `Source/Diagnostics/ReducedDiags/FieldProbeParticleContainer.H`
- kind: `class`, bases: `public amrex::ParticleContainerPureSoA<FieldProbePIdx::nattribs, 0>`
- public methods: 7; public members: 4; private members: 0

```cpp
explicit FieldProbeParticleContainer (amrex::AmrCore* amr_core)
~FieldProbeParticleContainer() override = default
FieldProbeParticleContainer ( FieldProbeParticleContainer const &) = delete
FieldProbeParticleContainer& operator= ( FieldProbeParticleContainer const & ) = delete
FieldProbeParticleContainer ( FieldProbeParticleContainer&& ) = default
FieldProbeParticleContainer& operator= ( FieldProbeParticleContainer&& ) = default
void AddNParticles (int lev, amrex::Vector<amrex::ParticleReal> const & x, amrex::Vector<amrex::ParticleReal> const & y, amrex::Vector<amrex::ParticleReal> const & z)
```

Public members:

```cpp
static constexpr int NStructReal = 0
static constexpr int NStructInt = 0
static constexpr int NReal = FieldProbePIdx::nattribs
static constexpr int NInt = 0
```

## `FieldReduction`

- file: `Source/Diagnostics/ReducedDiags/FieldReduction.H`
- kind: `class`, bases: `public ReducedDiags`
- public methods: 4; public members: 0; private members: 3

```cpp
explicit FieldReduction(const std::string& rd_name)
void ComputeDiags(int step) final
void BackwardCompatibility()
void ComputeFieldReduction()
```

## `LoadBalanceCosts`

- file: `Source/Diagnostics/ReducedDiags/LoadBalanceCosts.H`
- kind: `class`, bases: `public ReducedDiags`
- public methods: 3; public members: 8; private members: 0

```cpp
explicit LoadBalanceCosts(const std::string& rd_name)
void ComputeDiags(int step) final
void WriteToFile(int step) const final
```

Public members:

```cpp
amrex::Vector<char> m_data_string_recvbuf
int m_data_string_recvbuf_length = 0
std::vector<std::string> m_data_string
amrex::Vector<int> m_data_string_recvcount
amrex::Vector<int> m_data_string_disp
static const int m_nDataFields = 9
static const int m_nDataFields = 8
int m_nBoxesMax = -1
```

## `LoadBalanceEfficiency`

- file: `Source/Diagnostics/ReducedDiags/LoadBalanceEfficiency.H`
- kind: `class`, bases: `public ReducedDiags`
- public methods: 2; public members: 0; private members: 0

```cpp
explicit LoadBalanceEfficiency(const std::string& rd_name)
void ComputeDiags(int step) final
```

## `MultiReducedDiags`

- file: `Source/Diagnostics/ReducedDiags/MultiReducedDiags.H`
- kind: `class`
- public methods: 9; public members: 3; private members: 0

```cpp
MultiReducedDiags ()
void InitData ()
void LoadBalance ()
void ComputeDiags (int step)
void ComputeDiagsMidStep (int step, amrex::Real dt)
void WriteToFile (int step)
bool DoDiags(int step)
void WriteCheckpointData (std::string const & dir)
void ReadCheckpointData (std::string const & dir)
```

Public members:

```cpp
int m_plot_rd = 0
std::vector<std::string> m_rd_names
std::vector<std::unique_ptr<ReducedDiags>> m_multi_rd
```

## `ParticleEnergy`

- file: `Source/Diagnostics/ReducedDiags/ParticleEnergy.H`
- kind: `class`, bases: `public ReducedDiags`
- public methods: 2; public members: 0; private members: 0

```cpp
explicit ParticleEnergy(const std::string& rd_name)
void ComputeDiags(int step) final
```

## `ParticleExtrema`

- file: `Source/Diagnostics/ReducedDiags/ParticleExtrema.H`
- kind: `class`, bases: `public ReducedDiags`
- public methods: 2; public members: 1; private members: 2

```cpp
explicit ParticleExtrema(const std::string& rd_name)
void ComputeDiags(int step) final
```

Public members:

```cpp
std::string m_species_name
```

## `aux_header_index`

- file: `Source/Diagnostics/ReducedDiags/ParticleExtrema.H`
- kind: `struct`
- public methods: 0; public members: 2; private members: 0

Public members:

```cpp
std::string header
int idx
```

## `ParticleHistogram`

- file: `Source/Diagnostics/ReducedDiags/ParticleHistogram.H`
- kind: `class`, bases: `public ReducedDiags`
- public methods: 2; public members: 10; private members: 0

```cpp
explicit ParticleHistogram(const std::string& rd_name)
void ComputeDiags(int step) final
```

Public members:

```cpp
int m_norm
int m_bin_num
int m_selected_species_id = -1
amrex::Real m_bin_max
amrex::Real m_bin_min
amrex::Real m_bin_size
static constexpr int m_nvars = 7
std::unique_ptr<amrex::Parser> m_parser
std::unique_ptr<amrex::Parser> m_parser_filter
bool m_do_parser_filter = false
```

## `ParticleHistogram2D`

- file: `Source/Diagnostics/ReducedDiags/ParticleHistogram2D.H`
- kind: `class`, bases: `public ReducedDiags`
- public methods: 3; public members: 23; private members: 0

```cpp
explicit ParticleHistogram2D(const std::string& rd_name)
void ComputeDiags(int step) final
void WriteToFile (int step) const final
```

Public members:

```cpp
std::string m_openpmd_backend
int m_file_min_digits = 6
int m_bin_num_abs
int m_bin_num_ord
amrex::TableData<amrex::ParticleReal,2> m_h_data_2D
int m_selected_species_id = -1
amrex::Real m_bin_max_abs
amrex::Real m_bin_min_abs
amrex::Real m_bin_max_ord
amrex::Real m_bin_min_ord
amrex::Real m_bin_size_abs
amrex::Real m_bin_size_ord
std::string function_string_abs
std::string function_string_ord
std::string filter_string
std::string value_string
static constexpr int m_nvars = 8
std::unique_ptr<amrex::Parser> m_parser_abs
std::unique_ptr<amrex::Parser> m_parser_ord
std::unique_ptr<amrex::Parser> m_parser_filter
bool m_do_parser_filter = false
std::unique_ptr<amrex::Parser> m_parser_value
bool m_do_parser_value
```

## `ParticleMomentum`

- file: `Source/Diagnostics/ReducedDiags/ParticleMomentum.H`
- kind: `class`, bases: `public ReducedDiags`
- public methods: 2; public members: 0; private members: 0

```cpp
explicit ParticleMomentum(const std::string& rd_name)
void ComputeDiags(int step) final
```

## `ParticleNumber`

- file: `Source/Diagnostics/ReducedDiags/ParticleNumber.H`
- kind: `class`, bases: `public ReducedDiags`
- public methods: 2; public members: 0; private members: 0

```cpp
explicit ParticleNumber(const std::string& rd_name)
void ComputeDiags(int step) final
```

## `ReducedDiags`

- file: `Source/Diagnostics/ReducedDiags/ReducedDiags.H`
- kind: `class`
- public methods: 15; public members: 8; private members: 0

```cpp
explicit ReducedDiags (const std::string& rd_name)
virtual ~ReducedDiags () = default
ReducedDiags(const ReducedDiags&) = default
ReducedDiags& operator=(const ReducedDiags&) = default
ReducedDiags(ReducedDiags&&) = default
ReducedDiags& operator=(ReducedDiags&&) = default
virtual void InitData ()
virtual void LoadBalance ()
virtual void ComputeDiags (int step) = 0
virtual void ComputeDiagsMidStep (int step, amrex::Real dt)
virtual void WriteToFile (int step) const
[[nodiscard]] bool DoDiags(int step) const
virtual void WriteCheckpointData (std::string const & dir)
virtual void ReadCheckpointData (std::string const & dir)
void BackwardCompatibility () const
```

Public members:

```cpp
std::string m_path = "./diags/reducedfiles/"
std::string m_extension = "txt"
std::string m_rd_name
ablastr::utils::text::IntervalsParser m_intervals
bool m_write_header = false
std::string m_sep = " "
int m_precision = 14
std::vector<amrex::Real> m_data
```

## `RhoMaximum`

- file: `Source/Diagnostics/ReducedDiags/RhoMaximum.H`
- kind: `class`, bases: `public ReducedDiags`
- public methods: 2; public members: 0; private members: 1

```cpp
explicit RhoMaximum(const std::string& rd_name)
void ComputeDiags(int step) final
```

## `Timestep`

- file: `Source/Diagnostics/ReducedDiags/Timestep.H`
- kind: `class`, bases: `public ReducedDiags`
- public methods: 2; public members: 0; private members: 0

```cpp
explicit Timestep (const std::string& rd_name)
void ComputeDiags (int step) final
```

## `WarpXOpenPMDPlot`

- file: `Source/Diagnostics/WarpXOpenPMD.H`
- kind: `class`
- public methods: 12; public members: 0; private members: 11

```cpp
WarpXOpenPMDPlot (openPMD::IterationEncoding ie, const std::string& filetype, const std::string& operator_type, const std::map< std::string, std::string >& operator_parameters, const std::string& engine_type, const std::map< std::string, std::string >& engine_parameters, const std::vector<bool>& fieldPMLdirections, const std::string& authors)
~WarpXOpenPMDPlot ()
WarpXOpenPMDPlot ( WarpXOpenPMDPlot const &) = delete
WarpXOpenPMDPlot& operator= ( WarpXOpenPMDPlot const & ) = delete
WarpXOpenPMDPlot ( WarpXOpenPMDPlot&& ) = default
WarpXOpenPMDPlot& operator= ( WarpXOpenPMDPlot&& ) = default
void SetStep (int ts, const std::string& dirPrefix, int file_min_digits, bool isBTD=false)
void CloseStep (bool isBTD = false, bool isLastBTDFlush = false)
void WriteOpenPMDParticles ( const amrex::Vector<ParticleDiag>& particle_diags, amrex::Real time, bool use_pinned_pc = false, bool isBTD = false, bool isLastBTDFlush = false)
void WriteOpenPMDFieldsAll ( const std::vector<std::string>& varnames, const amrex::Vector<amrex::MultiFab>& mf, amrex::Vector<amrex::Geometry>& geom, int output_levels, int iteration, double time, bool isBTD = false, const amrex::Geometry& full_BTD_snapshot=amrex::Geometry() ) const
std::string OpenPMDFileType ()
void FlushBTDToDisk ()
```

## `WarpXParticleCounter`

- file: `Source/Diagnostics/WarpXOpenPMD.H`
- kind: `class`
- public methods: 2; public members: 2; private members: 4

```cpp
explicit WarpXParticleCounter (ParticleContainer* pc)
[[nodiscard]] unsigned long GetTotalNumParticles () const
```

Public members:

```cpp
std::vector<unsigned long long> m_ParticleOffsetAtRank
std::vector<unsigned long long> m_ParticleSizeAtRank
```

