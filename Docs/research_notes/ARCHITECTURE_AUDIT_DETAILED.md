# WarpX 插件化 Runtime 架构审计与改造建议（详细版）

> 审计对象：`Docs/research_notes/最终的技术路线.md`（下称 **Plan**）
> 审计范围：`Source/` 全部 **648** 个 `.H/.cpp` 文件，14 个批次逐文件阅读
> 审计目标：在不损失原版功能、不损失用户自定义能力的前提下，判断 Plan 是否正确；并给出可直接执行的修正建议
> 说明：本文中所有行号、成员数量、调用点数量均来自逐文件阅读记录；部分统计为区间估计时会显式标注

---

## 0. 一页结论（TL;DR）

| 问题 | 结论 |
|---|---|
| Route B（Runtime 重写 + strangler 渐进迁移）方向对不对？ | **对**。代码确认存在 235 个文件含静态/单例耦合、69 个 `WarpX` static 成员、79 项 `FieldType` 枚举，上帝类问题是真实的，不是审美问题 |
| 按 Plan 原文 A0→A6 推进能到终点吗？ | **不能**。会在 A1 后半或 A3 卡死。Plan 有 10 处设计层缺口（第 4 章），其中 4 处是硬阻塞 |
| 会丢功能吗？ | **按 Plan 原文会丢 4 项**（隐式子步进、Hybrid-PIC 求解器持粒子、动态 regrid 底座、in-situ 单例通路）；**补接口后全部可挽回**（第 6 章分级） |
| 会丢用户扩展能力吗？ | **按 Plan 原文会丢 3 项**（EB scrape 任意 callable、in-situ、PETSc 运行期配置）；**其余 224 个 EXT 钩子可等价映射**；3 项也可通过 host service + 编译期插件源通道挽回 |
| 最该先做什么？ | 三个"对齐层"：**A0.5 统一 DataBus 与 MultiFabRegister**、**A0.6 Python 回调 → 阶段子位点表**、**A1.5 20+ 运行时 enum → 工厂注册表** |
| 真正的成本在哪？ | 不在功能，在 **嵌套子调度器**（隐式）与 **动态 regrid 底座**（约 +2~4 人月），以及两条热路径的逐位/性能验证 |

---

## 1. 审计方法与覆盖证明

### 1.1 覆盖方式

把 `Source/` 全部源文件按子系统切成 **14 个互不重叠的批次** A–N，用 `diff` 验证批次并集与全量清单**完全一致**（无遗漏、无重复）：

| 批次 | 范围 | 文件数 |
|---|---|---:|
| A | `Source/` 根（`WarpX.H` 1818 行、`WarpX.cpp` 3786 行、`Fields.H`、`main.cpp`）、`Evolve/`、`Initialization/`、`Runtime/` | 46 |
| B | `Source/FieldSolver/` 根、`FiniteDifferenceSolver/`（含 `MacroscopicProperties/`） | 41 |
| C | `Source/FieldSolver/SpectralSolver/`（PSATD / FFT / RZ） | 44 |
| D | `ImplicitSolvers/`、`ElectrostaticSolvers/`（含 `MagnetostaticSolver/`） | 36 |
| E | `Particles/` 核心（Multi/Physical/Photon/Laser/Rigid 容器、Pusher、Gather、Deposition、Sorting、Boundary、IO） | 52 |
| F | `Particles/Collision/` | 53 |
| G | `Particles/ElementaryProcess/`（含 QEDInternals）、`ParticleCreation/`、`Resampling/`、`ParticleThermalizer/` | 36 |
| H | `Diagnostics/` 顶层 + `ComputeDiagFunctors/` | 56 |
| I | `Diagnostics/ReducedDiags/` | 49 |
| J | `Diagnostics/FlushFormats/`、`ParticleDiag/`、`Utils/` | 19 |
| K | `BoundaryConditions/`、`EmbeddedBoundary/`、`Filter/`、`Parallelization/` | 43 |
| L | `Fluids/`、`MomentsSolver/`、`NonlinearSolvers/` | 63 |
| M | `AcceleratorLattice/`、`Laser/`、`Utils/`、`Python/` | 53 |
| N | `ablastr/` | 57 |
| **合计** | | **648** |

每批次要求：逐文件读完（>60KB 的文件按 ≤400 行分页到文件末尾），输出四节：PER-FILE CENSUS（逐文件，标注 `[EXT]`/`[STATIC]`/`[LIFE]`）、FEATURE INVENTORY、EXTENSIBILITY INVENTORY、MIGRATION RISKS。

### 1.2 从源码直接统计出的硬事实

| 事实 | 数值 | 来源 |
|---|---:|---|
| 源文件总数 | 648 | `find Source -name '*.H' -o -name '*.cpp'` |
| 含静态/单例耦合的文件 | **235** | 逐文件 CENSUS 的 `[STATIC]` 标记计数 |
| 含用户扩展钩子的文件 | **227** | `[EXT]` 标记计数 |
| `WarpX` 类 static 数据成员 | **69** | `WarpX.H` 逐行 |
| `RuntimeConfig` 已覆盖的 static 成员 | 41 | `RuntimeConfig.cpp` 逐项列举 |
| 尚无快照归属的 static 成员 | **约 28** | PML 运行态、moving window、filter、Galilean、`dt` 等 |
| `FieldType` 枚举项 | **79** | `Fields.H` `AMREX_ENUM` |
| Python 具名回调 | **18** | `Source/Python/callbacks.H` + 各调用点 |
| 运行时算法 enum 分发点 | **≥20** | 见 §2.3 |
| `GetInstance()` 调用点（真实计数） | A 批 17、B 批（仅 HybridPICModel 约 30）、D 批"处处"、E 批 20+、H 批 36、J 批 13 | 各批 CENSUS |
| `ablastr::MultiFabRegister` 是否已是字符串键 | **是**：`std::map<std::string, MultiFabOwner>` → Plan 严重低估 |
| `MakeNewLevelFromCoarse` 是否可用 | **否**：直接 `abort`（`WarpX.cpp:L2364`），`regrid_int` 是死参数 | Plan 自述 + 源码确认 |
| 谱求解器算法选择 | 运行时 `if/else` 硬编码（`SpectralSolver.cpp:L60-109`；RZ 版 `L58-71`），非注册表 | C 批 |
| 诊断格式选择 | 运行时 `if` 链（`Diagnostics.cpp:L563-591`），非注册表 | J 批 |
| 重采样算法选择 | `if/else`（`Resampling.cpp:L22-31`），非注册表 | G 批 |

### 1.3 18 个 Python 具名回调（必须逐个映射到阶段子位点）

`beforestep`、`afterstep`、`afterdiagnostics`、`onbreaksignal`、`oncheckpointsignal`、`allocdata`、`loadExternalFields`、`beforeInitEsolve`、`afterInitEsolve`、`afterInitatRestart`、`afterBpush`、`afterEpush`、`beforeParticles`、`beforeCollisions`、`beforeEsolvePoisson`、`afterEsolvePoisson`、`beforeImplicitSolve`、`afterImplicitSolve`

这些回调不是 Plan §3.4 十三个标准阶段能一一容纳的：`afterBpush/afterEpush` 位于场求解器**内部**；`loadExternalFields` 位于 `InitLevelData` 之后、div cleaner 之前；`beforeCollisions` 位于粒子循环内部；`afterImplicitSolve` 位于隐式 `FinishStep` 之后。**这是 Plan 的第一个硬缺口。**

---

## 2. 关键代码事实（Plan 未覆盖或低估的）

### 2.1 全局状态比 Plan 统计的多，而且类型不同

Plan 统计的是"66 个 static 成员 + 241 处 `GetInstance()`"，即**被引用点**。真正决定迁移难度的是**引用者**：235 个文件。此外还有一批 Plan 清单里没有的进程级全局：

