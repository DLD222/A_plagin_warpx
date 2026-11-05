# 技术路线审计报告：不损失功能、不损失用户插件能力

<!-- SPDX-License-Identifier: BSD-3-Clause -->

> 状态：**审计结论**，2026-11-05。
> 审计对象：[`最终的技术路线.md`](最终的技术路线.md)（下称"路线"，自称"唯一准绳"）。
> 审计口径：逐文件阅读 `Source/` 全部 **648** 个 `.H/.cpp`（167,741 行），核对路线是否覆盖现有全部能力，以及路线声称的插件能力是否真的可实现。
> 审计分区：Particles(141) / FieldSolver(121) / Diagnostics(124) / 核心生命周期(WarpX+Runtime+Evolve) / 其余子系统(203)。
> 结论摘要：**路线的方向正确，但当前版本不足以保证"不损失功能"**。发现 **13 条结构性缺口**，其中 **5 条会直接使路线自己列出的用户能力无法实现**。路线声称的 16–30 人月**偏乐观**，缺口的修复量未计入。

---

## 0. 一句话结论

路线的**方向**是对的（内核不含物理、插件零互认、命名状态总线），但**契约层太薄**：它定义了"插件长什么样"，没有定义"插件能摸到什么"。源码里 6 类核心机制——**时间步长 dt、隐式求解器重入、Strang 半步序列、粒子对过程、形状因子模板、PML 分裂域**——都在现有契约的表达能力之外。后果不是"实现起来麻烦"，而是**路线自己列出的 7 项用户能力里有 4 项在契约下不可能实现**。

另有一条**硬门自相矛盾**：A4 的 checkpoint schema 化必然改写字段写出顺序，与 I5/A7.1 的"A/B 逐位"硬门直接冲突，路线未识别。

---

## 1. 数字核实结果

路线的审计底稿数字基本可靠，但有 4 处需要修正（这些数字是 A1 工作量估算的基础，不能带错）。

| 路线声称（行号） | 实测 | 判定 |
|---|---|---|
| `Source/` 636 文件 / 100,539 行（:37） | tracked **648** 文件 / **167,741** 行 | 🟡 行数被低估 67%，工作量估算基数偏小 |
| 66 个数据 static（:39） | 65（其中 20 个 `static inline`） | 🟢 一致 |
| 241 处 `GetInstance()` / 82 文件（:40） | **240 处 / 75 文件**（`Source/`） | 🟢 量级一致 |
| `AllocLevelData` 409 行 / 52 处 `FieldType`（:45） | `AllocLevelData` 实为 `WarpX.cpp:2392-2471`（**80 行，0 处 FieldType**）；FieldType 集中在 `AllocLevelMFs`（`2473-3226`，**152 处 / 48 个唯一枚举**） | 🔴 **函数与计数归属均需修正**——路线说的"AllocLevelData 硬编码"实际发生在另一个函数里 |
| `ClearLevel` 漏清 factory/EB/borrowing/lattice（:47） | 全部属实，**另漏** `current/gather_buffer_masks`（`WarpX.cpp:3203,3215`）、`m_eb_reduce_particle_shape`（`:2696`）、`m_flag_ext_face`（`:2755-2766`） | 🟢 正确但不完整 |
| 动态 regrid"不存在"（:46） | 属实：`WarpX.cpp:2364`、`WarpXRegrid.cpp:341` 均 abort | 🟢 正确 |
| Python 钩子 46 处 / 22 名（:49） | C++ 侧 **44 处 / 21 名**；Python 侧另注册 25 名（其中 `afterrestart`、`appliedfields` **无任何触发点**，是死钩子） | 🟡 略偏，且漏了 Python 侧注册表 |
| `ReadParameters` 1,477 行 / 178 参数 / 36 块（:44） | 1,478 行；ParmParse 块实为 **21 个（16 个不同名）**；query/get 71 次 | 🟡 参数数偏差大 |

**关键修正的影响**：`AllocLevelData` 真实硬编码量是 **152 处 / 48 个唯一 FieldType**，不是 52 处。A2「`AllocLevelData` schema 化」的实际工作量是路线的 **约 3 倍**。

---

## 2. 五条致命缺口（会让路线自列的用户能力无法实现）

