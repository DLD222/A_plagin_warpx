# WarpX 源码审计（Route B 前置）

> 直接回答：**没有看完。** 636 个源文件、100,539 行，我此前深读约 25 个、泛读约 40 个，覆盖率约 10–15%。
> 本文把“还差什么、为什么差、怎么补、补到什么程度算够”写成可核对的清单，并给出目前已确认的结构性事实。
> 目的：Route B（Runtime 重写）不是靠“读几个关键文件”就能立项的；先把审计做完整，再谈拆分。

---

## 0. 覆盖率与“看完”的定义

### 0.1 已深读（逐行/逐函数）

| 文件 | 关注点 |
|---|---|
| `Source/WarpX.H` | 254 个成员声明、191 个函数、68 个 static |
| `Source/WarpX.cpp`（构造/ReadParameters 部分/AllocLevelData/ClearLevel/MakeNewLevel*） | 生命周期、全局状态、按 `FieldType` 分配 |
| `Source/Evolve/WarpXEvolve.cpp`（Evolve/OneStep/OneStep_nosub/OneStep_sub1 的调用序列） | 时间步序列、物理分支 |
| `Source/FieldSolver/WarpXPushFieldsEM.cpp`（EvolveB/E、MacroscopicEvolveE、PushPSATD 入口） | 场推进路径 |
| `Source/FieldSolver/FiniteDifferenceSolver/FiniteDifferenceSolver.H` | FD 求解器接口 |
| `Source/ablastr/fields/MultiFabRegister.H` | 字符串键注册表、`alloc_init` |
| `Source/Initialization/WarpXInitData.cpp`（InitData/InitFromScratch/InitLevelData/AddExternalFields/InitializeEBGridData 入口） | 初始化顺序 |
| `Source/Parallelization/WarpXRegrid.cpp`（RemakeLevel/LoadBalance） | 重分配事务 |
| `Source/Diagnostics/WarpXIO.cpp`（InitFromCheckpoint/GetRestartDMap） | 重启布局 |
| `Source/Diagnostics/MultiDiagnostics.cpp`、`MultiReducedDiags.cpp`、`Diagnostics.H`、`FlushFormat.H` | 诊断工厂/写盘 |
| `Source/Particles/MultiParticleContainer.H`、`WarpXParticleContainer.H`、`PhysicalParticleContainer.H`（接口） | 物种容器 |
| `Source/Fluids/MultiFluidContainer.H`、`WarpXFluidContainer.H`、`MomentFluidSolver.H` | 流体状态 |
| `Source/BoundaryConditions/PML.H`、`WarpXFieldBoundaries.cpp`（入口） | 边界归属 |
| `Source/Utils/WarpXAlgorithmSelection.H` | 算法枚举 |
| `Source/main.cpp`、`CMakeLists.txt`（目标/选项）、`Examples/CMakeLists.txt`（测试） | 入口与构建 |

### 0.2 只泛读/通过子代理报告

- `Particles/` 141 文件：只读了容器基类/物理容器接口、推进/沉积入口；未读全部 pusher/deposition/collision/QED 实现。
- `Diagnostics/` 124 文件：只读了基类/工厂/FlushFormat/ReducedDiags 框架；未读全部 functor 与 InSitu 格式。
- `FieldSolver/` 121 文件：只读了 FD/PSATD/ES/Hybrid/Implicit 的入口；未读全部算法/kernel。
- `Initialization/` 28 文件：只读了 `WarpXInitData`/`ExternalField`/`PlasmaInjector` 入口；未读注入分布细节。
- `EmbeddedBoundary/` 11 文件：只读了 `InitEB`/`Enabled`/`InitializeEBGridData` 入口；未读 cut-cell 算法。
- `Python/` 11 文件 + `Python/pywarpx/`：只读了 `WarpX.cpp` 的绑定入口与 `WarpX.py` 的 init/evolve；未读全部 PICMI 映射。
- `MomentsSolver/` 25 文件：只读了 5 矩入口/方程表；未读 10 矩与耦合细节。
- `NonlinearSolvers/`、`Filter/`、`AcceleratorLattice/`、`Utils/`：未系统读。

