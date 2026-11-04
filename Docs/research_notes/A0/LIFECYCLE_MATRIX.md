# A0：12 个子系统的生命周期矩阵

<!-- SPDX-License-Identifier: BSD-3-Clause -->

> 目标：回答“每个对象谁拥有、何时创建、何时初始化、每步怎么用、regrid/restart 怎么处理、Route B 后归内核还是插件”。
> 证据以 `file:line` 给出；“待验证”表示只读到入口、尚未逐行确认。
> 结论列：**K** = 必须归 Runtime 内核；**P** = 可下放插件；**K+P** = 生命周期归内核、算法归插件。

---

## 1. 粒子 / PIC

| 对象 | 所有者 / 创建 | 初始化 | 每步 | regrid | restart | 结论 |
|---|---|---|---|---|---|---|
| `MultiParticleContainer` | `WarpX` 构造 `WarpX.cpp:365`；构造内 `ReadParameters` + 建容器 `MultiParticleContainer.cpp:97-128` | `AllocData` `:434`、`InitData` `:444`、`InitMultiPhysicsModules` `:464` | `mypc->Evolve`（`PushParticlesandDeposit`，`WarpXEvolve.cpp:1554`） | `mypc->Redistribute` + `defineAllParticleTiles`（`WarpXRegrid.cpp:145` 附近） | `mypc->ReadHeader/PostRestart`（`WarpXIO.cpp:220`、`MultiParticleContainer.cpp:454`） | **K**：物种注册/迁移/重启；**P**：推进/沉积 |
| `WarpXParticleContainer` 派生 | `PCTypes` if/else `MultiParticleContainer.cpp:106-115`（Physical/RigidInjected/Photon）；激光 `:120-122` | `AllocData/InitData` 逐容器 | `Evolve`/`PushPX`/`Deposit*` | `Redistribute`、`SetParticleBoxArray/DistributionMap` | `ReadHeader/WriteHeader`、属性 SoA | **K+P**：容器/migration 归 K；pusher/deposition 归 P |
| `LaserParticleContainer` | MPC 构造 `:120-122` | 随容器 | 作为物种 `Evolve`，J 沉积 | 随容器 | 随容器 | **K+P**：它是粒子物种，不是网格源 |
| `ParticleBoundaryBuffer` | `WarpX` 构造 `WarpX.cpp:392` | — | 边界处理写 buffer | `redistribute()`（`WarpXRegrid.cpp:150` 附近） | 随 checkpoint | **K** |
| `CollisionHandler` | MPC 构造 `:126` | `AllocData` `:440` | `doCollisions`（`MultiParticleContainer.cpp:1221`） | 随 MPC | 无独立状态 | **K+P**：handler 归 K，collision model 归 P |

**提取结论**：粒子状态所有权最复杂。Runtime 必须提供 `SpeciesRegistry` + 统一的 `ParticleContainer` 生命周期/迁移/重启；插件只拿 `SpeciesHandle` + `FieldView` 做 push/deposit/gather。物种创建要从 `PCTypes` if/else 改为 `SpeciesFactory`（A4）。

---

## 2. 流体 / 矩流体

| 对象 | 所有者 / 创建 | 初始化 | 每步 | regrid | restart | 结论 |
|---|---|---|---|---|---|---|
| `MultiFluidContainer` | `WarpX` 构造 `WarpX.cpp:380`（`do_fluid_species`） | `AllocLevelMFs` / `InitData`（`MultiFluidContainer.cpp:58-72`） | `myfl->Evolve`（`WarpXEvolve.cpp:1595`） | **不支持**（`WarpX.cpp:1201` assert `max_level<=1`） | 通过注册表场 `fluid_density_*`/`fluid_momentum_density_*` | **K+P**：注册/分配归 K；Evolve/Deposit 归 P |
| `WarpXFluidContainer` | `MultiFluidContainer` | 同上 | `Evolve`/`GatherAndPush`/`MusclHancock`/`DepositCurrent` | 无 | 同上 | **P**：但它**没有虚函数**，需要先抽接口或适配器 |
| `MomentFluidSolver`（5 矩） | `WarpX` 惰性创建 `WarpXEvolve.cpp:605` | 私有 `m_U` | `AdvanceAndDeposit`，在 PIC J 之后、E 之前 | 无（lev 0、2D-XZ、周期） | 私有状态，**不在注册表** | **P**：需要 `IFluidModel` + 非 MultiFab 状态句柄 |
| `QdsmcParticleContainer` | `HybridPICModel` 内部 | 随 hybrid | 由 hybrid 驱动 | 随 hybrid | 随 hybrid | **K+P**：归 Hybrid 包 |

