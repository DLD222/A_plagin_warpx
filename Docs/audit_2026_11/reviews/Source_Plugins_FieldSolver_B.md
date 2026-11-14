# Source_Plugins_FieldSolver_B 审查

分区范围：场求解器后半 —— PSATD（谱求解器 + PML/算法工厂）、隐式/半隐式（含 PETSc/Newton 非线性求解器、稀疏雅可比装配）、静磁求解器。

**重要范围说明**：本分区**不包含**张量介质求解器（`3x3 ε/μ/σ`、subpixel smoothing、矩阵指数电导积分器）。这些文件
（`FieldSolver/FiniteDifference/MacroscopicProperties/SubpixelTensor.H`、`MacroscopicEvolveE.cpp`、
`MacroscopicProperties*`）在 `/tmp/audit_parts/Source_Plugins_FieldSolver_A.txt` 里。
对 50 个文件做了 `Subpixel|MaterialRegistry|chi_inv|epsilon_inv` 关键词扫描：**零引用**，
本分区与张量介质实现没有任何代码接触点（唯一的 "tensor" 是 `SemiImplicitDarwin.H:97` 指质量矩阵的 3x3 块张量，与本项目介质张量无关）。

方法说明：清单中 38 个文件在上游同名（目录映射：`Plugins/FieldSolver/PSATD` ↔ `Source/FieldSolver/SpectralSolver`、
`Plugins/FieldSolver/Implicit` ↔ `Source/FieldSolver/ImplicitSolvers`、
`.../Implicit/NonlinearSolvers` ↔ `Source/NonlinearSolvers`、
`Plugins/FieldSolver/Magnetostatic` ↔ `Source/FieldSolver/MagnetostaticSolver`）。
对 26 个"仅 include 路径/命名空间改名"的文件用逐行 `diff -u` 全覆盖（未变部分与官方逐字节相同，等价于通读）；
对 12 个语义变更文件（`diff` 10–760 行）与 12 个**新文件**逐行通读，并对关键函数做整段 diff（例：`SparseJacobianMatrix::Assemble`
与上游 `MatrixPC<T,Ops>::Assemble` 逐行比对，499 行 → 499 行、系数零差异）。

## 覆盖