### 0.3 “看完”对 Route B 的定义

Route B 不需要逐行读每个 GPU kernel，但必须做到：

1. **所有 368 个 `.H` 的公共接口读完**（类型、所有权、生命周期）。
2. **所有非 kernel `.cpp` 的入口函数读完**（构造/Init/Step/Regrid/Restart/Finalize）。
3. **跨切面**：全局状态（68 static + 241 `GetInstance`）、IO/checkpoint、Python/PICMI、构建矩阵、测试。
4. **kernel `.K` 只抽样**：它们属于算法实现，不影响 Runtime 的边界；但必须知道它们读写哪些数据键。

按 500 个有效文件、每文件 10–20 分钟，约 **80–160 小时**（2–4 周全职）。当前完成约 15%。

---

## 1. 规模与结构（已确认）

| 目录 | 文件 | 行数 |
|---|---:|---:|
| `Source/Particles/` | 141 | 21,178 |
| `Source/Diagnostics/` | 124 | 16,949 |
| `Source/FieldSolver/` | 121 | 27,922 |
| `Source/ablastr/` | 57 | 3,632 |
| `Source/Initialization/` | 28 | 4,878 |
| `Source/NonlinearSolvers/` | 26 | 1,096 |
| `Source/MomentsSolver/` | 25 | 1,686 |
| `Source/Utils/` | 25 | 1,840 |
| `Source/BoundaryConditions/` | 18 | 4,590 |
| `Source/Fluids/` | 12 | 2,502 |
| `Source/Python/` | 11 | 1,293 |
| `Source/EmbeddedBoundary/` | 11 | 2,268 |
| `Source/Parallelization/` | 7 | 2,855 |
| `Source/Filter/` | 7 | 568 |
| `Source/Laser/` | 4 | 934 |
| `Source/Evolve/` | 2 | 2,028 |
| **合计** | **636**（.H/.cpp） | **100,539** |

> 另有 `.K`/`.h` kernel 与测试文件未计入；`class/struct` 声明 418 处、唯一类名 343 个（Particles 91、Diagnostics 68、FieldSolver 54、Initialization 33、NonlinearSolvers 28、ablastr 17、MomentsSolver 13、其余 39）。

---

## 2. 上帝类解剖（Route B 的真正成本）

| 指标 | 数值 | 含义 |
|---|---:|---|
| `WarpX.H` | 1,811 行 | 内核/物理/配置/IO 全部混在一起 |
| 成员声明（非函数） | 254 | 状态 + 配置 + 缓存 |
| **static 成员** | **68** | 进程级全局状态；多实例/多包不可能 |
| 成员函数声明 | 191 | 场/粒子/流体/诊断/AMR/IO 全在类里 |
| `WarpX::GetInstance()` 调用 | **241 处 / 82 文件** | 单例耦合；Runtime 必须换成 Context 注入 |
| `#include "WarpX.H"` | **134 文件** | 任何改动都触发大面积重编 |
| `ReadParameters` | **1,477 行 / 178 个参数查询 / 36 个 ParmParse 块** | 配置解析与状态构造耦合 |
| `AllocLevelData` | **409 行 / 52 处 `FieldType`** | 场分配是硬编码 schema，不是数据驱动 |
| `InitFromCheckpoint` 头部 | 手写 `WarpXHeader` 行序 | 重启格式是事实 ABI |
| 动态 regrid | **不存在** | `regrid_int` 死参数；`MakeNewLevelFromCoarse` abort |

**结论**：Route B 的第一年不是“写 Runtime”，而是**拆 68 个 static + 241 个单例调用 + 1,477 行参数解析 + 409 行硬编码分配**。这部分工作量必须计入，否则工期会低估 2–3 倍。

### 2.1 68 个 static 的分类