| 全局 | 位置 | 语义 |
|---|---|---|
| `ParticleType::NextID` | `AddParticles.cpp:L905-914, L1391-1398`；`SmartUtils.H:L52-71` | 全局粒子 ID 计数器 + `omp critical` + 溢出断言 |
| `initialization_policies`（匿名命名空间 `std::map`） | `ParticleCreation/DefaultInitialization.H:L41-62` | 按组件名字符串索引；`operator[]` 读未知名会**静默插入 Zero**（`SmartUtils.cpp:L25`） |
| QED lookup tables（`shared_ptr`，跨对象共享可变） | `MultiParticleContainer.H:L484-485`；分发至 species 见 `PhysicalParticleContainer.cpp:L1873,1880` | 初始化一次后只读，但所有权在容器而非 Runtime |
| `WarpX::m_costs`（`LayoutData`） | 负载均衡 | 每步更新；诊断、碰撞、粒子创建都读同一份 |
| `WarpXSolverVec::m_dofs`（进程级 static） | `WarpXSolverVec.H:L354`、`.cpp:L11,105-109` | 首个向量类型惰性创建，`ExecOnFinalize` 重置；DOF 字段名硬编码（`WarpXSolverDOF.cpp:L24-43`） |
| `WarpXRuntimeData::Legacy()` 内嵌 static local | `WarpXRuntimeData` | **第二个隐藏单例**，首次调用即绑定 |
| `warpx_callback_py_map`（全局 map） | `Python/callbacks.H:L21-48` | 回调注册表 |
| `VisMF::SetHeaderVersion`（进程级开关） | I/O | 全局 |
| FFT openBC solver：static lazy singleton | `ablastr/.../IntegratedGreenFunctionSolver.cpp:L56` | `ExecOnFinalize` 释放 |
| `SignalHandling`：static-only class | `ablastr/.../SignalHandling` | rank 0 安装，MPI 广播 |
| `WarnManager`：Meyer singleton | `ablastr/.../WarnManager` | 唯一显式单例（ablastr 侧） |

**含义**：Plan 的 A1"去单例"清单需要扩充；`NextID`、`initialization_policies`、QED 表、`m_costs`、`m_dofs` 这五类不改变就永远存在"插件隐式依赖全局"。

### 2.2 `ablastr::MultiFabRegister` 已经是字符串键注册表（Plan 的最大误判）

事实（N 批）：

- 类型：`std::map<std::string, MultiFabOwner>`
- 键：完全限定字符串 `name[dir=X][level=N]`
- 每个 entry 持有 `amrex::MultiFab` + 元数据：`level`、`dir`、`m_remake`、`m_redistribute_on_remake`、`m_checkpoint_restart`、`m_owner`（alias 追踪）
- `m_mf_register` 字段是 **public**（`L1017`），外部可直接遍历
- 已有 `alloc_init` / `alias_init` / `remake_level` / `write_checkpoints` API
- 是 **simulation instance 成员**（不是单例）

Plan §2 写"typed registry 是必经之路"，隐含假设"目前只有类型枚举、没有字符串键"。这个假设是错的：字符串键早就存在，缺的是 **schema 声明（produces/reads/单位/能力）**，不是键机制本身。

**后果**：若按 Plan 另建 DataBus，会出现"同一块 MultiFab 被两个系统声明 owner"→ 双重 free / 双重写 checkpoint / `remake_level` 无人调用 / alias 语义重复实现（物化 alias 会让 `Efield_aux` 默认内存翻倍，见 §5）。

### 2.3 ≥20 个运行时算法 enum 分发点（Plan 只列了 5 类角色）

| 分派 | 位置 | 形态 |
|---|---|---|
| EM solver | `WarpX` FDTD 构造 | 枚举 + if |
| grid type（collocated/nodal/staggered） | `FiniteDifferenceSolver.cpp` | 枚举 × 求解器 |
| FDTD algo（Yee/CKC/Nodal） | `EvolveB/E/PML` 内 | 运行时 switch，大量组合直接 abort |
| ECT scheme 1/2/4/6/7/8/9 | `WarpXInitEB` / `ComputeDualGeometry` | 多 scheme 并存 |
| macroscopic solver（LW/BE） | `MacroscopicEvolveE.cpp` | 运行时 |
| PSATD 变体（Galilean/comoving/RZ/PML/cleaning） | `SpectralSolver.cpp:L60-109`、`RZ:L58-71` | if/else |
| implicit solver（theta/semi/Strang/Darwin） | `ImplicitSolverLibrary.H` | include + 工厂 |
| pusher（Boris/Vay/HigueraCary） | `PushSelector.H:L23-104` | 运行时 switch |
| deposition（Esirkepov/Direct/Vay/Villasenor） | `WarpXParticleContainer.cpp:L636-1959` | 六维运行时分派 |
| gather（nox 1-4） | `FieldGather.H` | 运行时 |
| shape factor | `ShapeFactors.H` | 模板 × 运行时 |
| collision type（12+） | `CollisionHandler` | 工厂/if |
| scattering process type（5 种） | `ScatteringProcess.H` | enum |
| resampling（2 种） | `Resampling.cpp:L22-31` | if/else |
| EB scrape policy | `scrapeParticlesAtEB` 模板 | 模板 + callable |
| filter type | `Filter` 子类 | 继承 |
| BC type（PEC/PMC/PEMC/insulator/periodic/...） | `BoundaryConditions` | 枚举 |
| diag type（full/BTD/boundary-scraping） | `MultiDiagnostics` | 字符串 + enum |
| diag format（6 种） | `Diagnostics.cpp:L563-591` | if 链 |
| QED process（QS/BW/Schwinger） | `ElementaryProcess` | 运行时开关 |
| laser injection（antenna/waist/Lab/Boosted） | `Laser/` | 枚举 |
| particle injection style（6 种） | `PlasmaInjector` | 字符串多态 |
| load balance algo（costs/heuristic/knapsack/SFC） | `Parallelization/` | 枚举 |
| bottom solver（bicgstab/hypre/petsc） | `MLMGOptions` | 字符串→enum |

**含义**：Plan 的"插件角色目录"只覆盖了其中约一半。必须补一张**"哪些是插件、哪些是内核分支、哪些必须升级为工厂"**的完整对照表，否则 A1.5 无从下手。

### 2.4 能力矩阵靠运行时 abort 表达

谱求解器（C 批）：`VayDeposition` / `CurrentCorrection` 是纯虚接口，但 RZ 全 abort（`:L490-492`）、Galilean RZ abort（`:L360-362`）、Comoving abort（`:L506-510`）、PML abort（`:L459-467`）、`JRhomFirstOrder` 是空操作（`:L354-370`）。接口签名无法表达"不支持"。

FDTD（B 批）：Silver-Mueller 仅 Yee；`MacroscopicEvolveE` 禁 RZ；`ComputeCurlB` 仅 Cartesian。

**含义**：Plan 的 `HandlerSpec` 只有 produces/reads/stage，**没有 capability 元数据**。调度器需要能查询"这个 handler 在该 grid_type × 维度 × 开关组合下是否可用"，否则只能在运行中 abort（用户体验甚至比现在差，因为现在 abort 在构造期）。

### 2.5 别名语义：Plan 必须保留 `alias`，不能物化

`Bfield_aux` / `Efield_aux` 默认是 `Bfield_fp` / `Efield_fp` 的别名（`WarpX.cpp:L2935-2986`）。MultiFabRegister 的 `m_owner` 已跟踪 alias。

**风险**：若 DataBus 把"读某个 key"实现成"返回该 key 的 MultiFab 拷贝/独立分配"，默认配置内存直接翻倍，并在 `remake_level` 后出现两份不同步的场。

**建议**：DataBus 直接复用 `alias_init`，`FieldSpec` 增加 `alias_of: string`；`Runtime` 在 on_schema 时把 alias 关系注册给 Register，禁止插件对 alias 目标 `alloc_init`。

---

## 3. 逐子系统核对：功能、扩展点、风险、建议

下列每一小节对应一个批次。格式统一为：**功能** / **用户扩展点** / **迁移风险** / **建议**。

### 3.1 内核与生命周期（批次 A：46 文件）