| 文件 | 行数 | 类型 | 结论 |
|---|---|---|---|
| PSATD/SpectralSolverRegistration.cpp | 74 | new | 可疑（F1） |
| PSATD/SpectralSolver.H | 205 | modified（运行期注入 + ISpectralSolver 接口） | OK |
| PSATD/SpectralSolver.cpp | 185 | modified | OK |
| PSATD/SpectralSolverRZ.H | 170 | modified | OK（F8 备注） |
| PSATD/SpectralSolverRZ.cpp | 207 | modified | OK |
| PSATD/SpectralFieldData.H | 92 | modified（`SpectralFieldIndex` 抽出到 `Core/Runtime`） | 缺陷（F4，重复前置声明） |
| PSATD/SpectralFieldData.cpp | 347 | modified | OK（`data()` 回退 `legacy_runtime_data()`，静态持有，无悬垂） |
| PSATD/SpectralFieldDataRZ.H | 138 | modified | OK |
| PSATD/SpectralFieldDataRZ.cpp | 931 | modified（8 处 `WarpX::getCosts/getUpperCorner` → `data()`） | OK（`upperCorner` 等价性已核对，见下） |
| PSATD/SpectralAlgorithms/SpectralBaseAlgorithm.H/.cpp | 110/115 | moved | OK |
| PSATD/SpectralAlgorithms/SpectralBaseAlgorithmRZ.H | 104 | moved | OK |
| PSATD/SpectralAlgorithms/PsatdAlgorithmComoving.H | 92 | moved | OK |
| PSATD/SpectralAlgorithms/PsatdAlgorithmGalilean.H | 142 | moved | OK |
| PSATD/SpectralAlgorithms/PsatdAlgorithmGalileanRZ.cpp | 363 | moved（删 `WarpX.H`） | OK |
| PSATD/SpectralAlgorithms/PsatdAlgorithmJRhomFirstOrder.H | 100 | moved | OK |
| PSATD/SpectralAlgorithms/PsatdAlgorithmJRhomSecondOrder.H | 141 | moved | OK |
| PSATD/SpectralAlgorithms/PsatdAlgorithmPml.H/.cpp | 120/469 | moved | OK |
| PSATD/SpectralAlgorithms/PsatdAlgorithmPmlRZ.cpp | 172 | moved | OK |
| PSATD/SpectralAlgorithms/PsatdAlgorithmRZ.cpp | 493 | moved | OK |
| Implicit/NonlinearSolvers/MatrixSolverOps.H | 55 | new | OK |
| Implicit/NonlinearSolvers/NewtonSolverParams.H | 181 | new | OK（F3 关联） |
| Implicit/NonlinearSolvers/PreconditionerTypes.H | 18 | new（上游 `PreconditionerLibrary.H` 内容搬移） | OK（无枚举重复定义） |
| Implicit/NonlinearSolvers/PreconditionerLibrary.H | 11 | modified（改为伞头） | OK |
| Implicit/NonlinearSolvers/Preconditioner.H | 123 | moved（换 include） | OK |
| Implicit/NonlinearSolvers/NonlinearSolver.H | 252 | modified（参数结构体 + 诊断文件 + 虚接口） | OK |
| Implicit/NonlinearSolvers/PicardSolver.H | 204 | modified | OK（诊断列数 6=6 一致） |
| Implicit/NonlinearSolvers/NewtonSolver.H | 365 | modified（收敛/发散判定重排） | 可疑（F3） |
| Implicit/NonlinearSolvers/WeightedJacobiImpl.H | 264 | new（上游 `JacobiPC::Apply` 抽取） | 缺陷（F2） |
| Implicit/NonlinearSolvers/PETScKSP_Wrapper.H | 130 | modified | OK（未编译，见 F7） |
| Implicit/NonlinearSolvers/PETScSNES_Wrapper.H | 145 | modified | OK（未编译，见 F7） |
| Implicit/NonlinearSolvers/WarpX_PETSc.H | 377 | modified（大重构） | 可疑（F7） |
| Implicit/NonlinearSolvers/WarpX_PETSc.cpp | 933 | modified（大重构） | 可疑（F7） |
| Implicit/PETScImplicitBridge.H/.cpp | 82/80 | new | OK（薄转发，签名一致） |
| Implicit/SparseJacobianMatrix.H/.cpp | 159/651 | new（上游 `MatrixPC::Assemble` 抽取） | 缺陷（F9，仅参数改名）；装配算法 OK |
| Implicit/Implicit 组外：SemiImplicitEM.H/.cpp | 105/207 | modified | OK（F8 备注） |
| Implicit/SemiImplicitDarwin.H/.cpp | 186/556 | modified + 新物理（gather 阶数解耦、chi_dd 三分量） | 缺陷（F5、F6） |
| Implicit/ThetaImplicitEM.H/.cpp | 151/532 | modified（OneStep 拆 Setup/DoSolve/Reset/Finish） | OK（F10 已核对为良性） |
| Implicit/StrangImplicitSpectralEM.H/.cpp | 125/153 | modified | OK |
| Magnetostatic/MagnetostaticSolver.H | 87 | modified（全局量改构造参数） | OK |
| Magnetostatic/MagnetostaticSolverImpl.H/.cpp | 41/432 | new（上游 `MagnetostaticSolver.cpp` 搬移 + config 化） | 可疑（F11） |
| Magnetostatic/MagnetostaticSolverRegistration.cpp | 24 | new | OK |

覆盖：清单 50/50 文件（12 个新文件逐个通读；38 个上游同名文件逐个整文件 diff）。

## 缺陷清单

### F1 PSATD 谱求解器工厂：PML 调用点漏填 `lev` / `n_rz_azimuthal_modes`（RZ 下静默用 0 层、1 个方位模）
- 文件:行：`Source/Plugins/FieldSolver/PSATD/SpectralSolverRegistration.cpp:21-38`（RZ 分支使用 `config.lev`、`config.n_rz_azimuthal_modes`）；
  调用点 `Source/Plugins/PicEm/Modules/BoundaryConditions/PML.cpp:952-971`（`SpectralSolverConfig` 只填了 19 个字段，**没有** `config.lev`、`config.n_rz_azimuthal_modes`、`config.context`）
