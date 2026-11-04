# A0 API catalog — NonlinearSolvers

> Generated from headers; 23 classes/structs. Signatures are parsed heuristically; check the header for exact defaults/templates.

## `AMReXGMRES`

- file: `Source/NonlinearSolvers/AMReXGMRES_Wrapper.H`
- kind: `class`, bases: `public LinearSolver<Vec,LinOp>`
- public methods: 6; public members: 0; private members: 1

```cpp
AMReXGMRES () = default
~AMReXGMRES () override = default
AMReXGMRES(const AMReXGMRES&) = delete
AMReXGMRES& operator=(const AMReXGMRES&) = delete
AMReXGMRES(AMReXGMRES&&) noexcept = delete
AMReXGMRES& operator=(AMReXGMRES&&) noexcept = delete
```

## `AlgMGPC`

- file: `Source/NonlinearSolvers/AlgMGPC.H`
- kind: `class`, bases: `public Preconditioner<T,Ops>`
- public methods: 11; public members: 0; private members: 0

```cpp
AlgMGPC () = default
~AlgMGPC () override = default
AlgMGPC (const AlgMGPC&) = delete
AlgMGPC& operator= (const AlgMGPC&) = delete
AlgMGPC (AlgMGPC&&) noexcept = delete
AlgMGPC& operator= (AlgMGPC&&) noexcept = delete
void Define (const T&, Ops*) override
void Update () override
void Apply (T&, const T&) override
void printParameters () const override
[[nodiscard]] inline bool IsDefined () const override
```

## `CurlCurlMLMGPC`

- file: `Source/NonlinearSolvers/CurlCurlMLMGPC.H`
- kind: `class`, bases: `public Preconditioner<T,Ops>`
- public methods: 11; public members: 0; private members: 0

```cpp
CurlCurlMLMGPC () = default
~CurlCurlMLMGPC () override = default
CurlCurlMLMGPC(const CurlCurlMLMGPC&) = delete
CurlCurlMLMGPC& operator=(const CurlCurlMLMGPC&) = delete
CurlCurlMLMGPC(CurlCurlMLMGPC&&) noexcept = delete
CurlCurlMLMGPC& operator=(CurlCurlMLMGPC&&) noexcept = delete
void Define (const T&, Ops*) override
void Update () override
void Apply (T&, const T&) override
void printParameters() const override
[[nodiscard]] inline bool IsDefined () const override
```

## `DarwinMLMGPC`

- file: `Source/NonlinearSolvers/DarwinMLMGPC.H`
- kind: `class`, bases: `public Preconditioner<T,Ops>`
- public methods: 11; public members: 0; private members: 0

```cpp
DarwinMLMGPC () = default
~DarwinMLMGPC () override = default
DarwinMLMGPC (const DarwinMLMGPC&) = delete
DarwinMLMGPC& operator= (const DarwinMLMGPC&) = delete
DarwinMLMGPC (DarwinMLMGPC&&) noexcept = delete
DarwinMLMGPC& operator= (DarwinMLMGPC&&) noexcept = delete
void Define (const T&, Ops*) override
void Update () override
void Apply (T&, const T&) override
void printParameters () const override
[[nodiscard]] inline bool IsDefined () const override
```

## `ExternLibPC`

- file: `Source/NonlinearSolvers/ExternLibPC.H`
- kind: `class`, bases: `public Preconditioner<T,Ops>`
- public methods: 13; public members: 0; private members: 0

```cpp
ExternLibPC () = default
~ExternLibPC () override = default
ExternLibPC(const ExternLibPC&) = delete
ExternLibPC& operator=(const ExternLibPC&) = delete
ExternLibPC(ExternLibPC&&) noexcept = delete
ExternLibPC& operator=(ExternLibPC&&) noexcept = delete
void Define (const T&, Ops*) override
void Update () override
void Apply (T&, const T&) override
inline void getPCMatrix (amrex::Gpu::DeviceVector<int>& a_r_indices_g, amrex::Gpu::DeviceVector<int>& a_num_nz, amrex::Gpu::DeviceVector<int>& a_c_indices_g, amrex::Gpu::DeviceVector<RT>& a_a_ij, int& a_n, int& a_ncols_max) override
void printParameters() const override
[[nodiscard]] inline bool IsDefined () const override
inline void setName (const std::string& a_name) override
```

