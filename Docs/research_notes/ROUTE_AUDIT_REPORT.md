# WarpX 插件化 Runtime 技术路线：全面审计报告

<!-- SPDX-License-Identifier: BSD-3-Clause -->

> **文档性质**：对 [`最终的技术路线.md`](最终的技术路线.md)（下称"路线"，自称"唯一准绳"）的完整技术审计。
> **审计日期**：2026-11-05
> **审计问题**：该路线在实施后，是否会**损失现有功能**，是否会**损失用户插件的能力**（即路线第 1.1 节承诺的 8 项用户能力）。
> **审计结论**：**方向正确，但当前版本不足以保证"不损失功能"。** 发现 **13 条结构性缺口**（其中 **5 条致命**，直接导致路线自列的用户能力无法实现）+ **1 条路线内部硬门矛盾** + **8 处需修正的数字**。缺口修复量**未计入**路线的 16–30 人月，修正后为 **30–60 人月**。
> **前置文档**：结论速览版见 [`ROUTE_AUDIT_REVIEW.md`](ROUTE_AUDIT_REVIEW.md)；本文是完整版，含全部证据、失败场景与修复方向。

---

## 目录

- [第 0 章 结论摘要](#第-0-章-结论摘要)
- [第 1 章 审计范围与方法](#第-1-章-审计范围与方法)
- [第 2 章 需修正的数字](#第-2-章-需修正的数字)
- [第 3 章 五条致命缺口](#第-3-章-五条致命缺口)
- [第 4 章 八条结构性缺口](#第-4-章-八条结构性缺口)
- [第 5 章 路线内部硬门矛盾](#第-5-章-路线内部硬门矛盾)
- [第 6 章 用户能力覆盖矩阵](#第-6-章-用户能力覆盖矩阵)
- [第 7 章 契约 v2 修订建议](#第-7-章-契约-v2-修订建议)
- [第 8 章 阶段计划与工作量修正](#第-8-章-阶段计划与工作量修正)
- [第 9 章 行动清单](#第-9-章-行动清单)
- [附录 A 源码证据索引](#附录-a-源码证据索引)
- [附录 B 复核命令](#附录-b-复核命令)

---

## 第 0 章 结论摘要

### 0.1 一句话

路线的**方向是对的**——内核不含物理、插件零互认、命名状态总线、先去单例再拆生命周期，这几个判断都经得起源码检验，**不需要推翻**。

但路线的**契约层太薄**：它定义了"插件长什么样"（`IPlugin` / `HandlerSpec`），**没有定义"插件能摸到什么"**（`SpeciesHandle` 未定义、`FieldSpec` 缺 11 类元数据、`DataBus` 无快照/回滚语义）。源码里 6 类核心机制——**时间步长 dt、隐式求解器重入、Strang 半步序列、粒子对过程、形状因子模板、PML 分裂域**——全部落在现有契约的表达能力之外。

后果不是"实现起来麻烦"，而是**路线自己列出的 8 项用户能力里有 4 项在契约下不可能实现**。

### 0.2 缺口一览

| 编号 | 缺口 | 严重度 | 直接后果 |
|---|---|---|---|
| **F1** | `dt` 无归属层级 | 🔴 致命 | 内核无法驱动时间步（路线全文 0 次讨论 dt） |
| **F2** | 隐式求解器把标准阶段整体重入 | 🔴 致命 | "拓扑序一次性执行"模型失效 |
| **F3** | 14 个标准阶段无法表达真实时间序 | 🔴 致命 | 所有 FD/PSATD 场求解器插件都会 abort |
| **F4** | `SpeciesHandle` 未定义 | 🔴 致命 | 粒子侧插件能力整体落空 |
| **F5** | 新沉积形状是编译期模板 | 🔴 致命 | 与"运行期插件"根本冲突 |
| **S1** | BTD（回溯变换诊断）无处安放 | 🟠 高 | 路线 0 次提及 boosted frame |
| **S2** | 插件无法自行注册任何诊断 | 🟠 高 | "新诊断"能力不成立 |
| **S3** | "schema 驱动 IO"当前源码不支持 | 🟠 高 | A4 工作量被低估 |
| **S4** | PML 是并行 Level 资源，非"求解器内部" | 🟠 高 | 分裂域拷贝/回写无处安放 |
| **S5** | Filter 的 stencil 宽度反向决定 guard cell | 🟡 中 | 内核↔插件反向依赖 |
| **S6** | `FieldBoundaryType` 是内核静态枚举 | 🟡 中 | `IFieldBoundary` 须回写内核 → 违反 I2 |
| **S7** | Python 兼容面远大于路线承认范围 | 🟠 高 | A3 会打断整个 Python 层 |
| **S8** | `ablastr` 是"物理 + 基础设施混居" | 🟡 中 | 插件化会分裂共享层 |
| **C1** | 逐位硬门 vs A4 checkpoint 改序 | 🔴 矛盾 | 路线验收 1 与 A4 验收互斥 |

### 0.3 三点最重要的判断

1. **不要推翻路线，但不要在契约 v2 落地前开 A3。** 当前 A1/A2 的工作（去单例、统一 Level 资源）本来就必要，可以继续做；但 A3「生命周期拆包」会按当前 14 阶段契约硬拆代码，一旦 F1/F3/F4 暴露，返工量远大于现在补契约。

2. **路线的 16–30 人月是"没算缺口"的数字。** 5 条致命缺口各自都需要接口层重设计（不是加字段），合计 **+10–19 人月**；A2 实际工作量是路线的 3 倍；A6 是从零实现而非补机制。

3. **"逐位"这条硬门不能靠实现绕过。** checkpoint 字节序依赖字段写出顺序（`std::map` 名字序 + 纯行序 header），A4 schema 化必然改序。这必须在**开工前**做决策，不能留给实现阶段发现。

---

## 第 1 章 审计范围与方法

### 1.1 覆盖范围

审计**穷尽阅读** `Source/` 下全部 tracked `.H` / `.cpp`，非抽样：

| 分区 | 文件数 | 重点目录 |
|---|---:|---|
| 核心生命周期 | 15 | `WarpX.H`、`WarpX.cpp`、`main.cpp`、`Evolve/`、`Runtime/` |
| 粒子 | 141 | `Particles/{Pusher,Deposition,Collision,ElementaryProcess,Resampling,ParticleThermalizer}/`、`Physics/` |
| 场求解器 | 121 | `FieldSolver/`、`Fields.H` |
| 诊断与 IO | 124 | `Diagnostics/`、`Particles/ParticleIO.H` |
| 其余子系统 | 203 | `BoundaryConditions/`、`EmbeddedBoundary/`、`MomentsSolver/`、`NonlinearSolvers/`、`Fluids/`、`Initialization/`、`Filter/`、`Laser/`、`AcceleratorLattice/`、`Utils/`、`Parallelization/`、`ablastr/`、`Python/` |
| **合计** | **648** | **167,741 行** |

另核实 `Python/pywarpx/`（PICMI / 字段访问 / 回调注册表）。

### 1.2 三重核对方法

1. **能力清单法**：枚举每个子系统里由输入参数选择的算法分支，逐条判定"能否用插件契约表达"。
2. **概念覆盖法**：把路线的核心机制（`dt`、`CFL`、隐式、矩、PML、BTD、boost、注入器、RNG 等）逐词 `grep -ci` 检索路线文档，命中为 0 即为路线未覆盖。
3. **独立复核**：5 个分区并行审查中结论互相冲突处，由审计者亲自执行 `grep` / `sed` 验证。**本文所有行号均经此复核**，包括：
   - Strang 序列（`WarpXEvolve.cpp:661-682`）
   - 碰撞插入动量推动内部（`WarpXEvolve.cpp:447-473`）
   - PML 在 FillBoundary 中的位置（`WarpXComm.cpp:747-765`）
   - `m_mf_register` 的 map 名字序（`MultiFabRegister.cpp:281,309,336,356`）
   - `Services` 无任何实现类（全仓库 0 命中）
   - `Runtime.cpp` 自身 5 处 `GetInstance()`

### 1.3 判定标准

| 标记 | 含义 |
|---|---|
| 🔴 致命 | 该能力在路线现有契约下**不可能实现**，且它是路线自列的用户能力之一 |
| 🟠 高 | 该能力可实现但需**改内核代码**（破坏"插件不改内核"承诺），或路线对其表述与源码不符 |
| 🟡 中 | 契约表述不足，需补字段/补说明，但不改架构 |

---

## 第 2 章 需修正的数字

路线的审计底稿（`SOURCE_AUDIT.md`）质量可靠，绝大多数数字经核实成立。但以下 8 处需要修正——**这些数字是 A1/A2 工作量估算的基础，不能带错**。

| # | 路线声称（行号） | 实测 | 影响 |
|---|---|---|---|
| 1 | `Source/` 636 文件 / 100,539 行（`:37`） | **648** 文件 / **167,741** 行 | 🟡 行数低估 **67%**，工作量估算基数偏小 |
| 2 | `AllocLevelData` 409 行 / 52 处 `FieldType`（`:45`） | `AllocLevelData` 实为 `WarpX.cpp:2392-2471`（**80 行，0 处 FieldType**）；FieldType 集中在 `AllocLevelMFs`（`2473-3226`）＝ **152 处 / 48 个唯一枚举** | 🔴 **函数归属与计数均错**。A2 工作量实为路线的 **约 3 倍** |
| 3 | 241 处 `GetInstance()` / 82 文件（`:40`） | **240 处 / 75 文件**（`Source/`） | 🟢 量级一致，无需改 |
| 4 | 66 个数据 static（`:39`） | 65（其中 20 个 `static inline`） | 🟢 一致 |
| 5 | `ReadParameters` 1,477 行 / **178 参数** / **36 ParmParse 块**（`:44`） | 1,478 行；ParmParse 块实为 **21 个（16 个不同名）**；query/get 共 71 次 | 🟡 参数数偏差大，块数偏差近一半 |
| 6 | Python 钩子 46 处 / 22 名（`:49`） | C++ 侧 **44 处 / 21 名**；Python 侧另注册 **25 名**（其中 `afterrestart`、`appliedfields` **全仓库无触发点**，是死钩子） | 🟡 漏了 Python 侧注册表这一半 |
| 7 | `ClearLevel` 漏清 factory/EB/borrowing/lattice（`:47`） | 全部属实，**另漏** `current/gather_buffer_masks`（`WarpX.cpp:3203,3215`）、`m_eb_reduce_particle_shape`（`:2696`）、`m_flag_ext_face`（`:2755-2766`） | 🟢 正确但不完整 |
| 8 | 动态 regrid"不存在"（`:46`） | 属实：`WarpX.cpp:2364`、`WarpXRegrid.cpp:341` 均 abort | 🟢 正确 |

**另有一处路线文档的表述误差**：路线第 0 节称"源码审计约完成 15%"，第 9 节称"整体覆盖约 20%"。而 `AUTOMATION.md` 的阶段表已把 A0 记为 **5/5 完成**。三个数字互相矛盾，且实际覆盖约 20%（详见第 4 章 S1–S8：多个子系统根本不在 12 子系统列表内）。建议统一为"覆盖约 20%，且 5 个子系统未纳入"。

---

## 第 3 章 五条致命缺口

### F1. `dt` 没有归属层级 —— 内核无法驱动时间步

#### 现状

`dt` 完全由物理量决定，且**每步可重算**（受 `m_dt_update_interval` 控制，`WarpXEvolve.cpp:204`）：

| 计算 | 位置 | 物理依赖 |
|---|---|---|
| `ComputeDt()` | `Source/Evolve/WarpXComputeDt.cpp:51-114` | `m_const_dt`(`:68`)、`m_max_dt`(`:70-76`)、`cfl*minDim(dx)/c`(`:77-103`)、按 `electromagnetic_solver_id` 分派 Yee/CKC/ECT/PSATD |
| `ApplyDtLimiters()` | `WarpXComputeDt.cpp:270-345` | 粒子速度 CFL `mypc->maxParticleDtInv()`(`:276`)、等离子体频率 `omegap_max`(`:277`)、回旋频率 `omegac_max`(`:278`，**读 `Bfield_aux` + `B_external_particle`**)、`m_max_dt`(`:300`) |
| 等离子体频率 | `WarpXComputeDt.cpp:116-122` | `mypc->GetGlobalPlasmaFrequency` |
| 每步重算 | `WarpXEvolve.cpp:204` | `m_dt_update_interval` |

#### 缺口

**路线全文检索结果**：`CFL` → 0 命中；`时间步` → 0 命中；`步长` → 0 命中；`dt` 大小写不敏感检索唯一一次命中是 `Hardcoded` 的子串（`:62`），并非时间步概念。

路线第 4 节角色表给隐式/显式留了 `ITimeIntegrator`（T3，A6+），但那一行只写"显式/隐式"，**从未声明谁拥有 dt 计算权**。

这构成一个二难：

- **dt 归插件** → 内核必须做 `while (t < t_end)` 与 dt 收敛判定，即内核驱动时间步 → **违背不变量 I1「内核不含物理」**
- **dt 归内核** → 内核必须知道粒子速度、等离子体频率、回旋频率 → **同样违背 I1**

#### 失败场景

一个按路线实现 `IPlugin` 的用户，写了个"新激光脉冲"插件（想按自己的脉冲宽度限制 dt）。内核的 `ComputeDt` 在 `WarpXComputeDt.cpp` 里，物理量全在 `WarpX` 成员里。插件无法影响 dt，只能在 `BeforeStep` 之后发现 dt 已经被内核算好并写死。要支持它，内核必须新增一个"插件可覆盖 dt"的钩子——但路线里没有任何这样的接口。

#### 修复方向

设计 `ITimePolicy`（或等价物），把"dt 计算"明确划为插件职责、内核只持有数值：

- 内核持有 `dt` 的**数值**（`TimeState` 已有 `dt` 字段）
- 插件在 `BeforeStep` 声明"我建议下一步 dt 为 X"，并声明其 `reads_species` / `reads_fields`（如 `Bfield_aux`）
- 内核负责：取所有建议值的 min、施加 `m_max_dt` 截断、执行收敛判定、推进时钟
- `HandlerSpec` 需新增 `bool provides_dt` 或独立的 `TimePolicySpec`

这样内核不含物理（CFL 公式在插件里），但仍能驱动时间步。

---

### F2. 隐式求解器把标准阶段整体重入 —— "拓扑序一次性执行"模型失效

#### 现状

`Source/NonlinearSolvers/`（**26 个文件**：Picard / Newton / PETSc SNES+GMRES / 矩阵自由 / 预条件器库）在**路线第 4 节角色表中零出现**（`grep Nonlinear\|Picard\|Newton\|隐式` 只命中 `:239` 的 `ITimeIntegrator` 一行）。

更严重的是语义层面的冲突：

| 机制 | 证据 | 与路线的冲突 |
|---|---|---|
| Picard 循环体**每次迭代重跑"推粒子 + 沉积 + 场推进"** | `FieldSolver/ImplicitSolvers/ImplicitSolver.cpp:1148-1150` 调 `PushParticlesandDeposit`；`:1156-1160` 调 `ApplyBoundaryAndSync` | 🔴 路线：Stage 严格按拓扑序**一次性执行** |
| Newton 的有限差分探针**污染 DataBus 中的场** | `NonlinearSolvers/JacobianFunctionMF.H` | 🔴 路线 DataBus 只有"读写权限 + 访问审计"，**无状态快照/回滚语义** |
| 预条件器类型 = "用哪种场方程组装线性算子" | `NonlinearSolvers/PreconditionerTypes.H`（`CurlCurl`/`Darwin`/`Laplace`/`None`） | 🔴 与 `IFieldSolver` **同一层级**，不能作为平行插件 |
| 隐式路径持有**自己的一份场** | `ImplicitSolver.cpp` 内 `m_E`/`m_B`（`SemiImplicitEM` 局部场） | 🟡 与主 DataBus 的 `field.E/B` 是两套状态，路线未区分"主推进场"与"隐式工作场" |
| 隐式求解器已暴露给 Python | `Source/Python/ImplicitSolver.cpp` | 🟡 若 `ITimeIntegrator` 插件化，这条通路需明确归属 |

#### 失败场景

用户实现一个"新流体耦合模型"，需要隐式迭代求解电场与流体密度的耦合方程。插件的 `on_stage(AfterDeposit)` 只能被调用一次，但隐式求解需要把"推粒子 → 沉积 → 解场"这个组合**重复 N 次**。插件无法让内核重入自己的阶段序列——要么插件自己接管整个时间步（那就等于实现了自己的内核，违背架构），要么这个能力无法实现。

#### 修复方向

两条路，需在路线层面决策：

- **路线 A（推荐）**：承认"隐式求解器是内核的一部分"，其**求解循环归内核**，插件只提供"残差/算子"部件（`IOperator` / `IPreconditioner` / `IResidual`）。这与"内核不含物理"不矛盾——隐式求解器是**数值机制**，不含具体物理方程。同时 DataBus 需要增加**状态快照/回滚**语义（供探针使用）。
- **路线 B**：阶段模型改为**可递归重入**（`on_stage` 可请求重跑本阶段），但这会让拓扑排序的确定性保证复杂化，需要额外定义"重入次数上限"与"重入不改全局状态"的校验。

无论哪条，都必须**先决策再进 A3**。

---

### F3. 14 个标准阶段无法表达真实时间序

#### 现状（已逐行核实）

`Source/Evolve/WarpXEvolve.cpp:661-682` 的 Strang 序列：

```
EvolveF(0.5·dt) → EvolveG(0.5·dt) → FillBoundaryF / FillBoundaryG
  → EvolveB(0.5·dt, FirstHalf)   → FillBoundaryB          // 得到 B^{n+1/2}
  → EvolveE(dt)                  → FillBoundaryE          // 得到 E^{n+1}
  → EvolveF(0.5·dt) → EvolveG(0.5·dt)
  → EvolveB(0.5·dt, SecondHalf)                            // 得到 B^{n+1}
```

**B / F / G 各被写两次**，且每次写后紧跟一次 `FillBoundary`。

#### 缺口

路线第 3.4 节调度器规则：

> 4. 同阶段写同键且无先后 → **abort**

按路线语义，`EvolveB` 的两个半步若挂同一阶段（如 `BeforeFieldSolve`），**第一次写 B 时就会 abort**。而若挂两个不同阶段（`BeforeFieldSolve` + `AfterFieldSolve`），中间的 `EvolveE` 必须发生在两次 `EvolveB` **之间**——扁平阶段列表表达不了"半步序列"。

#### 完整缺口清单

| 真实环节 | 证据 | 14 阶段映射 |
|---|---|---|
| PIC 段（先推粒子后推场） | `WarpXEvolve.cpp:556-699` | 🟢 可映射 |
| **Maxwell-ES 段（先算场）** | `WarpXEvolve.cpp:303-339`（`ComputeSpaceChargeField`/`ComputeMagnetostaticField`/`AddExternalFields`） | 🔴 **是另一种时间序**，线性阶段列表表达不了 |
| **推粒子与沉积在同一循环内交织** | `PushParticlesandDeposit`（`WarpXEvolve.cpp:1503-1616`） | 🔴 `BeforeParticlePush`/`AfterParticlePush` 切不开 |
| **子循环 coarse/fine 交错 i–v** | `OneStep_sub1`（`WarpXEvolve.cpp:1195-1250`） | 🔴 14 阶段无"子循环层"概念，粒度错配 |
| **Strang 半步序列 B½→E→B½** | `WarpXEvolve.cpp:661-682` | 🔴 见上 |
| `ApplyDtLimiters` | `WarpXEvolve.cpp:204` | 🔴 无对应阶段（并见 F1） |
| `HybridPICInitializeRhoJandB` | `WarpXEvolve.cpp:227` | 🔴 无对应阶段 |
| EB 辅助场显式填充 | `WarpXEvolve.cpp:215` | 🟡 无对应阶段 |
| Silver-Mueller 硬编码在 **B 场第一半步** | `BoundaryConditions/WarpXFieldBoundaries.cpp:231-242`（`lev==0` 且 `FirstHalf`） | 🔴 无法作为独立边界 handler |
| 残差重采样 / 镜面 / 热化 / 隐式碰撞 | `WarpXEvolve.cpp:249,252,294,298` | 🟡 无标准阶段 |
| boosted frame 窗口移动 / 晶格 | `WarpXEvolve.cpp:278,284-286` | 🟡 无阶段 |

#### 附带发现：FieldSpec 缺 11 类元数据

路线的 `FieldSpec { ncomp, nodal, ngrow, checkpoint, unit }` 不足以描述真实场：

| # | 缺失项 | 证据 |
|---|---|---|
| 1 | **逐分量 centering / ixType**（`Ex`/`Ey`/`Ez` 各异） | `WarpX.cpp:2522-2530, 2591-2597` |
| 2 | 方向轴 / 是否向量场 | `Fields.H:110-172`；`MultiFabRegister.H:86-92` |
| 3 | 分配 ghost 与交换 ghost 分离（12 类宽度） | `Parallelization/GuardCellManager.H:88-117` |
| 4 | 是否 nodal 平均（`nodal_sync`） | `WarpXComm.cpp:663-694, 718-782` |
| 5 | 是否参与 FillBoundary / 交换；fp/cp 粗网格变体 | `WarpXComm.cpp:724-782`；`Fields.H:62-67` |
| 6 | remake / redistribute_on_remake | `MultiFabRegister.H:268-271` |
| 7 | alias / owner 关系 | `MultiFabRegister.H:277` |
| 8 | initial_value（如 `dual_A_y=1.0`） | `WarpX.cpp:2747` |
| 9 | 分量语义（rho 的 old/new 分量，有效分量在末尾） | `FlushFormatPlotfile.cpp:665-666` |
| 10 | RZ 多模 `ncomps=2*nmodes-1` 实虚交错 | `WarpX.cpp:2575-2580` |
| 11 | 组件级 axis（PSATD 谱空间索引语义） | `FieldSolver/PSATD/SpectralFieldData.H:109-137` |

**第 1 项最致命**：`nodal` 是单值布尔，但 `Ex`/`Ey`/`Ez` 的 centering 各不相同——**连最基础的新场求解器插件都无法写出 `E_x`/`E_y`/`E_z`**。

#### 修复方向

**阶段模型必须从"扁平 14 个"改为"两层嵌套"**：

- **内核层**：固定的**步级**骨架（`BeforeStep` / `Step` / `AfterStep` / `OnRegrid` / `OnCheckpoint` / `OnFinalize`）——这些是真的物理中立、顺序固定的
- **包层**：包可定义**步内序列**（如 `PicEmPackage` 声明 `deposit → B½ → E → B½ → push`），包内序列由 `IPhysicsPackage::declare_stages(StagePlan&)` 给出——路线第 3.6 节**已经提到 `StagePlan` 这个名字，但没有定义它的语义**
- 插件的 handler 挂在**包内子阶段**上，而非直接挂内核阶段

这样 Strang 半步、子循环层、Maxwell-ES 的另一种时间序都有了位置。

`FieldSpec` 则需扩为完整的字段描述（含逐分量 centering 数组、变体标记、remake 策略）。

---

### F4. `SpeciesHandle` 未定义 —— 粒子侧插件能力整体落空

#### 现状

路线第 3.3 节：

```cpp
[[nodiscard]] SpeciesHandle          species (SpeciesKey);           // 不透明句柄
```

`grep -n "SpeciesHandle" 最终的技术路线.md` → **仅 1 处命中，即上面这行**。**全文没有定义它暴露什么**。

而粒子侧最核心的插件能力，全都需要"可写地遍历并改写粒子"：

| 目标插件 | 必需能力 | 源码现状 |
|---|---|---|
| **新碰撞** | 双物种可写 tile 迭代；跨物种复制分量（`SmartCopy.H:132-164`）；产生新粒子（`SmartCreate`）；`defineAllParticleTiles`；`deleteInvalidParticles`；读物性（`getMass`/`getCharge`） | 🔴 全部缺失。现有实现靠 `MultiParticleContainer*` **裸指针**（`Collision/BinaryCollision/BinaryCollision.H:196-218`） |
| **新电离** | 写源物种 `ionizationLevel`（`PhysicalParticleContainer.cpp:1499-1501`）；向产物物种注入粒子；读 E/B aux 场 | 🔴 缺失 |
| **新粒子模型** | 替换 `Evolve`/`PushPX`/`PushP`/`DepositCurrent`/`DepositCharge`/`DepositMassMatrices`（`WarpXParticleContainer.H:235-296, 340-441`）；设置电荷/质量/形状阶/推压器 | 🔴 无替换点 |
| **新沉积形状** | 注册新 shape factor；按物种切换 `nox`/galerkin（`WarpXParticleContainer.H:180-185`） | 🔴 编译期模板（见 F5） |
| **新 QED 路径** | 与 pusher 融合在同一 kernel；跨步粒子状态 `opticalDepthQSR`（`PhysicalParticleContainer.cpp:1536-1539, 1605-1650`） | 🔴 缺失 |
| **相对时间沉积** | 临时改写粒子位置（`MultiParticleContainer.H:170-174`） | 🔴 缺失 |

#### 连带的角色表缺口

路线第 4 节角色表**缺三类高频能力**：

| 缺失角色 | 源码 | 证据 |
|---|---|---|
| **粒子边界条件** | 每物种、每方向、可带解析器（`reflection_model_*`）；EB 边界 `Absorbing`/`Reflecting`/`Thermal` | `Particles/ParticleBoundaries.H:41-93`；`PhysicalParticleContainer.cpp:354-375`；`MultiParticleContainer.cpp:1250-1273` |
| **粒子注入** | 7 种 `injection_style` + `injection_sources`，**每物种一个注入器向量** | `PhysicalParticleContainer.cpp:126-139`；`PhysicalParticleContainer.H:456` |
| **粒子分裂** | `particlePostLocate` 改 `id()`；依赖 `plasma_injectors[0]->num_particles_per_cell_each_dim` | `WarpXParticleContainer.cpp:2945-2954`；`PhysicalParticleContainer.cpp:1023-1054` |

粒子边界与注入是**粒子侧最常用的输入面**。路线把它们塞进 `SpeciesHandle` 会违反 I2（插件只声明、不持有）；留作内核则用户无法替换。

此外 `HandlerSpec`（路线 `:159-167`）**没有"本物种在 level 边界分裂"的声明位**，而 I3 要求"生命周期归内核、插件只声明"——这条声明位缺失。

#### 附带发现：per-species 命名字段无作用域模型

`FieldKey` 是扁平 `std::string`，但真实场带物种作用域：

- `current_fp_<spec>` / `rho_fp_<spec>` / `Vs_fp_<spec>`（`FieldSolver/HybridPICModel.cpp:323-336`）
- `variance_buffer_w` / `w2` / `vbar_<spec>`（`Particles/Deposition/VarianceAccumulationBuffer.cpp:39-41, 69`）

路线 schema 没有 species→field 作用域概念。

#### 修复方向

**必须显式定义 `SpeciesHandle`**，至少给出这些能力：

```cpp
class SpeciesHandle {
public:
    // 遍历：可写的 tile 迭代
    template <typename F> void for_each_writeable_tile(int lev, F&&);
    // 跨物种复制分量
    void copy_components_from(SpeciesHandle& src, const std::vector<std::string>& comps);
    // 产生新粒子
    void create_particles(int lev, const ParticleCreateSpec&);
    // 属性
    void set_component(int lev, int comp, const std::string& name, amrex::ParticleReal v);
    // 物性
    amrex::ParticleReal mass() const;  amrex::ParticleReal charge() const;
};
```

并**新增角色**：`IParticleBoundary`、`IInjector`；`HandlerSpec` 增加物种级元数据（分裂声明、边界声明、注入声明）。

---

### F5. 新沉积形状是编译期模板 —— 与"运行期插件"根本冲突

#### 现状

`Source/Particles/ShapeFactors.H:27-84` 用 `if constexpr` 展开 `nox = 0..4`，`Compute_shifted_shape_factor` 同理。运行期 `nox` 在 `PhysicalParticleContainer.cpp:2034-2061` 分发到**模板实例**。

形状因子参与 **GPU kernel 内部的插值计算**（`Compute_shifted_shape_factor` 在 deposition kernel 内被调用）。

#### 缺口

路线第 3.5 节：

> 加载：编译期静态注册为主；运行期 `.so` 可选，**只允许 host 侧 handler**

路线第 8 节"不做"：

> 不做"运行期替换 GPU kernel"。

但新形状因子**必须改模板**，才能进 GPU kernel。这两条合起来意味着：**新形状因子在契约下不可能实现**。

且 `nox` 是**全局**而非按物种 → 用户想要的"电子用二阶、离子用一阶"也无法表达。

#### 失败场景

用户想给电子用二阶形状、离子用一阶形状以减少数值色散。按路线：`IDeposition` 是 T2 插件，运行期 `.so` 只允许 host handler——但形状因子在 GPU kernel 里 → 做不了。改成编译期插件 → 路线没有这一档。

#### 修复方向

路线第 8 节"不做"清单需改为**分层表述**：

> 不做"运行期替换**既有** GPU kernel"；
> **新增**进 kernel 的算法能力（形状因子、PML 组件、场算子）用**编译期插件**（头文件级注册 + 模板实例化），运行期 `.so` 仅支持 host 侧 handler。

这要求路线引入**两档插件**概念：
- **编译期插件**：`.hpp` + 模板参数化，编译进二进制，可进 GPU kernel
- **运行期插件**：`.so`，host 侧 handler，通过 DataBus 交换数据

这不是"多写一点文档"，而是插件契约的**根本分档**，必须在 A3 之前决定。

---

## 第 4 章 八条结构性缺口

### S1. BTD（回溯变换诊断）无处安放 🟠

**现状**：
- BTD 在 `OneStep_nosub` 内**第二次**独立 flush：`WarpXEvolve.cpp:273`（`FilterComputePackFlush(step, false, true)` 走 BackTransform 分支，`MultiDiagnostics.cpp:89-104`）
- 需要跨步 boosted-frame 历史：每步把场存进 `m_mf_cc`（`Diagnostics/BTDiagnostics.cpp:531`）、粒子存 `*_n_btd` 属性（`MultiParticleContainer.cpp:1136-1144`）、维护 `m_buffer_k_index_hi`/`m_t_lab`/`m_flush_count`
- 逆变换直接依赖 `WarpX::GetInstance().GetPartContainer()` 与 `warpx.m_fields.get(...)`（`Diagnostics/BackTransformParticleFunctor.cpp:60-95`）

**缺口**：路线第 3.4 节的 `BeforeDiagnostics` 只有**一个**挂点，无法表达 BTD 的独立 flush 时点；`BTD`/`BackTransform`/`Lorentz`/`boost` 在路线全文 **0 命中**。

**后果**：BTD 是 WarpX 的招牌能力（激光驱动研究的主力诊断），A3 拆包时无处安放。

**修复方向**：增加独立的 `AfterStepBackTransform` 阶段 + 跨步状态通道（诊断私有状态需在 DataBus 之外有合法位置，或扩展 `ScalarKey` 为"诊断私有状态存储"）。

---

### S2. 插件无法自行注册任何诊断 🟠

**现状**：

| 注册路径 | 机制 | 证据 |
|---|---|---|
| Reduced Diags | 硬编码 `std::map<std::string, std::function<...>>`，21 个 `if (name==...)` 式工厂 | `Diagnostics/ReducedDiags/MultiReducedDiags.cpp:56-100`（已核实原文） |
| Full / TimeAveraged / BTD / BoundaryScraping | 内核枚举白名单**硬断言** | `Diagnostics/MultiDiagnostics.cpp:68-74` |
| 配置文件解析 | 逐个 `if (name==...)` 构造，含 `type` 键 + 每类各自额外参数 | `MultiReducedDiags.cpp:120-140` |

**缺口**：两处都是**内核代码**。不改造则"插件新增诊断"根本不成立——这与 A3 把 `IDiagnostic` 定位为 T1 插件基类的意图直接矛盾。

**修复方向**：诊断注册必须从"硬编码 map"改为"插件在 `on_schema` 阶段注册诊断描述符"。这要求 `IDiagnostic` 的元数据（名字、类型、参数键、依赖的字段键）成为**声明式数据**。

---

### S3. "schema 驱动 IO / 诊断自动生成"当前源码不支持 🟠

**路线声称**（第 3.3 节）：

> 单位/分量在 schema 里，诊断/IO 自动生成。

**源码现状**：

| IO 路径 | 现状 | 证据 |
|---|---|---|
| Full Diag 变量名 | **硬编码 if/else 链**（12 个 FieldType 分支） | `Diagnostics/FullDiagnostics/FullDiagnostics.cpp:459-620` |
| plotfile | **写死 12 个 FieldType** | `Diagnostics/FlushFormats/FlushFormatPlotfile.cpp:463-560` |
| openPMD 单位 | **按 field_name 首字符猜**（`E`/`B`/`j`/`rho`） | `Diagnostics/WarpXOpenPMD.cpp:348-375` |
| openPMD 组件名 | **白名单** `vector_fields` | `WarpXOpenPMD.cpp:1391-1420` |
| checkpoint 写侧 | **逐字段硬编码** `VisMF::Write` | `FlushFormatCheckpoint.cpp:83-188` |
| checkpoint 读侧 | **`getline` 逐行解析**（无标签） | `Diagnostics/WarpXIO.cpp:110-165` |

**好消息**（修正路线的一处悲观表述）：`MultiFabRegister` **已经**有字符串 API（`get(std::string)`/`has(std::string)`，`MultiFabRegister.H:461-490`）和 `checkpoint_restart` 开关（`MultiFabRegister.H:274`，默认 `false`）。所以"自定义场默认不持久化"**属实**，但**修复路径比路线设想的短**：复用现有注册表即可，不必新造 schema 机制。

**后果**：A4 的工作量被低估——"自动生成"是**尚未存在的能力**，不是"重构现有机制"。

---

### S4. PML 是并行 Level 资源，非"场求解器内部" 🟠

**路线声称**（第 4 节末）：

> **PML/Silver-Mueller/Damped 归场求解器内部**；PEC/PMC/绝缘体/轴归 `IFieldBoundary`。EB 的 `EB2::Build`/工厂生命周期归内核，cut-cell 算法归插件。

**源码现状**（已核实 `WarpXComm.cpp:747-765` 原文）：

```cpp
// Exchange data between valid domain and PML
// Fill guard cells in PML
if (do_pml) {
    if (pml[lev] && pml[lev]->ok()) {
        const std::array<amrex::MultiFab*,3> mf_pml = ...;
        pml[lev]->Exchange(mf_pml, mf, patch_type, do_pml_in_domain);
        pml[lev]->FillBoundary(mf_pml, patch_type, nodal_sync);
    }
    ...
}
// Fill guard cells in valid domain      ← 普通场 FillBoundary 在这之后
```

即：**PML 的 Exchange + FillBoundary 先于普通场 FillBoundary 执行**。PML 还拥有：

- **自己的 DM 与 FieldFactory**（`BoundaryConditions/PML.H:141-157` 构造参数含 `ba`/`dm`/`do_similar_dm_pml`）
- **自己的 10 个 FieldType 场键**：`pml_E_fp`/`pml_B_fp`/`pml_j_fp`/`pml_F_fp`/`pml_G_fp` 及 `_cp` 版本（`Fields.H:79-88`）
- **自己的 CheckPoint / Restart**（`PML.H:192-215`）
- **分裂域拷贝与回写**：`CopyJtoPMLs` / 每步 `Exchange` 回主域

**后果**：PML 是**与场求解器平行的 Level 级资源**，不是求解器内部。路线的"T0 不可插件化"清单里没有它，而"PML 归场求解器内部"这句表述会让分裂域拷贝/回写无处安放。

**另外两处相关事实**：
- PML sigma 是**各向同性单标量**（`PMLComponent.H:49-84`，只存一个 `sigma` + 每方向 `fac`）——任务描述里提到的"各向异性 PML"在当前实现中**不存在**
- PML 有 `do_pml_Lo`/`do_pml_Hi` 每方向开关 + `do_pml_in_domain`（`PML.H:156-157`），路线未提

---

### S5. Filter 的 stencil 宽度反向决定 guard cell 数 🟡

**现状**（已核实 `GuardCellManager.cpp:365-392` 原文）：

```cpp
IntVect ng_NCIFilter = IntVect::TheZeroVector();
if (do_fdtd_nci_corr) {
    ng_NCIFilter[WARPX_ZINDEX] = NCIGodfreyFilter::m_stencil_width;
}
ng_FieldGather = ng_FieldGather_noNCI + ng_NCIFilter;      // NCI filter
ng_UpdateAux    = 2*ng_FieldGather_noNCI + ng_NCIFilter;    // aux grid
```

即：**插件（滤波器）的 stencil 宽度反向决定了内核的 guard cell 分配**。这是一条**内核→插件→内核**的反向依赖。

同时滤波在 SumBoundary 之前执行（`WarpXComm.cpp:1453-1480, 1661-1685` 的 `ApplyFilterJ`/`ApplyFilterMF`），独立成插件会打断内核 SumBoundary 的正确性。

**附带修正**：路线角色表把 `IFilter` 归 T2，但源码里的 Filter **不是"激光包络滤波"**，而是 **J/rho 的双线性滤波 + NCI Godfrey 滤波**（`Source/Filter/Filter.H` 的 `BilinearFilter`、`NCIGodfreyFilter.H`）。

---

### S6. `FieldBoundaryType` 是内核静态枚举 🟡

**现状**：`Source/Utils/WarpXAlgorithmSelection.H:125-140` 定义 `FieldBoundaryType`（含 `Damped`/`Open`/`PEC_Insulator`/`Neumann=PMC`），被 `WarpX.H:242-247` 的全局静态数组 `field_boundary_lo`/`field_boundary_hi` 持有。

**缺口**：`IFieldBoundary` 插件必须**回写**这个内核枚举 → 违反 I2「插件不互认，只通过 DataBus」。

**附加约束**（路线未提）：`Damped` **仅 PSATD 支持**（`WarpX.cpp:1970-1972`），且**必须 lo/hi 同时为 Damped**（`WarpX.cpp:1936-1949`）。

**附带修正**：路线角色表把 `IFilter` 归 T2，但源码里的 Filter **不是"激光包络滤波"**，而是 **J/rho 的双线性滤波 + NCI Godfrey 滤波**（`Source/Filter/Filter.H` 的 `BilinearFilter`、`NCIGodfreyFilter.H`）。

---

### S7. Python 兼容面远大于路线承认的范围 🟠

**现状**：

| 耦合点 | 机制 | 证据 |
|---|---|---|
| 单例直连 | pywarpx 通过 `libwarpx.libwarpx_so.get_instance()` 访问 C++ 内部；`warpx.multifab_register()` 直接返回 `MultiFabRegister&` | `Python/pywarpx/fields.py:212-213, 221-222` |
| **字段名命名空间冲突** | Python 侧字段名是 `Efield_fp`/`Efield_aux`/`current_fp`；路线 `FieldKey` 举例是 `E_x`/`rho` | `Python/pywarpx/fields.py:264-296`；`Fields.H:26-108` |
| 旁路写场 | Python 绑定直接暴露 `MultiFabRegister::alloc_init` | `Source/Python/MultiFabRegister.cpp`；`Python/pywarpx/fields.py:220-240` |
| PICMI 全局参数 | 通过**全局参数字典**驱动 ParmParse（与"Runtime 拥有输入解析"直接冲突：参数从哪来、谁校验、能否在 `initialize()` 前改） | `Python/pywarpx/picmi.py:3927-3933`；`Python/pywarpx/WarpX.py` 的 `attrlist` 机制 |
| 隐式求解器 | 已暴露给 Python | `Source/Python/ImplicitSolver.cpp` |

**缺口**：路线第 7 节验收清单（7 条）**无任何 Python 条目**。A3「WarpX 降级为包」会同时打断这三层。

**附带修正**：路线称 Python 钩子"46 处 / 22 个名字"，实测 C++ 侧 **44 处 / 21 名**；Python 侧另注册 **25 名**，其中 `afterrestart`、`appliedfields` **全仓库无触发点**（死钩子），`afterinit`/`particleloader` 只由 Python 侧 `execute_python_callback` 触发。

**且 14 阶段覆盖不了 Python 钩子**——至少 12 个落在阶段之间：

| 钩子 | 位置 |
|---|---|
| `afterBpush` / `afterEpush` | `FieldSolver/WarpXPushFieldsEM.cpp:957, 1012`（场推中间） |
| `beforedeposition` / `afterdeposition` | `FieldSolver/ES/LabFrameExplicitES.cpp:42,48`、`EffectivePotentialES.cpp:63,68`（ES 求解器内部） |
| `poissonsolver` | `EffectivePotentialES.cpp:71` |
| `afterInitEsolve` / `afterInitatRestart` | `Initialization/WarpXInitData.cpp:920, 928` |
| `loadExternalFields` | `WarpXInitData.cpp:1819` |
| `allocdata` | `Diagnostics/WarpXIO.cpp:224`、`WarpXInitData.cpp:1050` |
| `particlescraper` | `WarpXEvolve.cpp:772` |
| `onbreaksignal` / `oncheckpointsignal` | `WarpXEvolve.cpp:410, 1686`（**信号处理器**） |

**修复方向**：需定义 Python 回调的**插入点模型**（哪些是阶段内、哪些是阶段间专用），并把"旧 pywarpx / PICMI 用例全过"加进第 7 节验收清单。字段命名需**双名兼容**（保留 `Efield_fp` 等旧名，或提供映射表）。

---

### S8. `ablastr` 是"物理 + 基础设施混居" 🟡

**现状**：

| 文件 | 性质 | 问题 |
|---|---|---|
| `ablastr/fields/PoissonSolver.H`、`EffectivePotentialPoissonSolver.H`、`IntegratedGreenFunctionSolver.H` | **物理**（Poisson/IGF 方程求解） | 与 I1「内核不含物理」直接冲突——它们在共享层 |
| `ablastr/constant.H` | **物理常量**（c、ε0、μ0） | 同上 |
| `ablastr/particles/DepositCharge.H`、`NodalFieldGather.H` | PIC 实现 | 不该被插件共享 |
| `ablastr/particles/ParticleMoments.H` | 只算 **4 阶矩（10 个量）** | 与 MomentsSolver 要的 **10 矩（45 量）不兼容** |
| `ablastr/coarsen/{average,sample}.H` | 纯 AMReX 内核工具 | 插件也要用 → 需下沉内核；路线 `Services` 清单**未列 coarsen/interpolate** |
| `ablastr/utils/{Communication,Serialization,TextMsg,timer}` | 纯内核服务 | 路线 `Services` 清单**未列 TextMsg / MPIInit** |
| `ablastr/warn_manager`、`profiler` | 纯内核 | 🟢 归内核正确 |

**后果**：ablastr 现状不是"可直接复用的纯抽象层"。插件化会把它**撕裂**：物理部分随包下沉，基础设施部分留内核，两者之间需要一层新边界。路线第 3.1 节的内核清单没有意识到这件事。

---

### 附：次级缺口（需在 A0 一并记账，不单列编号）

| # | 缺口 | 证据 |
|---|---|---|
| N1 | **RNG 只有全局一条流且不入 checkpoint**：`set_random_seed` 只在初始化调一次（`WarpX.cpp:735-737`），粒子 checkpoint 只存 SoA 分量。调度器改为拓扑排序后，**增删任一插件都会移动全局随机流** → 违反 I5 逐位门；且"重启可复现"从"续流"退化为"重开流"，碰撞/电离/热化/重采样的统计结果在 restart 前后不可比 | `WarpX.cpp:735-737`；`Particles/Physics/ParticleIO.cpp:153-221` |
| N2 | **每 handler 自己的时间步进**（`ndt_supercycle`/`ndt_subcycle`/`start_step`）不在 `HandlerSpec` 字段内 | `Particles/Collision/CollisionBase.cpp:26-61`；`CollisionHandler.cpp:165-187` |
| N3 | **FieldSpec 缺 fp/aux/buf 变体维度**：真实场有 `Efield_fp/aux/avg_fp/external`；电离用的是 `Efield_aux` 而非 fp | `WarpX.cpp:2595, 2944, 2953, 3020`；`MultiParticleContainer.cpp:1470-1478` |
| N4 | **`PCTypes` if/else 硬编码**（`MultiParticleContainer.cpp:107-121`），新粒子容器类型需改内核 | 已核实 |
| N5 | **BTD 需要每物种运行时分量 `prev_x/y/z`**，push 时复制 x/u | `PhysicalParticleContainer.cpp:340-344`；`ImplicitPushPX.cpp:535` |
| N6 | **相对时间沉积会临时改写粒子位置**，不透明句柄无法表达 | `MultiParticleContainer.H:170-174` |
| N7 | **QED 光学深度是跨步粒子状态**（`opticalDepthQSR`），融合在 pusher kernel 内 | `PhysicalParticleContainer.cpp:1536-1539, 1605-1650` |
| N8 | **`MomentsSolver` 是与 WarpX 完全平行的独立仿真器**：不进 `Source/CMakeLists.txt`（独立 `add_subdirectory`），`Real` 是裸 `double` 裸数组，自带 BC 枚举/RK 积分/FVND driver，与 WarpX 的 level/AMR/MPI 无共享。路线把它写成 `PicEmPackage` 注册的 `moment5` 插件，在**数据模型层就不成立** | `Source/MomentsSolver/Tests/CMakeLists.txt`；`MomentsSolver/Equations/MomentsConfig.H`；`MomentsSolver/FV/MomentFVNDDriver.H` |
| N9 | **10 矩/Euler/Maxwell-Eqn 是"方程族"而非"模型"**：每个方程自带 `Update`/`Conserved`/`Flux`，closure 是每方程的自由函数且依赖 EOS（需 `γ`、`q` 参数）。`IFluidModel` + `ISource` 无法容纳"一套方程 + 一套 closure + 一套 EOS"的成套依赖 | `MomentsSolver/Equations/{MomentEquation,TenMomentEqn,MaxwellEqn,Euler5Eqn,EquationOfState,TenMomentClosure}.H` |
| N10 | **Friction 是 Moments↔PIC 双向耦合**：同时读 PIC 粒子速度/密度与矩流体状态。路线把 `IInteraction`（摩擦/引力）放 T2，意味着它不能读 PIC 粒子状态（T1 的数据）→ 违反 I2 | `MomentsSolver/Coupling/FluidFriction.H` |
| N11 | **`ExternalField` 与 `AcceleratorLattice` 是两套东西**：`ExternalField` 是 E/B 场（3 种来源：`constant`/`read_from_file`/`load_from_python`），`AcceleratorLattice` 给**粒子**加外加速度 | `Initialization/ExternalField.H:28-120`；`AcceleratorLattice/AcceleratorLattice.H` |
| N12 | **`InitData` 物理成分很重**：粒子注入、外场注入、EB 场初始化、激光注入。`PicEmPackage` 角色表里**没有"外场源/激光注入/EB 几何初始化"角色** | `Initialization/WarpXInitData.cpp:799, 1034, 1257`；`:1169, 1171` |
| N13 | **碰撞的三个"前置全局动作"**不在任何标准阶段内：RZ 下 `TransformMomentumToCurvilinear`、虚光子生成、全局 Debye 长度计算 | `CollisionHandler.cpp:147, 155, 162` |
| N14 | **碰撞跨 handler 的动量守恒无原子边界**：`HandlerSpec` 的写权限是"按键"而非"按键 + 事务"，同阶段写同键即 abort，但碰撞同时改写 A、B 两物种动量并可能创建 C | `BinaryCollision/BinaryCollision.h:196-218`；路线 `:175` |
| N15 | **虚光子每步覆写整个物种**，与 `OnInitialize` 的"一次性初始化"语义冲突 | `Collision/BinaryCollision/VirtualPhotonCreation.cpp` |
| N16 | **Reduced Diag 的 ASCII 输出路径全局共享**（`m_path`+`m_extension`+`m_sep`），多插件同名会互相覆盖 | `ReducedDiags/ReducedDiags.cpp:145-165` |
| N17 | **A1 现有 facade 不承重**：`Services` **无任何实现类**（全仓库 `public Services` 0 命中）；`Runtime.cpp:49` 传 `nullptr`；facade 自身 5 处 `GetInstance()`；65 个 static 中 21 个无归属 | `Source/Runtime/Runtime.cpp:24, 45, 49, 66, 77, 92` |
| N18 | **A2 缺"哪些 Level 级对象存在"的全集清单**：`ClearLevel` 仅在 `WarpX.cpp:559` 被调用（唯一路径是重网格） | `WarpX.cpp:559` |

---

## 第 5 章 路线内部硬门矛盾

### 5.1 矛盾陈述

> **路线第 7 节验收 1**："**同一二进制 A/B 逐位**：默认路径 vs 包路径；探针 ASCII + plotfile payload 字节。"
>
> **路线 A4 验收**："checkpoint schema 化（兼容旧格式）"。

这两条**直接冲突**。

### 5.2 根因：checkpoint 字节序依赖字段写出顺序

| 事实 | 证据 |
|---|---|
| `WarpXHeader` 是**纯行序、无标签** | 写 `Diagnostics/FlushFormats/FlushFormatPlotfile.cpp:248-358`；读 `Diagnostics/WarpXIO.cpp:121-176` 用 `getline` 逐行 |
| `MultiFabRegister` 按 `std::map` **名字字典序**写出 | `ablastr/fields/MultiFabRegister.cpp:281, 309, 336, 356` 遍历 `m_mf_register`（`MultiFabRegister.H:996` 声明为 `std::map`） |
| → **新增一个字段名就改变字节序** | A4 schema 化的必然结果 |

### 5.3 其余顺序敏感操作（路线 R1–R6 未提）

| 操作 | 证据 | 破坏方式 |
|---|---|---|
| 沉积用 `amrex::For` + `amrex::Gpu::Atomic::AddNoRet` | `Particles/Physics/CurrentDeposition.H` | GPU 端浮点加顺序不确定 |
| `amrex::Reduce` + `ReduceOpMax` 求 dt 限制 | `WarpXParticleContainer.cpp:2705-2745`；`WarpXComputeDt.cpp:124+` | 归约树/线程数变化 → dt 微变 → **全轨迹发散** |
| 粒子按 bin 排序 | `WarpXEvolve.cpp:861` `SortParticlesByBin` | 排序键/稳定性变化 → 浮点求和序变化 |
| `InitData` 内 MultiFab 分配顺序 | `WarpX.cpp:2472-3226`（152 处 FieldType） | 改分配序 → map 序、守卫单元分配序、随机流序全变 |
| 全局 RNG 流 | `WarpX.cpp:735-737` 只在初始化播种一次 | 插件增删移动随机流 |

路线风险表 R1–R6 中，R4 只写"调度器不确定性 / 拓扑排序 / 写冲突 abort"，**未提 checkpoint 行序、map 名字序、原子加顺序、归约顺序、粒子排序、全局随机流**这 6 条。

### 5.4 建议（需路线层面决策，不能留给实现阶段）

**方案一（推荐）**：把逐位门**限定条件化**——
- "默认路径 vs 旧基线，**在字段集不变时**逐位一致"
- "字段集变化后"改用 **≤1e-12 数值门 + 显式白名单**（路线第 7 节验收 2 已有此模式，可复用）

**方案二**：在 A4 里**先冻结 checkpoint 字节序**——
- 写侧保持旧序（`WarpXHeader` 行序不变、`MultiFabRegister` 改为按"旧字段优先 + 新字段追加"写出，而非 map 字典序）
- schema 化推迟到 A4 之后（单独一阶段）

**无论哪种，都必须在开工前决策**，因为它影响 A4 的整个实现策略。

---

## 第 6 章 用户能力覆盖矩阵

路线第 1.1 节列的 8 项用户能力，逐条对照插件契约：

| # | 用户能力 | 可实现？ | 阻断原因 |
|---|---|---|---|
| 1 | 新场方程/场求解器（Maxwell+引力、新 stencil） | 🔴 **否** | F3（Strang 序列 + 扁平阶段）+ FieldSpec 缺逐分量 ixType（第 1 项） |
| 2 | 新物质模型（流体+引力、新 PIC 模型） | 🔴 **否** | F4（SpeciesHandle 未定义）+ F5（形状模板） |
| 3 | 新材料（ε/σ/μ、相变、损耗） | 🟡 部分 | `IMaterialModel` 方向对，但缺"材料状态随时间演化"的通道（材料状态 = 标量/场，跨阶段一致性未定） |
| 4 | 新边界（PEC/PMC/自定义壁） | 🟡 部分 | S6（须回写内核枚举）+ Silver-Mueller 硬编码在 B 场半步（F3） |
| 5 | 新源项（外场、电流源、化学反应源） | 🟡 部分 | 缺 `IInjector`（F4）；`ExternalField` 与 `AcceleratorLattice` 语义未分（N11） |
| 6 | 新诊断 | 🔴 **否** | S2（注册表硬编码）+ S3（变量名硬编码） |
| 7 | 新碰撞 | 🔴 **否** | F4（SpeciesHandle）+ 碰撞需插在动量推动**内部**（`WarpXEvolve.cpp:447-473`） |
| 8 | 新 AMR 策略（tagging/负载均衡） | 🟡 部分 | A6 是**从零实现**（`WarpX.cpp:2364` abort），非"补机制" |

**汇总**：8 项中 **4 项在当前契约下不可能实现**（新场求解器、新物质模型、新诊断、新碰撞），4 项部分可行。

路线第 5 节列的 5 个"证明互不认知也能协作"的场景，同样受影响：

| 场景 | 判定 |
|---|---|
| 介质内部场（`medium_tensor` + `yee`） | 🟡 依赖 F3（场求解器阶段） |
| 介质气化（材料状态 + `vaporization` + 气体物种） | 🔴 依赖 F4（`SpeciesHandle` 无写接口） |
| 化学反应（`chemistry`） | 🔴 依赖 F4 + N14（无原子边界） |
| 自适应网格（内核 `regrid()` + `IAmrPolicy`） | 🟡 依赖 A6 从零实现 |
| 新边界（`IBoundary`） | 🟡 依赖 S6 |

---

## 第 7 章 契约 v2 修订建议

以下 6 项是**进入 A3 之前必须落地**的契约修订。不改方向，只补齐表达能力。

### 7.1 新增 `ITimePolicy`（解 F1）

```cpp
struct TimePolicySpec {
    std::string name;
    std::vector<SpeciesKey> reads_species;   // e.g. 速度 CFL 需要
    std::vector<FieldKey>   reads_fields;     // e.g. 回旋频率需要 B
    bool provides_dt = true;
};
```

- 内核持有 `dt` 数值（`TimeState.dt`）
- 插件在 `BeforeStep` 返回建议 dt，内核取 min + 施加 `m_max_dt` + 收敛判定
- 现有 `ComputeDt` / `ApplyDtLimiters` 拆为：物理部分 → 内置 `CflTimePolicy` 插件；截断与推进 → 内核

### 7.2 阶段模型改为两层嵌套（解 F3）

```cpp
// 内核层：真正物理中立、顺序固定
enum class KernelStage { OnSchema, OnInitialize, BeforeStep, Step, AfterStep,
                         OnRegrid, OnCheckpoint, OnFinalize };

// 包层：步内序列，由包声明
class StagePlan {
public:
    void add_substage(std::string name, int position);   // 包内有序
    void bind(std::string substage, std::string handler_name);
};
```

- `PicEmPackage::declare_stages(StagePlan&)` 声明 `deposit → B½ → E → B½ → push`
- 插件 handler 挂在**子阶段**上
- 调度器做两遍：先按包顺序展开子阶段，再在子阶段内做数据依赖拓扑排序
- 子循环（`OneStep_sub1`）作为**独立的递归层**，由内核在 `Step` 内驱动

### 7.3 显式定义 `SpeciesHandle`（解 F4）

见 F4 的接口草案。最低要求：可写 tile 迭代、跨物种复制、产生粒子、属性读写、物性查询。

### 7.4 新增角色 `IParticleBoundary` / `IInjector`（解 F4 连带）

```cpp
class IParticleBoundary {
public:
    virtual BoundaryAction apply(const ParticleBoundaryContext&) = 0;  // Absorb/Reflect/Thermal
};
class IInjector {
public:
    virtual void inject(int lev, InjectionContext&) = 0;   // 7 种 injection_style
};
```

并在 `HandlerSpec` 增加物种级元数据：分裂声明、边界声明、注入声明。

### 7.5 插件分两档（解 F5）

| 档 | 形式 | 可进 GPU kernel | 用途 |
|---|---|---|---|
| **编译期插件** | `.hpp` + 模板参数化，编译进二进制 | ✅ | 形状因子、PML 组件、场算子、gather |
| **运行期插件** | `.so`，host 侧 handler | ❌ | 化学、气化、新材料、新诊断、新边界 |

路线第 8 节"不做"清单相应改为：

> 不做"运行期替换**既有** GPU kernel"；**新增**进 kernel 的算法能力用编译期插件。

### 7.6 补齐 `FieldSpec` 与 IO schema（解 F3/S3）

`FieldSpec` 扩为完整描述（逐分量 centering 数组、变体标记 fp/aux/buf、是否参与 FillBoundary、remake 策略、component direction、initial_value、RZ 多模布局、是否持久化）。

同时把诊断注册从硬编码 map 改为声明式（解 S2），并把 plotfile/openPMD 的变量名与单位来源改为 schema 驱动（解 S3）——后者可**复用 `MultiFabRegister` 已有的字符串 API**，工作量比路线设想的小。

### 7.7 补齐 IO 与 Python 验收（解 S7 / C1）

- 第 7 节验收清单新增：**"旧 pywarpx / PICMI 用例全过"**
- 字段命名**双名兼容**：保留 `Efield_fp` 等旧名，或提供显式映射表
- 定义 Python 回调的插入点模型（阶段内 vs 阶段间专用）
- 决策并落地第 5 章的 checkpoint 字节序方案

---

## 第 8 章 阶段计划与工作量修正

### 8.1 各阶段缺口影响

| 阶段 | 路线估时 | 受影响缺口 | 修正估时 | 修正理由 |
|---|---:|---|---:|---|
| A0 审计 | 2–4 周 | S1–S8、N1–N18 | **4–6 周** | 需扩到 12+ 子系统（`NonlinearSolvers`/`MomentsSolver`/`Filter`/`AcceleratorLattice`/BTD 均不在现列表内）+ 契约 v2 调研 |
| A1 去单例 | 2–3 月 | N17 | **2–3 月** | 数字基本对（240 处 / 75 文件）；但 `Services` 需从零实现，且 21 个 static 需新增归属 |
| A2 Level 资源 | 2–3 月 | 第 2 章 #2 | **4–6 月** | 硬编码实为 **152 处 / 48 枚举**（不是 52 处）；`ClearLevel` 漏清项增至 8 处 |
| A3 生命周期拆包 | 3–6 月 | F1–F5、S1–S8 | **前置契约 v2（+3–5 月），随后 4–6 月** | 当前契约下会硬拆出错误结构 |
| A4 状态 schema | 3–6 月 | S3、C1 | **4–6 月** | "自动生成"是尚不存在的能力；须先决策 checkpoint 字节序 |
| A5 调度器 | 3–6 月 | F2、F3、7.2 | **4–6 月** | 需实现嵌套阶段 + 隐式重入策略 |
| A6 动态 regrid | 3–6 月 | 第 8 章 A6 说明 | **6–9 月** | `MakeNewLevelFromCoarse` 与 `RemakeLevel` 非等 BA 分支**都是 abort**，是从零实现 |

### 8.2 总账

| 项 | 路线 | 修正 | 说明 |
|---|---:|---:|---|
| 路线原估（A0–A6） | 16–30 人月 | 16–30 人月 | |
| + 契约 v2（F1/F3/F4/F5 + 7.2–7.6） | — | **+7–13 人月** | 接口层重设计，非加字段 |
| + 隐式重入决策（F2） | — | **+3–6 人月** | 需路线层面选路 + 快照语义 |
| + S1–S8 补缺 | — | **+4–8 人月** | BTD、诊断注册、IO schema、PML、Filter、FieldBoundaryType、Python、ablastr 边界 |
| + A2 修正 / A6 上调 | — | **+3–6 人月** | 见上表 |
| **合计** | **16–30 人月** | **约 33–63 人月** | 取整 **30–60 人月** |

**这个数字必须写进路线**，否则 A3 开工时会发现契约不成立。

---

## 第 9 章 行动清单

### 9.1 立即（不写代码）

| # | 动作 | 产出 | 依赖 |
|---|---|---|---|
| A-1 | 补写 **契约 v2** 章节（7.1–7.7）进 `最终的技术路线.md` | 路线文档新增一节 | — |
| A-2 | 决策 **隐式求解器归属**（F2 路线 A/B 二选一） | 决策记录 | — |
| A-3 | 决策 **checkpoint 字节序方案**（第 5 章方案一/二） | 决策记录 | — |
| A-4 | 修正路线文档 **8 处数字**（第 2 章） | 路线文档修订 | — |
| A-5 | 统一"A0 完成度"表述（15% / 20% / 5-5 三者矛盾） | 路线文档修订 | — |
| A-6 | 把本报告的缺口清单链接进路线文档 §8 风险表 | 路线文档修订 | A-1..A-5 |

### 9.2 A0 阶段补充（扩范围）

| # | 动作 |
|---|---|
| B-1 | 把 `NonlinearSolvers` / `MomentsSolver` / `Filter` / `AcceleratorLattice` / BTD 纳入 12 子系统清单（扩到 17+） |
| B-2 | 补齐 N1–N18 全部次级缺口的归属判定（内核 / 包 / 插件） |
| B-3 | 产出 **FieldSpec v2 完整元数据表**（第 4 章 11 项 + 变体维度） |
| B-4 | 产出 **SpeciesHandle v2 接口草案**（第 3 章 F4） |
| B-5 | 产出 **Level 级对象全集清单**（N18），作为 A2 的验收基线 |

### 9.3 可以立即继续做的工作

| # | 动作 | 为什么不受缺口影响 |
|---|---|---|
| C-1 | **A1.4 Batch C6**（剩余 76 处 `ReducedDiags` 去单例） | 去单例是任何路线的必要前置；且这批已完成一半（19 处残留） |
| C-2 | A1.5 `FullDiagnostics` / A1.6–A1.7 场求解器与粒子配置 | 同上 |
| C-3 | A1.8 移除 `WarpX::m_instance` | 同上 |
| C-4 | `Services` 接口的**首个实现**（N17） | 无论契约 v2 怎么定，Services 都是需要的 |

### 9.4 明确不要做的

- ❌ **在契约 v2 落地前启动 A3**（生命周期拆包）——会按错误的 14 阶段契约硬拆代码，返工量大于现在补契约
- ❌ 把 `MomentSolver` 当作 `moment5` 插件处理——数据模型层就不成立（N8）
- ❌ 承诺"运行期 `.so` 插件可换形状因子"——形状因子在 GPU kernel 内（F5）
- ❌ 在未决策 checkpoint 字节序前开工 A4（C1）

---

## 附录 A 源码证据索引

### A.1 致命缺口

| 缺口 | 关键证据 |
|---|---|
| F1 | `Source/Evolve/WarpXComputeDt.cpp:51-114`（`ComputeDt`）、`:270-345`（`ApplyDtLimiters`）、`:116-122`（等离子体频率）、`:124+`（回旋频率）；`Source/Evolve/WarpXEvolve.cpp:204`（每步重算） |
| F2 | `Source/NonlinearSolvers/`（26 文件）；`FieldSolver/ImplicitSolvers/ImplicitSolver.cpp:1148-1150, 1156-1160`（迭代重跑）；`NonlinearSolvers/JacobianFunctionMF.H`（FD 探针污染场）；`NonlinearSolvers/PreconditionerTypes.H` |
| F3 | `Source/Evolve/WarpXEvolve.cpp:661-682`（Strang）、`:447-473`（碰撞插入动量推）、`:303-339`（Maxwell-ES 段）、`:1195-1250`（子循环）、`:1503-1616`（推+沉积交织）；`BoundaryConditions/WarpXFieldBoundaries.cpp:231-242`（Silver-Mueller 半步） |
| F4 | 路线 `:133`（SpeciesHandle 仅 1 处）；`Collision/BinaryCollision/BinaryCollision.H:196-218`（裸指针）；`Particles/ShapeFactors.H:27-84`；`WarpXParticleContainer.H:235-296, 340-441`；`PhysicalParticleContainer.cpp:1499-1501` |
| F5 | `Source/Particles/ShapeFactors.H:27-84`（`if constexpr` 0..4）；`PhysicalParticleContainer.cpp:2034-2061`（运行期分发到模板实例） |

### A.2 结构性缺口

| 缺口 | 关键证据 |
|---|---|
| S1 | `Diagnostics/BTDiagnostics.cpp:531`；`MultiParticleContainer.cpp:1136-1144`；`Diagnostics/BackTransformParticleFunctor.cpp:60-95`；`WarpXEvolve.cpp:273` vs `:372` |
| S2 | `Diagnostics/ReducedDiags/MultiReducedDiags.cpp:56-100`、`:120-140`；`Diagnostics/MultiDiagnostics.cpp:68-74` |
| S3 | `FullDiagnostics/FullDiagnostics.cpp:459-620`；`FlushFormatPlotfile.cpp:463-560`；`WarpXOpenPMD.cpp:348-375, 1391-1420`；`FlushFormatCheckpoint.cpp:83-188`；`WarpXIO.cpp:110-165`；`MultiFabRegister.H:274, 461-490` |
| S4 | `Parallelization/WarpXComm.cpp:747-765`（PML 先于普通场）；`BoundaryConditions/PML.H:141-157, 190-215`；`Fields.H:79-88`（10 个 PML 场键）；`PMLComponent.H:49-84`（各向同性 sigma） |
| S5 | `Parallelization/GuardCellManager.cpp:365-392`；`WarpXComm.cpp:1453-1480, 1661-1685`；`Source/Filter/Filter.H`、`NCIGodfreyFilter.H` |
| S6 | `Utils/WarpXAlgorithmSelection.H:125-140`；`WarpX.H:242-247`；`WarpX.cpp:1936-1949, 1970-1972` |
| S7 | `Python/pywarpx/fields.py:212-222, 264-296`；`Python/pywarpx/picmi.py:3927-3933`；`Source/Python/MultiFabRegister.cpp`；`Source/Python/callbacks.{H,cpp}`；`Python/pywarpx/callbacks.py:287-313` |
| S8 | `ablastr/fields/PoissonSolver.H`、`IntegratedGreenFunctionSolver.H`、`EffectivePotentialPoissonSolver.H`；`ablastr/constant.H`；`ablastr/particles/ParticleMoments.H`；`ablastr/coarsen/`；`ablastr/utils/` |

### A.3 核心类与阶段

| 主题 | 证据 |
|---|---|
| 核心类规模 | `Source/WarpX.H`（1,811 行 / 254 成员 / 191 函数）、`Source/WarpX.cpp` |
| 成员分类 | `WarpX.H:242-247`（边界枚举）、`:299-398`（未进 facade 的 static）、`:1206`（`m_instance`）、`:1407`（`m_implicit_solver`） |
| 字段枚举 | `Source/Fields.H:26-108`（85 项 `FieldType` 声明 / 48 个唯一）、`:110-172`（`isFieldArray`） |
| 分配路径 | `WarpX.cpp:2392-2471`（`AllocLevelData`）、`:2473-3226`（`AllocLevelMFs`） |
| 通信 | `Parallelization/WarpXComm.cpp`（`:663-694` nodal sync、`:718-782` FillBoundary、`:747-765` PML） |
| Runtime facade | `Source/Runtime/Runtime.cpp:24, 45, 49, 66, 77, 92` |
| 阶段定义 | `Source/Evolve/WarpXEvolve.cpp:152`（`Evolve`）、`:543`（`OneStep_nosub`）、`:503`（`OneStep_JRhom`）、`:1195`（`OneStep_sub1`） |
| 初始化 | `Source/Initialization/WarpXInitData.cpp:799, 1034, 1257` |

---

## 附录 B 复核命令

```bash
cd /workspace

# 规模
git ls-files 'Source/*.H' 'Source/*.cpp' 'Source/**/*.H' 'Source/**/*.cpp' | wc -l   # 648
git ls-files 'Source/*.H' 'Source/*.cpp' 'Source/**/*.H' 'Source/**/*.cpp' \
  | xargs cat | wc -l                                                          # 167741

# 单例与 static
grep -rn "GetInstance()" Source/ | wc -l        # 240
grep -rln "GetInstance()" Source/ | wc -l       # 75
grep -n "static .*m_" Source/WarpX.H | wc -l

# 字段硬编码
grep -o "FieldType::[A-Za-z_0-9]*" Source/WarpX.cpp | sort -u | wc -l   # 48
sed -n '2473,3226p' Source/WarpX.cpp | grep -c "FieldType::"            # 152

# 路线文档的概念覆盖
grep -ci "CFL"        Docs/research_notes/最终的技术路线.md   # 0
grep -ci "时间步"      Docs/research_notes/最终的技术路线.md   # 0
grep -ci "步长"       Docs/research_notes/最终的技术路线.md   # 0
grep -ni "dt"         Docs/research_notes/最终的技术路线.md   # 仅 Hardcoded 子串
grep -ci "boost"      Docs/research_notes/最终的技术路线.md   # 0
grep -ci "Lorentz"    Docs/research_notes/最终的技术路线.md   # 0
grep -n  "SpeciesHandle" Docs/research_notes/最终的技术路线.md # 仅 1 处（未定义）

# Runtime facade 承重能力
grep -rn "public Services" Source/ | wc -l       # 0（无实现类）
grep -n "GetInstance()" Source/Runtime/Runtime.cpp   # 5 处

# 关键结构
sed -n '661,682p'  Source/Evolve/WarpXEvolve.cpp    # Strang 序列
sed -n '447,473p'  Source/Evolve/WarpXEvolve.cpp    # 碰撞插入动量推
sed -n '747,765p'  Source/Parallelization/WarpXComm.cpp  # PML 先于普通场
sed -n '365,392p'  Source/Parallelization/GuardCellManager.cpp  # filter→guard 反向依赖
sed -n '56,100p'   Source/Diagnostics/ReducedDiags/MultiReducedDiags.cpp  # 硬编码注册表
```

---

## 附：与其它文档的关系

| 文档 | 关系 |
|---|---|
| [`最终的技术路线.md`](最终的技术路线.md) | **被审计对象**。方向仍为唯一准绳；本文的缺口清单应回写进该文档 |
| [`ROUTE_AUDIT_REVIEW.md`](ROUTE_AUDIT_REVIEW.md) | 本文的**结论速览版**（约 1/4 篇幅） |
| [`SOURCE_AUDIT.md`](SOURCE_AUDIT.md) | 路线的事实底稿；数字基本可靠，8 处需修正（见第 2 章） |
| [`AUTOMATION.md`](AUTOMATION.md) | 任务图与门禁；A0 完成度表述与路线 §9 矛盾 |
| [`PLUGIN_RUNTIME_ARCHITECTURE_V2.md`](PLUGIN_RUNTIME_ARCHITECTURE_V2.md) | Runtime/DataBus/Stage 展开设计；本文指出的缺口多在此展开层 |

> 本报告只报告缺口与修正建议，不重新设计架构。与 `最终的技术路线.md` 冲突时，**以本报告的缺口清单为准**；路线本身仍是方向上的唯一准绳。
