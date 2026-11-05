# 审计报告：WarpX 插件化 Runtime 技术路线的正确性

**审计对象**：`Docs/research_notes/最终的技术路线.md`（以下简称 **Plan**）
**审计范围**：`Source/` 下全部 648 个 `.H/.cpp` 文件（14 批代理逐文件阅读）
**审计问题**：
1. 会不会丢失原版功能？（Plan 声称"默认路径逐位不变"，能否守住？）
2. 会不会丢失用户注入自定义能力的能力？（Plan 声称"插件只认命名状态+阶段+数据依赖"，现有 227 个 EXT hook 能否全部保留等价通道？）

---

## 0. 审计前必知的硬事实

从源码直接读出来、Plan **没有或没有充分覆盖** 的事实（数字均来自 grep 实测）：

| 维度 | 代码事实 | Plan 文档 | 含义 |
|---|---:|---:|---|
| 含静态/单例耦合的文件 | **235** | 估计 66 static 成员 + 241 处 GetInstance | 审计范围比 Plan 估的大——Plan 数的是"被引用数"，实际是"谁引用" |
| 含用户扩展钩子的文件 | **227** | 部分枚举 | Plan 的"角色目录"只列出 5 类扩展点，漏掉约 2/3 |
| FieldType 枚举 | **79** 项（Plan 估 76） | Plan 估 76 | 每差一项就是 DataBus schema 必须显式列出的状态 |
| Python 具名回调 | **18 个**（beforestep/afterstep/afterdiagnostics/onbreaksignal/allocdata/loadExternalFields/afterInitEsolve/beforeInitEsolve/afterInitatRestart/afterBpush/afterEpush/beforeParticles/beforeCollisions/beforeEsolvePoisson/afterEsolvePoisson/beforeImplicitSolve/afterImplicitSolve/oncheckpointsignal） | Plan §1.2 I4 一句"旧输入不破坏" | Plan 没有把 Python 回调映射进阶段表 |
| ablastr::MultiFabRegister 已经是 | `std::map<std::string, MultiFabOwner>`（字符串键） | Plan §2 称"typed registry 是必经之路" | Plan 的 DataBus 与现有 MultiFabRegister 语义重叠但设计冲突 |
| Reconfigurable 算法 enum 共 | ≥20 个（EM solver / pusher / deposition / gather / RZ mode / grid type / QED / implicit / resampling / boundary / EB / filter / ...） | 部分列出 | Plan 的"插件角色目录"漏了约一半 |
| Plan 没提到但代码里存在的全局 | `ParticleType::NextID`(omp critical)、`DefaultInitialization` 全局 map、QED table shared_ptr、`WarpX::m_costs` LayoutData、`VisMF::SetHeaderVersion`、FFT openBCSolver static lazy singleton、SignalHandling static-only class、`Warpx_callback_py_map` static | 未提及 | 这些不在 66 static 清单但同样是进程级全局 |

---

## 1. 结论先行

**结论 A（可行）**：Route B（Runtime 重写 + strangler 渐进迁移）作为长期方向**正确**——代码审计确认 235 处静态耦合和上帝类是真实痛点，Plan 选择"先收全局状态（A1+A2）再拆生命周期（A3）"的顺序合理。

**结论 B（不能直接按 Plan 执行）**：Plan 存在 7 类**未覆盖或低估的风险**，其中部分会导致"逐位不变"或"不丢扩展性"的硬门失守。这些不是"工期问题"，而是**设计模型层面的遗漏**——不先修正 Plan 本身，按 A0→A6 顺序推进会在 A1 或 A3 阶段卡死。

**结论 C（最紧急的 4 个缺口）**：
1. **Plan 的 DataBus 模型**与现有 `ablastr::MultiFabRegister` **语义重叠 + 设计冲突**，必须先统一
2. **Python 18 个具名回调**未被映射进 Plan 的阶段表——这是当前用户扩展能力最密集的通道
3. **20 个以上的"运行时算法 enum 分发"**（不是插件角色目录里的 5 个）必须先决定哪些是"插件"、哪些是"内核分支"
4. **隐式求解器自带子步进循环**，与 Plan 的"统一 stage 调度"直接冲突