## `JacobiPC`

- file: `Source/NonlinearSolvers/JacobiPC.H`
- kind: `class`, bases: `public Preconditioner<T,Ops>`
- public methods: 11; public members: 0; private members: 0

```cpp
JacobiPC () = default
~JacobiPC () override = default
JacobiPC(const JacobiPC&) = delete
JacobiPC& operator=(const JacobiPC&) = delete
JacobiPC(JacobiPC&&) noexcept = delete
JacobiPC& operator=(JacobiPC&&) noexcept = delete
void Define (const T&, Ops*) override
void Update () override
void Apply (T&, const T&) override
void printParameters() const override
[[nodiscard]] inline bool IsDefined () const override
```

## `JacobianFunctionMF`

- file: `Source/NonlinearSolvers/JacobianFunctionMF.H`
- kind: `class`, bases: `public LinearFunction<T,Ops>`
- public methods: 23; public members: 0; private members: 9

```cpp
JacobianFunctionMF() = default
~JacobianFunctionMF() override = default
JacobianFunctionMF(const JacobianFunctionMF&) = default
JacobianFunctionMF& operator=(const JacobianFunctionMF&) = default
JacobianFunctionMF(JacobianFunctionMF&&) noexcept = default
JacobianFunctionMF& operator=(JacobianFunctionMF&&) noexcept = default
void apply ( T& a_dF, const T& a_dU ) override
inline void precond ( T& a_U, const T& a_X ) override
inline void updatePreCondMat () override
inline void getPCMatrix( amrex::Gpu::DeviceVector<int>& a_ridx_g, amrex::Gpu::DeviceVector<int>& a_nnz, amrex::Gpu::DeviceVector<int>& a_cidx_g, amrex::Gpu::DeviceVector<RT>& a_aij, int& a_n, int& a_ncols_max ) override
T makeVecLHS () const override
T makeVecRHS () const override
[[nodiscard]] inline bool isDefined() const
[[nodiscard]] inline bool usePreconditioner() const
inline void setBaseSolution ( const T& a_U )
inline void setBaseRHS ( const T& a_R )
inline void setJFNKEps ( RT a_eps )
inline void setIsLinear ( bool a_isLinear )
inline void curTime ( RT a_time )
inline void curTimeStep ( RT a_dt )
inline void printParams () const
void define( const T&, Ops*, const PreconditionerType& ) override
[[nodiscard]] inline PreconditionerType pcType () const override
```

## `LinearFunction`

- file: `Source/NonlinearSolvers/LinearFunction.H`
- kind: `class`
- public methods: 23; public members: 0; private members: 0

```cpp
LinearFunction() = default
virtual ~LinearFunction() = default
LinearFunction(const LinearFunction&) = default
LinearFunction& operator=(const LinearFunction&) = default
LinearFunction(LinearFunction&&) noexcept = default
LinearFunction& operator=(LinearFunction&&) noexcept = default
virtual void apply ( T& a_Ax, const T& a_x ) = 0
virtual void precond ( T& a_U, const T& a_X ) = 0
virtual void updatePreCondMat () = 0
virtual void getPCMatrix( amrex::Gpu::DeviceVector<int>&, amrex::Gpu::DeviceVector<int>&, amrex::Gpu::DeviceVector<int>&, amrex::Gpu::DeviceVector<RT>&, int&, int& ) = 0
inline void create ( T& a_Z, const T& a_U )
[[nodiscard]] virtual T makeVecLHS () const = 0
[[nodiscard]] virtual T makeVecRHS () const = 0
inline void assign( T& a_Z, const T& a_U )
inline void increment( T& a_Z, const T& a_U, RT a_scale )
inline void scale ( T& a_U, RT a_scale )
inline void linComb ( T& a_U, RT a, const T& X, RT b, const T& Y )
inline void setToZero ( T& a_U )
inline void setVal ( T& a_U, RT a_val )
inline RT dotProduct( const T& a_X, const T& a_Y )
inline RT norm2( const T& a_U )
virtual void define( const T&, Ops*, const PreconditionerType& ) = 0
[[nodiscard]] virtual PreconditionerType pcType () const = 0
```