**提取结论**：冷流体可先做适配器（`IFluidModel` 包 `WarpXFluidContainer::Evolve`）；5 矩必须允许“非 MultiFab 状态”，否则接口会假设注册表。

---

## 3. FD 场求解器（Yee/CKC/ECT）

| 对象 | 所有者 / 创建 | 初始化 | 每步 | regrid | restart | 结论 |
|---|---|---|---|---|---|---|
| `FiniteDifferenceSolver` fp/cp | `AllocLevelData` 按 level 创建 `WarpX.cpp:2929` | 构造时算 stencil（`dt/dx`） | `EvolveB/E/PML/ECTRho`（`WarpXPushFieldsEM.cpp`） | 同 BoxArray 不变；`RemakeLevel` 不重建 FD solver；BoxArray 变化走 `MakeNewLevelFromCoarse` abort | 无状态，重启后按 `AllocLevelData` 重建 | **K+P**：对象可由插件持有，创建/销毁由 LevelResources |
| `m_flag_info_face` | `WarpX` ctor resize `:418`；EB 初始化填 | `InitializeEBGridData` | ECT 借用 | `RemakeLevel` 重建 `m_borrowing`，flags 随 InitializeEBGridData | 无（几何重算） | **K**：ECT 算法数据 |
| `m_borrowing` | 同上 | 同上 | ECT | `RemakeLevel` 重建 | 无 | **K** |
| `m_eb_update_E/B_fp/cp` | ctor resize `:405-411`；`AllocLevelData`/EB 填 | `InitializeEBGridData` | solver 读 | `RemakeLevel` 重建 | 无 | **K** |

**提取结论**：场求解器对象可以插件私有，但 EB flags/borrowing、按 level 的创建/销毁必须由 `LevelResources` 管；否则插件无法安全 regrid。

---

## 4. PSATD

| 对象 | 所有者 / 创建 | 初始化 | 每步 | regrid | restart | 结论 |
|---|---|---|---|---|---|---|
| `spectral_solver_fp/cp` | `AllocLevelData` `WarpX.cpp:2890-2920`；`WarpX` ctor resize `:536` | `AllocLevelSpectralSolver`（构造即初始化 FFT/谱系数） | `PushPSATD`（`WarpXPushFieldsEM.cpp:773`） | `RemakeLevel` 重建 `spectral_solver_fp/cp`（`WarpXRegrid.cpp:240-300`） | 重启后 `AllocLevelData` 重建；`m_dt` 从 dt 重新设 | **K+P**：插件可持有，但重建/`m_dt` 生命周期归 K |
| `SpectralFieldData` / `SpectralKSpace` | `SpectralSolver` 内部 | 随 solver | 谱空间读写 | 随 solver | 不持久化 | **P**（solver 私有） |
| `PML`（PSATD 分支） | `WarpX::InitPML` | `ComputePMLFactors` | `pml->PushPSATD` | 随 PML 重建 | 重建 | **K**：PML 对象归内核，solver 调用 |

**提取结论**：PSATD 是“整步 + 谱数据 + per-level solver”，`IFieldSolver::advance_whole_step` 可包住，但 `LevelResources` 必须能重建 spectral solver；否则 `on_regrid` 必崩。

---

## 5. 静电 / 静磁

| 对象 | 所有者 / 创建 | 初始化 | 每步 | regrid | restart | 结论 |
|---|---|---|---|---|---|---|
| `ElectrostaticSolver` 派生 | `WarpX` 构造 `WarpX.cpp:414-428` | `InitData`（`WarpXInitData.cpp:866`）；Poisson boundary handler | `ComputeSpaceChargeField`（`WarpXSolveFieldsES.cpp:16`） | 无特殊；随 level 几何 | 随重启 | **K+P**：对象可插件，调用阶段归 K |
| `PoissonBoundaryHandler` | ES solver 内 | `InitData` | 边界势 | — | — | **P** |
| `MagnetostaticSolver` | WarpX 成员（`m_vector_poisson_boundary_handler`） | — | LabFrameElectroMagnetostatic 分支 | — | — | **P** |

**提取结论**：静电是“步末重解”，不是时间推进；Runtime 需要独立阶段 `AfterParticlePush/ElectrostaticSolve`，不能硬塞进 `advance_e`。

---

## 6. HybridPIC

| 对象 | 所有者 / 创建 | 初始化 | 每步 | regrid | restart | 结论 |
|---|---|---|---|---|---|---|
| `HybridPICModel` | `WarpX` 构造 `WarpX.cpp:430` | `InitData`（`WarpXInitData.cpp:868`） | `HybridPICEvolveFields`（`WarpXPushFieldsHybridPIC.cpp:26`），**lev 0 only** | 不支持多层 | 随重启 | **K+P**：模型可插件，粒子/QDSMC 生命周期归 K |
| `QdsmcParticleContainer` | `HybridPICModel` | 随 hybrid | 由 hybrid 调用 | 随 hybrid | 随 hybrid | **K+P** |