- 严重度：major
- 置信度：likely（代码路径已确证；运行影响未跑）
- 现象与依据：
  - `SpectralSolverConfig` 默认值：`lev = 0`、`n_rz_azimuthal_modes = 1`
    （`Source/Core/Runtime/SpectralSolverFactory.H:64-65`）。
  - `PML::PML` 在 `lev` 层构造求解器时并未传入 `lev`，因此 RZ 构建里工厂会造出
    `SpectralSolverRZ(lev=0, n_rz_azimuthal_modes=1, data=nullptr)`：
    `SpectralFieldDataRZ` 的 `m_ncomps = 2*n_modes-1 = 1`（`PSATD/SpectralFieldDataRZ.cpp:44-47`），
    而 PML 的场按 `WarpX::ncomps = 2*n_rz_azimuthal_modes-1` 分配。
  - 可达性：`WarpX::CreatePMLForLevel` 只在 RZ+FFT 的 `lev==0` 早退（`WarpXInitData.cpp:1215-1221`），
    **`lev>0` 仍会构造普通 `PML`**（`WarpXInitData.cpp:1250-1271`）；而 `WarpX::PushPSATD` 的
    `pmlPtr(lev)->PushPSATD(m_fields, lev)`（`Modules/FieldSolver/WarpXPushFieldsEM.cpp:926-934`）
    不被 `WARPX_DIM_RZ` 排除 ⇒ RZ + AMR + PSATD + PML 时该求解器会被真正调用，
    届时只变换 m=0 模、且 `Cost`/几何取 level 0。
  - 上游对照：上游该处直接构造笛卡尔 `SpectralSolver`（`_upstream/.../BoundaryConditions/PML.cpp:942-946`），
    RZ 下同样不成立 ⇒ 这是上游遗留的 RZ 空白，本 fork 的工厂化把它变成"看起来能跑但模数/层级错"的静默错误。
- 建议修法：在 `PML::PML` 里补 `config.lev = lev; config.n_rz_azimuthal_modes = WarpX::n_rz_azimuthal_modes;
  config.context = ctx;`；若 RZ+PML 本就不支持，改为 `WARPX_ALWAYS_ASSERT` 明确拒绝。
- 验证方法：`Examples/Tests/` 里挑一个 RZ+PSATD+`maxwell_solver=PSATD`+PML 且 `n_rz_azimuthal_modes>1` 的
  用例，加 `amr.max_level=1`；在 `SpectralSolverRegistration.cpp` 打印 `config.lev/n_rz_azimuthal_modes` 确认。

### F2 `WeightedJacobiImpl::Update()` 每次 PC 更新都用 Gershgorin 界覆盖已自适应的 omega
- 文件:行：`Source/Plugins/FieldSolver/Implicit/NonlinearSolvers/WeightedJacobiImpl.H:136-137`
  （`if (m_adaptive_omega) { m_omega = m_matrix_ops->EstimateOmega(); }`），
  上游对应实现 `_upstream/.../NonlinearSolvers/JacobiPC.H`（`Update()` 完全不改 `m_omega`，
  只在 `Apply()` 里因残差上升降 0.9 并把结果缓存回 `m_omega`）
- 严重度：minor（收敛性能/PC 质量，不改变解）
- 置信度：confirmed（代码差异确定）；量级 needs-verification
- 现象与依据：上游语义是"omega 从 1.0 起，残差上升才降 0.9，并把调好的值缓存供后续 `Apply()`
  复用"（`JacobiPC.H` 类注释与 `Apply()` 末尾 `m_omega = omega`）。本 fork 把它抽成
  `WeightedJacobiImpl` 后，每次 `Update()`（即每次 PC 更新，默认每 Newton 迭代/每步）
  都用 `EstimateOmega()` 重置 omega，之前降下来的阻尼被丢弃；配合
  `MatrixSolverOps.H:32-35` 的 `omega = 2/(2+max_i r_i)`（比经典界 `2/(1+r)` 更保守），
  `pc_jacobi` 会周期性地从"过阻尼"点重新开始，每次只有 10 次 ×0.9 的自适应预算。