---

## 2. 问题 Q1：会不会丢失原版功能？

逐家族核对。✓ = Plan 已覆盖，⚠ = Plan 没充分覆盖，✗ = Plan 没覆盖。

### 2.1 场求解器家族

| 求解器 | 功能 | Plan 角色 | 风险 |
|---|---|---|---|
| Yee / CKC / Nodal / Leithe | FDTD 显式 | IFieldSolver ✓ | Plan 没考虑 Yee/CKC/Nodal 三者共享同一类运行时 switch（`m_fdtd_algo × m_grid_type`）；拆成三个插件后仍然要**运行时分派**，插件粒度可能比必要的更细 |
| ECT | Embedded-Cartesian-Transverse | IFieldSolver + IBoundary | **⚠ Plan 没提 ECT**。ECT 不是独立求解器，是**对 Yee/CKC 的 boundary 修正**。把 ECT 当作"插件"会导致它必须"知道"宿主 FDTD 的内部 stencil 状态（`dual_A_y`/`dual_L_x`/`dual_L_z` 等）——违反"插件互不认"原则 |
| PSATD（Galilean/comoving/voxel） | 谱显式 | IFieldSolver ✓ | **⚠ SpectralFieldIndex 硬编码 int 索引**（按 8 个开关 `pushSpectralFields/CurrentCorrection/VayDeposition/divCleaning/...` 分配整数索引），RZ 叠加 `+mode*n_fields`。DataBus 的 string key 模型与之**不对等**——每个 PSATD 实例需要一个**显式的 field schema 声明**，Plan 的 `FieldSpec` 目前只定义了 `ncomp/nodal/ngrow/checkpoint/unit` |
| 隐式（theta / semi-implicit / Strang spectral / semi-implicit Darwin） | 隐式 | 无明确角色 | **✗ 隐式求解器自带子步进循环**：`ImplicitSolver::OneStep` 含 ≤8 次 substep、dt 减半、ResetStep/FinishStep、中途执行 reduced diagnostics 和边界粒子处理，完全覆写了外部调度器。Plan §3.4 的 stage 表（BeforeStep/AfterStep 等）**装不下隐式子步进** |
| 静电（labframe/relativistic EB Poisson + MLMG） | 静电 | 无明确角色 | **⚠ Plan 没提静电**。静电用 `WarpXSolverVec::m_dofs` 进程级静态 + PETSc/MLMG 深度耦合，且 `MagnetostaticSolver` 禁 MR、RZ 单模 |
| Hybrid-PIC | 混合 | 无 | **✗ Plan 未提 Hybrid-PIC**。它**在 field register 分配 10+ 个 `hybrid_*` FieldType 且持有 QdsmcParticleContainer**——求解器持有粒子态，完全不符合 Plan 的"求解器只管场" |
| FDTD PML 双轨 | PML | IFieldBoundary | **⚠ PML 非单一实现**：笛卡尔完整 Split-Field 9 分量，RZ 独立指数阻尼 `exp(-4*c*dt/dr * (rr/nr_damp)^2)`。Plan 角色目录只写"归场求解器内部"，没提 RZ 与笛卡尔的实现完全不兼容 |

### 2.2 粒子/碰撞/基元过程家族