### 🔴 F1. `dt` 没有归属层级 —— 内核无法驱动时间步

**现状**：`dt` 完全由物理量决定，且**每步可重算**（`m_dt_update_interval` 控制）。
- `ComputeDt()`（`Source/Evolve/WarpXComputeDt.cpp:51-114`）：`cfl*minDim(dx)/c`、按 `electromagnetic_solver_id` 分派 Yee/CKC/ECT/PSATD
- `ApplyDtLimiters()`（`WarpXComputeDt.cpp:270-345`）：粒子速度 CFL `mypc->maxParticleDtInv()`、等离子体频率、回旋频率（**读 `Bfield_aux` 与 `B_external_particle`**）
- 每步重算：`Source/Evolve/WarpXEvolve.cpp:204`

**缺口**：路线全文检索 `CFL`/`时间步`/`步长` → **0 命中**；`dt` 的大小写不敏感检索也只命中一次，且是 `Hardcoded` 的子串（`:62`），并非时间步概念。路线**从未讨论 dt 由谁计算**（T3 `ITimeIntegrator` 一行只写"显式/隐式"，未声明是否拥有 dt 计算权）。内核"不含物理"却必须做 `while (t < t_end)` 与 dt 收敛判定：
- dt 归插件 → 内核驱动时间步违背 I1；
- dt 归内核 → 内核必须知道粒子速度、等离子体/回旋频率，同样违背 I1。

**后果**：这不是补一行角色表能解决的，是**内核与插件之间缺一个"时间推进策略"接口**。必须显式设计 `ITimePolicy`（或等价物）：内核持有"当前 dt"的**数值**，由插件在 `BeforeStep` 提供"下一步 dt 的建议值"，内核负责收敛与截断。

---

### 🔴 F2. 隐式求解器把标准阶段整体重入 —— "拓扑序一次性执行"模型失效

**现状**：`Source/NonlinearSolvers/`（26 文件：Picard / Newton / PETSc SNES+GMRES / 矩阵自由 / 预条件器）在**路线第 4 节角色表中零出现**。

更严重的是语义冲突：
- Picard 循环体每次迭代都**重跑"推粒子 + 沉积 + 场推进"**：`FieldSolver/ImplicitSolvers/ImplicitSolver.cpp:1148-1150` 调 `PushParticlesandDeposit`，`:1156-1160` 调 `ApplyBoundaryAndSync`
- `NewtonSolver` + `JacobianFunctionMF` 用有限差分探针重算 RHS，**探针会污染 DataBus 中的场**；而路线 DataBus 只有"读写权限 + 访问审计"，**没有状态快照/回滚语义**
- 预条件器类型（`CurlCurl` / `Darwin` / `Laplace`）= "用哪种场方程组装线性算子"，与 `IFieldSolver` **同一层级**，不能作为平行插件
- 隐式路径持有**自己的一份场**（`ImplicitSolver.cpp` 内 `m_E`/`m_B`），与主 DataBus 的 `field.E/B` 是两套状态

**后果**：路线的"Stage 严格按数据依赖拓扑序、一次性执行"**无法表达递归重入**。路线给隐式体系只留了 `ITimeIntegrator`（T3，A6+）一个标签，这是结构性缺口。

---

### 🔴 F3. 14 个标准阶段无法表达真实时间序

**现状（已逐行核实）**：`Source/Evolve/WarpXEvolve.cpp:661-682` 的 Strang 序列是

```
EvolveF(0.5dt) → EvolveG(0.5dt) → FillBoundaryF/G
  → EvolveB(0.5dt, FirstHalf)  → FillBoundaryB
  → EvolveE(dt)                → FillBoundaryE
  → EvolveF(0.5dt) → EvolveG(0.5dt)
  → EvolveB(0.5dt, SecondHalf)
```

**B/F/G 各被写两次**，且每次写后立刻跟一次 `FillBoundary`。路线规则 4 规定"同阶段写同键且无先后 → abort"——按路线语义，**所有 FD/PSATD 场求解器插件都会在第一次写 B 时就 abort**。

其他无法映射的环节：