**提取结论**：Hybrid 同时拥有场和粒子，短期不适合作为第一个插件；先保留为内置整步包。

---

## 7. 隐式

| 对象 | 所有者 / 创建 | 初始化 | 每步 | regrid | restart | 结论 |
|---|---|---|---|---|---|---|
| `ImplicitSolver` 派生 | `WarpX.cpp:1346-1355`（按 `evolve_scheme`） | `Define(this, from_restart)`（`InitFromScratch` `:1033`） | `OneStep` 拥有整步（`WarpXEvolve.cpp:422`） | 禁止子循环（`WarpX.cpp:820-825`） | `Define(..., true)` | **K**：短期不插件化；它是 TimeIntegrator，不是 FieldSolver |
| `WarpXSolverVec` / PETSc | solver 内 | `Define` | `DoSolve` | — | — | **K** |

**提取结论**：隐式求解器拥有粒子+场整步，属于 `ITimeIntegrator` 角色，A6+ 再动。

---

## 8. 诊断

| 对象 | 所有者 / 创建 | 初始化 | 每步 | regrid | restart | 结论 |
|---|---|---|---|---|---|---|
| `MultiDiagnostics` | `WarpXInitData.cpp:815`（`InitData` 内） | `InitData(*mypc)` `:1021` | `FilterComputePackFlush`（`WarpXEvolve.cpp:268/367/404`） | `InitializeFieldFunctors(lev)` | `InitDataBefore/AfterRestart` | **K+P**：注册/调度归 K；具体 diag 归 P |
| `FullDiagnostics` / `BTDiagnostics` | MultiDiagnostics 工厂 `MultiDiagnostics.cpp:22-30` | 构造 + `InitData` | compute/pack/flush | 重建 field functors | 重启 buffer 状态 | **P**：但需要 `on_regrid/on_restart` |
| `MultiReducedDiags` | `WarpXInitData.cpp:818` | `InitData` | `ComputeDiags/WriteToFile` | `LoadBalance()` | checkpoint data | **P**：最便宜，先做 |
| `FlushFormat` | FullDiagnostics 内 | 构造 | `WriteToFile` | — | checkpoint | **K+P**：格式可插件，文件布局归 K |

**提取结论**：诊断是“最容易插件化”的子系统，但 `MultiDiagnostics` 的构造时机（InitData 内、场未分配）要求 Runtime 把“注册诊断”和“初始化诊断”分成两步。

---

## 9. IO / checkpoint

| 对象 | 所有者 / 创建 | 初始化 | 每步 | regrid | restart | 结论 |
|---|---|---|---|---|---|---|
| `WarpXHeader` 写 | `FlushFormatCheckpoint::WriteToFile` `FlushFormatCheckpoint.cpp:41` → `WriteWarpXHeader` `FlushFormatPlotfile.cpp:248` | — | 每 checkpoint | 写 BoxArray | — | **K**：格式是 ABI |
| `WarpXHeader` 读 | `WarpX::InitFromCheckpoint` `WarpXIO.cpp:97` | — | — | 读 BoxArray/DM | — | **K** |
| checkpoint 场 | `FlushFormatCheckpoint` 逐字段 `VisMF::Write` | — | — | — | 逐字段读 | **K**：需 schema 化 |
| openPMD/plotfile | `FlushFormatOpenPMD/Plotfile` | — | 每 N 步 | — | — | **K+P**：格式插件，调度归 K |
| reduced diag checkpoint | `MultiReducedDiags::WriteCheckpointData` | — | — | — | 读 | **P** |

**提取结论**：检查点读写是两套手写布局，且逐字段硬编码；Runtime 必须先做 schema 驱动，否则任何新场/新插件状态都无法持久化。

---

## 10. EB

| 对象 | 所有者 / 创建 | 初始化 | 每步 | regrid | restart | 结论 |
|---|---|---|---|---|---|---|
| `m_eb_index_space` / `EB2::Level` | `InitEB` `WarpXInitEB.cpp:74` | `EB2::Build`（per level 或 finest） | 查询 | 不重建（几何静态） | 从输入重建 | **K** |
| `m_field_factory`（`EBFArrayBoxFactory`） | `AllocLevelData` `WarpX.cpp:2438` | 随 level | 分配场 | `RemakeLevel` 重建 | `AllocLevelData` | **K** |
| `edge_lengths` / `face_areas` / `area_mod` | `AllocLevelData` | `InitializeEBGridData` `WarpXInitData.cpp:1603` | ECT 读 | `InitializeEBGridData` | 重算 | **K+P**：几何量归 K，cut-cell 算法归 P |
| `m_eb_update_E/B_*` / `m_flag_info_face` / `m_borrowing` | ctor/`AllocLevelData` | `InitializeEBGridData` | solver 读 | `RemakeLevel` 重建 | 重算 | **K** |
| `dotMask` / `distance_to_eb` | ctor/`AllocLevelData` | `ComputeDistanceToEB` | 诊断/边界 | 重建 | 重算 | **K** |