- 建议修法：`Update()` 里只在 `m_omega` 尚未自适应过时使用 `EstimateOmega()`，或保留
  `m_omega = std::min(m_omega, EstimateOmega())`；并把 `EstimateOmega` 的推导写进注释。
- 验证方法：`Examples/Tests/implicit/inputs_test_2d_semi_implicit_planar_pinch` 系列把
  `jacobian.pc_type = pc_jacobi`，比较 `newton` 迭代数与 `pc_jacobi.omega` 打印；
  同一算子下 `EstimateOmega()` 与最优 omega 的比值可直接打印比对。

### F3 Newton 发散判据从"直接 abort"改为"警告 + exit_status=-9"
- 文件:行：`Source/Plugins/FieldSolver/Implicit/NonlinearSolvers/NewtonSolver.H:276-284`
- 严重度：minor
- 置信度：confirmed（改动确定；是否可接受属设计选择）
- 现象与依据：上游 `if (norm_abs > 100._rt*norm0) { ... WARPX_ABORT_WITH_MESSAGE(...) }`；
  本 fork 改为 `m_exit_status = -9; WMRecordWarning(...); break;`，阈值也变成可配的
  `newton.divergence_tolerance`（默认 1e4，`NewtonSolverParams.H:31`）。
- 缓解证据（非缺陷理由）：`ImplicitSolver::OneStep` 会消费该状态并二分步长重试
  （`Implicit/ImplicitSolver.cpp:192-216`），最终 `WarpXEvolve.cpp:456-461` 在 `exit_status<0` 时 abort，
  所以"发散被忽略"不会静默放过。仅提示：`-9` 与 `-3`/`-5` 现在共享"重试"语义。
- 建议修法：无需改；建议在 `Docs` 的 implicit 章节列出新的 exit_status 表。

### F4 `SpectralFieldData.H` 重复的前置声明
- 文件:行：`Source/Plugins/FieldSolver/PSATD/SpectralFieldData.H:28` 与 `:30`（两行完全相同）
- 严重度：minor
- 置信度：confirmed
- 现象与依据：`namespace warpx::runtime { struct RuntimeContext; class RuntimeData; }` 连写两次，
  且被夹在 `#include <AMReX_MultiFab.H>` 与 `#include <AMReX_Vector.H>` 之间（include 组内插声明）。
- 建议修法：删掉第 30 行（或合并到文件顶部的前置声明区）。
- 验证方法：`grep -n "namespace warpx::runtime" Source/Plugins/FieldSolver/PSATD/SpectralFieldData.H`

### F5 Darwin MLMG 预条件系数 `chi_dd` 去掉交错插值，改成"同下标直取"（半格偏移）
- 文件:行：`Source/Plugins/FieldSolver/Implicit/SemiImplicitDarwin.cpp:493-540`，关键行 `:535`
  （`s += S(i,j,k,c);`）、`:537`（`chi(i,j,k) = fac*s;`）、`:510`（`fac = 2 mu0/dt`，去掉了上游的 `/3`）
- 严重度：major（改动的是一个"算子系数"，但只影响预条件器 ⇒ 不改变收敛解）
- 置信度：needs-verification
- 现象与依据：上游实现按质量矩阵块自身的 E 型交错，把 `(e0+1)(e1+1)(e2+1)` 个亚格点求和后
  平均到格心（`wt = fac/(3*(e0+1)(e1+1)(e2+1))`，`a_chi_cc += wt*s`）。
  本 fork 改为：每个分量单独 `chi_dd = (2 mu0/dt) * rowsum(S_dd(i,j,k))`，
  用**同一下标**读入格心数组。目标数组 `m_chi_cc[comp]` 定义在 `DarwinMLMGPC` 的
  **格心** `m_grids` 上（`NonlinearSolvers/DarwinMLMGPC.H:278-280`、`:297-299` 用于
  `setACoeffs` 的 Helmholtz 解），而 `S_dd` 至少在一个方向是节点型 ⇒ 同下标读取等价于半个格子的
  系统性平移（不是插值），代码注释里"collocated by index identification"只是断言，没有推导。
  去掉 `/3` 与该 PC 改为三分量逐分量 Helmholtz 是自洽的（不是 bug）。