| 真实环节 | 证据 | 14 阶段 |
|---|---|---|
| PIC 段（先推粒子后推场） | `WarpXEvolve.cpp:556-699` | 可映射 |
| **Maxwell-ES 段**（先算场） | `WarpXEvolve.cpp:303-339` | 🔴 **是另一种时间序**，线性阶段列表表达不了 |
| **推粒子与沉积在同一循环内交织** | `PushParticlesandDeposit`（`WarpXEvolve.cpp:1503-1616`） | 🔴 `BeforeParticlePush`/`AfterParticlePush` 切不开 |
| **子循环 coarse/fine 交错 i–v** | `OneStep_sub1`（`WarpXEvolve.cpp:1195-1250`） | 🔴 14 阶段无"子循环层"概念，粒度错配 |
| `ApplyDtLimiters` | `WarpXEvolve.cpp:204` | 🔴 无对应阶段 |
| `HybridPICInitializeRhoJandB` | `WarpXEvolve.cpp:227` | 🔴 无对应阶段 |
| Silver-Mueller 硬编码在 **B 场第一半步** | `WarpXFieldBoundaries.cpp:231-242` | 🔴 无法作为独立边界 handler |

**后果**：`IFieldSolver` 作为 T1 角色的定位不成立——场求解器不是"挂在某个阶段的 handler"，而是**控制着整个阶段序列的东西**。路线的阶段模型选错了抽象层：阶段应该是**可嵌套的**（包内子阶段 + 步内半步序列），而不是扁平的 14 个。

---

### 🔴 F4. `SpeciesHandle` 未定义 —— 粒子侧插件能力整体落空

**现状**：路线第 3.3 节把 `species(SpeciesKey)` 标为"不透明句柄"，**全文没有定义它暴露什么**。而插件侧最核心的几类能力全都要可写地遍历并改写粒子：

| 目标插件 | 必需能力 | 现状 |
|---|---|---|
| 新碰撞 | 双物种可写 tile 迭代、跨物种复制分量（`SmartCopy.H:132-164`）、产生新粒子、读物性（`getMass`/`getCharge`） | 🔴 全部缺失；现有实现靠 `MultiParticleContainer*` **裸指针**（`BinaryCollision.H:196-218`） |
| 新电离 | 写源物种 `ionizationLevel`（`PhysicalParticleContainer.cpp:1499-1501`）、向产物物种注入粒子 | 🔴 缺失 |
| 新粒子模型 | 替换 Evolve/PushPX/PushP/DepositCurrent/DepositCharge | 🔴 无替换点 |
| 新沉积形状 | 注册 shape factor、按物种切换 `nox` | 🔴 见 F5 |

**连带缺口**（角色表里也没有）：

- **粒子边界条件**：`ParticleBoundaries.H:41-93`，每物种、每方向、可带解析器（`reflection_model_*`）；EB 边界 `Absorbing/Reflecting/Thermal`（`MultiParticleContainer.cpp:1250-1273`）→ 路线只有 `IFieldBoundary`，**没有 `IParticleBoundary`**
- **粒子注入**：`PhysicalParticleContainer.cpp:126-139` 的 7 种 `injection_style` + `injection_sources`，**每物种一个注入器向量**（`PhysicalParticleContainer.H:456`）→ 没有 `IInjector` 角色
- **粒子分裂**：`WarpXParticleContainer.cpp:2945-2954`（`particlePostLocate` 改 `id()`）——I3 要求"生命周期归内核、插件只声明"，但 `HandlerSpec` **没有"本物种在 level 边界分裂"的声明位**

**后果**：路线目标里明写的"新物质模型""新碰撞""新电离""新沉积形状"**四项，在当前契约下全部无法实现**。这是最直接的"损失用户插件能力"。

---

### 🔴 F5. 新沉积形状是编译期模板 —— 与"运行期插件"根本冲突

**现状**：`Source/Particles/ShapeFactors.H:27-84` 用 `if constexpr` 展开 `nox=0..4`，`Compute_shifted_shape_factor` 同理；运行期 `nox` 在 `PhysicalParticleContainer.cpp:2034-2061` 分发到**模板实例**。

**缺口**：
- 路线第 3.5 节："运行期 `.so` 可选，**只允许 host 侧 handler**"；第 8 节"不做运行期替换 GPU kernel"
- 但 shape factor 参与 **GPU kernel 内部**的插值计算 → **新形状必须改模板**，不是运行期插件
- 且 `nox` 是**全局**而非按物种 → 用户想要的"电子二阶、离子一阶"也无法表达