| 类别 | 例子 | Runtime 归属 |
|---|---|---|
| 算法选择（配置） | `electromagnetic_solver_id`、`current_deposition_algo`、`nox/noy/noz`、`grid_type`、`field_boundary_lo/hi`、`particle_boundary_*`、`load_balance_costs_update_algo`、`time_dependency_*` | `RuntimeConfig` + 包级 schema；插件只读 |
| 数值/运行状态 | `gamma_boost`、`beta_boost`、`boost_direction`、`moving_window_dir`、`moving_window_v`、`do_moving_window` | `TimeState`/`FrameState`；由内核维护 |
| 缓存/工作区 | `shared_tilesize`、`m_fill_guards_fields/current`、`sort_bin_size`、`n_field_gather_buffer`、`n_current_deposition_buffer` | 内核服务；不进插件 |
| 物理常量/编译期 | `ncomps`、`n_rz_azimuthal_modes`、`villasenor_mass_matrices_max_grid_crossings`、`sync_nodal_points` | 构建期常量；`BuildConfig` |
| 单例 | `m_instance` | 删除；Runtime 实例 + 显式 Context |

> 静态成员不是“顺手清理”就能解决的：82 个文件直接读它们。必须先把它们收进 `RuntimeConfig`/`TimeState`/`Services`，再逐个改调用点。这是一个可机械化的重构，但量很大。

### 2.2 `WarpX` 生命周期（已确认）

```
WarpX::WarpX()                      // WarpX.cpp:337
  m_instance = this
  initialize_warning_manager
  ReadParameters()                  // 564–2041, 1477 行
  BackwardCompatibility()
  if (EB::enabled()) InitEB()       // 348
  InitSignalHandling
  resize istep/nsubsteps/t_new/t_old/dt
  mypc = MultiParticleContainer(this)
  myfl (if fluid)
  allocate dotMask/EB flags/borrowing
  m_electrostatic_solver = ...
  m_hybrid_pic_model = ...
  pml.resize / costs / m_field_factory / m_macroscopic_properties
  m_fdtd_solver_* or spectral_solver_*
  nci filters / accelerator lattice

InitData()                          // WarpXInitData.cpp:797
  multi_diags = MultiDiagnostics()
  reduced_diags = MultiReducedDiags()
  ComputeDt / InitFromScratch 或 InitFromCheckpoint
  ComputeMaxStep / ComputePMLFactors / BuildBufferMasks
  m_macroscopic_properties->InitData
  m_electrostatic_solver->InitData
  m_hybrid_pic_model->InitData
  PrintMainPICparameters / WriteUsedInputsFile
  ProjectionCleanDivB
  ComputeSpaceChargeField / AddExternalFields
  multi_diags->FilterComputePackFlush
  CheckKnownEMSolverIssues

InitFromScratch()                   // 1027
  AmrCore::InitFromScratch
    → MakeNewLevelFromScratch     // WarpX.cpp:2352
        AllocLevelData(lev,ba,dm) // 2392–2800, 409 行
        InitLevelData(lev,time)   // 1251
  m_implicit_solver->Define / CreateParticleAttributes
  mypc->AllocData / InitData
  InitPML
  ExecutePythonCallback("allocdata")

Evolve()                            // WarpXEvolve.cpp:150
  for step:
    CheckLoadBalance
    ApplyDtLimiters
    ExplicitFillBoundaryEBUpdateAux
    HybridPICInitializeRhoJandB (first)
    doFieldIonization
    doQEDEvents / doQEDSchwinger
    ExecutePythonCallback("particleinjection")
    OneStep()                       // 412
      implicit → m_implicit_solver->OneStep
      ES/hybrid → collisions + PushParticlesandDeposit
      EM → OneStep_nosub 或 OneStep_JRhom 或 OneStep_sub1
    doResampling
    applyMirrors
    ++istep / cur_time += dt
    ShiftGalileanBoundary
    multi_diags->FilterComputePackFlush
    MoveWindow
    ContinuousFluxInjection
    HandleParticlesAtBoundaries
    particle thermalizer
    implicit collisions
    electrostatic solve / hybrid solve
    diagnostics

Finalize()                          // WarpX.cpp:329
  ResetInstance() → delete WarpX
  WMClear()

~WarpX()                            // 555
  for lev: ClearLevel(lev)          // 2368
```