| 功能 | Plan 角色 | 风险 |
|---|---|---|
| Pusher（Boris/Vay/HigueraCary + 辐射反作用） | IParticleModel ✓ | PushSelector.H 是**运行时 switch 不是工厂**——加新 pusher 必须改 switch；Plan 没提怎样把 switch 升级为插件 |
| 沉积（Esirkepov/Direct/Vay/Villasenor/质量矩阵） | 同上 | DepositCurrent.cpp 单文件 1300+ 行内**algo × explicit/implicit × shape1-4 × shared-mem × PEC × RZ 六维运行时分派**。DataBus 模型拆不掉这种密集内部分派 |
| 碰撞 10+ 种（Coulomb/Perez/DSMC/Bremsstrahlung/LBW/LCompton/Fusion/BackgroundMCC/Stopping/HybridDrag/InverseBrems/PulsedDecay） | ICollisionModel ✓ | **⚠ DSMC 硬编码 max 4 个 ScatteringProcess**（SplitAndScatterFunc.H），Fusion 重写 AllocData 且直接用 `WarpX::GetInstance()` MultiFab。Plan 没提碰撞如何声明 field 依赖 |
| QED（QS/BW/Schwinger + lookup table） | IQedModel ✓ | **⚠ QED table 是进程级共享可变状态**：MultiParticleContainer 持有主表并初始化（`shared_ptr`），副本分发给各 species。Plan 的 StateSubstrate 只说"注册状态"，没提**跨对象共享可变状态**怎么在 schema 里表达 |
| ADK 场致电离 | IQedModel ✓ | **⚠ functor 构造时读 WarpX 静态成员**：`galerkin_interpolation/nox/n_rz_azimuthal_modes/CellSize/LowerCorner`。静态成员虽然会进 RuntimeConfig，但 Plan 没说 functor 如何从 DataBus 拿这些值——目前是构造期直接读静态 |
| 流体（冷流体 + 5/10-moment + 摩擦 + EOS） | IFluidModel ✓ | **⚠ 两套流体系统不共享任何组件**：冷流体用 WarpXFluidContainer（Muscl-Hancock Rusanov flux），5-moment 用 MomentFluidSolver（FVND SSP-RK3）。Plan 说"PicEmPackage 注册 cold/moment5/medium_tensor"，但没说**两套流体如何在同一 stage 协同**（都写 rho/J/N/NU，Plan 的 stage+依赖模型需要声明读写冲突） |

### 2.3 诊断家族

| 功能 | Plan 角色 | 风险 |
|---|---|---|
| FullDiagnostics + 16 个 ComputeDiagFunctor | IDiagnostic ✓ | functor 基类是 C++ 继承（非插件）；Plan 的"插件"定义与 functor 继承体系**模型不兼容** |
| BTDiagnostics（boosted→lab Lorentz 变换 + buffer 切片/合并 + openPMD BTD resize workaround） | IDiagnostic ✓ | **⚠ Plan 没提 BTD**。BTD 的 buffer 状态（`m_buffer_counter/m_max_buffer_multifabs/m_snapshot_full/...`）已写入 checkpoint（WarpXHeader），是**字节级契约**——DataBus schema 必须包含这些 buffer 状态并声明为 checkpoint，否则重启破坏 |
| ReducedDiags 21 子类 | IDiagnostic ✓ | A1.4 迁移已推进约 15 个到 RuntimeContext 路径，6 个（BeamRelevant/Collider/DiffLum/DiffLum2D/LoadBalanceCosts/ChargeOnEB）仍直调 `WarpX::GetInstance()`。Plan 的 A1.4 batch C6 就是扫这些 |
| FlushFormat 6 子类 + in-situ（Ascent/Catalyst/Sensei） | 无 | **⚠ Plan 没提输出格式是插件**。Format 选择在 Diagnostics.cpp:563-591 是硬编码 if 链；Sensei 直接把 WarpX 单例 const_cast 成 `amrex::AmrMesh*`——完全违反"插件不认内核" |
| Checkpoint（WarpXIO + FieldType 枚举硬编码键名） | IOSubstrate ✓ | **⚠ Plan 没提 checkpoint 的硬编码耦合**：Checkpoint.cpp:83-189 逐字段硬枚举 `FieldType::Efield_fp/Bfield_fp/...`，不是 MultiFabRegister 的字符串键。schema 化必须同时迁移 FieldType 枚举和 checkpoint 读写路径 |
| OpenPMD（1722 行单文件，ADIOS2/HDF5/JSON 后端 + ED-PIC 元数据 + RZ thetaMode） | 无 | **⚠ Plan 没提 OpenPMD 的全局状态**：`WarpXOpenPMD` writer 为跨 flush 有状态 `unique_ptr`；Catalyst 全局 init/finalize 绑定在构造/析构。Plan §3.1 的 IOSubstrate 只说"schema 驱动 checkpoint"，没说 openPMD 的跨 flush 状态 |