**后果**：路线第 1.1 节列的"新物质模型（新 PIC 模型）"里包含形状自由度，这一项**在契约下不可能实现**。要支持它，`IDeposition` 必须允许编译期插件（头文件级注册），而路线明确把插件限定为运行期 `.so`——这是一个**方向性冲突**，不是细节缺失。

---

## 3. 其余 8 条结构性缺口

| # | 缺口 | 关键证据 |
|---|---|---|
| **S1** | **BTD（回溯变换诊断）无处安放**：需独立 flush 时点（`WarpXEvolve.cpp:273` vs `:372`）、Lorentz 逆变换、跨步 boosted-frame 场/粒子历史（`BTDiagnostics.cpp:531`、`MultiParticleContainer.cpp:1136-1144`）。路线全文 `BTD`/`BackTransform`/`Lorentz`/`boost` → **0 命中** | `WarpXEvolve.cpp:273` |
| **S2** | **插件无法自行注册任何诊断**：Reduced 走 `MultiReducedDiags.cpp:56-100` 硬编码 map（20 个 `if (name==...)`）；Full 走 `MultiDiagnostics.cpp:68-74` 枚举白名单硬断言。两处都是内核代码 | `MultiReducedDiags.cpp:56-100` |
| **S3** | **"schema 驱动 IO / 诊断自动生成"当前源码不支持**：Full 变量名硬编码 if/else 链（`FullDiagnostics.cpp:459-620`）、plotfile 写死 12 个 FieldType（`FlushFormatPlotfile.cpp:463-560`）、openPMD 单位**按名字首字母猜**（`WarpXOpenPMD.cpp:348-375`） | 三处均已核实 |
| **S4** | **PML 不是"场求解器内部"，是持自有 DM/FieldFactory/10 个 FieldType 场键的并行 Level 资源**，且在 FillBoundary 路径中**先于**普通场执行（`pml[lev]->Exchange()` + `FillBoundary()` 在 `ablastr::...::FillBoundary` 之前，`WarpXComm.cpp:747-765`）。其"分裂域拷贝/回写"（`CopyJtoPMLs`/`Exchange`）无处安放 | `WarpXComm.cpp:747-765` |
| **S5** | **`Filter` 的 stencil 宽度反向决定内核 guard cell 数**（`GuardCellManager.cpp:370,388` → `ng_FieldGather`/`ng_UpdateAux`），构成内核↔插件反向依赖；且滤波在 SumBoundary 之前执行（`WarpXComm.cpp:1453-1480`），独立成插件会打断内核正确性 | `GuardCellManager.cpp:370` |
| **S6** | **`FieldBoundaryType` 是内核级静态枚举**（`Utils/WarpXAlgorithmSelection.H:125-140`，被 `WarpX.H:242-247` 全局数组持有）。`IFieldBoundary` 插件必须**回写**它 → 违反 I2。另：`Damped` 仅 PSATD 支持且 lo/hi 必须同时设置（`WarpX.cpp:1936-1949,1970-1972`） | 已核实 |
| **S7** | **Python 兼容面远大于路线承认的范围**：`pywarpx` 通过 `get_instance()` 单例 + `multifab_register()` 直连 C++ 内部（`Python/pywarpx/fields.py:212-222`）；字段名是 `Efield_fp`/`current_fp` 而非路线的 `E_x`/`rho`；PICMI 通过**全局参数字典**驱动 ParmParse（`picmi.py:3927`）；Python 还能旁路 `alloc_init` 写场（`Source/Python/MultiFabRegister.cpp`）。路线第 7 节验收清单**无任何 Python 条目** | 已核实 |
| **S8** | **`ablastr` 是"物理 + 基础设施混居"，插件化会分裂**：Poisson/IGF 求解器是物理却在共享层（`ablastr/fields/PoissonSolver.H`、`IntegratedGreenFunctionSolver.H`）、`ablastr/constant.H` 是物理常量（与 I1 冲突）；而 `coarsen/`、`utils/Communication` 是纯内核服务，路线 `Services` 清单**未列 coarsen/interpolate/TextMsg** | 已核实 |