**已确认的结构性问题**：

1. **构造期就创建物理对象**（`MultiParticleContainer`、ES/Hybrid/Macroscopic/PSATD solver），但它们依赖尚未分配的场。Runtime 必须把“构造”和“分配”分开。
2. **`InitData` 里混合了物理**：静电自场求解、外场加性、div cleaning、诊断初写。
3. **`Evolve` 里混合了 6 类物理**：场电离、QED、碰撞、静电、Hybrid、移动窗口、镜面。
4. **`ClearLevel` 不清理全部 level 资源**：`m_field_factory`、`m_eb_update_*`、`m_flag_info_face`、`m_borrowing`、`m_accelerator_lattice` 不在其中；Runtime 需要统一的 `LevelResources` 生命周期。
5. **重启布局手写**：`WarpXHeader` 行序与 `WriteWarpXHeader` 重复，字段名/顺序变化会破坏兼容。

---

## 3. 子系统矩阵（当前已确认部分）

> 说明：为补齐本表启动的 4 个并行子审计（诊断/IO、Python/构建、EB/求解器、粒子/流体）在时限内没有返回完整报告，已被终止；因此 §3 只列已由主代理确认的结构性结论，细节留待 §5 的审计清单逐项完成。这不是“已完成”，是“已定位”。

| 子系统 | 文件/行 | 状态所有权 | 生命周期 | 提取难度 |
|---|---|---|---|---|
| 粒子 | 141 / 21k | `MultiParticleContainer` 拥有 `WarpXParticleContainer`；粒子 SoA + per-level MultiFab | 构造于 `WarpX` 构造；`AllocData/InitData` 于 `InitFromScratch`；restart 读 header；regrid `Redistribute` | **高**：状态异构、130 处 `mypc->` |
| 流体/矩 | 12+25 / 4k | `MultiFluidContainer` + 注册表场；5 矩私有 `m_U` | 冷流体构造于 `WarpX` 构造；5 矩 hook 在 `OneStep_nosub` | **高**：冷流体无虚函数、5 矩非 AMReX |
| FD 场求解 | 121 / 28k | `m_fdtd_solver_fp/cp` per level；EB flags/PML 私有 | 构造于 `WarpX` 构造；per-level 分配在 `AllocLevelData`；regrid 重建 | **中高**：私有状态多，但入口集中 |
| PSATD | 同上 | `spectral_solver_fp/cp` per level | 构造/regrid 分配；`m_dt` 缓存 | **高**：谱数据/FFT/RZ |
| 静电 | 同上 | `m_electrostatic_solver` | 构造于 `WarpX` 构造；`InitData` | **中** |
| Hybrid | 同上 | `m_hybrid_pic_model` + QDSMC 容器 | 构造；`InitData` | **高**：粒子+场耦合 |
| 隐式 | 同上 | `m_implicit_solver` | 构造；`Define` | **很高**：拥有整步 |
| 诊断 | 124 / 17k | `MultiDiagnostics` + `MultiReducedDiags` | `InitData` 构造；每步；regrid 重建 functor | **中**：基类已虚，工厂硬编码 |
| IO/checkpoint | 同上 | 无独立对象；写侧 `FlushFormat`，读侧 `WarpXIO` | 每 N 步/重启 | **高**：格式是 ABI、读写重复 |
| EB | 11 / 2k | `m_field_factory`、`m_eb_update_*`、`m_flag_info_face`、`m_borrowing`、`m_eb_index_space` | `InitEB` 构造；`InitializeEBGridData` per level；regrid 重建 | **高**：AMReX 对象生命周期 |
| 边界/PML | 18 / 5k | `pml[lev]`、`do_pml_*`、field BC 数组 | `InitPML`/`ComputePMLFactors`；每步调用 | **中**：PML 对象大但入口集中 |
| 初始化/外场 | 28 / 5k | `ExternalFieldParams`、parser | 构造/`InitData` | **中** |
| Python/PICMI | 11 / 1.3k + `Python/` | `libwarpx` 单例绑定 | `initialize/initialize_data/evolve/finalize` | **中高**：必须保持 API |
| 并行/regrid | 7 / 2.9k | `costs`、`load_balance_efficiency`、`RemakeLevel` | 每步 `CheckLoadBalance`；无动态 regrid | **高**：事务式重分配 |
| 全局状态 | 68 static + 241 单例 | 进程级 | 全生命周期 | **最高**：横切所有子系统 |