**功能**：`WarX::InitData`（含 restart 分支）、`Evolve` 主循环、时间步 `ComputeDt`、moving window、load balance、checkpoint/restart、signal handling、外部场初始化、PICMI 入口、`Runtime` 薄壳。

**扩展点**：
- 18 个 Python 具名回调（见 §1.3）
- `parse_field_function` 系列（外部 E/B 场，`1`/`2`/`3` 分量 + 时间依赖）
- `loadExternalFields` 允许 Python 直接写 external field MultiFab
- PICMI（`pywarpx`）API
- `warpx.serialize_initial_conditions`、`warpx.eb_implicit_function` 等输入开关

**风险**：
1. `RuntimeConfig` 只覆盖 41/69 个 static；28 个（PML 运行态、moving window、filter、Galilean、`dt`）无归属 → 插件读到旧值
2. `Runtime` 目前 `initialize()/evolve()` 直接转发 `WarpX::GetInstance()`（`Runtime.cpp:L60-79`），是空壳
3. `WarpXRuntimeData::Legacy()` 内嵌隐藏单例；构造顺序隐患
4. `MakeNewLevelFromCoarse` 直接 abort → regrid 只能走 `FromScratch` + `ClearLevel`
5. `InitData` 顺序链含 restart 分支：restart 跳过初始 div-clean（`L890-896`）、外部场走 `PostRestart`（`L1249-1251`）

**建议**：
- A1 的 `RuntimeConfig` 改为**引用访问**（`RuntimeConfig::nox()` 每次实时读 `WarpX` 当前值）或持有非拥有指针，不做值快照
- `WarpXRuntimeData::Legacy()` 的 static local 在 A1 末期删除，改为 Runtime 注入
- 明确 `MakeNewLevelFromCoarse` 的处置：要么实现，要么在 Plan 中把"A6 动态 regrid"降级为"新增能力"而非"迁移保真"

### 3.2 FDTD 与宏观介质（批次 B：41 文件）

**功能**：Yee/CKC/Nodal/ECT 的 `EvolveB/E`、`EvolveBPML/EPML`、`EvolveM`、`MacroscopicEvolveE`、宏观介质（`epsilon/mu/sigma` 标量 + 张量各分量）、subpixel averaging、Lax-Wendroff / backward-Euler、Hybrid-PIC push、QED field push、`ComputeCurl*/Laplacian` 供 Darwin 使用。

**扩展点**：
- 宏观材料：`macroscopic.sigma_function(x,y,z)`、`epsilon_function`、`epsilon_tensor_*`、`sigma_tensor_*`（每分量独立 parser，`MacroscopicProperties.cpp:L36-199`）
- 环境变量 `WARPX_YEE_CELL_EPS`（`MacroscopicEvolveE.cpp:L202`，隐藏扩展口）
- A 外场 parser + 文件输入
- per-species `ParserExecutor<7>` 电阻率
- `afterBpush` / `afterEpush` Python 回调（`WarpXPushFieldsEM.cpp:L957, L1012`）

**风险**：
1. 构造期按 `m_fdtd_algo × m_grid_type` 静态选项 + 运行时 switch；大量非法组合直接 abort
2. `HybridPICModel.cpp` 单文件约 30 处 `WarpX::GetInstance()`
3. PML 由 `WarpX` 驱动：`Evolve*PML` 从 `pml[lev]` 取 EB flag 和 `MultiSigmaBox`（`WarpXPushFieldsEM.cpp:L988-1064`）
4. EB/ECT 依赖 WarpX 成员：`m_flag_info_face`、`m_borrowing`、`dual_A_y`、`dual_L_x/L_z`、`ECTRhofield`
5. 模板双分发 `T_Algo × T_MacroAlgo`，新算法 = 新 struct + 新 enum + 新分支，**无工厂**
6. 状态型生命周期：`m_rho_medium_seeded`、`gauss_check_step`、惰性 `InitSubpixelRows`、`m_evolve_initialized`、RKF45 运行期改写 `m_substeps`
7. **Hybrid-PIC 在 field register 分配 10+ 个 `hybrid_*` FieldType，且持有 `QdsmcParticleContainer`** → 求解器持粒子态
8. `SubcyclingHalf` 半步 B 推进 + ECT 原子 scatter（禁 `ParallelFor`）

**建议**：
- ECT 不设为独立插件，而是 Yee/CKC 求解器的**同族变体**（共用 `m_fdtd_algo` 分支与 dual geometry），避免跨插件互相依赖
- Hybrid-PIC 作为"复合 handler"：`HandlerSpec` 声明 `reads_species/writes_species` + `writes_fields(hybrid_*)`，Qdsmc 容器注册为具名 species
- capability 元数据：`HandlerSpec::supports = {grid_types, dims, requires_flags}`，构造期校验而非运行期 abort
- 环境变量 `WARPX_YEE_CELL_EPS` 提升为一等输入参数（或明确列为不支持迁移）

### 3.3 谱求解器 PSATD（批次 C：44 文件）

**功能**：`SpectralSolver`（非 RZ）/ `SpectralSolverRZ`、Galilean/comoving/voxel、time averaging、`JLinearInTime`、`rho` update scheme、div E/B cleaning、current correction、Vay deposition、spectral PML、RZ 模式、FFT 后端（FFTW/cuFFT/heFFTe）。

**扩展点**：运行期算法选择（`SpectralSolver.cpp:L60-109`），`psatd.*` 输入（含 `psatd.JRhom` 两字符编码，`L1691`）。

**风险**：
1. `SpectralFieldIndex` **硬编码整型索引**：17+ 字段按 8 个开关顺序分配（`SpectralFieldData.H:L101-138`），内核直接 `fields(i,j,k,Idx.Ex)` 数值访问；RZ 叠加 `+ mode*n_fields`
2. FFTW guru plan 绑定临时场数据指针（`SpectralFieldDataRZ.cpp:L181-199`），重分配即失效
3. RZ 系数惰性初始化于首次 push（`PsatdAlgorithmRZ.cpp:L88-120`）
4. RZ 单一共享 Hankel 变换器，断言所有盒同 r 轴（`L205-213`）；主机 LAPACK SVD/LU，每模式 3 个 DHT，构建昂贵
5. RZ 模式耦合：矢量 r/t 分量用 mode±1 阶 DHT（`SpectralHankelTransformer.cpp:L106-122`）
6. guard 填充逻辑内嵌在变换层（`SpectralFieldDataRZ.cpp:L634-756`）
7. `WarpX::getCosts` 计时遍布变换函数；`WarpX::UpperCorner` 取 rmax
8. 能力矩阵靠 abort（见 §2.4）
9. RZ PML 与主算法**同对象双指针**（`SpectralSolverRZ.H:L145-146`），笛卡尔 PML 是独立实例 → 两种结构并存

**建议**：每个谱实例在 `on_schema` 里**显式声明字段顺序表**（`schema = {Ex, Ey, ..., Jx_old, ...}`），`SpectralFieldIndex` 退化为 `name → offset` 查表，只在阶段边界解析一次；内核仍用 int 偏移，零热路径开销。

### 3.4 隐式 / 静电 / 静磁（批次 D：36 文件）

**功能**：`SemiImplicitEM`、`ThetaImplicitEM`、`StrangImplicitSpectralEM`、`SemiImplicitDarwin`、静电（labframe/relativistic、EB Poisson、MLMG/FFT）、静磁（vector potential）、Poisson 边界（parser 势函数）。

**扩展点**：
- **Python `poissonsolver` 回调可整体替换 labframe 求解**（`LabFrameExplicitES.cpp:L68-72`）——这是用户扩展的强通道
- 边界势 parser（`PoissonBoundaryHandler.cpp:L158-192`）
- PETSc 运行期选项（`petsc_*` 参数块、`petsc_reinit_interval`）