**另需记账的次级缺口**：

- **RNG 只有全局一条流且不入 checkpoint**：`ablastr::math::set_random_seed` 只在初始化调一次（`WarpX.cpp:735-737`），粒子 checkpoint 只存 SoA 分量，不存 RNG 状态。调度器改为拓扑排序后，**增删任一插件都会移动全局随机流** → 直接违反 I5 逐位门；且"重启可复现"从"续流"退化为"重开流"
- **每 handler 的时间步进**（`ndt_supercycle`/`ndt_subcycle`/`start_step`，`CollisionBase.cpp:26-61`）不在 `HandlerSpec` 字段内
- **FieldSpec 缺 11 类元数据**：逐分量 ixType（`FieldSpec.nodal` 是单值，但 E_x/E_y/E_z 各异，`WarpX.cpp:2522-2530`）、fp/aux/buf 变体、是否参与 FillBoundary、remake/redistribute_on_remake、component direction、initial_value、RZ 多模 `ncomps=2*nmodes-1` 实虚交错等
- **`FieldKey` 是扁平字符串，无 species→field 作用域**：真实场有 `current_fp_<spec>`/`rho_fp_<spec>`/`variance_buffer_<spec>`
- **`MomentsSolver` 是与 WarpX 完全平行的独立仿真器**（不进 `Source/CMakeLists.txt`，`Real` 是裸 `double` 裸数组，自带 BC/RK/FVND），路线把它写成 `PicEmPackage` 注册的 `moment5` 插件在**数据模型层就不成立**；且 `ablastr/particles/ParticleMoments.H` 只算 4 阶矩（10 量），MomentsSolver 要 10 矩（45 量），**两套矩定义不兼容**
- **`InitData` 的物理成分很重**（`WarpXInitData.cpp:799,1034,1257`）：粒子注入、外场注入、EB 场初始化、激光注入——`PicEmPackage` 角色表里**没有"外场源/激光注入/EB 几何初始化"角色**
- **`ExternalField` 与 `AcceleratorLattice` 是两套东西**（前者是 E/B 场，后者给粒子加外加速度），路线混为"外场"
- **`Filter` 不是"激光包络滤波"**，是 J/rho 双线性滤波 + NCI Godfrey 滤波
- **14 阶段覆盖不了 Python 钩子**：至少 12 个落在阶段之间（`afterBpush`/`afterEpush`、`beforedeposition`/`afterdeposition`（ES 求解器内部）、`poissonsolver`、`allocdata`、`particlescraper`、`onbreaksignal`/`oncheckpointsignal`（信号处理器））
- **A1 现有 facade 不承重**：`Services` **无任何实现类**（grep `public Services` → 0 命中）、`Runtime.cpp:49` 传 `nullptr`、facade 自身有 5 处 `GetInstance()`（`Runtime.cpp:24,45,66,77,92`）；65 个 static 中 21 个无归属

---

## 4. 一条路线自身未识别的硬门矛盾

**路线第 7 节验收 1**："**同一二进制 A/B 逐位**：默认路径 vs 包路径；探针 ASCII + plotfile payload 字节。"
**路线 A4 验收**："checkpoint schema 化（兼容旧格式）"。

这两条**直接冲突**，原因是 checkpoint 的字节序依赖字段写出顺序：

- `WarpXHeader` 是**纯行序**、无标签：写 `FlushFormatPlotfile.cpp:248-358`，读 `WarpXIO.cpp:121-176` 用 `getline` 逐行
- `MultiFabRegister` 按 `std::map` **名字字典序**写出（`MultiFabRegister.cpp:281,309,336,356` 遍历 `m_mf_register`）→ **新增一个字段名就改变字节序**
- 沉积用 `amrex::Gpu::Atomic::AddNoRet`（GPU 端浮点加顺序不确定）
- `amrex::Reduce` 求 dt 限制（`WarpXComputeDt.cpp:124+`）→ 归约树/线程数变化 → dt 微变 → 全轨迹发散
- 粒子按 bin 排序（`WarpXEvolve.cpp:861` `SortParticlesByBin`）→ 排序键变化 → 浮点求和序变化

A4 必然改写 `WarpXHeader` 顺序 → **逐位门必然失败**。