---

## 3.1 已确认的 Python 钩子（现有“事实阶段”）

`ExecutePythonCallback` 共 46 处，22 个名字，已经是一套“准阶段”：

```
beforestep / afterstep
beforedeposition / afterdeposition
beforecollisions / aftercollisions
beforeEsolve / afterEsolve
beforeInitEsolve / afterInitEsolve / afterInitatRestart
afterBpush / afterEpush
particleinjection / particlescraper / poissonsolver
loadExternalFields / allocdata
afterdiagnostics / oncheckpointsignal / onbreaksignal
```

**含义**：Runtime 的 Stage 设计应从这套已有钩子演化（保持名字兼容），而不是另造一套；Python 用户已经可以用它们做“外挂多物理”。同时暴露了缺的阶段：`BeforeFieldSolve`/`AfterFieldSolve` 只有 `beforeEsolve/afterEsolve`，没有区分 E/B/PML。

## 3.2 构建矩阵（已确认）

`CMakeLists.txt` 的关键开关：

| 开关 | 默认 | 对 Runtime 的影响 |
|---|---|---|
| `WarpX_DIMS` | 3 | 1/2/3/RZ 各自编译；Runtime 必须按维度生成 |
| `WarpX_APP` | ON | 目标可执行；Runtime 入口 |
| `WarpX_LIB` | OFF | 静态/动态库；插件链接方式 |
| `WarpX_EB` | ON | `AMREX_USE_EB` 条件编译；EB 插件必须处理 |
| `WarpX_FFT` | OFF | PSATD/RZ 谱求解器条件编译 |
| `WarpX_OPENPMD` | ON | 诊断/IO 格式 |
| `WarpX_PYTHON` | OFF | pybind 模块；Python 兼容 |
| `WarpX_QED` | ON | QED 条件编译 |
| `WarpX_MPI` | ON | 并行底座的硬依赖 |
| `WarpX_COMPUTE` | — | OMP/CUDA/HIP/SYCL；kernel 插件必须按后端编译 |

**含义**：插件不能假设“所有后端/所有维度”；`Caps` 必须包含 `dims/backend/eb/fft` 或由构建期过滤。

## 3.3 checkpoint/restart 的硬编码程度（已确认）

写侧 `FlushFormatCheckpoint::WriteToFile`（`FlushFormatCheckpoint.cpp:41`）**逐字段硬编码**：

```
Ex_fp / Ey_fp / Ez_fp
Ex_old / Ey_old / Ez_old        (条件)
Bx_fp / By_fp / Bz_fp
Ex_avg_fp ... (条件 fft_do_time_averaging)
F_fp / G_fp (条件 dive/divb cleaning)
PML 分量 (条件 do_pml)
rho_fp / J_fp / rho_medium_fp / ECT 几何 ...
```

读侧 `WarpX::InitFromCheckpoint`（`WarpXIO.cpp:97`）按**行序**解析 `WarpXHeader`：
nlevs → istep → nsubsteps → t_new → t_old → dt → moving_window_x → is_synchronized →
prob_lo → prob_hi → BoxArray → `mypc->ReadHeader` → injection positions → moving_window →
galilean shift → BT diag buffer 时间/索引。