### 2.4 其他家族

| 功能 | Plan 角色 | 风险 |
|---|---|---|
| 激光注入（天线/Waist/LabFrame/BoostedFrame + 4 种轮廓） | 无 | **⚠ Plan 没提激光**。激光轮廓可用 ParserExecutor\<4\>(t,x,y,z) 自定义表达式，是**高价值 EXT hook** |
| AcceleratorLattice（Plasma Lens 解析光学 + Gauss/Kolmogorov 模型） | 无 | ⚠ 同上，高价值 EXT hook |
| EB 几何（ParserIF 隐函数 + STL + CSG + level-set） | EbSubstrate ✓ | **⚠ Plan 的 EbSubstrate 注释说"算法在插件"，但 parser 路径的 EB2::Build 是**内核**调用。硬编码 `coarsenTo = maxLevel()+20`（三处），用户无法配置** |
| PEC Insulator（双 parser：insulator.area_x_lo(y,z) + insulator.By_x_lo(y,z,t)，每方向 6 个实例） | IFieldBoundary | **⚠ Plan 没提 PEC_Insulator**。这是**每个方向两个独立 parser 实例**的用户扩展机制 |
| 滤波（Filter 基类 GPU/CPU 双路径卷积） | IFilter | ⚠ Plan 角色目录提了 IFilter 但没说 Filter 是**继承式扩展**（填 m_stencil_0/1/2），与插件模型不同 |
| 动态 regrid | IAmrPolicy ✓ | **✗ Plan 的 A6 是空想**。WarpX.cpp:L2364 `MakeNewLevelFromCoarse` 直接 abort，所有 regrid 走 `FromScratch` + `ClearLevel`。Plan 说"A6.1 AmrCore::regrid"，但内核里**根本没有**动态 regrid 的基础设施（Plan §2 事实表自己承认了这点） |

### 2.5 结论：Q1（丢功能）

- **高风险（✗）**：隐式求解器子步进、Hybrid-PIC 求解器持粒子态、动态 regrid（根本不存在）、FlushFormat in-situ 的 const_cast 单例
- **中风险（⚠）**：ECT 作为求解器插件但需要 FDTD 内部状态、PSATD int 索引 vs DataBus string key、QED 共享可变表、流体双轨协同、BTD buffer 状态 checkpoint 契约、openPMD 全局状态、Checkpoint FieldType 硬枚举键名
- **低风险（✓）**：显式 FDTD、显式粒子、大部分 reduced diags（A1.4 已迁移）、激光/AcceleratorLattice/Filter 虽然 Plan 没列角色但 Ext 通道可保留

---

## 3. 问题 Q2：会不会丢失用户扩展能力？

代码审计发现 **227 个文件含 EXT hook**。按 Plan 的 DataBus + Stage + 插件模型逐个核对：

### 3.1 Plan 已覆盖且等价的

| EXT 类型 | 现机制 | Plan 映射 | 等价性 |
|---|---|---|---|
| 用户定义场表达式 | `ParserExecutor<4>(t,x,y,z)`：macroscopic sigma/epsilon/mu/tensor、PEC_Insulator area/By | 插件用 DataBus + 自定义 expression parser | ✓ Plan 的 plugin 可内建 parser |
| 自定义碰撞类型 | CollisionHandler if-chain → 需改代码 | ICollisionModel 插件 | ✓ |
| 自定义重采样算法 | Resampling.cpp:22-31 if-chain | IParticleModel 插件 | ✓ |
| 自定义 QED lookup table | `WarpX_QED_TABLE_GEN` 编译期或 `load_table_from` | IQedModel 插件 | ✓ |
| 自定义激光轮廓 | ParserExecutor\<4\>(t,x,y,z) 或 openPMD 文件 | 插件 + parser | ✓ |
| 自定义几何 | `eb_implicit_function = "f(x,y,z)"` | EbSubstrate + 用户 EB 函数插件 | ✓ Plan 列了 EbSubstrate |
| 粒子注入 | 6 种 injection_style + openPMD 文件 | PicEmPackage Injector 插件 | ✓ |