- 建议修法：若确认要保留"同下标"，请在 `DarwinMLMGPC` 里把 `m_chi_cc` 定义到与 `Z` 分量相同的
  交错（`B` 型 / 每分量各自 BoxArray），否则恢复亚格点平均；并把 `chi_dd` 的推导
  （`chi = 2 mu0/dt · Σ_bands M_dd`）与离散一致性写成注释。
- 验证方法：跑 `Examples/Tests/implicit/*darwin*`，打印 `pc_darwin_mlmg` 的 MLMG 迭代数/残差；
  对比 F2 前后的线性迭代数；也可用均匀 `dA` 的解析校验（`curl(chi curl dA)` 对常数 chi 应为 0）。

### F6 `SemiImplicitDarwin::m_magnetostatic_particle_shape` 未初始化
- 文件:行：`Source/Plugins/FieldSolver/Implicit/SemiImplicitDarwin.H:159`（`int m_magnetostatic_particle_shape;`，无初值）
- 严重度：minor
- 置信度：confirmed
- 现象与依据：唯一赋值点在 `SemiImplicitDarwin.cpp:56`（`Define()` 内）；`PrintParameters()`
  （`cpp:143` 打印它）与 `OneStep` 在其之前被调用即读未初始化值。当前调用序安全，但属未初始化成员
  （`-Wmaybe-uninitialized` 之外的静默 UB）。
- 建议修法：`int m_magnetostatic_particle_shape = 1;`

### F7 PETSc 分支（含全部新逻辑）在本审计构建中**完全没有编译**
- 文件:行：`audit_build/fork/CMakeCache.txt:102`（`AMReX_PETSC:BOOL=OFF`）
- 严重度：major（验证覆盖缺口，不是代码缺陷）
- 置信度：confirmed
- 现象与依据：`WarpX_PETSc.cpp` 整体在 `#ifdef AMREX_USE_PETSC`（`:22`）内，`PETScKSP_Wrapper.H`/
  `PETScSNES_Wrapper.H` 的类只在 PETSc 打开时实例化。因此本分区新写的 SNES 生命周期管理
  （`createObjects`/`destroyObjects`/`SNESSetUpdate`/`SNESLineSearchSetPreCheck`/
  `SNESSetDivergenceTolerance`/`SNESSetLagPreconditioner`/`KSPSetReusePreconditioner`/
  `MatXAIJSetPreallocation`/`SNESGetConvergenceHistory`）与 `SparseJacobianMatrix`→PETSc 的接线
  （`WarpX_PETSc.cpp:495-523` 的 `MPI_Exscan` 预分配）**没有任何编译证据**。
  已人工核对：`MPI_Exscan` 的 contiguous-by-rank 假设与 `WarpXSolverDOF::fill_global_dof`
  （`Source/Core/Solvers/WarpXSolverDOF.cpp:173-207`，`global = local + m_row_offsets[MyProc]`）一致 ✓；
  `MatSetValues` 传入的列都 ≥0（`insertOrAdd` 已丢弃负列）✓。
  待验证项：`SNESGetConvergenceHistory(&norm_history, nullptr /*its*/, &num_norms)` 后
  取 `norm_history[0]` 作初值（`WarpX_PETSc.cpp:854-858`）——需确认 PETSc 的历史数组 0 号
  就是第一次残差、且 `num_norms` 是已填长度而非容量。
- 建议修法：加一个 `-DAMReX_PETSC=ON` 的 CI 编译 job；或至少在 `Docs` 标注 PETSc 路径未受当前 CI 覆盖。
- 验证方法：`cmake -S . -B build_petsc -DWarpX_DIMS=2 -DAMReX_PETSC=ON` 后编译；
  `Examples/Tests/implicit/inputs_test_2d_semi_implicit_planar_pinch_petsc_snes`。

### F8 类型/契约不一致：`SpectralFieldData` 持 `RuntimeContext*`，`SpectralFieldDataRZ` 持 `RuntimeData*`
- 文件:行：`PSATD/SpectralFieldData.H:87-89`；`PSATD/SpectralFieldDataRZ.H:117-119`
- 严重度：minor
- 置信度：confirmed
- 现象与依据：同族两个类注入的东西不同（一个是 context 再取 `->data`，一个直接是 data），
  `SpectralSolver` 与 `SpectralSolverRZ` 的构造参数随之不同（`ctx` vs `data`），
  工厂两分支（`SpectralSolverRegistration.cpp:37-38` / `:63`）也必须分别处理。
  另 `SpectralSolverRZ.H:70-77` 用重载忽略 `fill_guards`（"RZ 自己补 ghost"），
  该重载目前无调用者（调用点按 `WARPX_DIM_RZ` 分叉，`Modules/FieldSolver/WarpXPushFieldsEM.cpp:88-95`），
  属潜在陷阱而非现行缺陷。