**风险**：
1. **自带子步进循环**：`ImplicitSolver::OneStep` 含 ≤8 次 substep、dt 减半、`ResetStep`/`FinishStep`，中途跑 reduced diagnostics + `HandleParticlesAtBoundaries`（`ImplicitSolver.cpp:L203-275`）；`StrangImplicitSpectralEM` **完全覆写 OneStep** 绕过基类
2. 到处 `WarpX* m_WarpX` 回指（`ImplicitSolver.H:L244`）+ WarpX 静态
3. **进程级 static DOF**：`WarpXSolverVec::m_dofs`，字段名硬编码
4. 求解器在**中央注册表**分配状态：`MassMatrices_*`、`Efield_fp_save`、`B_old`、`dA_fp`、`curl2_BC_mask`、`effective_potential_sigma`、`vector_potential_grad_buf_*`
5. checkpoint 耦合：`from_restart` 时从 `FieldType::E_old` 初始化 `m_Eold`
6. PETSc 深度耦合（编译期开关；设备向量直交 PETSc PC）
7. Darwin 依赖 FDTD 内核（`ComputeCurlA/BiLaplacian`）
8. 全部硬编码单 AMR 层；磁静态禁 MR、禁非周期 BC、RZ 单模
9. 无注册表：enum + WarpX.cpp if-chain 工厂，新增格式需改 ≥6 处

**建议**：
- 调度器引入**嵌套子调度**（composite handler 内可再开一个 scheduler 实例）。这是 Plan 必须补的架构能力，不补就无解
- `m_dofs` 改为 Runtime 拥有的资源（init-once，字段名走 schema）
- `poissonsolver` 回调映射为 `HandlerSpec::override_hook`（阶段级替换），保持通道

### 3.5 粒子核心（批次 E：52 文件）

**功能**：species 类型（physical/photon/laser/rigid-injected）、pusher、沉积、gather、shape、边界（absorbing/reflecting/thermal/open）、scraping buffer、sorting、连续/刚性注入、外场、粒子 IO 属性。

**扩展点**：
- `<species>.addRealAttributes/addIntegerAttributes` + parser `(x,y,z,ux,uy,uz,t)`（7 参固定签名）
- 外部 E/B：4 种风格（parser / plasma lens / read-from-file / global）
- boundary buffer per-species opt-in `<species>.save_particles_at_*`
- `plot_filter_function(t,x,y,z,ux,uy,uz)`（诊断侧）
- pusher/deposition/gather 的运行时 enum

**风险**：
1. `WarpXParticleContainer::do_tiling` 是 **static**，由 `particles.do_tiling` 设置
2. `WarpXParIter` 构造直接 `SetDynamic(WarpX::do_dynamic_scheduling)`（`WarpXParticleContainer.cpp:L134,140`）；CPU 非 tiling 路径按 `num_threads` 分配 per-thread 暂存 FAB + `lockAdd`
3. `DepositCurrent` 单函数六维运行时分派（`L636-1959`），含隐式质量矩阵沉积
4. 粒子代码直读大量 WarpX 静态：`particle_boundary_lo/hi`、`nox`、`galerkin_interpolation`、`gamma_boost`、`electrostatic_solver_id`、`load_balance_costs_update_algo`
5. `ParticleBoundaryBuffer` 由 `MultiParticleContainer` 持有，按 boundary id × species 懒构造，EB 用二分求交
6. species 按名查找只在 `MultiParticleContainer`（`mapSpecies`）
7. 温度沉积需 explicit + 禁 shared-mem，并写 `T_*`/`variance_buffer_*` 进 `warpx.m_fields`
8. **沉积核必须 `amrex::For` 而非 `ParallelFor`**（散射累加，issue #7097）
9. checkpoint 粒子格式为 plotfile 粒子；IO 前后**就地**按质量换算动量单位

**建议**：
- `do_tiling`/`do_dynamic_scheduling`/`num_threads` 进 `RuntimeConfig`（引用访问）
- Species 具名注册表从 `MultiParticleContainer` 提升到 Runtime（现有名字映射语义不变）
- 调度器重写时**逐函数对照**"是否 scatter-add"，把 `amrex::For` 列为不可丢约束（写进 CI grep 守卫）

### 3.6 碰撞（批次 F：53 文件）

**功能**：`BackgroundMCC`（含 impact ionization）、`BackgroundStopping`（electrons/ions）、`BinaryCollision`（Coulomb/Perez、DSMC、Bremsstrahlung、LinearBreitWheeler、LinearCompton、NuclearFusion）、`HybridResistiveDrag`、`InverseBremsstrahlung`、`PulsedDecay`。

**扩展点**：`CollisionHandler` 工厂（字符串 type → 具体类）；DSMC 的 `ScatteringProcessType`；背景密度/温度 parser。

**风险**：
1. DSMC **硬编码最多 4 个 ScatteringProcess**（`SplitAndScatterFunc.H`）
2. 三处复制粘贴 `threshold(0.02)/target(0.002)` 概率钳制（LBW/Compton/Fusion）
3. `NuclearFusionFunc` 唯一重写 `AllocData`，且用 `WarpX::GetInstance()` 分配 MultiFab
4. 电离能量口径不一致（filter 用相对论、产物用非相对论，`E_out` clamp 到 0）
5. LBW/Compton/Fusion 禁单精度 → abort；DSMC 无此检查
6. 碰撞读 `WarpX::getCosts(lev)` + `load_balance_costs_update_algo`

**建议**：碰撞不需要新机制，只需 `reads_species/writes_species` + `reads_fields` 声明齐全；`CollisionHandler` 已经是工厂雏形，A1.5 直接升级为注册表。

### 3.7 基元过程 / QED / 注入 / 重采样（批次 G：36 文件）

**功能**：ADK 场致电离、QS 量子同步、BW 对产生、Schwinger、粒子注入 6 风格、重采样 2 算法、热化器。

**扩展点**：
- `<species>.do_field_ionization` 等开关 + 内置电离能表（编译期 Z=1..118，非文件）
- QED `lookup_table_mode = generate|load|builtin` + `load_table_from` / `save_table_in` + `WarpX_QED_TABLE_GEN`
- `<species>.resampling_algorithm`（字符串 if/else）
- `particle_thermalizer.*`（`momentum_threshold` 是 parser 数组）
- 用户属性 parser（7 参）

**风险**：
1. `DefaultInitialization.H:L41-62` 的全局可变 map；未知名静默取 Zero
2. functor 构造期读 WarpX 静态（`Ionization.cpp:L52-54,78-82` 等）
3. `AddParticles.cpp:L1262` 直接 `PhysicalParticleContainer tmp_pc(&WarpX::GetInstance())`
4. QED 表 collective I/O（`ReadAndBcastFile`）+ 跨对象共享 `shared_ptr`
5. 初始化顺序：表必须在 functor build 前就绪（`ALWAYS_ASSERT`）；`ResamplingTrigger` 惰性初始化
6. `AddPlasma` 契约"调用者负责 redistribute"；EB 刮除要求 `distance_to_eb` 已注册
7. `ParticleType::NextID` 全局计数器 + `omp critical`
8. `AddPlasmaFromFile` 的 openPMD `Series` 以 `std::any` 存于 Injector，首调 `std::move` 取走 → 不可重复读
9. 魔法字符串 `"opticalDepthQSR"/"opticalDepthBW"/"ionizationLevel"` 多处匹配
10. `SchwingerFilterFunc` 要求 `src_FABs` 有名为 `Ex..Bz` 的 `Array4` 成员
11. 热化器是 `WarpX` 单例成员，每步 Evolve 调用
12. `Resampling.cpp:L22-31` 手工工厂

**建议**：
- 全局 map 迁为 Runtime 拥有的 schema 默认值表（键在 `SpeciesSpec` 声明时注册，未知名**报错**而非静默 Zero）
- QED 表建模为 init-once 资源，`ResourceSpec{lifetime=session, mutable=false}`
- `NextID` 交由 Runtime 的 `IdService`（保证全局唯一），插件不直接碰计数器
- `AddPlasmaFromFile` 的 one-shot 语义写入 `InjectorSpec{streaming=true, replayable=false}`，调度器不得重放

### 3.8 诊断核心（批次 H：56 文件）