### 3.2 Plan 覆盖但等价性有问题的

| EXT 类型 | 现机制 | Plan 映射 | 问题 |
|---|---|---|---|
| **Python 具名回调（18 个）** | `warpx_callback_py_map<std::string, std::function<void()>>` + GIL 派发 | "旧输入不破坏"（I4） | **⚠ Plan 没给每个回调名分配明确的 stage 位置**。`afterBpush/afterEpush` 在 field solver 内部、`afterInitEsolve/beforeInitEsolve/afterInitatRestart` 在 init 路径、`loadExternalFields` 在外部场读取后、`beforeParticles/beforeCollisions/beforeEsolvePoisson/...` 在 evolve 各子点——这些不是 Plan 的"标准阶段"里能放的，需要 **stage 的子位点** |
| **用户扩展粒子属性** | `<species>.addRealAttributes/addIntegerAttributes` + parser 签名固定 (x,y,z,ux,uy,uz,t) | SpeciesSpec 自定义组件 | **⚠ Plan 的 SpeciesSpec 只说 `ncomp/components`，没说如何把 parser 自动求值写进新组件** |
| **粒子 filter 表达式** | `plot_filter_function(t,x,y,z,ux,uy,uz)` 签名 7 变量固定 | 诊断插件内 parser | ✓ |
| **PICMI 接口** | Python 侧 `pywarpx` API | Plan 没提 | **⚠ Plan 的 Python 策略只说了"API 迁移到 Runtime"，没说 PICMI 的 Python API 兼容** |

### 3.3 Plan 未覆盖的 EXT（会丢失）

| EXT 类型 | 现机制 | Plan | 影响 |
|---|---|---|---|
| **EB scrape functor（任意用户 callable）** | `scrapeParticlesAtEB(pc, distance_to_eb, callable)` 模板接受任意 C++ functor | Plan EbSubstrate 只说"cut-cell 算法在插件" | **✗ 最灵活的用户 C++ 注入点**——用户可以写任意 EB 刮除行为，不限 specular/thermal。插件模型无法表达这种模板 + 任意 functor |
| **PETSc nonlinear solver 选项（运行期解析）** | `petsc_*` 参数块 | Plan 的 ImplicitSolver 角色未定 | **✗ 用户可通过 PETSc 运行期参数配置预条件器/非线性解**，这不是插件但必须保留 |
| **Sensei/Catalyst in-situ const_cast WarpX 单例** | Diagnostics.cpp:574 `const_cast<amrex::AmrMesh*>(&WarpX::GetInstance())` | Plan 没提 in-situ | **✗ 这是 in-situ 唯一的数据通路**——Plan 的"插件不认单例"会直接杀死 in-situ |
| **Hybrid-PIC HybridRho/RhoNew 运行期** | WarpXPushFieldsHybridPIC.cpp: 含 `WarpX::GetInstance()` 约 10 处 | Plan 没提 Hybrid-PIC | **✗ Hybrid-PIC 是 HybridQED/WarpX 的共享模型**，用户靠 `hybrid_PIC = 1` 打开 |
| **环境变量 WARPX_YEE_CELL_EPS / WARPX_DUMP_GRID_TYPE** | CMakeLists.txt + runtime 环境变量 | Plan 没提 | ⚠ 开发/调试用 EXT |
| **`external_E/B_particle_init_style`（4 种外场：parser + plasma lens + file + global）** | GetExternalFields.cpp 运行期 switch | Plan 归"插件" | ✓ 可映射 |