- 建议修法：统一为 `const RuntimeContext*`（或统一为 `RuntimeData*`），代价很小。

### F9 `precon_mat.verbose` 改名：旧输入 `pc_petsc.verbose` 直接 abort
- 文件:行：`Implicit/SparseJacobianMatrix.cpp:25-38`（`WARPX_ALWAYS_ASSERT_WITH_MESSAGE(!pp_old.query("verbose",...))`）
- 严重度：minor
- 置信度：confirmed
- 现象与依据：上游 `MatrixPC::readParameters()` 读的是 `m_name`（`pc_petsc`）前缀下的 `verbose`
  与 `pc_diagonal_only`。本 fork 把 verbose 挪到 `precon_mat`，并对旧键**硬失败**。
  仓库内 8 个 `Examples/Tests/implicit/*petsc*` 已同步改成 `precon_mat.verbose` ✓，
  但用户旧输入会直接 abort（而不是 warning + 忽略）；而且现在同一个 PETSc 预条件器同时由
  `precon_mat.*`（矩阵装配）与 `pc_petsc.*`（PETSc PC 选项，见 `WarpX_PETSc.cpp:356-423`）两个前缀控制，
  文档未说明分工。
- 建议修法：`pp_old.query("verbose", backward_bool)` 时改为 `WMRecordWarning` 并把值接过来，
  同时 `pc_diagonal_only` 也一并做兼容处理（上游读了但从未使用，可只 warning）。

### F10 备注（已核对为**良性**，但看起来像缺陷）：`ThetaImplicitEM::ResetStep()` 不恢复 `Efield_fp`/`B_old`
- 文件:行：`Implicit/ThetaImplicitEM.cpp:128-137` vs `Implicit/SemiImplicitEM.cpp:124-165`
- 严重度：非缺陷（记录以阻止重复审查）
- 置信度：confirmed
- 依据：子步失败后重试**不会**重跑 `SetupStep()`（`Implicit/ImplicitSolver.cpp:172-216` 的内层
  `while(true)` 直接重试 `DoSolve`），因此 `SetupStep` 不会再从被污染的 `Efield_fp` 覆盖 `E_old`。
  Theta 的 `B` 每次 RHS 都从 `B_old` 重算（`WarpXImplicitOps.cpp` 的
  `UpdateMagneticFieldAndApplyBCs` 先 `Copy(Bfp, B_old)` 再 `EvolveB`），`E_old` 全程不被写，
  故无需恢复；SemiImplicit 的 `B` 是**增量**推进（`SetupStep` 里 +0.5dt，`ResetStep` 里重做一次），
  所以它必须显式恢复 `Efield_fp`/`Bfield_fp` —— 两者差异是有意的，不是漏写。

### F11 `MagnetostaticSolverImpl::defineVectorPotentialBCs(geom)` 公开接口会用到默认初始化的边界类型
- 文件:行：`Magnetostatic/MagnetostaticSolverImpl.cpp:220-226`（`m_bc.defineVectorPotentialBCs(geom, m_boundary_lo, m_boundary_hi)`），
  `m_boundary_lo/hi` 只在 `computeField()` 内被赋值（`:60-64`）
- 严重度：minor
- 置信度：confirmed（当前无调用者 ⇒ 潜在）
- 现象与依据：`IMagnetostaticSolver::defineVectorPotentialBCs`（`Source/Core/Runtime/IMagnetostaticSolver.H:99`）
  是公开虚接口；若内核照上游习惯在初始化阶段单独调它，`m_boundary_lo/hi` 仍是
  `FieldBoundaryType{}`（枚举首值），会把所有边界当成该类，静默给出错误 BC（或直接 abort）。