**功能**：`FullDiagnostics`、`BTDiagnostics`（boosted→lab Lorentz + buffer 切片合并 + openPMD BP4/BP5 workaround）、`BoundaryScrapingDiagnostics`、16 个 `ComputeDiagFunctor`、RZ openPMD 路径、plotfile/openPMD 双路径。

**风险**：
1. `WarpXOpenPMD.cpp` 1722 行单文件承载全套 openPMD：后端、ED-PIC 元数据、RZ thetaMode、ADIOS2 resize workaround
2. `MergeBuffersForPlotfile` 依赖 `std::rename` + `Barrier` + `IOProcessorSub`，NFS 不可靠
3. BTD 硬约束直接 abort：`gamma_boost>1`、boost 方向必须 z、moving window 必须开启且不停止
4. BTD plotfile header 四种手写二进制交织（`BTD_Plotfile_Header_Impl.cpp`），AMReX 升级易破
5. 36 处 `GetInstance()`（BTD×14、Full×8、Diagnostics×4、functor×1）
6. RZ 有独立路径，新增诊断量需两处同步
7. **BTD buffer 状态写进 checkpoint**（`m_buffer_counter`/`m_max_buffer_multifabs`/`m_snapshot_full`/`m_lastValidZSlice`/`m_flush_counter`…）

**建议**：`HandlerSpec` 增 `checkpoint_fields`；BTD 作为"复合 handler"（自带 buffer 生命周期 + 子调度），不是简单 diag 插件。

### 3.9 ReducedDiags（批次 I：49 文件）

**现状最好**：21 个子类中 13 个已完全走 `RuntimeContext` 路径（`data()/config()/frame()`）；基类有 fallback 到 `WarpXRuntimeData::Legacy()`。

仍直调 `GetInstance()` 的 6 个：`BeamRelevant`、`ChargeOnEB`、`ColliderRelevant`、`DifferentialLuminosity`、`DifferentialLuminosity2D`、`LoadBalanceCosts`。

`ReducedDiags::WriteToFile` 仍 `WarpX::GetInstance().gett_new(0)`（应换 `frame().time(0)`）。

**建议**：A1.4 Batch C6 按现状推进即可；优先 `ChargeOnEB`（需 `data()` 暴露 `fieldEBFactory(lev)`）与 `LoadBalanceCosts`（需 `data()` 暴露 `getCosts/ComputeCostsHeuristic/get_load_balance_intervals`）。

### 3.10 输出格式（批次 J：19 文件）

**功能**：`FlushFormat` 6 子类（Plotfile / OpenPMD / Ascent / Catalyst / Checkpoint / Sensei）、`ParticleDiag`、`FieldIO`、`ParticleIO`。

**风险**：
1. 格式选择在 `Diagnostics.cpp:L563-591` 硬编码 if 链 + 3 处白名单
2. `FlushFormat::WriteToFile` 是 19 参数巨型签名，其中 6 个只给 BTD 用
3. in-situ 全部断言 `!isBTD`
4. `Sensei` 工厂把 `WarpX::GetInstance()` **const_cast** 成 `amrex::AmrMesh*` 存成员（`Diagnostics.cpp:L574`、`Sensei.H:L76`）
5. Checkpoint 直接用类型化 `FieldType::Efield_fp` 等键枚举 `warpx.m_fields`（`Checkpoint.cpp:L83-189`），另有通用 `write_checkpoints` 钩子
6. 静态/全局：`VisMF::SetHeaderVersion`；openPMD writer 为跨 flush 有状态 `unique_ptr`；Catalyst 全局 init/finalize 绑定构造/析构
7. plotfile 粒子写出时**就地改单位再改回**（`Plotfile.cpp:L448-472`）
8. Checkpoint 为 `final`，`WarpXHeader` 含 BTD 缓冲状态与 `hybrid_pic_substeps` → **字节级重启契约**

**建议**：
- 格式选择升级为注册表（`register_flush_format("openpmd", ...)`）
- in-situ 通过 `IMeshAccess{ AmrMesh const* }` 服务，去掉 const_cast
- checkpoint 双读：新 schema 写 + 旧 `WarpXHeader` 读兼容层，至少保留一个发布周期

### 3.11 边界 / EB / 滤波 / 并行（批次 K：43 文件）

**功能**：PML（笛卡尔 Split-Field 9 分量 + RZ 独立指数阻尼）、PEC/PMC/PEMC/Insulator、EB（parser 隐函数 / STL / CSG / level-set）、ECT scheme 1/2/4/6/7/8/9、filter、load balance。

**扩展点**：
- `warpx.eb_implicit_function = "f(x,y,z)"` → `ParserIF`（GPUable）
- `scrapeParticlesAtEB(pc, distance_to_eb, callable)` **模板接受任意 functor**（NoOp / Absorb / ParticleBoundaryInteraction）
- `PEC_Insulator` 双 parser：`insulator.area_x_lo(y,z)`（`ParserExecutor<2>`）+ `insulator.By_x_lo(y,z,t)`（`ParserExecutor<3>`），每方向 6 个实例

**风险**：
1. **PML 双轨不兼容**：笛卡尔 Split-Field vs RZ 指数阻尼
2. EB 硬编码 `coarsenTo = maxLevel()+20` 三处，用户不可配
3. ECT scheme 多套并存
4. `RemakeLevel` 是 EB 重建枢纽（`eb_reduce_particle_shape`、`eb_update_E/B_fp/cp[3]`、`borrowing`、`fieldEBFactory`、PML 重标）
5. guard cells 偶数强制 `(ng % 2) ? ng+1 : ng`，分散在 `GuardCellManager::Init`/`RemakeLevel`/`FillBoundary`，无统一断言
6. Filter 继承式（填 `m_stencil_0/1/2` + `slen`），与插件接口模型不同
7. PML 构造函数内直接调 EB 标记；`DampJPML` 内检查 `EB::enabled() && eb_update_E[0]`
8. 笛卡尔 PML 与 RZ PML 生命周期/算法完全不兼容

**建议**：
- PML 统一为"边界插件 + 能力声明（dim/grid_type）"，不接受"一个插件两种实现"
- EB scrape 保留模板内核路径；插件侧提供类型擦除 callable；同时保留**编译期插件源**（随内核编译）以支持模板特化需求
- guard cells 偶数规则集中为 `FieldSpec::ngrow` 校验

### 3.12 流体（批次 L：63 文件）

**功能**：冷流体 `WarpXFluidContainer`（N/NUx/NUy/NUz，Muscl-Hancock + Rusanov flux + Higuera-Cary push，支持 3D/XZ/RZ/1D_Z/RCYLINDER/RSPHERE）；5-moment `MomentFluidSolver`（5 分量含内能，FVND SSP-RK3，可插拔 EOS，仅 2D XZ/单层/周期/host）；`MomentsSolver` 模板 `<Eq,DIM>`；`NonlinearSolvers`（PETSc 装配预分配、WeightedJacobi 自适应 omega、6 种预条件器 enum）。

**风险**：
1. **两套流体不共享任何组件**
2. `MomentFluidSolver` 明确是 prototype（2D XZ、单 AMR、周期、host、不感受场）
3. 电荷守恒依赖 FVND 的 RK 加权质量通量 → `(rho^{n+1}-rho^n)/dt + div J = 0`
4. `EquationOfState.H` 用**函数指针表**
5. PETSc 矩阵预分配 `d_nnz/o_nnz` 首次精确装配（issue #7438）
6. 6 种预条件器通过 enum 分发 → `LinearSolverLibrary/PreconditionerLibrary`

**建议**：两套流体在同一 stage 同时写 `rho/J/N/NU` → `HandlerSpec` 必须声明写冲突，调度器需支持"串行化两个 handler 的写集"，否则并发写同一字段。

### 3.13 激光 / 晶格 / 工具 / Python（批次 M：53 文件）

**功能**：激光注入（LabFrame 天线 vs BoostedFrame 全 Lorentz）、4 种轮廓（解析高斯/平顶/外部 HDF5/用户 parser）、Plasma Lens（解析光学 + Gauss/Kolmogorov）、AcceleratorLattice、MoveWindow（shift 20+ 字段类型）、utils（parser、FFT、profiler、RNG、timing）。