### 3.4 结论：Q2（丢扩展性）

- **高风险（会丢）**：EB scrape 任意 C++ callable 模板（Plan 的插件模型是标准接口，装不下任意 functor）、in-situ const_cast 单例（Plan 的"插件不认单例"直接禁止）、PETSc 运行期参数配置（Plan 未提隐式）
- **中风险（等价性不足）**：Python 18 个具名回调的阶段映射、用户粒子属性 parser 自动求值、PICMI Python API
- **低风险（等价）**：大部分 parser 驱动的 EXT、自定义算法枚举、自定义几何/材料属性

---

## 4. 对 Plan 本身的修正建议

### 4.1 Plan 的 DataBus 与现有 MultiFabRegister 重叠 + 冲突

**事实**：`ablastr::MultiFabRegister` 已经是 `std::map<std::string, MultiFabOwner>`（字符串键），key 完全限定 `name[dir=X][level=N]`，`m_mf_register` 是 **public**，外部可直接遍历；且已有 `m_remake`/`m_redistribute_on_remake`/`m_checkpoint_restart`/`m_owner`（alias 追踪）四个语义钩子——这就是 Plan §3.3 想要的 DataBus，但 Plan 没看到它。

**Plan 的错误假设**：§2 说"typed registry 是必经之路"，实际上 MultiFabRegister 已经是字符串键——**typed key 是 Plan 自己多加的一层**。Plan 应该：

1. 把 `MultiFabRegister` 作为 DataBus 的唯一实现，不要另造 DataBus
2. Plan 的 `FieldSpec` 必须补齐 MultiFabRegister 已有的 `remake/redistribute/checkpoint/alias` 四个语义，加上 `unit` 和插件声明的 `produces/reads`
3. Plan 的 `FieldKey = std::string` 与 MultiFabRegister 的 `name[dir][level]` key 格式统一

### 4.2 Plan 漏了"阶段子位点"概念

**事实**：Python 18 个回调不是 Plan §3.4 的 13 个标准阶段能容纳的：
- `afterBpush/afterEpush` 在 field solver 的**内部**（不是 AfterFieldSolve）
- `loadExternalFields` 在 `InitLevelData` **之后、div cleaner 之前**
- `beforeCollisions` 在 particle 循环内部
- `afterImplicitSolve` 在 implicit 内部的 **FinishStep 之后**

**Plan 应修正**：把阶段表从 13 个扩展为带子位点的树形结构，例如 `AfterFieldSolve.afterBpush` / `InitData.afterLoadExternalFields`。

### 4.3 Plan 的插件模型需要"插件持有内核状态引用"的例外

**事实**：
- ECT 不是独立求解器，是 Yee 的 boundary 修正，需要读 FDTD 内部的 `dual_A_y` 等临时数组
- 隐式求解器持有 WarpX 指针 + 运行自己的子步进循环
- Hybrid-PIC 求解器**持有 QdsmcParticleContainer**（求解器 → 粒子容器依赖）
- in-situ 需要把 AmrCore 指针传进插件

**Plan §1.2 I2 "插件不互认"过于严格**：需要引入 **host_service interface**——插件可以向内核请求服务（`IFieldInternalAccess`、`IParticleContainerAccess`、`IInSituBridge`），这些 service 接口由内核实现、插件依赖注入。这样 ECT 可以通过 `IFieldInternalAccess` 拿到 dual geometry，in-situ 可以通过 `IInSituBridge` 拿到 AmrCore，而不需要知道 `WarpX::GetInstance()`。

### 4.4 A1.1 RuntimeConfig 应从"快照"改为"引用访问"