**含义**：
- 新加一个场（插件自定义）默认**不会**被 checkpoint；必须同时改读写两侧。
- Runtime 的 schema 驱动 checkpoint 是必须的，不是“以后再说”。
- 兼容旧格式需要“双读”：schema 找不到的键回退到旧字段名。

## 3.4 诊断对象的构造/状态（已确认）

- `MultiDiagnostics` 在 `WarpX::InitData` **内部**构造（`WarpXInitData.cpp:815`），早于 `InitFromScratch`；`InitData` 需要 `MultiParticleContainer`。
- `FullDiagnostics` 持有 `m_mf_output`、`m_all_field_functors`、`m_flush_format`、`ParticleDiag`、`geom_output`；`InitializeFieldFunctors(lev)` 在初始化与负载均衡后重建。
- `ReducedDiags` 是 5 方法虚基类 + 工厂 map（`MultiReducedDiags.cpp:56-79`），是全仓库**最容易插件化**的部分。
- `FlushFormat` 已是虚接口，但 `FlushFormatCheckpoint` 直接 `WarpX::GetInstance()` 并硬编码字段名。

## 4. 对 Runtime 的直接含义（已可下结论）

### 4.1 必须由 Runtime 拥有（不能下放插件）

1. **AMReX 生命周期**：`AmrCore`、`InitFromScratch`、`RemakeLevel`、`ClearLevel`、`FillBoundary`。
2. **Level 资源表**：每个 level 的 `m_field_factory`、EB flags、PML、spectral solver、costs 需统一登记与释放（现在散落且 `ClearLevel` 漏项）。
3. **68 个 static 的替代物**：`RuntimeConfig`（只读配置）、`TimeState`（step/time/dt/substep）、`FrameState`（boost/moving window）、`Services`（RNG/parser/FFT/reduce）。
4. **单例 → Context 注入**：241 处 `WarpX::GetInstance()` 必须逐步换成 `StageContext`/`Services`；否则 Runtime 无法多实例、无法测试。
5. **checkpoint 格式**：schema 驱动的读写是 Runtime 的职责；否则插件无法自定义状态持久化。
6. **Python 入口**：`libwarpx.initialize/initialize_data/evolve/finalize` 的语义必须保持；Runtime 可以在其后，但 API 不能断。

### 4.2 可以下放插件（但需要内核提供视图/服务）

1. 场方程+离散（`IFieldSolver`），但 PML 对象、EB flags、谱求解器生命周期仍归内核。
2. 物质模型（PIC/冷流体/矩流体），但粒子迁移/排序/缓冲/checkpoint 归内核。
3. 材料系数、边界算法、源项、诊断实现、碰撞/QED/滤波。
4. AMR 策略（tagging/负载均衡/基础网格），但动态 regrid 事务归内核。

### 4.3 Route B 的真实阶段（据此修正工期）

| 阶段 | 内容 | 前置 | 估时 |
|---|---|---|---|
| **A0 审计** | 本文补全：500 文件、生命周期表、依赖图、Python/IO 细节 | — | 2–4 周 |
| **A1 去单例** | `RuntimeConfig`/`TimeState`/`Services`；241 处 `GetInstance` 改注入；68 static 分类迁移 | A0 | 2–3 月 |
| **A2 Level 资源** | 统一 `LevelResources`；`AllocLevelData` schema 化；`ClearLevel` 完整化 | A1 | 2–3 月 |
| **A3 生命周期拆包** | `InitData`/`Evolve`/`Finalize` 拆成包阶段；`WarpX` 降为 `PicEmPackage` | A2 | 3–6 月 |
| **A4 状态 schema** | `FieldRegistry`/`SpeciesRegistry`/`MaterialRegistry`；checkpoint schema 化 | A2 | 3–6 月 |
| **A5 调度器** | Stage + Handler + 数据依赖排序；插件接入 | A3/A4 | 3–6 月 |
| **A6 动态 regrid** | `AmrCore::regrid` + 通用 `RemakeLevel` + 插件 `on_regrid` | A2 | 3–6 月 |