## `LinearSolver`

- file: `Source/NonlinearSolvers/LinearSolver.H`
- kind: `class`
- public methods: 17; public members: 0; private members: 0

```cpp
LinearSolver () = default
virtual ~LinearSolver() = default
LinearSolver(const LinearSolver&) = delete
LinearSolver& operator=(const LinearSolver&) = delete
LinearSolver(LinearSolver&&) noexcept = delete
LinearSolver& operator=(LinearSolver&&) noexcept = delete
virtual void define (LinOp& linop) = 0
virtual void solve (Vec& a_sol, Vec const& a_rhs, RT a_tol_rel, RT a_tol_abs, int a_its=-1) = 0
virtual void invalidatePreconditioner ()
virtual bool reinitializePETSc ()
virtual void setVerbose (int v) = 0
virtual void setRestartLength (int rl)
virtual void setMaxIters (int niters) = 0
[[nodiscard]] virtual int getNumIters () const = 0
[[nodiscard]] virtual int getStatus () const = 0
[[nodiscard]] virtual bool converged () const = 0
[[nodiscard]] virtual RT getResidualNorm () const = 0
```

## `MatrixPCAlgMG`

- file: `Source/NonlinearSolvers/MatrixPCAlgMG.H`
- kind: `class`
- public methods: 11; public members: 0; private members: 2

```cpp
MatrixPCAlgMG (const std::string& a_name, const amrex::Vector<amrex::Long>& a_row_offsets)
~MatrixPCAlgMG ()
MatrixPCAlgMG (const MatrixPCAlgMG&) = delete
MatrixPCAlgMG& operator= (const MatrixPCAlgMG&) = delete
MatrixPCAlgMG (MatrixPCAlgMG&&) noexcept = delete
MatrixPCAlgMG& operator= (MatrixPCAlgMG&&) noexcept = delete
void printParameters () const
void Setup (int a_nrows, int a_ncols_max, const int* a_num_nz, const int* a_cols, const amrex::Real* a_vals)
[[nodiscard]] amrex::Real* rhs ()
[[nodiscard]] const amrex::Real* sol () const
void Apply ()
```

## `MatrixSolverOps`

- file: `Source/NonlinearSolvers/MatrixSolverOps.H`
- kind: `class`
- public methods: 14; public members: 0; private members: 0

```cpp
MatrixSolverOps () = default
virtual ~MatrixSolverOps () = default
MatrixSolverOps (const MatrixSolverOps&) = delete
MatrixSolverOps& operator= (const MatrixSolverOps&) = delete
MatrixSolverOps (MatrixSolverOps&&) noexcept = delete
MatrixSolverOps& operator= (MatrixSolverOps&&) noexcept = delete
virtual void Update () = 0
virtual void InitialGuess (T& a_x, const T& a_b) = 0
virtual void JacobiSweep (T& a_x, const T& a_b, RT a_omega) = 0
[[nodiscard]] virtual RT ResidualNorm (const T& a_x, const T& a_b) = 0
[[nodiscard]] virtual RT EstimateOmega () const = 0
[[nodiscard]] virtual bool HasOmegaEstimate () const
[[nodiscard]] virtual bool HasOffDiag () const = 0
[[nodiscard]] virtual bool IsDefined () const = 0
```

## `NewtonSolver`

- file: `Source/NonlinearSolvers/NewtonSolver.H`
- kind: `class`, bases: `public NonlinearSolver<Vec,Ops>`
- public methods: 15; public members: 0; private members: 10