**事实**：Plan §3.1 让 Runtime 拥有 `RuntimeConfig m_config{}` 值成员（快照）。但 69 个 WarpX static 成员中，约 41 个是**运行时可变**的（`galerkin_interpolation` 初始构造但运行期读取、`moving_window_v` 每步刷新、`dt` 每步变、`finestLevel` regrid 后变）。`RuntimeContext::refresh_time` 只是刷新时间状态，不刷新其他可变配置。

**Plan 应修正**：RuntimeConfig 应持有指向 WarpX 可变状态的**非拥有指针**，或 RuntimeConfig 本身就是一个 accessor 接口（`RuntimeConfig::galerkin_interpolation() const`），返回时从 WarpX 实时读。否则"快照"意味着 A1 迁移后会出现"插件看到旧值"的 bug。

### 4.5 A2 LevelResources 必须先修"不存在的基础设施"

**事实**：Plan A2 依赖 `ClearLevel` 完整化 + `AllocLevelData` schema 化 + `RemakeLevel` 完整化。但：
- `MakeNewLevelFromCoarse` **直接 abort**（WarpX.cpp:L2364）
- AMReX regrid 能力根本没被 WarpX 使用——所有 regrid 是 FromScratch
- Plan A6 "动态 regrid" 建立在不存在的基础设施上

**Plan 应修正**：把 A2 拆成 **A2a 静态多层 LevelResources**（当前 MR_levels 路径）和 **A6a 动态 regrid 基础设施**（先修 MakeNewLevelFromCoarse 不 abort，实现 AmrCore::regrid），再推进 A6b IAmrPolicy。

### 4.6 Plan 没提 20+ 运行时算法 enum 分发怎么处理

**事实**：代码里有至少 20 个枚举驱动的运行时分派（§0 表格），Plan 只在角色目录列了 IFieldSolver/IPusher/IDeposition 等 5 个。每个枚举都对应 if/else/switch 链在 WarpX 或子模块里。

**Plan 应新增 §2.5 运行时算法 enum 迁移策略**：
- 编译期模板分发（Yee 路径的 `T_Algo` × `T_MacroAlgo`）→ 保留编译期分支，不是插件
- 纯运行时 switch（PushSelector.H）→ 升级为工厂注册表（`register_pusher("Boris", make_unique<BorisPusher>)`）
- 运行时 if-chain（Resampling.cpp:22-31、Diagnostics.cpp:563-591 格式选择）→ 升级为工厂注册表

### 4.7 Plan 应新增"插件生命周期钩子"覆盖 checkpoint 契约

**事实**：BTD buffer 状态、Hybrid-PIC substeps、ReducedDiags 都有 checkpoint/restart 状态写入；Plan §3.5 的 `on_checkpoint/on_restart` 只字未提需要声明哪些字段是 checkpoint 成员。

**Plan 应修正**：`HandlerSpec` 新增 `checkpoint_fields` 字段（与 `reads_fields/writes_fields` 对称），Runtime 在 checkpoint 时扫描所有插件的 checkpoint_fields 写入，restart 时自动恢复。

---

## 5. Plan 隐含的隐藏耦合（代码里有但 Plan 没说）