**路线风险表 R1–R6 完全未提这条**（R4 只写"调度器不确定性/拓扑排序/写冲突 abort"）。

**建议**（不属本审计范围，仅指出必须决策）：要么把逐位门**降级为"默认路径 vs 旧基线，在字段集不变时逐位"**，把"字段集变化后"的验收改成"≤1e-12 数值门 + 显式白名单"；要么在 A4 里**先冻结 checkpoint 字节序**（写侧保持旧序，新字段追加到末尾），把 schema 化推迟到 A4 之后。

---

## 5. 路线自列用户能力的覆盖情况

路线第 1.1 节列的用户能力，逐条核对插件契约：

| 用户能力 | 契约可实现？ | 依据 |
|---|---|---|
| 新场方程/场求解器（Maxwell+引力、新 stencil） | 🔴 **否** | F3（Strang 序列）+ FieldSpec 缺逐分量 ixType |
| 新物质模型（流体+引力、新 PIC 模型） | 🔴 **否** | F4（SpeciesHandle 未定义）+ F5（形状模板） |
| 新材料（ε/σ/μ、相变、损耗） | 🟡 部分 | `IMaterialModel` 方向对，但缺"材料状态随时间演化"的通道（材料状态=标量/场，跨阶段一致性未定） |
| 新边界（PEC/PMC/自定义壁） | 🟡 部分 | S6（须回写内核枚举）+ Silver-Mueller 硬编码在半步 |
| 新源项（外场、电流源、化学反应源） | 🟡 部分 | 缺 `IInjector`（F4）；`ExternalField` 与 `AcceleratorLattice` 语义未分 |
| 新诊断 | 🔴 **否** | S2（注册表硬编码）+ S3（变量名硬编码） |
| 新碰撞 | 🔴 **否** | F4 + 碰撞需插在动量推动**内部**（`WarpXEvolve.cpp:447-473`，`m_collisions_split_momentum_push`） |
| 新 AMR 策略（tagging/负载均衡） | 🟡 部分 | A6 从零实现（`WarpX.cpp:2364` abort），非"补机制" |

**结论**：8 项用户能力中 **4 项在当前契约下不可能实现**（新场求解器、新物质模型、新诊断、新碰撞），2 项部分可行。路线把"插件可插拔"作为第一卖点，但**第一版契约只覆盖了材料与部分边界**。

---

## 6. 路线阶段与工作量的修正建议

不改方向，只补缺口：

| 修正项 | 建议 |
|---|---|
| **补 `ITimePolicy`** | 内核持有 dt 数值，插件在 `BeforeStep` 建议下一步 dt；内核负责收敛/截断。这是 F1 的最小修复。 |
| **阶段改为可嵌套** | 包内可定义子阶段（路线第 3.4 节已提"包可定义子阶段"，但**没有机制**）；步内半步序列（B½/E/B½）需在包层表达，不能是扁平 14 个 |
| **显式定义 `SpeciesHandle`** | 至少给出：可写 tile 迭代、跨物种复制、产生粒子、属性读写、物性查询。否则 F4 无法解决 |
| **加 `IParticleBoundary` / `IInjector` 角色** | 路线第 4 节角色表缺这两类，而它们是粒子侧最常用的输入面 |
| **新增"编译期插件"这一档** | 形状因子、PML 组件这类进 kernel 的能力，只能编译期扩展。路线第 8 节"不做运行期替换 GPU kernel"应改为"**运行期 .so 只支持 host 侧 handler；kernel 内能力用编译期插件**" |
| **A2 工作量 ×3** | `AllocLevelData` 真实硬编码是 152 处 / 48 枚举，不是 52 处 |
| **A6 工作量上调** | `MakeNewLevelFromCoarse` 是 abort（`WarpX.cpp:2364`）、`RemakeLevel` 非等 BA 分支 abort（`WarpXRegrid.cpp:341`）→ 从零实现，不是补机制 |
| **A0 需扩到 12+ 子系统** | 路线第 9 节自述"整体覆盖约 20%"，且 `NonlinearSolvers`/`MomentsSolver`/`Filter`/`AcceleratorLattice` **不在 12 子系统列表内** |
| **逐位门必须先决策** | 见第 4 节。这是路线内部矛盾，不能靠实现绕过 |
| **Python 兼容进验收清单** | 路线第 7 节需加"旧 pywarpx/PICMI 用例全过"一条，否则 A3 必然打断 Python 层 |