**提取结论**：EB 是 AMReX 对象生命周期最密集的地方；`RemakeLevel` 一次性重建 8 类对象。插件只能提供 cut-cell 算法，不能拥有这些对象。

---

## 11. 边界 / PML

| 对象 | 所有者 / 创建 | 初始化 | 每步 | regrid | restart | 结论 |
|---|---|---|---|---|---|---|
| `pml[lev]` / `PML_RZ` | `WarpX` ctor resize `:528-533`；`InitPML` `WarpXInitData.cpp:1048` | `ComputePMLFactors` `:1130` | `Evolve*PML`、`DampPML`、`PushPSATD` | 随 level 重建 | 重建 | **K**：PML 对象归内核 |
| `field_boundary_lo/hi` | `ReadParameters`（`WarpX.cpp:579` 块） | — | `Apply*Boundary` | — | 随输入 | **K**：配置；**P**：PEC/PMC/绝缘体算法 |
| `particle_boundary_lo/hi` | 同上 | — | 粒子边界处理 | — | 随输入 | **K+P** |
| `do_pml_Lo/Hi` | ctor resize | `InitPML` | — | — | — | **K** |

**提取结论**：PML 是 solver 层对象，不能交给通用 `IBoundary::apply`；PEC/PMC/绝缘体/轴才是边界插件。

---

## 12. 横切：全局状态 / Python / 构建

| 项 | 现状 | Route B 归属 | 证据 |
|---|---|---|---|
| 68 个 `static` 成员 | 配置 + 运行状态 + 缓存混在一起 | `RuntimeConfig` / `TimeState` / `FrameState` / `Services` | `WarpX.H` 190-400、861-897 |
| 241 处 `WarpX::GetInstance()` | 单例 | `StageContext` / `Services` 注入 | 82 文件 |
| Python `libwarpx` | `initialize/initialize_data/evolve/finalize` 绑定 `WarpX` 单例 | Runtime facade；保持 API | `Source/Python/WarpX.cpp:100-135` |
| 22 个 Python callback | 事实阶段 | Runtime Stage 兼容名 | `ExecutePythonCallback` 46 处 |
| 构建矩阵 | DIMS/EB/FFT/OPENPMD/PYTHON/QED/MPI/COMPUTE | `BuildConfig` + 插件 Caps 过滤 | `CMakeLists.txt:56-70` |
| 测试 | pytest + ctest | 回归门 | `tests/unit`、`Examples/CMakeLists.txt:28` |

**提取结论**：横切项是 A1/A2 的主战场；不先做，Runtime 无法多实例、无法测试、无法按后端裁剪。

---

## 13. 汇总：内核 vs 插件

| 类别 | 必须归内核（K） | 可下放插件（P） |
|---|---|---|
| 网格/AMR | `AmrCore`、BA/DM、`RemakeLevel`、`ClearLevel`、guard cell、通信 | tagging/负载均衡/基础网格策略 |
| 场 | `LevelResources`、EB flags、PML 对象、谱 solver 生命周期 | 场方程/离散、PML stencil、边界算法 |
| 物质 | 物种注册、粒子容器/迁移、流体注册表、checkpoint | pusher/deposition/gather、EOS/closure、碰撞/化学 |
| 材料 | 系数场注册、`rho_medium` 场 | ε/σ/μ、相变、subpixel |
| 诊断/IO | 调度、checkpoint schema、格式 ABI | 具体诊断、输出格式 |
| 全局 | 配置/时钟/服务/插件宿主 | 无 |

---

## 14. 待验证项

- PSATD `spectral_solver` 在 restart 时是否完全由 `AllocLevelData` 重建（已读入口，未逐行确认 `m_dt`/谱系数）。
- `RemakeLevel` 对所有 level 资源的覆盖是否完整（已发现 `ClearLevel` 漏项）。
- `MultiDiagnostics` 在 `InitData` 内构造、场未分配时的 `InitData` 时序。
- Python callback 在 implicit/hybrid 分支的完整性。
- EB `m_build_eb_data_per_level` 与 `RemakeLevel` 的交互。

> 这些项在 A0 第二遍逐行确认；未确认前，`LevelResources` 的设计不能定稿。