| 隐藏耦合 | 代码位置 | 影响 |
|---|---|---|
| **FieldType enum 既是字段枚举又是 checkpoint 键名** | Fields.H + Checkpoint.cpp:83-189 | 改 FieldType → restart 破坏 |
| **WarpXHeader checkpoint 格式是字节级契约** | WarpXIO.cpp | Plan 说"schema 驱动 checkpoint"但没说兼容旧格式的双读机制 |
| **OpenPMD writer 跨 flush 有状态** | WarpXOpenPMD.cpp | 插件必须声明生命周期（不是每 flush 新建） |
| **BTD buffer 状态在 checkpoint 里** | WarpXHeader | A1.4 IDiagnostics 插件必须声明 checkpoint_fields |
| **Hybrid-PIC 求解器分配 hybrid_* FieldType + 持粒子容器** | WarpXPushFieldsHybridPIC.cpp | 求解器插件会同时产生粒子状态和场状态 |
| **PPIC 动态 scheduling 标志** | WarpXParIter ctor 读 `WarpX::do_dynamic_scheduling` | Runtime 的并行调度不是 AMReX 默认行为 |
| **particle ID 全局计数器 NextID（omp critical）** | AddParticles.cpp:905-914 | 插件化后必须保证 ID 生成仍是全局唯一 |
| **MovingWindow shift 20+ 字段类型** | WarpXMovingWindow.cpp | 每个字段类型单独 switch，插件声明的新字段必须在 switch 里加 case |
| **Guard Cells 偶数强制（多处隐式）** | GuardCellManager + RemakeLevel + FillBoundary | 插件声明的新 MultiFab 必须声明 ngrow 的偶数规则 |
| **RZ 模式 Hankel 变换分块必须 ≥ 径向格点数的 2 次幂** | SpectralHankelTransformer.cpp | 用户输入被硬丢弃 |
| **LoadBalance cost LayoutData 所有权** | WarpX::m_costs | 每步更新，所有诊断和碰撞读同一个 |
| **ParseFieldFunction 作为标准用户扩展** | Laser/ParticleCreation/MacroscopicProperties | 是用户最常写的扩展 |

---

## 6. 建议的实施顺序修正

原 Plan：A0 → A1 → A2 → A3 → A4 → A5 → A6

**建议修正为**（括号内为新增/拆分的阶段）：

| 阶段 | 内容 | 说明 |
|---|---|---|
| A0 | 源码审计（已做） | Plan 的 A0 覆盖约 20%；本审计补齐 |
| **A0.5** | 统一 MultiFabRegister 为 DataBus 实现 | 解决 §4.1 的核心冲突 |
| **A0.6** | Python 18 回调 → 阶段子位点表 | 解决 §4.2 |
| A1 | 去单例（Plan 的 A1，RuntimeConfig 改为引用访问） | 解决 §4.4 |
| **A1.5** | 20+ 运行时 enum → 工厂注册表 | 解决 §4.6 |
| A2a | 静态多层 LevelResources | 当前 MR 路径 |
| **A2b** | EB scrape functor 桥接（模板插件接口） | 解决 §3.3 的 EB 任意 callable |
| A3 | 生命周期拆包，隐式求解器挂为 substage | 解决隐式子步进与 §4.3 的 host_service |
| A4 | Field/Species/Material registry + checkpoint schema | MultiFabRegister 语义对齐 |
| A5 | Stage + Handler + 依赖 + host_service 接口注册 | 引入 §4.3 的 IFieldInternalAccess/IInSituBridge |
| **A6a** | 动态 regrid 基础设施（MakeNewLevelFromCoarse 不 abort + AmrCore::regrid） | Plan 的 A6 依赖不存在的底座 |
| A6b | IAmrPolicy | A6a 之后 |

---

## 7. 最终判断

**技术路线可行，但 Plan 本身需要上述修正才能"不丢功能、不丢扩展性"**。

Plan 最大的问题不是"工期低估"（Plan 自己承认了 16–30 人月），而是：
1. **DataBus 与 MultiFabRegister 重叠**（Plan 多加了一层设计，应该复用）
2. **Python 回调阶段映射缺失**（当前最密集的 EXT 通道）
3. **隐式求解器自带子步进**（与 Plan 的统一调度模型直接冲突）
4. **EB 任意 callable / in-situ const_cast**（插件模型装不下的扩展点，需要 host_service 例外）

不先修正 Plan，按 A0→A6 顺序推进到 A1 后半或 A3 时会卡死。先做 A0.5 + A0.6 + A1.5 三个"对齐层"（约 2–3 人月），再进入原有 A1 → A2 → A3 主线。

---

*审计底稿：14 批次代理逐文件阅读，共 648 文件落盘于 `/tmp/census/A_root_evolve_init_runtime.md` 至 `/tmp/census/N_ablastr.md`。Plan 原文：`Docs/research_notes/最终的技术路线.md`。事实引用均含文件路径，行号来自代理记录。*