**成本修正**：路线估 16–30 人月。上述缺口（F1–F5 + S1–S8）**均未计入**。粗估：

- F1/F3（时间步 + 阶段嵌套）—— 阶段模型重设计，**+3–5 人月**
- F4/F5（SpeciesHandle + 编译期插件档）—— 契约重设计 + 粒子侧大改，**+4–8 人月**
- F2（隐式重入）—— 需要"状态快照/回滚"语义 + 隐式子系统归属决策，**+3–6 人月**
- S1–S8 —— **+4–8 人月**
- A2 ×3、A6 上调 —— **+3–6 人月**

**修正区间：约 30–60 人月**（原 16–30）。这个数字要写进路线，否则 A5 开工时会发现契约不成立。

---

## 7. 结论

1. **方向正确**：内核不含物理、插件零互认、命名状态总线、先去单例再拆生命周期的顺序，都是对的，不需要推翻。
2. **契约层太薄**：路线定义了"插件长什么样"，没定义"插件能摸到什么"。6 类核心机制（dt、隐式重入、Strang 半步、粒子对过程、形状模板、PML 分裂域）在表达能力之外。
3. **会损失功能**：8 项用户能力中 4 项在当前契约下不可能实现。最直接的损失是**新碰撞**（碰撞需插在动量推动内部）、**新场求解器**（Strang 序列 + FieldSpec 缺逐分量 ixType）、**新诊断**（注册表与变量名全硬编码）。
4. **硬门自相矛盾**：A4 改 checkpoint 顺序 vs A7.1 逐位门，路线未识别。
5. **建议**：路线**先补一版"契约 v2"**（补 `ITimePolicy`、可嵌套阶段、显式 `SpeciesHandle`、`IParticleBoundary`/`IInjector`、编译期插件档），再进 A3。当前 A1/A2 的工作**仍然值得做**（去单例与 Level 资源本来就是必要的），但**不要在契约 v2 落地前开始 A3**——否则 A3 会按当前 14 阶段契约硬拆代码，返工量比现在补契约大得多。

---

## 8. 审计方法与可复现性

- 覆盖：`Source/` 全部 **648** 个 `.H/.cpp`（167,741 行），按 5 个分区穷尽阅读，非抽样。
- 重点复核目录：`Source/WarpX.H`、`Source/WarpX.cpp`、`Source/Evolve/`、`Source/Fields.H`、`Source/Particles/{Pusher,Deposition,Collision,ElementaryProcess,Resampling,ParticleThermalizer}/`、`Source/FieldSolver/`、`Source/Diagnostics/`、`Source/BoundaryConditions/`、`Source/EmbeddedBoundary/`、`Source/MomentsSolver/`、`Source/NonlinearSolvers/`、`Source/Fluids/`、`Source/Initialization/`、`Source/Filter/`、`Source/Laser/`、`Source/AcceleratorLattice/`、`Source/Utils/`、`Source/Parallelization/`、`Source/ablastr/`、`Source/Python/`、`Source/Runtime/`、`Python/pywarpx/`。
- 路线文档的关键概念用 `grep -ci` 逐词核对覆盖率（第 1 节表后附的检索结果）。
- 本报告中所有行号均经复核；跨 agent 结论冲突处已由审计者独立执行 `grep`/`sed` 验证（Strang 序列、碰撞插入点、PML 在 FillBoundary 中的位置、`m_mf_register` 的 map 序、`Services` 无实现类、`Runtime.cpp` 的 GetInstance）。
- 复核命令示例：
  ```bash
  grep -n "GetInstance()" -r Source/ | wc -l          # 240
  grep -o "FieldType::[A-Za-z_0-9]*" Source/WarpX.cpp | sort -u | wc -l   # 48
  grep -rn "public Services" Source/ | wc -l           # 0
  grep -ci "CFL" Docs/research_notes/最终的技术路线.md   # 0
  ```

---

> 本报告只报告缺口与修正建议，不重新设计架构。与 `最终的技术路线.md` 冲突时，**以本报告的缺口清单为准**；路线本身仍是方向上的唯一准绳。