**合计 16–30 人月**（与 V2 的 12–18 人月相比，审计后上调；因为去单例和 Level 资源比预想大）。这是“真正 Route B”的成本；任何“6 个月完成”的说法都是把 A1/A2 藏起来了。

---

## 5. 尚未完成的审计（必须补）

| # | 子系统 | 文件数 | 要回答的问题 |
|---|---|---:|---|
| 1 | 粒子推进/沉积/收集 | 141 | 每个 pusher/deposition 的输入输出键、shape/order 模板、GPU 假设、restart 属性 |
| 2 | 碰撞/QED/电离 | 同上 | 调用点、物种可见性、随机数、每步顺序 |
| 3 | 诊断 functor/InSitu | 124 | 每个 functor 读哪些场/粒子、regrid 重建、checkpoint |
| 4 | FD 算法/kernel | 121 | 每个算法的 stagger、EB 依赖、PML 依赖 |
| 5 | PSATD/RZ/FFT | 同上 | 谱数据布局、PML、RZ 模式、checkpoint |
| 6 | 隐式求解器 | 同上 | 整步拥有权、粒子属性、PETSc 生命周期 |
| 7 | 初始化/注入 | 28 | PlasmaInjector/ExternalField 的状态与 Python 映射 |
| 8 | EB cut-cell | 11 | 每个几何量的生产者/消费者、regrid 重建 |
| 9 | Python/PICMI | 11 + py | 每个 WarpX 字段的 Python 绑定、callback 语义 |
| 10 | 构建/测试 | CMake/tests | 维度/后端/EB/QED 组合、测试依赖 |
| 11 | Utils/parser/filter/lattice | 32 | 服务化边界 |
| 12 | 全局状态 | 68 static + 241 调用 | 每个 static 的读写者、迁移目标 |

**完成标准**：对每个子系统给出
- 状态表（对象/类型/所有者/创建/初始化/regrid/restart）；
- 入口函数与调用序；
- 数据键（读/写）；
- 迁移到 Runtime 的归属（内核/包/插件）；
- 回归门（A/B 或数值）。

---

## 6. 结论

1. **我没有看完源码**；当前约 15%，且集中在“入口/生命周期/全局状态”，不是全部算法。
2. **Route B 可行，但成本被低估**：真正的第一年花在“去单例 + Level 资源 + schema 化”，不是写调度器。
3. **审计必须先完成**：至少补齐 §5 的 12 项；否则 Runtime 的边界会拍脑袋，重写会变成“把上帝类换个名字”。
4. **建议顺序**：A0 审计（2–4 周）→ A1 去单例（2–3 月）→ A2 Level 资源（2–3 月）→ 再谈 Runtime/调度器。
5. **本文会随审计补全**；每个子系统一节，带 file:line 和迁移归属。

---

## 附录：本次审计新增的量化事实

- `Source/`：636 文件、100,539 行、17 个一级目录。
- `WarpX.H`：1,811 行、254 个成员声明、191 个函数、**68 个 static**。
- `WarpX::GetInstance()`：**241 处 / 82 文件**；`#include "WarpX.H"`：**134 文件**。
- `ReadParameters`：**1,477 行、178 个参数查询、36 个 ParmParse 块**。
- `AllocLevelData`：**409 行、52 处 `FieldType`**。
- 动态 regrid：**不存在**；`MakeNewLevelFromCoarse` 在 `WarpX.cpp:2361` abort。
- `ClearLevel`（`WarpX.cpp:2368`）**不清理** `m_field_factory`、`m_eb_update_*`、`m_flag_info_face`、`m_borrowing`、`m_accelerator_lattice`。
- checkpoint 写侧在 `FlushFormatCheckpoint.cpp:77`/`FlushFormatPlotfile.cpp:248`，读侧在 `WarpXIO.cpp:97`，**两套手写布局**。