```cpp
NewtonSolver() = default
~NewtonSolver() override = default
NewtonSolver(const NewtonSolver&) = delete
NewtonSolver& operator=(const NewtonSolver&) = delete
NewtonSolver(NewtonSolver&&) noexcept = delete
NewtonSolver& operator=(NewtonSolver&&) noexcept = delete
void Define (const Vec& a_U, Ops* a_ops) override
void Solve (Vec& a_U, const Vec& a_b, amrex::Real a_time, amrex::Real a_dt, int a_step, bool verbose_step) const override
inline void CurTime ( amrex::Real a_time ) const
inline void CurTimeStep ( amrex::Real a_dt ) const
PreconditionerType GetPreconditionerType () const override
void ForcePreconditionerUpdate (bool force_update) override
void InvalidatePreconditioner () override
bool ReinitializePETSc () override
void PrintParams () const override
```

## `NewtonSolverParams`

- file: `Source/NonlinearSolvers/NewtonSolverParams.H`
- kind: `struct`
- public methods: 3; public members: 10; private members: 0

```cpp
void Parse (bool a_query_linear_solver = true)
[[nodiscard]] bool UpdatePC (int a_step, int a_iter, bool a_pc_initialized) const
void Print (const std::string& a_name) const
```

Public members:

```cpp
amrex::Real divergence_tolerance = 1.0e4
int pc_update_newton_interval = 1
int pc_update_time_step_interval = 1
LinearSolverType linear_solver_type = LinearSolverType::amrex_gmres
amrex::Real linsol_rtol = 1.0e-4
amrex::Real linsol_atol = 0.
int linsol_maxits = 1000
int linsol_verbose_int = 2
int linsol_restart_length = 30
PreconditionerType pc_type = PreconditionerType::none
```

## `NonlinearSolver`

- file: `Source/NonlinearSolvers/NonlinearSolver.H`
- kind: `class`
- public methods: 17; public members: 0; private members: 0

```cpp
NonlinearSolver() = default
virtual ~NonlinearSolver() = default
NonlinearSolver(const NonlinearSolver&) = delete
NonlinearSolver& operator=(const NonlinearSolver&) = delete
NonlinearSolver(NonlinearSolver&&) noexcept = delete
NonlinearSolver& operator=(NonlinearSolver&&) noexcept = delete
virtual void Define ( const Vec&, Ops* ) = 0
[[nodiscard]] bool IsDefined () const
virtual void Solve (Vec&, const Vec&, amrex::Real, amrex::Real, int, bool) const = 0
int GetExitStatus() const
virtual void PrintParams () const = 0
void GetSolverParams (amrex::Real& a_rtol, amrex::Real& a_atol, int& a_maxits) const
virtual PreconditionerType GetPreconditionerType () const
virtual void ForcePreconditionerUpdate (bool )
virtual void InvalidatePreconditioner ()
virtual bool ReinitializePETSc ()
virtual void Verbose ( bool a_verbose )
```

## `NonlinearSolverParams`

- file: `Source/NonlinearSolvers/NonlinearSolver.H`
- kind: `struct`
- public methods: 3; public members: 7; private members: 0

```cpp
void Parse (const std::string& a_prefix)
void ResetInvalidRelativeTolerance (const std::string& a_prefix)
void Print (const std::string& a_name) const
```

Public members:

```cpp
bool verbose = true
amrex::Real rtol = 1.0e-6
amrex::Real atol = 0.
int maxits = 100
bool require_convergence = true
std::string diagnostic_file
int diagnostic_interval = 1
```

## `PETScKSP`

- file: `Source/NonlinearSolvers/PETScKSP_Wrapper.H`
- kind: `class`, bases: `public LinearSolver<Vec,LinOp>`
- public methods: 17; public members: 0; private members: 1