- 建议修法：把 `m_boundary_lo/hi` 也做成构造/config 参数，或让 `defineVectorPotentialBCs(geom)`
  断言 `m_bc.bcs_set || config 已提供`。

## 值得复核/已核对无问题的点

- `SpectralFieldDataRZ` / `SpectralFieldData` 里 `WarpX::UpperCorner(bx, lev, 0._rt)` →
  `data().upperCorner(bx, lev)`：`WarpXRuntimeData::upperCorner` 传 `time_shift_delta = 0`
  （`Source/Plugins/PicEm/RuntimeBridge/Runtime/WarpXRuntimeData.cpp:84`），与上游 `0._rt` 完全一致，
  且 `WarpX::UpperCorner` 内部实时取 `gett_new(lev)`（galilean 移动窗）⇒ 语义等价 ✓。
- `SpectralFieldIndex` 从 `SpectralFieldData.H` 搬到 `Source/Core/Runtime/SpectralFieldIndex.{H,cpp}`：
  `Source/Core/Runtime/SpectralFieldIndex.cpp` 的构造体与上游
  `_upstream/.../SpectralSolver/SpectralFieldData.cpp:36-120` 逐行一致（含 `divE = 3` 复用、
  `n_fields = c`），谱数组分量布局未变 ✓。
- `SparseJacobianMatrix`：`Assemble()` 与上游 `MatrixPC<T,Ops>::Assemble()` 499/499 行 diff，
  仅改名字与数据来源（`m_ops->GetCurl2BCmask` → `a_bc_masks`、`T::getDOFsObject()` → `a_dofs`、
  `m_bcoefs` → `a_mass_matrices`），所有 curl-curl 系数（含 RZ 的
  `i/(i±1/2)`、`1±0.5/i` 几何因子、XZ 的 `geom_m/geom_p`、3D 的 `comp_shift`）**零改动** ✓。
  RZ 径向-轴向系数抽查：`Et` 对角 `i/(i-1/2)+i/(i+1/2)`、左 `(i-1)/(i-1/2)`、右 `(i+1)/(i+1/2)`
  与 `-(1/r)d/dr[r d/dr]` 的 Yee 离散一致 ✓。
- 迭代解算器框架（`NewtonSolver`/`PicardSolver`/`NonlinearSolver`/`NewtonSolverParams`）：
  诊断文件列数与 `WriteDiagnostics` 实参个数一致（Newton 9 列、Picard 6 列）✓；
  `GetSolverParams` 由纯虚改为非虚实现后，两个派生类的 `override` 都已删除（否则会编译失败）✓；
  `resetInvalid` 系列不会在参数求值时改值（no side effect in argument list）✓。
- `psatd` 滤波器接线：工厂的 `use_kspace_filter` 分支只在 RZ 生效（`SpectralSolverRegistration.cpp:39-41`），
  与上游一致（笛卡尔 `SpectralSolver` 无 `InitFilter`，`WarpXPushFieldsEM.cpp:511-539` 的滤波
  只在 `WARPX_DIM_RZ` 内）✓。
- `SparseJacobianMatrix` 的 `while(true)` 重分配循环、`nnz_actual` 归约、`GetMatrix` 拷贝
  与上游 `MatrixPC::Update/GetMatrix` 同构 ✓（仅重分配次数统计的 `m_num_realloc++` 位置略变，等价）。
- 隐式求解器的 `m_nsubsteps`/`dt_scale = 1/m_nsubsteps` 与 `m_dt /= 2` 配套
  （`ThetaImplicitEM.cpp:279`、`SemiImplicitEM.cpp:207`、`StrangImplicitSpectralEM.cpp:126`），
  与 `PreRHSOp(..., a_dt_scale, ...)` 的消费侧（`ImplicitSolver.cpp:1083-1129`）一致 ✓；
  `ForcePreconditionerUpdate(m_nsubsteps>1)` 在每次 `OneStep` 开头被复位 ✓。
- 未覆盖（超出本分区）：PML/边界、`ImplicitSolver.cpp`/`MassMatrixSolverOps.H`/`JacobiPC.H`/`DarwinMLMGPC.H`
  属 `FieldSolver_A` 或 `Core_*` 分区；张量介质求解器全部在 `FieldSolver_A`。