**扩展点**：
- 激光：`ParserExecutor<4>(t,x,y,z)` 用户表达式 + 外部文件
- Plasma Lens：`betaX/Y/alphaX/Y` parser + Gauss/Kolmogorov 模型
- **PICMI** / `pywarpx`
- Python 回调全生命周期 + GIL 派发

**风险**：
1. MoveWindow 单函数内 shift 20+ 字段类型（E/B/j/rho/F/G/PML/流体 N/NU/时间平均/粗网格），每个类型单独 case → 插件新字段必须加 case，否则静默不 shift
2. MoveWindow 用 `ParserExecutor<3>` 重曝露格点外场，依赖 node/centered 索引类型
3. RZ-PSATD 强制 Hankel 分块 `blocking_factor_x ≥ 2^k`，**丢弃用户输入**
4. 电离能表为编译期 constexpr（Z=1..118），生成脚本已丢失风险
5. 负载均衡 cost 依赖三一致性
6. Python 回调在 GPU 并行区附近时序不可预测，回调碰 GPU resident MultiFab 有死锁风险

**建议**：MoveWindow 的"字段类型 switch"必须升级为"遍历 register 中所有声明 `moving_window_shift=true` 的字段"，否则 DataBus 的新字段永远漏 shift。这是 Plan 完全没提的一条。

### 3.14 ablastr（批次 N：57 文件）

**事实**：见 §2.2（MultiFabRegister）；另：
- `WarnManager` 是唯一显式 Meyer singleton
- `FFT OpenBCSolver` 是 static lazy singleton（`IntegratedGreenFunctionSolver.cpp:L56`）
- `SignalHandling` 是 static-only class
- `Direction` 类 compile-time 切维度（`WARPX_DIM_RZ|3D|RSPHERE` 宏决定静态成员）→ Runtime 不能运行时切维度
- **ablastr 没有插件机制**：没有字段工厂、求解器注册、粒子插件；外部扩展唯一方式是直接调 `alloc_init/alias_init`
- Solver 全是 header-only 模板纯函数（`computePhi<Rho,Phi,beta,...>`、`computeVectorPotential<Curr,A>`、`computeEffectivePotentialPhi`），不持状态
- Communication 有 single-precision 中间层（`do_single_precision_comms`），`ablastr.fillboundary_always_sync` 控制 `FillBoundaryAndSync`
- `MLMGOptions` 是 POD struct，`parseBottomSolver()` 映射字符串→enum

**结论**：**Runtime 应该拥有 `MultiFabRegister` 实例并把接口注入插件，而不是另建 DataBus**。ablastr 求解器是纯函数，适合作为 `Services` 暴露给插件（`computePhi` 等），不需要包成插件。

---

## 4. Plan 的 10 处设计缺口与逐条修正

### D1（硬阻塞）DataBus 与 MultiFabRegister 语义重叠

**问题**：见 §2.2。Plan §2 误判"typed registry 是必经之路"，实际字符串键早已存在。

**修正**：
1. `Runtime` 持有 `ablastr::fields::MultiFabRegister` 实例（或引用），**不新建 DataBus**
2. 把 `MultiFabRegister` 的 `m_remake`/`m_redistribute_on_remake`/`m_checkpoint_restart`/`m_owner` 提升为一等 schema 字段
3. Plan 的 `FieldKey` 改为 `MultifabKey` 别名，格式统一为 `name[dir=X][level=N]`
4. 能力声明放新层（`FieldSpec`），字段存储仍由 Register 负责

```cpp
// 建议接口（草案）
class DataBus {                       // 薄封装，不拥有存储
public:
    // 注册声明（schema）
    void declare(FieldSpec const&);
    void declare(SpeciesSpec const&);
    void declare(ResourceSpec const&);

    // 访问：全部转发到 RunContext 的 MultiFabRegister
    amrex::MultiFab&       field(FieldKey, int dir, int lev);
    amrex::MultiFab const& field_c(FieldKey, int dir, int lev) const;
    WarpXParticleContainer& species(SpeciesKey);

    // 别名语义：显式，禁止物化
    void alias(FieldKey dst, FieldKey src, int dir, int lev);
};
```

### D2（硬阻塞）Python 18 回调没有阶段子位点

**问题**：见 §1.3。

**修正**：阶段名从 13 个扁平常量改为**扁平 + 子位点**，且顺序语义写死：

```cpp
enum class Stage {
    InitData, AfterInitEsolve, AfterInitAtRestart,
    BeforeStep, AfterStep, BeforeParticles, AfterParticles,
    BeforeFieldSolve, AfterFieldSolve, BeforeCollisions, AfterCollisions,
    BeforeDeposition, AfterDeposition, BeforeDiagnostics, AfterDiagnostics,
    BeforeCheckpoint, AfterCheckpoint, OnBreakSignal,
};
// 子位点：允许 "AfterFieldSolve.afterBpush" 这样的 key
struct StagePoint { Stage stage; std::string sub; };  // sub 为空 = 阶段末
```

并新增 `StagePoint` 的排队规则：同一 `StagePoint` 上多个 handler 按 `priority` 排序；`sub` 位点的执行顺序由父 handler 内部显式调用 `Runtime::run_subpoints("AfterFieldSolve.afterBpush")` 保证。

### D3（硬阻塞）"插件不互认"过严，缺 host service

**问题**：ECT 要读 FDTD 内部 dual geometry；in-situ 要 AmrMesh 指针；Darwin 要 FDTD 的 `ComputeCurlA`；隐式要粒子属性。这些不是"插件互相调用"，是"插件请求内核服务"。

**修正**：新增 **host service 接口族**（内核实现，按需注入），插件只依赖接口，不依赖 `WarpX`：

```cpp
struct HostServices {
    IFieldInternalAccess* fields;   // 取 dual geometry / ECT 临时数组（受限只读）
    IParticleAccess*      particles; // 属性 buffer / 按名取 species
    IMeshAccess*          mesh;      // AmrMesh const*（只读）
    IGeometryAccess*      geom;      // CellSize/LowerCorner/UpperCorner
    ITimeAccess*          time;      // t/dt/istep/gett_new
    ICommunication*       comm;      // FillBoundary/SumBoundary/ReduceSum
    IRng*                 rng;
    ILog*                 log;
};
```

约束写在文档里：service 只暴露**数据与基础操作**，不暴露物理算法；插件调 service 计为"允许"，直接 `WarpX::GetInstance()` 计为"违规"，用 grep 守卫在 CI 强制。

### D4（硬阻塞）RuntimeConfig 用"值快照"会读到旧值

**问题**：69 个 static 里约 28 个运行时可变（`dt`、`finestLevel`、`moving_window_v`、PML 状态、filter 状态、Galilean 速度）。

**修正**：二选一，推荐 (2)：
1. `RuntimeConfig` 持有 `WarpX*` 非拥有指针，所有 getter 实时转发
2. `RuntimeConfig` 是纯虚接口，`WarpXConfigAdapter` 实现；测试可注入 fake

```cpp
struct RuntimeConfig {                 // 接口，不是 POD
    virtual amrex::Real      dt() const = 0;
    virtual int              finest_level() const = 0;
    virtual int              nox() const = 0;
    virtual bool             galerkin_interpolation() const = 0;
    virtual amrex::IntVect  moving_window_dir() const = 0;
    // ...
};
```

`RuntimeContext` 的 `refresh_time` 只刷新时间状态 → 扩展为 `refresh_state()`，或在每次 stage 前由调度器调用。

### D5（硬阻塞）A2/A6 建立在不存在的基础设施上

**事实**：`MakeNewLevelFromCoarse` 直接 abort；`regrid_int` 死参数；所有 regrid 走 `FromScratch` + `ClearLevel`。