```cpp
PETScKSP () = default
virtual ~PETScKSP () = default
PETScKSP(const PETScKSP&) = delete
PETScKSP& operator=(const PETScKSP&) = delete
PETScKSP(PETScKSP&&) noexcept = delete
PETScKSP& operator=(PETScKSP&&) noexcept = delete
void define (LinOp& a_linop) override
bool reinitializePETSc () override
void solve (Vec& a_sol, Vec const& a_rhs, RT a_tol_rel, RT a_tol_abs, int a_its=-1) override
void invalidatePreconditioner () override
void setVerbose (int v) override
void setRestartLength (int rl) override
void setMaxIters (int niters) override
[[nodiscard]] int getNumIters () const override
[[nodiscard]] int getStatus () const override
[[nodiscard]] bool converged () const override
[[nodiscard]] RT getResidualNorm () const override
```

## `PETScSNES`

- file: `Source/NonlinearSolvers/PETScSNES_Wrapper.H`
- kind: `class`, bases: `public NonlinearSolver<Vec,Ops>`
- public methods: 14; public members: 0; private members: 2

```cpp
PETScSNES () = default
~PETScSNES () override = default
PETScSNES(const PETScSNES&) = delete
PETScSNES& operator=(const PETScSNES&) = delete
PETScSNES(PETScSNES&&) noexcept = delete
PETScSNES& operator=(PETScSNES&&) noexcept = delete
void Define ( const Vec& a_U, Ops* a_op ) override
void Solve (Vec& a_U, const Vec& a_b, RT a_time, RT a_dt, int a_step, bool verbose_step) const override
void PrintParams () const override
void Verbose (bool a_verbose) override
PreconditionerType GetPreconditionerType () const override
void ForcePreconditionerUpdate (bool force_update) override
void InvalidatePreconditioner () override
bool ReinitializePETSc () override
```

## `PicardSolver`

- file: `Source/NonlinearSolvers/PicardSolver.H`
- kind: `class`, bases: `public NonlinearSolver<Vec,Ops>`
- public methods: 9; public members: 0; private members: 3

```cpp
PicardSolver() = default
~PicardSolver() override = default
PicardSolver(const PicardSolver&) = delete
PicardSolver& operator=(const PicardSolver&) = delete
PicardSolver(PicardSolver&&) noexcept = delete
PicardSolver& operator=(PicardSolver&&) noexcept = delete
void Define (const Vec& a_U, Ops* a_ops) override
void Solve (Vec& a_U, const Vec& a_b, amrex::Real a_time, amrex::Real a_dt, int a_step, bool verbose_step) const override
void PrintParams () const override
```

## `Preconditioner`

- file: `Source/NonlinearSolvers/Preconditioner.H`
- kind: `class`
- public methods: 15; public members: 0; private members: 0

```cpp
Preconditioner () = default
virtual ~Preconditioner () = default
Preconditioner(const Preconditioner&) = default
Preconditioner& operator=(const Preconditioner&) = default
Preconditioner(Preconditioner&&) noexcept = default
Preconditioner& operator=(Preconditioner&&) noexcept = default
virtual void Define (const T&, Ops*) = 0
virtual void Update () = 0
virtual void Apply (T& a_x, const T& a_b) = 0
virtual void getPCMatrix( amrex::Gpu::DeviceVector<int>&, amrex::Gpu::DeviceVector<int>&, amrex::Gpu::DeviceVector<int>&, amrex::Gpu::DeviceVector<RT>&, int&, int& )
[[nodiscard]] virtual bool IsDefined () const = 0
virtual void setName (const std::string&)
virtual void printParameters() const
inline void CurTime (const RT a_time)
inline void CurTimeStep (const RT a_dt)
```

## `KSP_impl`

- file: `Source/NonlinearSolvers/WarpX_PETSc.H`
- kind: `class`, bases: `public PETScSolver_impl`
- public methods: 18; public members: 1; private members: 10

```cpp
KSP_impl (LinOpType&)
~KSP_impl () override
KSP_impl(const KSP_impl&) = delete
KSP_impl& operator=(const KSP_impl&) = delete
KSP_impl(KSP_impl&&) noexcept = delete
KSP_impl& operator=(KSP_impl&&) noexcept = delete
void createObjects (const VecType&)
void destroyObjects () override
void setRestartLength (int)
void setTolerances(amrex::Real, amrex::Real, int)
void setMaxIters(int)
void invalidatePreconditioner ()
void solve(VecType&, const VecType&)
void setVerbose(int)
inline int getNumIters() const
inline int getStatus() const
inline amrex::Real getResidualNorm() const
[[nodiscard]] bool isDefined () const
```