**修正**：
- A2 拆为 **A2a 静态多层 LevelResources**（现有 MR 路径）
- 新增 **A6a 动态 regrid 基础设施**：实现 `MakeNewLevelFromCoarse` + `AmrCore::regrid` 路径，再谈 **A6b IAmrPolicy**
- Plan 明确写"A6 是新增能力，不是迁移保真"，避免把"新增能力"当成"回归门"而永远无法交付

### D6（高）20+ 运行时 enum 没有迁移策略

**修正**：新增 Plan §2.5，把每个 enum 归入三类：

| 类别 | 处置 | 例 |
|---|---|---|
| 编译期模板特化 | 保留模板，不进插件系统 | FDTD `T_Algo × T_MacroAlgo`、shape factors、`MomentsSolver<Eq,DIM>` |
| 纯运行时 switch | 升级为注册表 `register_<kind>(name, factory)` | pusher、resampling、diag format、preconditioner、collision |
| 运行期 if 链 + 能力 abort | 升级为注册表 + capability 元数据 | PSATD 变体、FDTD PML、EB scrape policy |

### D7（高）checkpoint 只有单向 schema，缺 `checkpoint_fields`

**修正**：`HandlerSpec` 增加：

```cpp
struct HandlerSpec {
    std::vector<FieldKey>      reads_fields, writes_fields;
    std::vector<SpeciesKey>    reads_species, writes_species;
    std::vector<ResourceKey>   reads_resources, writes_resources;
    std::vector<std::string>   checkpoint_fields;   // 新增：必须持久化的内部状态
    CapabilitySet              supports;            // 新增：dim/grid_type/flags
    std::vector<StagePoint>    stages;
    int                        priority;
};
```

`Runtime::checkpoint()` 只询问声明过的 handler 的 `checkpoint_fields`；`restart()` 反序列化后调用 `handler->on_restart()` 校验一致性。

### D8（高）capability 元数据缺失（运行期 abort 无法表达）

**修正**：`CapabilitySet` 参与**构造期**校验（在选择 handler 组合时提前报错并给出替代建议），而不是运行到一半 abort。

### D9（中）别名语义若物化会翻倍内存

**修正**：见 §2.5；`FieldSpec::alias_of`；Register 的 `alias_init` 是唯一实现。

### D10（中）线程/循环原语约束没写进 Plan

**修正**：Plan 增加"不可丢失的并行语义"附录：
- 散射累加核（沉积、ECT、histogram）必须 `amrex::For`
- 全循环归约用 `amrex::Reduce`
- CPU 非 tiling per-thread 暂存 + `lockAdd` 语义
- `amrex::Gpu::Atomic` 在 CPU 上非原子，且不使 `ParallelFor` 安全
- 写进 `Docs/source/developers/portability.rst` 并在 CI 加 grep 守卫

---

## 5. 隐藏耦合清单（Plan 未提及，但会咬人）

| # | 耦合 | 位置 | 后果 | 处置 |
|---|---|---|---|---|
| 1 | `FieldType` 既是字段枚举又是 checkpoint 键名 | `Fields.H` + `Checkpoint.cpp:L83-189` | 改枚举名 → restart 破 | schema 双读兼容层 |
| 2 | `WarpXHeader` 字节级重启契约 | `WarpXIO.cpp` | 新 schema 写入 ≠ 旧格式 | 保留旧读路径一个周期 |
| 3 | `Efield_aux/Bfield_aux` 是 fp 别名 | `WarpX.cpp:L2935-2986` | DataBus 物化 → 内存翻倍 | 复用 `alias_init` |
| 4 | BTD buffer 状态进 checkpoint | `WarpXHeader` | 重启丢 buffer → 输出错位 | `checkpoint_fields` |
| 5 | Hybrid-PIC 分配 `hybrid_*` + 持粒子容器 | `WarpXPushFieldsHybridPIC.cpp` | 求解器同时产场/粒子状态 | 复合 handler |
| 6 | `do_dynamic_scheduling` static | `WarpXParIter` ctor `L134,140` | 调度语义内嵌容器 | 进 RuntimeConfig |
| 7 | `NextID` 全局计数 + omp critical | `AddParticles.cpp:L905-914` | 插件并行 → ID 冲突 | `IdService` |
| 8 | MoveWindow shift 20+ 字段硬编码 | `WarpXMovingWindow.cpp` | 新字段静默不 shift | register 遍历 |
| 9 | guard cells 偶数强制 | `GuardCellManager`/`RemakeLevel`/`FillBoundary` | 插件字段 ngrow 非法 | `FieldSpec` 校验 |
| 10 | RZ Hankel 分块强制 2^k | `CheckSpectralGridding` | 用户输入被丢 | 文档化 + 校验 |
| 11 | `WarpX::m_costs` 单一 LayoutData | 负载均衡 | 多插件并发更新 | Runtime 拥有 |
| 12 | `WarpXSolverVec::m_dofs` static | `WarpXSolverVec.H:L354` | 第二个隐式求解器会串状态 | init-once 资源 |
| 13 | QED 表跨对象共享可变 | `MultiParticleContainer.H:L484` | 两个 QED 插件互踩 | init-once 只读资源 |
| 14 | openPMD writer 跨 flush 状态 | `WarpXOpenPMD.cpp` | 每 flush 重建 → 元数据丢 | 资源 lifetime 声明 |
| 15 | in-situ const_cast WarpX | `Diagnostics.cpp:L574` | 插件禁单例 → in-situ 死 | `IMeshAccess` |
| 16 | `initialization_policies` 全局 map | `DefaultInitialization.H:L41-62` | 未知名静默 Zero | 未知名报错 |
| 17 | `AddPlasmaFromFile` one-shot `std::move` | `AddParticles.cpp:L624` | 重放 → 空注入 | `replayable=false` |
| 18 | 粒子 IO 就地改单位 | `ParticleIO.H` / `Plotfile.cpp:L448-472` | IO 期间数据被改写，崩则坏 | IO 前后事务化 |
| 19 | 谱 FFTW plan 绑临时指针 | `SpectralFieldDataRZ.cpp:L181-199` | 重分配即失效 | plan 生命周期绑定 owner |
| 20 | `VisMF::SetHeaderVersion` 进程级开关 | I/O | 多格式并发写冲突 | 串行化 IO stage |
| 21 | PML 双轨（笛卡尔/RZ） | `BoundaryConditions/PML*` | 一个插件两种实现 | capability 分实现 |
| 22 | Filter 继承式扩展 | `Filter/` | 与插件接口不兼容 | 保留继承 + 适配器 |
| 23 | `Direction` 类编译期维度 | `ablastr/utils/Direction` | Runtime 不能运行时切维度 | 维持编译期多实例 |
| 24 | 单精度通信中间层 | `ablastr/communication` | 校验式迁移易丢 | 逐位回归门 |

---

## 6. 可挽回性分级（回答"是否不可挽回"）

**结论：没有一项是删掉就加不回来的能力。** 分三级：

### 6.1 零成本（补一句声明 / 换一个复用对象）

| 项 | 修法 |
|---|---|
| PETSc 运行期参数 | 隐式插件读自己的参数块 |
| Python 回调 → 阶段子位点 | 阶段名加 `sub` 字段 |
| ECT 依赖 FDTD 内部状态 | ECT 作 Yee 同族变体，不独立插件 |
| PSATD int 索引 | `on_schema` 声明字段顺序表，name→offset 一次解析 |
| QED 共享表 | init-once 只读资源 |
| Hybrid-PIC 持粒子容器 | `HandlerSpec` 已有 species 读写声明，Qdsmc 注册为具名 species |
| BTD buffer checkpoint | `checkpoint_fields` |
| openPMD 跨 flush 状态 | `ResourceSpec.lifetime=diagnostics` |
| DataBus vs MultiFabRegister | 直接复用 Register |

### 6.2 需小设计（接口 + 适配层，1–3 天/项）

| 项 | 修法 | 风险 |
|---|---|---|
| EB scrape 任意 callable | 内核保留模板；插件侧类型擦除 callable | 每粒子一次虚调用；刮除量小，可忽略 |
| in-situ | `IMeshAccess` 只读 AmrMesh | 零 |
| 用户粒子属性 parser 自动求值 | `SpeciesSpec` 声明 `components{name,type,init_expr}`，Runtime 在 `on_species_create` 求值 | 需固定 7 参签名 |
| 别名语义 | `FieldSpec::alias_of` + `alias_init` | 零 |
| guard cell 规则 | `FieldSpec::ngrow` 校验 | 零 |
| MoveWindow 字段遍历 | register 遍历 + `moving_window_shift` 标记 | 需审计 20+ 字段功能等价 |

### 6.3 需新架构（真正的增量工作）

| 项 | 工作量 | 说明 |
|---|---|---|
| **嵌套子调度**（隐式 ≤8 substep + 中途诊断 + 边界粒子） | +1~2 人月 | 调度器要支持 composite handler 内开子 scheduler；`StrangImplicitSpectralEM` 会覆盖整个单步，需"单步劫持"通道 |
| **动态 regrid 底座** | +1~2 人月 | 实现 `MakeNewLevelFromCoarse` + `AmrCore::regrid`，再谈 IAmrPolicy。**注意：这不是"迁移保真"，是新增能力**（今天也没有） |
| **逐位/性能保真** | 持续 | 别名不物化、沉积用 `amrex::For`、per-thread 暂存 + `lockAdd`；需数值门 + 性能门 |
| **编译期插件源通道** | 0.5 人月 | `WarpX_EXTRA_PLUGIN_SOURCES`，承载需要模板特化的用户（EB scrape 特化、自定义 pusher 内联） |

**总计**：约 +3~5 人月（相对 Plan 的 16–30 人月）。

---

## 7. 修正后的实施路线

```
A0    源码审计（已完成，本报告补齐到 100% 覆盖）
A0.5  统一 DataBus 与 MultiFabRegister（D1）           ← 新增
A0.6  Python 18 回调 → 阶段子位点表（D2）              ← 新增
A1    去单例 + RuntimeConfig 改引用访问（D4）
A1.5  20+ 运行时 enum → 工厂注册表 + capability（D6/D8） ← 新增
A2a   静态多层 LevelResources（现有 MR 路径）
A2b   EB scrape 桥梁 + 编译期插件源（D3/6.3）
A3    生命周期拆包；隐式挂为 substage（嵌套调度）      ← 需新架构
A4    Field/Species/Material/Resource registry + checkpoint schema（D7/D9）
A5    Stage/Handler/依赖拓扑 + HostServices 注入（D3）
A6a   动态 regrid 基础设施（MakeNewLevelFromCoarse）   ← 新增
A6b   IAmrPolicy
```

**每阶段验收门（建议写进 Plan，作为硬门）**：

| 门 | 内容 | 判据 |
|---|---|---|
| G1 逐位回归 | 默认路径 vs legacy 逐位一致 | `ctest` 全量 + checksum；关掉 checksum 容差差异 |
| G2 数值门 | 不依赖 checksum 的物理值断言 | 扩展 `tests/runtime/test_runtime_snapshot.cpp`（probe/坡印廷/extrema 已有先例） |
| G3 扩展性门 | 所有 `[EXT]` 钩子有等价测试 | 每个 parser 表达式 / 每个回调名 至少 1 个 CI 用例 |
| G4 违规门 | 插件目录不出现 `GetInstance()`/`WarpX::` 静态 | `grep -R` 守卫 + CI 失败 |
| G5 别名门 | `Efield_aux` 默认是别名 | 内存统计 + `m_owner` 断言 |
| G6 并行语义门 | 散射核必须 `amrex::For` | 静态检查 + 数值一致性 |
| G7 性能门 | 沉积 / EB scrape / 谱变换 不退化 | 基准计时，允许阈值内波动 |
| G8 重启门 | 旧 checkpoint 可读、新 checkpoint 可写再读 | 双向测试 |

---

## 8. 具体操作手册（先改什么、怎么验证）

### 8.1 第一批改动（低风险、立即见效）

1. `ReducedDiags::WriteToFile`：`GetInstance().gett_new(0)` → `frame().time(0)`
2. `ChargeOnEB`：`RuntimeData` 增 `fieldEBFactory(lev)`，去掉 `.m_fields` 直连
3. `LoadBalanceCosts`：`RuntimeData` 增 `getCosts/ComputeCostsHeuristic/get_load_balance_intervals`
4. `ColliderRelevant`/`DiffLum`/`DiffLum2D`：按 BeamRelevant 的迁移模式（A1.4 C6）
5. `DefaultInitialization` 全局 map：未知组件名从"静默 Zero"改为 `WARPX_ABORT_WITH_MESSAGE`

### 8.2 第二批（结构对齐）

6. `Runtime` 持有 `MultiFabRegister` 引用；`DataBus` 只做 schema + 转发
7. `FieldSpec` 增加 `alias_of/unit/ngrow/moving_window_shift`
8. `StagePoint{stage,sub}`；把 18 个回调名做映射表（附单测）
9. `HandlerSpec` 增加 `checkpoint_fields/supports/priority`

### 8.3 第三批（工厂化）

10. `PushSelector` → `register_pusher`
11. `Resampling.cpp` if/else → `register_resampling`
12. `Diagnostics.cpp` 格式 if 链 → `register_flush_format`
13. `CollisionHandler` 已是工厂 → 补注册 API 与 capability
14. PSATD 算法选择保留 if，但补 `on_schema` 字段顺序声明
15. `PreconditionerLibrary`/`LinearSolverLibrary` 已有工厂 → 对齐接口

### 8.4 验证方法（每批必须做）

- **逐位**：`ctest --test-dir build -LE slow`，默认路径必须零差异
- **数值**：扩 `tests/runtime/`，每类迁移加一个解析解断言
- **扩展点**：为每个 `[EXT]` 钩子加一个最小输入用例（parser 表达式、回调名、QED 表 load/builtin）
- **性能**：沉积/EB scrape/谱变换 各一条计时基准
- **重启**：旧格式读 + 新格式写读回
- **违规**：`grep -rn "WarpX::GetInstance" Source/Runtime Source/Plugins` 必须为空

---

## 9. 对 Plan 文档的具体修改点（逐条）

1. §1.2 I2"插件不互认" → 改为"插件不互认，但可通过 HostServices 请求内核能力"
2. §2 事实表"typed registry 是必经之路" → 删除，改为"MultiFabRegister 已是字符串键，缺 schema 与能力声明"
3. §3.1 `RuntimeConfig` 值快照 → 改为引用/接口访问
4. §3.4 阶段表 13 项 → 增加子位点（18 个 Python 回调逐个落到位点）
5. §3.5 `HandlerSpec` → 增加 `checkpoint_fields` / `supports` / `priority` / `override_hook`
6. §3.6 插件角色目录 → 补全 20+ enum 对照表与三类处置策略
7. §4 A0 → 补 648 文件审计与 235/227 统计
8. §4 A2 → 拆 A2a/A2b
9. §4 A3 → 明确"嵌套子调度"是新增架构能力
10. §4 A6 → 拆 A6a（基础设施）/A6b（策略），并标注"新增能力非保真"
11. 新增 §7"不可丢失的并行语义"与 §8"验收门 G1–G8"

---

## 10. 附：本审计可复现

- 批次清单：`/tmp/batches/{A..N}*.txt`，并集经 `diff` 与 `find Source` 全量清单一致（648/648）
- 逐文件底稿：`/tmp/census/{A..N}*.md`，每批含 PER-FILE CENSUS（该批全部文件编号条目）、FEATURE INVENTORY、EXTENSIBILITY INVENTORY、MIGRATION RISKS
- 前序报告：`Docs/research_notes/ARCHITECTURE_AUDIT.md`（首版，结论一致，本文为详细展开）
- Plan：`Docs/research_notes/最终的技术路线.md`

> 声明：本文所有"会丢/不会丢"的判断均为**基于源码的事实推断**；标注为"估计"的数值明确写出。没有未验证的性能或工期数字，工期区间仅为相对量级。