Public members:

```cpp
LinOpType* m_linop = nullptr
```

## `PETScSolver_impl`

- file: `Source/NonlinearSolvers/WarpX_PETSc.H`
- kind: `class`
- public methods: 12; public members: 10; private members: 0

```cpp
PETScSolver_impl ()
virtual ~PETScSolver_impl ()
PETScSolver_impl(const PETScSolver_impl&) = delete
PETScSolver_impl& operator=(const PETScSolver_impl&) = delete
PETScSolver_impl(PETScSolver_impl&&) noexcept = delete
PETScSolver_impl& operator=(PETScSolver_impl&&) noexcept = delete
void applyPC(VecType&, const VecType&, LinOpType* const) const
void applyOp(VecType& a_F, const VecType& a_U, LinOpType* const) const
[[nodiscard]] int assemblePCMatrix(LinOpType* const)
virtual void destroyObjects ()
PreconditionerType getPreconditionerType () const
void setRestartLength(int) const
```

Public members:

```cpp
mutable VecType m_U
mutable VecType m_F
long m_ndofs_l = 0
long m_ndofs_g = 0
std::unique_ptr<MatObj> m_A
std::unique_ptr<MatObj> m_P
std::unique_ptr<VecObj> m_x
std::unique_ptr<VecObj> m_b
bool m_is_defined = false
PreconditionerType m_pc_type = PreconditionerType::none
```

## `SNES_impl`

- file: `Source/NonlinearSolvers/WarpX_PETSc.H`
- kind: `class`, bases: `public PETScSolver_impl`
- public methods: 18; public members: 3; private members: 19

```cpp
SNES_impl (const VecType&, TIType*, const NonlinearSolverParams&, const NewtonSolverParams&)
~SNES_impl() override
void createObjects ()
void destroyObjects () override
[[nodiscard]] bool isDefined () const
void solve(VecType&, const VecType&, amrex::Real, amrex::Real, int) const
void computeRHS(VecType&, const VecType&)
void preLinearSolve()
void endFDJacobian() const
void setPCInitialized() const
void invalidatePreconditioner()
void setForcePreconditionerUpdate(bool force_update)
[[nodiscard]] bool usePC() const
[[nodiscard]] bool updatePC() const
void setVerbose(bool)
[[nodiscard]] bool verbose() const
void printParams () const
inline int getStatus() const
```

Public members:

```cpp
bool m_fd_jac_comput = false
mutable bool m_fd_jac_in_progress = false
std::unique_ptr<LinOpType> m_linop
```

## `WeightedJacobiImpl`

- file: `Source/NonlinearSolvers/WeightedJacobiImpl.H`
- kind: `class`
- public methods: 19; public members: 0; private members: 10

```cpp
WeightedJacobiImpl () = default
~WeightedJacobiImpl () = default
WeightedJacobiImpl (const WeightedJacobiImpl&) = delete
WeightedJacobiImpl& operator= (const WeightedJacobiImpl&) = delete
WeightedJacobiImpl (WeightedJacobiImpl&&) noexcept = delete
WeightedJacobiImpl& operator= (WeightedJacobiImpl&&) noexcept = delete
void Define (MatrixSolverOps<T>* a_ops, const std::string& a_pp_prefix)
void Update ()
void Solve (T& a_x, const T& a_b)
void printParameters (const std::string& a_name) const
[[nodiscard]] bool IsDefined () const
[[nodiscard]] bool isVerbose () const
[[nodiscard]] RT getOmega () const
[[nodiscard]] int getNumIters () const
[[nodiscard]] RT getResidualNorm () const
[[nodiscard]] int getStatus () const
void setVerbose (bool v)
void setMaxIters (int n)
void setTolerances (RT rtol, RT atol)
```

