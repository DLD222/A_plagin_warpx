# WarpX 插件化最终架构（源码核对版）

<!-- SPDX-License-Identifier: BSD-3-Clause -->

> 状态：V1，已被 `PLUGIN_RUNTIME_ARCHITECTURE_V2.md` 修正（V2 把内核改为 Runtime、接口改为通用数据键、协作改为数据依赖调度）。本文保留作为“WarpX 内开孔”路线的细节。

> 本文取代 `PLUGIN_ARCHITECTURE_DESIGN.md` 作为架构准绳；`PLUGIN_P1_IMPLEMENTATION_PLAN.md` 降级为“第一个里程碑的施工细节”。
> 依据：工作区 `cfb8f8d`（源码 = `WarpX_fluid` HEAD `0de0e26` 拷贝），逐文件核对，不引用未读代码。
> 目标只有一句话：**内核稳定、旧源码基本不动；用户新增能力 = 写一个模块 + 用标准插头接上；不改正旧源码。**

---

## 0. 摘要（结论先行）

1. **可以做到“用户只写一个模块”，但内核必须先做一次性的“开孔”**：在 13 个聚合入口各加一个 `if (plugin)` 分派 + `return`，旧函数体原样保留。这是物理上不可避免的最小改动；开孔完成后内核冻结，用户再也不碰核心。
2. **不能把 WarpX 改造成“纯接口内核”再重写全部物理**——源码实测：`WarpX` 是 1811 行、~305 成员的上帝类，`mypc->` 调用 130 处，场更新有 10 条路径，物质模型状态三种异构所有权（粒子 SoA / 注册表 MultiFab / 私有 `Real*`）。全量下沉是 12–18 人月，且逐位回归不现实。**最终架构采用“绞杀者 + 能力契约”，不采用“推倒重写”。**
3. **角色不是一个 `ISpeciesModel` 打天下**，而是按源码真实边界切：
   - 场：`IFieldSolver`（方程+离散+求解器捆绑，PSATD 需要 `advance_whole_step`，不能硬塞进 `advance_E/B`）；
   - 物质：`IParticleModel` / `IFluidModel` 两个子角色，由内核的 `CompositeSpeciesModel` 组合（因为 PIC/冷流体/矩流体的状态所有权完全不同）；
   - 边界：`IFieldBoundary`（PEC/PMC/绝缘体/轴）与求解器内的 PML/Silver-Mueller/Damped 分开；
   - AMR：只有“策略”可插件化（tagging / 负载均衡 / 基础网格），**本仓库没有动态 regrid**（`regrid_int` 是死参数，`MakeNewLevelFromCoarse` 直接 abort），所以不能承诺“插件实现新 AMR”。
4. **加载方式：编译期静态注册为主，运行期 `.so` 为可选**。GPU/AMR tagging 的 device functor 只能编译期；host 侧角色（diagnostic/source/material/boundary）可用运行期。
5. **首个可用里程碑（M1）**：场求解器（Yee/CKC/ECT）+ 物质组合模型 + 材料 + 边界 + 诊断，约 2–3 个月；用户写“Maxwell+引力”或“流体+引力”只需一个 `.cpp`。

---

## 1. 目标、边界与硬约束

### 1.1 目标

```
用户需求 = 新场方程 / 新场求解器 / 新物质模型 / 新边界 / 新源项 / 新材料 /
           新诊断 / 新碰撞 / 新 AMR 策略 / 新时间积分
实现方式 = 实现一个标准接口 + 注册（1 个 .cpp，不改内核）
连接方式 = 输入里选名字；内核在固定 seam 调用
```

### 1.2 硬约束（来自源码，不是偏好）

| # | 约束 | 证据 |
|---|---|---|
| C1 | 场状态是全局可变的 `MultiFabRegister`，消费者都按 `(lev, patch, stagger)` 直接索引 | `WarpX.H:1115`；`WarpXFieldBoundaries.cpp:51-61` |
| C2 | 场更新有 10 条路径，时间分裂写在**驱动**里，不在 solver 对象里 | `WarpXEvolve.cpp:627-692`、`:1253-1434` |
| C3 | 物质状态三种所有权：PIC 粒子 SoA、冷流体注册表 MultiFab、5 矩私有 `m_U` | `WarpXParticleContainer.H:409`；`WarpXFluidContainer.H:194`；`MomentFluidSolver.H:66` |
| C4 | 冷流体 `WarpXFluidContainer` **没有任何虚函数**，不能直接子类化 | `WarpXFluidContainer.H:29-197` |
| C5 | 诊断基类已经是虚接口，但工厂是 if/else 硬编码 | `Diagnostics.H:73-110`；`MultiDiagnostics.cpp:22-30` |
| C6 | 激光是粒子物种（`LaserParticleContainer`），不是网格源；外场是 E/B 加性源 | `LaserParticleContainer.cpp:560`；`WarpXInitData.cpp:963` |
| C7 | PML/Silver-Mueller/Damped 在 solver 层；PEC/PMC/绝缘体/轴在 kernel 层 | `WarpXFieldBoundaries.cpp:55-242`；`PML.H:138` |
| C8 | **没有动态 regrid**：`regrid_int` 无读者，`MakeNewLevelFromCoarse` abort，子循环只支持 2 层 | `WarpX.cpp:742/2361`；`WarpXEvolve.cpp:511-514` |
| C9 | `RemakeLevel` 一次性重分配场/EB/PML/谱求解器/诊断 functor，不能拆给插件 | `WarpXRegrid.cpp:176-350` |
| C10 | 重启格式是事实 ABI，读写两处重复实现 | `WarpXIO.cpp:97`；`FlushFormatCheckpoint` |

### 1.3 不做什么

- 不替换 AMReX 的 AMR 机制、MPI、FFT 后端、内存管理。
- 不把 GPU kernel 做成运行期 `.so`（device 函数无法跨 DSO 边界）。
- 不承诺“零内核改动”；只承诺“一次性开孔，之后冻结”。
- 不在 M1 动隐式求解器、HybridPIC、PSATD 的时间分裂（它们另起阶段）。

---

## 2. 源码核对：架构为什么必须这样切

### 2.1 规模与耦合（实测）

| 指标 | 数值 |
|---|---|
| `WarpX.H` | 1811 行，~305 成员 |
| `Source/Evolve/WarpXEvolve.cpp` | 1683 行 |
| `WarpXPushFieldsEM.cpp` | 2113 行 |
| `PhysicalParticleContainer.cpp` | 2287 行 |
| `mypc->` 调用 | 130 处 |
| `m_fdtd_solver_fp/cp` 引用 | 19 / 15 处 |
| `ApplyEfieldBoundary/BfieldBoundary` 调用 | 13 / 9 处 |
| `m_macroscopic_properties->` | 19 处 |

**结论**：不能按“每个调用点都接口化”来做，会把 200+ 处调用改成虚调用且无法回归。正确做法是**在聚合入口开孔**：`PushParticlesandDeposit`、`EvolveB/E`、`Apply*Boundary`、`FilterComputePackFlush`、`ErrorEst` 等，每个聚合函数内部仍然保留旧的 130 处调用。

### 2.2 场更新路径（10 条）

| # | 路径 | 入口 | 时间语义 | AMR |
|---|---|---|---|---|
| 1 | Yee | `EvolveB`/`EvolveE` | B 两个半步、E 全步 | 支持，子循环仅 2 层 |
| 2 | CKC | 同上（`m_fdtd_algo` 模板） | 同上 | 同上 |
| 3 | ECT | 同上 + `EvolveECTRho` | 同上 | 同上，仅 2D3V/3D3V |
| 4 | Macroscopic-E | `MacroscopicEvolveE` | E 全步（含 `rho_medium`） | **仅 lev 0 / fine** |
| 5 | PSATD | `PushPSATD` | E/B/F/G 同时全步（k 空间） | 支持 |
| 6 | PSATD-JRhom | `OneStep_JRhom` | 多次沉积 | PSATD only |
| 7 | 静电 | `ComputeSpaceChargeField` | 步末重解（非时间推进） | 支持 |
| 8 | HybridPIC | `HybridPICEvolveFields` | 步末 Ohm 定律 + Ampere | **仅 lev 0** |
| 9 | 隐式（Theta/SemiImplicit/…） | `ImplicitSolver::OneStep` | 自己拥有整步 | 禁止子循环 |
| 10 | 矩流体 J | `OneStep_nosub` 内联 | 仅加 J | lev 0 |

**结论**：`IFieldSolver` 不能只有 `advance_E/B`。必须允许“整步”入口（PSATD/Hybrid/隐式），并用能力标志告诉驱动走哪条序列。

### 2.3 物质模型（异构）

| 模型 | 入口 | 状态 | AMR 能力 |
|---|---|---|---|
| PIC | `MultiParticleContainer::Evolve` | 粒子 SoA | 是 |
| 冷流体 | `WarpXFluidContainer::Evolve` | 注册表里 `fluid_density_*`/`fluid_momentum_density_*` | **否**（assert `max_level<=1`） |
| 5 矩 | `MomentFluidSolver::AdvanceAndDeposit` | 私有 `m_U`，刻意不依赖 AMReX | 否（2D-XZ/周期/单 box/lev 0） |
| 10 矩 | 仅库，无 WarpX 调用点 | `Real*` SoA | n/a |
| 激光 | `LaserParticleContainer::Evolve` | 粒子 SoA | 是 |
| QDSMC | `QdsmcParticleContainer` | 粒子 | 由 HybridPIC 拥有 |

**结论**：`ISpeciesModel` 若强行统一，会假设 `MultiFabRegister`，直接排除 5 矩；若按容器类型分派，又需要内核知道具体类型。**正确切法**：内核面向 `ISpeciesModel` 组合器（内置），组合器内部再分 `IParticleModel` / `IFluidModel`；状态用不透明句柄 + 能力标志，接口不出现 `WarpXFluidContainer`/`MomentFluidSolver`。

### 2.4 诊断 / 源 / 边界 / 材料

- **诊断**：`Diagnostics` 已是纯虚基类（`Flush`/`DoComputeAndPack`/`DoDump`/`InitializeFieldFunctors`），缺的只是一个 name→factory 映射（照抄 `MultiReducedDiags.cpp:56-79` 的模式）。
- **源**：激光是粒子物种（J 沉积）；外场是 E/B 加性并与边界强序（`AddExternalFields` 后立刻 `Apply*Boundary`）。一个通用 `ISource` 必须带“阶段 + 汇类型”，否则二者无法共用。
- **边界**：PEC/PMC/绝缘体/轴是 kernel 层；PML/Silver-Mueller/Damped 是 solver 层。PML 对象拥有 split fields、SigmaBox，不能交给通用 `apply()`。
- **材料**：`MacroscopicProperties` 负责 ε/σ/μ、张量、subpixel 行、指数矩阵、`jcond`；但 `rho_medium` 是 WarpX 自己的场，不归材料对象。`InitSubpixelRows` 需要 EB 工厂且当前传入 `nullptr`。

### 2.5 AMR 的真实能力（必须写进架构，避免过度承诺）

- **静态 MR**：只在 `InitFromScratch` 建一次；`MakeNewLevelFromCoarse` abort；`RemakeLevel` 只接受同 BoxArray 的换 DM。
- **动态 regrid**：不存在。`regrid_int` 解析后无人读。
- **可插件化的只有三件事**：
  1. `ErrorEst` 的 tagging 规则（`WarpX.cpp:3574`，纯函数，只依赖 parser/几何）；
  2. 负载均衡的 cost 估计 + DM 选择（`WarpXRegrid.cpp:352`、`:88-140`）；
  3. 初始基础网格分解（`PostProcessBaseGrids`，`WarpXInitData.cpp:327`）。
- **不可插件化**：`RemakeLevel`、`AmrCore` 事务、guard cell、`FillBoundary`、粒子 `Redistribute`、EB 工厂生命周期、重启布局。

**结论**：`IAmrPolicy` 只能叫“AMR 策略插件”，不能叫“AMR 插件”；若用户要“新的 AMR 算法”，那是内核特性（先补动态 regrid），不是插件能解决的。

---

## 3. 架构总览

### 3.1 三层

```
┌────────────────────── Kernel（稳定内核，一次冻结）──────────────────────┐
│ 状态：MultiFabRegister / MultiParticleContainer / MultiFluidContainer    │
│ 机制：AMReX AMR、MPI、guard cell、PML 对象、IO、重启、时间步驱动骨架     │
│ 插头：PluginHost（role→name→plugin）+ Context + Services                 │
│ 开孔：13 个聚合入口各一个 if(plugin) 分派；旧函数体保留                  │
└───────────────────────────────▲─────────────────────────────────────────┘
                                │ 标准插头（SDK v1）
┌───────────────────────────────┴─────────────────────────────────────────┐
│ 官方内置插件（旧实现包一层；默认路径逐位不变）                            │
│ yee · ckc · ect · psatd · electrostatic · hybrid · pic · fluid_cold ·    │
│ moment5 · medium_tensor · pec · pml · laser · full_diag · …              │
└───────────────────────────────▲─────────────────────────────────────────┘
                                │ 同一接口
┌───────────────────────────────┴─────────────────────────────────────────┐
│ 外挂插件（用户只写这里）                                                  │
│ my_field · my_fluid · my_material · my_boundary · my_source · my_diag ·  │
│ my_amr_policy · …                                                        │
└─────────────────────────────────────────────────────────────────────────┘
```

### 3.2 内核/插件边界（一句话规则）

> **内核拥有“状态 + 生命周期 + 通信 + 步进骨架”；插件拥有“数值决策 + 物理闭包 + 离散 kernel”。**
> 判定标准：只决定“一个数/一个谓词/一组系数”的，是插件；涉及 `Alloc/Remake/Redistribute/FillBoundary` 对象生命周期的，是内核。

### 3.3 与旧设计稿的差异

| 旧设计稿 | 最终架构 | 原因 |
|---|---|---|
| `ISpeciesModel` 统一 PIC/流体/矩 | `ISpeciesModel` 组合器 + `IParticleModel`/`IFluidModel` | 状态所有权异构（C3/C4） |
| `IFieldEquation::advance(E/B)` | `IFieldSolver` 带 `advance_half_b/e` + `advance_whole_step` + caps | PSATD/Hybrid 是整步（C2） |
| `FieldView` 只给 Array4 | `FieldState` 给 `MultiFab*` 借用视图 + per-FAB `FieldView` | 一个 level 多 FAB，插件要 `MFIter` |
| `IMaterialModel` 三方法 | 系数 bundle + `rho_medium` 归还内核场 | `rho_medium` 是 WarpX 场 |
| 边界 `IBoundary` 一个接口 | `IFieldBoundary`（kernel 层）+ solver 内 PML | C7 |
| 每步一次虚调用 | 每个聚合入口一次；kernel 内零虚调用 | 性能 |
| AMR 未涉及 | `IAmrPolicy` 仅策略，明确无动态 regrid | C8/C9 |

---

## 4. 插头契约（SDK v1）

### 4.1 元数据与能力

```cpp
// Source/Plugin/PluginInfo.H（include 写作 "Plugin/PluginInfo.H"）
namespace warpx::plugin {

constexpr int api_version = 1;

enum class Role {
    FieldSolver, ParticleModel, FluidModel, SpeciesModel,
    Material, FieldBoundary, ParticleBoundary,
    Source, Diagnostic, Interaction,
    Pusher, Deposition, Gathering, Collision, Qed, Filter,
    AmrPolicy, TimeIntegrator, EmbeddedBoundary
};

struct Caps {
    bool amr = false;            // 支持多 level
    bool coarse_patch = false;   // 支持 PatchType::coarse
    bool gpu = false;            // 提供 device kernel
    bool half_step_b = false;    // 需要 B 两个半步
    bool whole_step = false;     // 自己完成整步 E/B（PSATD/hybrid/隐式）
    bool provides_current = false;
    bool requires_material = false;
    bool requires_eb = false;
};

struct PluginInfo {
    const char* name;      // "maxwell_yee_gravity"
    const char* version;   // "1.0.0"
    Role role;
    Caps caps;
    const char* description;
};

class IPlugin {
public:
    virtual ~IPlugin () = default;
    [[nodiscard]] virtual PluginInfo info () const = 0;
    virtual void on_init (InitContext&) {}
    virtual void on_level_init (int /*lev*/) {}
    virtual void on_regrid (int /*lev*/) {}
    virtual void on_finalize () {}
};

} // namespace warpx::plugin
```

**为什么要 `Caps`**：驱动要靠它决定走 FDTD 序列还是整步序列，并在初始化时校验“插件声明支持 AMR 但实际只实现了 lev 0”这类错误，避免静默错解。

**SDK 目录（Source 已在 include 根）**

```
Source/Plugin/
├─ Plugin.H              // 唯一公共入口：IPlugin/IFieldSolver/ISpeciesModel/.../TimeLayer
├─ PluginInfo.H          // Role/Caps/PluginInfo/api_version
├─ Context.H             // InitContext/StepContext/FieldState/FieldSource
├─ Services.H            // FillBoundary/SumBoundary/Parser/RNG/Print
├─ PluginHost.H          // 注册表 + 选择 + 依赖校验
├─ PluginRegistrar.H     // WARPX_REGISTER_PLUGIN
├─ Builtin/              // 官方内置插件（yee/pic/medium_tensor/...）
└─ CMakeLists.txt
```

外部插件只 include `Plugin/Plugin.H`；`Context.H`/`Services.H` 由它带入。

### 4.2 生命周期

```
静态注册（main 之前） → 读输入选择 → on_init（字段已分配）
   → [每步] role 方法
   → on_regrid(lev)（若发生 RemakeLevel/负载均衡）
   → on_level_init(lev)（新建 level；本仓库仅初始化时）
   → on_finalize（退出）
```

- 插件对象在读输入后立即构造/注册；`on_init` 发生在 `WarpXInitData` 之后：场、粒子、EB、材料都已存在。
- `on_regrid` 只在 `RemakeLevel` 成功后调用；插件必须重建所有依赖 BoxArray/DM 的缓存。
- 所有回调都是 host 侧；kernel 内不允许虚调用。

**初始化顺序（实现时必须照此）**

```
WarpX::WarpX()
  1. m_instance = this
  2. register_builtin_plugins()          // 内置插件，显式注册
  3. ReadParameters()
       a. 解析 algo.*（旧输入）
       b. 映射到 plugins.<role>（旧值 → 内置名）
       c. 解析 plugins.<role>（新输入，覆盖旧值）
       d. host().resolve()：查名字、查依赖、校验 Caps（amr/coarse/EB/复杂度）
  4. InitData()
       e. 分配场/粒子/流体/EB/材料
       f. 对每个插件 on_init(InitContext)
       g. 对每个 level 调 on_level_init(lev)
  5. Evolve() 每步调用 role 方法
  6. Finalize() → on_finalize()
```

> 注意：`on_init` 不能早于 `InitData`（字段还没分配），也不能晚于第一次 `Evolve`（插件缓存没建）。诊断/源的**对象构造**可以早于 `InitData`（工厂在构造 `MultiDiagnostics` 时需要），但它们的 `initialize()` 仍走 `on_init`。

### 4.3 Context（插件能看到的全部）

```cpp
struct InitContext {
    int max_level;
    const amrex::Vector<amrex::Geometry>& geom;
    ablastr::fields::MultiFabRegister& fields;
    MultiParticleContainer& particles;
    MultiFluidContainer& fluids;
    MaterialView material;          // 可能为空
    Services& services;
    Input& input;                   // 命名空间 "plugins.<name>."
};

struct StepContext {
    int step;
    amrex::Real time;
    amrex::Real dt;
    int finest_level;
    ablastr::fields::MultiFabRegister& fields;
    MultiParticleContainer& particles;
    MultiFluidContainer& fluids;
    MaterialView material;
    Services& services;
};
```

插件真正拿到的场视图（`Context.H`）：

```cpp
struct FieldView { amrex::Array4<amrex::Real> arr; amrex::Box bx; int ncomp; };

class FieldState {
public:
    [[nodiscard]] int lev () const noexcept;
    [[nodiscard]] ablastr::utils::enums::PatchType patch_type () const noexcept;
    [[nodiscard]] amrex::Real start_time () const noexcept;
    [[nodiscard]] ablastr::fields::VectorField Efield () const;  // 借用
    [[nodiscard]] ablastr::fields::VectorField Bfield () const;
};

class FieldSource {
public:
    [[nodiscard]] ablastr::fields::VectorField Jfield () const;  // 借用
    [[nodiscard]] amrex::MultiFab* rho () const;                 // 可能为空
};
```

**自定义状态**：`MultiFabRegister` 已支持任意字符串键（`alloc_init(name, lev, ba, dm, ncomp, ngrow, …, remake=true)`），并会在 `RemakeLevel` 时自动重分配；插件在 `on_init` 里注册自己的场（例如引力势 `phi`、新流体的高阶矩），在 `StepContext.fields` 里按名取用。粒子自定义属性走 `WarpXParticleContainer` 的 SoA 扩展，M3 提供正式 API。

**诚实说明**：Context 直接暴露 WarpX 的容器类型，因此 SDK 保证的是**源码兼容**，不是二进制 ABI 兼容；运行期 `.so` 必须与内核同版本、同编译器、同维度构建。换取的是插件能真正写 GPU kernel，而不是隔着一百个 accessor。

`Services`（host 侧粗粒度，不是 per-cell）：

```cpp
class Services {
public:
    virtual void FillBoundaryE (int lev, PatchType, int ng) = 0;
    virtual void FillBoundaryB (int lev, PatchType, int ng) = 0;
    virtual void FillBoundaryJ (int lev, int ng) = 0;
    virtual void SumBoundary (amrex::MultiFab&, int lev, int scomp, int ncomp) = 0;
    virtual amrex::Real ReduceSum (const amrex::MultiFab&, int lev, int comp) = 0;
    virtual amrex::Parser make_parser (const std::string& expr) = 0;
    virtual amrex::RandomEngine& rng () = 0;
    virtual void Print (const std::string&) = 0;
    virtual void Abort (const std::string&) = 0;
};
```

### 4.4 注册、选择、依赖

```cpp
// 用户唯一需要写的注册行
WARPX_REGISTER_PLUGIN(MyFieldSolver, "my_maxwell_gravity")
```

- 内置插件由 `register_builtin_plugins()` 显式注册，避免静态库 dead-strip。
- 输入：
  ```
  plugins.field_solver = my_maxwell_gravity
  plugins.material     = medium_tensor
  plugins.sources      = external_field, gravity
  plugins.diagnostics  = full, field_probe
  plugins.amr_policy   = gradient_tagging
  ```
- 旧输入不破坏（映射表）：

  | 旧输入 | 新键 | 内置插件 |
  |---|---|---|
  | `algo.maxwell_solver = yee/ckc/ect/psatd` | `plugins.field_solver` | 同名 |
  | `algo.em_solver_medium = macroscopic` | `plugins.material` | `medium_tensor` |
  | `warpx.do_electrostatic = 1` | `plugins.field_solver` | `electrostatic` |
  | `boundary.field_lo/hi = PEC/PMC/...` | `plugins.boundary` | `pec`/`pmc`/... |
  | `diagnostics.*` / `warpx.reduced_diags_names` | `plugins.diagnostics` | `full`/`FieldProbe`/... |
  | `algo.evolve_scheme = explicit` | `plugins.time_integrator` | `explicit_strang` |
- 依赖声明与校验：
  ```cpp
  struct Requires { std::vector<std::string> roles; }; // e.g. {"material"}
  ```
  初始化时解析；缺依赖 → abort，打印“谁需要谁、已注册什么”。
- 同名同角色重复注册 → abort；不同角色允许同名（但建议全局唯一）。
- 多个插件：`sources`/`diagnostics`/`interactions` 允许列表，按输入顺序执行；`field_solver`/`amr_policy`/`time_integrator` 只允许一个。

### 4.5 加载方式

**编译期（主路径，默认）**

```cmake
cmake -S . -B build -DWarpX_EXTRA_PLUGIN_SOURCES=/abs/my_field.cpp
```

- 编进 `app_${SD}`，注册器一定执行；
- 零 ABI 风险；GPU/AMR functor 只能走这条；
- 适合物理 kernel、需要 `amrex::ParallelFor` 的插件。

**运行期（可选，M3 之后）**

```cpp
extern "C" WARPX_EXPORT
warpx::plugin::IPlugin* warpx_plugin_entry_v1 (warpx::plugin::PluginHost&);
// 所有权转移给 host；另导出 void warpx_plugin_destroy_v1(IPlugin*) 以便卸载
```

- `dlopen` + `dlsym`；入口返回插件对象（host 侧角色）；
- 启动时校验 `api_version`、`WARPX_VERSION`、`sizeof(Real)`、`WARPX_DIMS`、GPU 编译标志；
- 只支持 host 侧角色（diagnostic/source/material/boundary/amr policy 的 host 部分）；
- 不保证跨编译器/跨标准库；文档里明确写“实验性”。

### 4.6 错误、版本、日志

- `api_version` 不匹配 → abort，打印“内核 API v1，插件要求 v2”。
- 未知插件名 → abort，列出该角色已注册名字（防拼写错误静默回退）。
- 插件抛异常 → 在 seam 处 catch 并转 `WARPX_ABORT_WITH_MESSAGE`（保持 WarpX 的 abort 风格）。
- 启动日志：每个角色打印 `role=name (version, caps)`；`verbose>0` 时打印全部可用插件。

---

## 5. 角色目录（最终分类）

### 5.1 总表

| 层 | 角色 | 内核 seam | 现有实现映射 | 难度 | 阶段 |
|---|---|---|---|---|---|
| T1 | `IFieldSolver` | `EvolveB/E`、`MacroscopicEvolveE`、`PushPSATD`、`ComputeSpaceChargeField`、`HybridPICEvolveFields` | Yee/CKC/ECT/PSATD/ES/Hybrid | 高 | M1/M2 |
| T1 | `ISpeciesModel`（组合） | `PushParticlesandDeposit(lev,…)` | PIC+cold fluid | 高 | M1 |
| T1 | `IParticleModel` | 同上（子角色） | `MultiParticleContainer` | 中 | M1 |
| T1 | `IFluidModel` | 同上（子角色） | `MultiFluidContainer` / `MomentFluidSolver` | 中 | M1/M3 |
| T1 | `IMaterialModel` | `MacroscopicProperties` 的 alloc/init/update | `medium_tensor` | 中 | M1 |
| T1 | `IFieldBoundary` | `ApplyEfieldBoundary/BfieldBoundary` | PEC/PMC/绝缘体/轴 | 低 | M1 |
| T1 | `IDiagnostic` | `MultiDiagnostics` / `MultiReducedDiags` | Full/BT/Reduced/Particle | 低 | M1 |
| T2 | `ISource` | `AddExternalFields` + 阶段钩子 | 外场（E/B 加性） | 低 | M2 |
| T2 | `ICurrentSource` | 沉积后、场推进前 | 矩流体 J、外部 J | 中 | M2 |
| T2 | `IPusher` | `PushPX` 内 | Boris/Vay/Higuera/CRR | 高 | M3 |
| T2 | `IDeposition` | `DepositCurrent/Charge` | Esirkepov/Direct/Vay/… | 高 | M3 |
| T2 | `IGathering` | `FieldGather` | EC/MC | 高 | M3 |
| T2 | `ICollisionModel` | `MultiParticleContainer::doCollisions` | 已有 `CollisionBase` 虚基类 | 低 | M2 |
| T2 | `IQedModel` | `doQEDEvents` / `doFieldIonization` | 硬编码 | 中 | M3 |
| T2 | `IFilter` | `FilterComputePackFlush` 前 | NCI Godfrey | 低 | M3 |
| T2 | `IInteraction` | 物种↔场耦合 | 摩擦/引力（部分库） | 中 | M3 |
| T3 | `IAmrPolicy` | `ErrorEst`、`CheckLoadBalance`、`PostProcessBaseGrids` | 见 §9 | 中 | M4 |
| T3 | `ITimeIntegrator` | `OneStep` 整步 | 显式/隐式 | 很高 | M4+ |
| T3 | `IEmbeddedBoundary` | `InitEB`/`InitializeEBGridData` | EB2 + cut-cell 库 | 高 | M3/M4 |
| T0 | 不可插件化 | `RemakeLevel`、`AmrCore`、guard cell、MPI、重启 | — | — | — |

### 5.2 T1 详解

#### `IFieldSolver`

```cpp
enum class FieldStage { Electromagnetic, Electrostatic, Hybrid };

class IFieldSolver : public IPlugin {
public:
    //! 是否需要在驱动里走 FDTD 序列（两个 B 半步 + E 全步）
    [[nodiscard]] virtual bool uses_half_step_b () const { return true; }

    //! FDTD 类：B 半步
    //! 非纯虚：整步类（PSATD/hybrid）默认 abort，避免被迫写空实现
    virtual void advance_half_b (StepContext&, int lev, PatchType,
                                 SubcyclingHalf) { /* default: abort */ }

    //! FDTD 类：E 全步
    virtual void advance_e (StepContext&, int lev, PatchType) { /* default: abort */ }

    //! 整步类（PSATD/hybrid/静电）：自己完成该步的场更新
    //! stage 告诉插件这是电磁/静电/Hybrid 的哪条整步路径
    virtual void advance_whole_step (StepContext&, FieldStage) {}

    //! 可选钩子：Vay/current correction
    virtual void finalize_j (StepContext&, int lev) {}
    virtual void finalize_rho (StepContext&, int lev) {}

    //! 可选钩子：div(E)/div(B) 清洁；内核按 WarpX::do_dive_cleaning/do_divb_cleaning 调用
    virtual void advance_f (StepContext&, amrex::Real /*dt*/, int /*rho_comp*/) {}
    virtual void advance_g (StepContext&, amrex::Real /*dt*/) {}

    //! 求解器内部的边界（PML/Silver-Mueller/Damped）；物理边界由 IFieldBoundary 负责
    virtual void apply_solver_boundary (StepContext&, int lev, PatchType) {}

};
```

驱动逻辑：

```
if (solver->uses_half_step_b()) {
    EvolveF(0.5*dt, 0); EvolveG(0.5*dt);          // M1 仍走内核；M2 换成 solver->advance_f/g
    solver->advance_half_b(ctx, lev, patch, FirstHalf);
    solver->advance_e(ctx, lev, patch);
    EvolveF(0.5*dt, 1); EvolveG(0.5*dt);
    solver->advance_half_b(ctx, lev, patch, SecondHalf);
} else {
    solver->advance_whole_step(ctx, stage);   // stage = Electromagnetic/Electrostatic/Hybrid
}
```

- Yee/CKC/ECT/Maxwell+gravity 实现 `advance_half_b/e`；
- PSATD/Hybrid/静电 实现 `advance_whole_step`，`uses_half_step_b=false`；
- 隐式求解器短期不做成插件（它拥有整步 + 粒子），M4 再议。
- **PML 归插件**：`advance_*` 内部调用自己的 PML stencil；`MultiSigmaBox` 由 PML 对象提供，仍属内核，但插件通过 `Context` 读取。
- **物理边界归 `IFieldBoundary`**：驱动在 `advance_*` 之后调用，顺序与旧代码一致（E 边界在 E 后；B 边界在 B 后，Silver-Mueller 只在 FirstHalf）。

#### `ISpeciesModel` / `IParticleModel` / `IFluidModel`

```cpp
class ISpeciesModel : public IPlugin {
public:
    virtual void advance (StepContext&, int lev, amrex::Real dt, SubcyclingHalf) = 0;
    virtual void deposit (StepContext&, int lev, FieldSource&) {}
    virtual void gather  (StepContext&, int lev, FieldState&) {}
};

class IParticleModel : public ISpeciesModel {
public:
    //! 用户只需实现这个；advance() 由基类桥接
    void advance (StepContext& ctx, int lev, amrex::Real dt, SubcyclingHalf h) final
    { advance_particles (ctx, lev, dt, PositionPushType::Full, MomentumPushType::Full); }

    virtual void advance_particles (StepContext&, int lev, amrex::Real dt,
                                    PositionPushType, MomentumPushType) = 0;
};

class IFluidModel : public ISpeciesModel {
public:
    void advance (StepContext& ctx, int lev, amrex::Real dt, SubcyclingHalf) final
    { advance_fluid (ctx, lev, dt); }

    virtual void advance_fluid (StepContext&, int lev, amrex::Real dt) = 0;
};
```

- 内置 `CompositeSpeciesModel`：按物种类型把调用分给 PIC/冷流体/矩流体；旧 `PushParticlesandDeposit` 的函数体原样搬进这个组合器。
- 用户写“流体+引力”：实现 `IFluidModel`，在 `advance_fluid` 里做 Strang 分裂（先引力半步、再流体 RK、再引力半步），引力场从 `IFieldSolver` 的 Poisson/引力解读取。
- 用户写“新的 PIC 物质模型”：实现 `IParticleModel`，用 `StepContext` 里的粒子容器 + `FieldSource` 描述符。
- **能力标志**必须声明 AMR 支持；冷流体/5 矩的 `caps.amr=false`，驱动在多层时直接 abort（比静默算错好）。

#### `IMaterialModel`

```cpp
struct MaterialCoefficients {
    amrex::MultiFab* eps = nullptr;      // cell 或 node，按 solver 约定
    amrex::MultiFab* sigma = nullptr;
    amrex::MultiFab* mu = nullptr;
    amrex::MultiFab* eps_tensor = nullptr;   // 可选
    amrex::MultiFab* sigma_tensor = nullptr;
    amrex::MultiFab* jcond = nullptr;
    int ncomp = 1;
};

class IMaterialModel : public IPlugin {
public:
    virtual void allocate (InitContext&, int lev) = 0;
    virtual void initialize (InitContext&, int lev) = 0;
    virtual void init_subpixel (InitContext&, int lev,
                                const amrex::EBFArrayBoxFactory*) = 0; // 可空
    virtual MaterialCoefficients coefficients (int lev) const = 0;
    virtual void advance (StepContext&, int lev) {}  // rho_medium/jcond 之类的材料演化
};
```

- `rho_medium` 仍是内核场（`Fields.H:41`），由材料插件写；这样不与 `MacroscopicProperties` 的所有权冲突。
- `init_subpixel` 在 EB 初始化后调用一次；当前代码传 `nullptr` 是已知缺陷，接口允许非空。

#### `IFieldBoundary`

```cpp
enum class Side { Low, High };

class IFieldBoundary : public IPlugin {
public:
    virtual void apply (StepContext&, int lev, PatchType, Side,
                        FieldState&, amrex::Real time, SubcyclingHalf) = 0;
};
```

- 覆盖 PEC/PMC/PEC_Insulator/axis/periodic；periodic 可保留内核（AMReX `geom.periodicity()`）。
- PML/Silver-Mueller/Damped 不在此接口，归 `IFieldSolver::apply_solver_boundary`。

#### `IDiagnostic`

```cpp
class IDiagnostic : public IPlugin {
public:
    virtual void initialize (InitContext&) = 0;
    virtual bool due (int step) const = 0;
    virtual void compute (StepContext&) = 0;
    virtual void flush (StepContext&) = 0;
    virtual void on_regrid (int lev) = 0;   // 重建 field functors
    virtual void on_restart (InitContext&) {}
};
```

- 现有 `Diagnostics` 基类已经具备 `Flush/DoComputeAndPack/DoDump/InitializeFieldFunctors`；插件化只需把 `MultiDiagnostics.cpp:22-30` 的 if/else 换成 name→factory。
- Reduced diag 是最便宜的插件：5 个方法、一个工厂 lambda。

### 5.3 T2 详解（摘要）

| 角色 | 关键点 |
|---|---|
| `ISource` | 两个族：`add_E/B`（外场）与 `add_J/rho`（电流源）；必须带 `stage`（before/after deposition / before field solve）。激光不要包成 `ISource`，它是 `IParticleModel`。 |
| `ICollisionModel` | 已有 `CollisionBase` 虚基类，只需工厂注册 + 统一物种列表（当前只能看 PIC）。 |
| `IPusher` | 每粒子 kernel，必须编译期；接口是 host 侧“选择器”，返回 device functor（`ParserIF` 模式），不能运行期 `.so`。 |
| `IDeposition`/`IGathering` | 与 shape/order 模板绑定；改造 `WarpXParticleContainer.cpp` 约 500 行，M3 再做。 |
| `IQedModel` | `doQEDEvents`/`doFieldIonization` 目前写死在 `Evolve`，先包成两个 seam。 |
| `IFilter` | `NCIGodfreyFilter` 已是类；可包成“场滤波”插件。 |
| `IInteraction` | 物种↔场耦合（摩擦/引力）；与 `ISource` 的区别：它读两个状态并写一个，需声明场方程依赖。 |

### 5.4 T3 详解

- `IAmrPolicy`：见 §9。
- `ITimeIntegrator`：拥有 `OneStep` 整步；M4 之后。显式 Strang 内置；隐式/Hybrid 先保留内核。
- `IEmbeddedBoundary`：`InitEB` 的几何输入（level set/STL）与 `InitializeEBGridData` 的 cut-cell 计算可以插件化；`EB2::Build` 和 `RemakeLevel` 仍是内核。
- `IIO`：**不建议**。写侧已由 `FlushFormat` 虚接口覆盖；读侧重启布局是事实 ABI，拆出去会破坏重启兼容。

### 5.5 不可插件化清单（必须写进文档，避免用户误解）

1. `AmrCore::InitFromScratch/regrid`、`RemakeLevel`、`ClearLevel`、`MakeNewLevelFromCoarse`。
2. guard cell 计算与所有 `FillBoundary/SumBoundary/WarpXSumGuardCells`。
3. 粒子 `Redistribute`、`defineAllParticleTiles`、buffer mask。
4. EB `EB2::Build`、`EBFArrayBoxFactory` 生命周期、`m_eb_update_*` 重分配。
5. PSATD 谱求解器对象的分配/重分配（插件可以用，但生命周期归内核）。
6. 重启/checkpoint 文件布局（`WarpXIO.cpp` + `FlushFormatCheckpoint`）。
7. MPI/FFT/内存分配。

---

## 6. 内核 seam 清单（一次性开孔）

原则：**每个聚合入口只在最前面加一个 `if (plugin)`；旧函数体逐字保留；默认路径 `plugin == builtin` 时逐位一致。**

| # | 文件:函数 | 行锚点 | 分派到 | 旧体保留 |
|---|---|---|---|---|
| S1 | `WarpXPushFieldsEM.cpp:WarpX::EvolveB(lev,patch,…)` | 972 | `field_solver->advance_half_b` | 是 |
| S2 | `WarpXPushFieldsEM.cpp:WarpX::EvolveE(lev,patch,…)` | 1027 | `field_solver->advance_e` | 是 |
| S3 | `WarpXPushFieldsEM.cpp:WarpX::MacroscopicEvolveE` | 1199/1207/1220 | `field_solver->advance_e`（带材料） | 是 |
| S4 | `WarpXPushFieldsEM.cpp:WarpX::PushPSATD` | 773 | `field_solver->advance_whole_step` | 是 |
| S5 | `WarpXSolveFieldsES.cpp:WarpX::ComputeSpaceChargeField` | 16 | `field_solver->advance_whole_step` | 是 |
| S6 | `WarpXPushFieldsHybridPIC.cpp:WarpX::HybridPICEvolveFields` | 26 | `field_solver->advance_whole_step` | 是 |
| S7 | `WarpXEvolve.cpp:WarpX::PushParticlesandDeposit(lev,…)` | 1525 | `species_model->advance/deposit` | 是 |
| S8 | `MultiParticleContainer.cpp:doCollisions` | 1221 | `collision_model->apply` | 是 |
| S9 | `WarpXFieldBoundaries.cpp:ApplyEfieldBoundary/ApplyBfieldBoundary` | 51/176 | `field_boundary->apply` | 是 |
| S10 | `WarpXInitData.cpp:WarpX::AddExternalFields` | 963 | `source->apply` | 是 |
| S11 | `MultiDiagnostics::FilterComputePackFlush` / `MultiReducedDiags::ComputeDiags` | — | `diagnostic->compute/flush` | 是 |
| S12 | `WarpX.cpp:ErrorEst` / `WarpXRegrid.cpp:CheckLoadBalance` / `WarpXInitData.cpp:PostProcessBaseGrids` | 3574/58/327 | `amr_policy->…` | 是 |
| S13 | `WarpXPushFieldsEM.cpp:MacroscopicEvolveE` 内的 `rho_medium` 段 | 1255-1370 | `material->advance` | 是（M2） |

> S7 还覆盖 `OneStep_nosub` 内联的矩流体 J 钩子（`WarpXEvolve.cpp:604-613`）：它可归 `ICurrentSource`，在 `FinalizeJ` 之后、场推进之前调用；M1 先留在内置组合器里。

> S10 的顺序：旧 `AddExternalFields` 在加完 E/B 后立刻调用 `Apply*Boundary`；拆分后驱动必须在 `source->apply` 之后保持同样的边界调用。

> S3 与 S13 的顺序：旧代码在 `MacroscopicEvolveE` 内部先推进 `rho_medium` 再调 solver。拆分后驱动必须保持“`material->advance`（S13）→ `field_solver->advance_e`（S3）”；S3 的适配器不得再重复推进 `rho_medium`，否则双重计数。

**改动量估计**：每个 seam 8–15 行，合计 ~150 行；不删任何旧代码；不碰 kernel/`.K`/粒子容器/流体容器/诊断内部。

**这就是“原有源码尽量不动”的实现方式**：不是零改动，而是**只在聚合入口开孔，旧实现整体变成内置插件的实现体**。

---

## 7. 执行顺序与组合

### 7.1 一步的调用序列（M1 目标）

```
Evolve(step):
  amr_policy->on_step?            (M4)
  species->advance/deposit        (S7; 内含 PIC gather+push + J/rho + cold fluid)
  field_solver->advance_half_b    (S1; B 半步 + PML)
  field_boundary->apply(B)        (S9)
  material->advance               (S13; rho_medium/jcond，用 E^n 与 B^{n+1/2})
  field_solver->advance_e         (S2/S3; E 全步 + PML)
  field_boundary->apply(E)        (S9)
  field_solver->advance_half_b    (S1; B 后半步)
  field_boundary->apply(B)        (S9)
  diagnostics->compute/flush      (S11)
```

PSATD/Hybrid/静电：`field_solver->advance_whole_step` 替代中间三步；驱动按 `uses_half_step_b()` 分支。

> 上表省略了 `EvolveF/G`、`FillBoundary*`、`DampPML`：`EvolveF/G` 由 `IFieldSolver::advance_f/g` 提供（内置 Yee 插件委托旧 `EvolveF/G`），`FillBoundary*`/`DampPML` 仍由内核按旧顺序执行（`WarpXEvolve.cpp:656-680`）。表只列“插件边界”。

### 7.2 组合规则

- `field_solver` 一个；`material` 0/1；`amr_policy` 0/1；`time_integrator` 0/1。
- `species_model` 一个组合器，内部按物种挂多个 `IParticleModel`/`IFluidModel`。
- `sources`/`diagnostics`/`interactions` 多个，按输入顺序执行。
- 依赖：`requires_material` 的场求解器没有材料插件时用内置 vacuum 材料；`requires_eb` 但构建无 EB → 初始化 abort。
- 冲突：同一角色重复选择 → abort；两个插件声明写同一个自定义场名 → abort。

### 7.3 能力校验（防静默错解）

初始化时对每个插件检查：
- 声明 `caps.amr=false` 但运行在 `max_level>0` → abort；
- 声明 `caps.coarse_patch=false` 但出现 `PatchType::coarse` → abort；
- 声明 `caps.gpu=true` 但构建为 CPU → 忽略/警告；
- `whole_step` 与 `half_step_b` 同时为 true → abort（语义冲突）。

---

## 8. 用户工作流（四个例子）

**先选角色**（不知道该实现哪个接口时查这张表）：

| 用户想加的能力 | 实现角色 | 参考内置插件 | 需要懂到什么程度 |
|---|---|---|---|
| 新场方程/新场求解器（Maxwell+引力、新 stencil） | `IFieldSolver` | `yee`、`psatd` | 会 `MFIter` + `Array4` + `ParallelFor` |
| 新物质模型（流体+引力、新 PIC 模型） | `IFluidModel` / `IParticleModel` | `fluid_cold` / `pic` | 懂粒子 SoA 或流体守恒量布局 |
| 新材料（ε/σ/μ、界面、损耗） | `IMaterialModel` | `medium_tensor` | 懂 `MultiFab` 系数场 |
| 新边界（PEC/PMC/自定义壁） | `IFieldBoundary` | `pec`、`pmc` | 懂场的 stagger |
| 新源项（外场、电流源） | `ISource` / `ICurrentSource` | `external_field` | 懂 E/B 或 J 的位置 |
| 新诊断（场/粒子/约化量） | `IDiagnostic` | `full_diag`、`FieldProbe` | 懂 `MultiFab` 读取 |
| 新碰撞/QED/滤波 | `ICollisionModel`/`IQedModel`/`IFilter` | 现有类 | 中 |
| 新 AMR 策略（tagging/负载均衡） | `IAmrPolicy` | 内置默认 | 懂 `TagBoxArray`/`DistributionMapping` |

> 插件作者需要懂 AMReX 的 `MultiFab`/`Array4`/`ParallelFor`；不想写 kernel 的用户应通过“参数组合 + 内置插件”完成任务，而不是写插件。

### 8.1 新场求解器：Maxwell + 引力场（一个 `.cpp`）

```cpp
#include "Plugin/Plugin.H"

using namespace warpx::plugin;

class MaxwellGravity final : public IFieldSolver {
public:
    PluginInfo info() const override {
        return {"maxwell_gravity", "1.0.0", Role::FieldSolver,
                Caps{.amr=true, .half_step_b=true}, "Maxwell + Newtonian gravity"};
    }
    void on_init(InitContext& ctx) override { /* 读 plugins.maxwell_gravity.* */ }

    void advance_half_b(StepContext& ctx, int lev, PatchType patch,
                        SubcyclingHalf half) override {
        // 1) 自己的 curl(E) kernel：MFIter + Array4 + amrex::ParallelFor
        // 2) 引力场（g = -grad phi）在这里加到 B 的推进里（若方程需要）
        // 3) 自己的 PML stencil（如果需要）
    }
    void advance_e(StepContext& ctx, int lev, PatchType patch) override {
        // 1) curl(B) - J - g_term
        // 2) 自己的 PML stencil
    }
};
WARPX_REGISTER_PLUGIN(MaxwellGravity, "maxwell_gravity")
```

```bash
cmake -S . -B build -DWarpX_EXTRA_PLUGIN_SOURCES=/abs/maxwell_gravity.cpp
./build/bin/warpx.2d inputs     # plugins.field_solver = maxwell_gravity
```

### 8.2 新物质模型：流体 + 引力源项

```cpp
using namespace warpx::plugin;

class FluidGravity final : public IFluidModel {
public:
    PluginInfo info() const override {
        return {"fluid_gravity", "1.0.0", Role::FluidModel,
                Caps{.amr=false}, "Euler fluid with gravity source"};
    }
    void advance_fluid(StepContext& ctx, int lev, amrex::Real dt) override {
        // Strang: gravity half -> Euler RK -> gravity half
        // 引力加速度从 ctx.fields 里的 phi/g 读取
    }
    void deposit(StepContext& ctx, int lev, FieldSource& src) override { /* J */ }
};
```

### 8.3 新边界

```cpp
class MyWall final : public warpx::plugin::IFieldBoundary {
public:
    PluginInfo info() const override { return {"my_wall", "1.0", Role::FieldBoundary, {}}; }
    void apply(StepContext&, int lev, PatchType, Side, FieldState&, amrex::Real, SubcyclingHalf) override;
};
```

### 8.4 新 AMR 策略

```cpp
class TagOnEGradient final : public warpx::plugin::IAmrPolicy {
public:
    PluginInfo info() const override { return {"tag_E", "1.0", Role::AmrPolicy, {}}; }
    void tag_cells(int lev, amrex::TagBoxArray& tags, amrex::Real time) override;
    void costs(int lev, amrex::LayoutData<amrex::Real>& c) override;
};
```

> 注意：在当前仓库，`tag_cells` 只在初始化时调用一次；动态 regrid 是内核特性，不是插件能力（§9）。

---

## 9. AMR 专项

### 9.1 接口

```cpp
class IAmrPolicy : public IPlugin {
public:
    virtual void configure (InitContext&) = 0;

    //! 初始基础网格分解（可返回 false 表示用 AMReX 默认）
    virtual bool base_grids (amrex::BoxArray&, const amrex::Geometry&, int nprocs,
                             const amrex::IntVect& max_grid_size) { return false; }

    //! tagging（初始化时调用；未来动态 regrid 时复用）
    virtual void tag_cells (int lev, amrex::TagBoxArray&, amrex::Real time) = 0;

    //! 负载均衡：成本估计
    virtual void costs (int lev, amrex::LayoutData<amrex::Real>&) {}

    //! 负载均衡：给出新 DM（nullopt = 不做）
    virtual std::optional<amrex::DistributionMapping>
    distribute (int lev, const amrex::LayoutData<amrex::Real>& costs,
                const amrex::DistributionMapping& current) { return std::nullopt; }
};
```

### 9.2 能做什么 / 不能做什么

| 能力 | 现状 | 插件化 |
|---|---|---|
| tagging 规则 | `ErrorEst` 只支持 `fine_tag_lo/hi` + `ref_patch_function` | ✅ `tag_cells` |
| 基础网格分解 | `PostProcessBaseGrids` 均分 | ✅ `base_grids` |
| 负载均衡 cost/DM | SFC/Knapsack + heuristic | ✅ `costs`/`distribute` |
| 动态 regrid | **不存在** | ❌ 先补内核 |
| 新 level 创建 | `MakeNewLevelFromCoarse` abort | ❌ 内核 |
| 子循环策略 | 仅 2 层、ref ratio 2 | ❌ 内核 |

**结论**：把 `IAmrPolicy` 定位为“策略插件”，不要宣传“插件实现 AMR”。如果用户需要新的 AMR 算法（例如按场梯度动态加密），先做内核特性 `dynamic regrid`，再让策略插件决定“何时/哪里加密”。

### 9.3 GPU 约束

`ErrorEst` 是 `ParallelFor` over tags；`tag_cells` 的**实现**必须能 launch kernel。接口本身是 host 侧虚调用（每个 level 一次），没问题；但 `tag_cells` 内部不能依赖运行期 `.so` 的 device 代码。**AMR 策略插件必须编译期注册。**

---

## 10. 迁移路线与验收

### 10.1 阶段

| 阶段 | 内容 | 验收 | 估时 |
|---|---|---|---|
| **M0** | SDK（PluginInfo/Context/Services/Host/Registrar）+ 13 个 seam + `plugins.*` 输入 + 启动日志；默认全 legacy | 现有测试全过；`plugins.field_solver=legacy` 与默认逐位 | 2–3 周 |
| **M1** | 内置插件：`yee`（真空 + 宏观 E）、`ckc`、`ect`、`pic`、`fluid_cold`、`medium_tensor`、`pec`、`full_diag`；A/B 逐位 | P0/E3/S5/E6 + 真空 ECT + 上游 ctest；yee vs legacy 逐位 | 6–8 周 |
| **M2** | `psatd`、`electrostatic`、`hybrid` 场插件；reduced diag；外场 source；collision 适配 | PSATD/ES/Hybrid 回归逐位或 ≤1e-12 | 3–4 月 |
| **M3** | `moment5`、激光粒子模型、pusher/deposition/gather、QED、filter、运行期 `.so` | 各角色 A/B；外挂示例可编 | 3–4 月 |
| **M4** | `IAmrPolicy`（tagging/LB/base grid）、`ITimeIntegrator`、EB 几何策略 | AMR/负载均衡回归 | 3–6 月 |

### 10.2 验收原则

1. **同一二进制 A/B 逐位**：`plugins.<role> = <builtin>` vs `legacy`；比较探针 ASCII 和 plotfile payload 字节。
2. **跨二进制数值门**：对旧二进制，相对误差 ≤1e-12；σ=0 等简单路径要求逐位。
3. **能力门**：声明 `caps.amr=false` 的插件在多层输入下必须 abort，不能静默算错。
4. **外挂门**：一个用户 `.cpp` 不改内核、只加 `-DWarpX_EXTRA_PLUGIN_SOURCES` 能编过并跑通。
5. **回归门**：现有 `ctest` 子集（忽略 checksum）+ P0/E3/S5/E6。

> **门覆盖陷阱**：P0/E3/S5/E6 走 `MacroscopicEvolveE`，不经过 `EvolveE`。如果 M1 的 `yee` 插件把宏观 E 分支也实现进 `advance_e`（S3），它们就同时覆盖 E 和 B；如果 M1 先不做 S3，它们只覆盖 **B 半步**。无论哪种，E 分派都必须另用真空/ECT 算例（`c02_0_pec/inputs_2d`、圆柱）和上游 `langmuir` ctest 证明；验收报告必须列覆盖矩阵，否则会出现“全绿但 E 没测”的假阳性。

### 10.3 每个阶段的提交边界

- 一个角色一个 commit；seam 与内置插件分开提交；
- 每个内置插件必须带“旧路径 vs 插件路径”的 A/B 脚本；
- 文档：`PLUGIN_<ROLE>_IMPL_LOG.md` 记录接口、seam、A/B 数字。

---

## 11. 性能、GPU、ABI

### 11.1 性能

- 虚调用只在 host 侧聚合入口：每步 ~10 次，每次 ~ns 级；相对全网格 curl 可忽略。
- kernel 内零虚调用、零 `std::function`；`FieldView` 是 POD，直接 `ParallelFor` 捕获。
- 不为插件引入额外 `FillBoundary`；顺序与旧代码一致。
- 插件不得在 per-cell 循环里调用 `Services`（文档写死）。

### 11.2 GPU

- 插件 kernel 用 `amrex::ParallelFor`/`amrex::For`，与现有代码相同；
- 运行期 `.so` 不保证 device 代码；GPU 插件编译期注册；
- `Caps.gpu` 仅用于日志/校验，不改变 ABI。

### 11.3 ABI

- 编译期：无 ABI 问题（同一构建）。
- 运行期：要求同 `WARPX_VERSION`、同编译器 major、同 `sizeof(Real)`、同 `WARPX_DIMS`、同 GPU/CPU；启动时校验，失败 abort。
- 不承诺跨编译器/跨标准库；文档明确“实验性”。

---

## 12. 风险与不做什么

| # | 风险 | 缓解 |
|---|---|---|
| R1 | 13 个 seam 仍算“改内核” | 一次性；旧体保留；默认逐位；冻结后版本化 |
| R2 | 接口设计过度，用户仍要读 WarpX 内部 | 提供 `examples/plugins/` 四个模板 + 最小 Context |
| R3 | PSATD/隐式的时间分裂无法用 `advance_E/B` 表达 | `advance_whole_step` + `Caps.whole_step`；M2 再做 |
| R4 | 冷流体/5 矩能力不对称，用户静默算错 | `Caps.amr/coarse_patch` 初始化校验 + abort |
| R5 | 运行期 `.so` ABI 崩溃 | 默认关闭；版本/编译器/维度校验；只支持 host 角色 |
| R6 | AMR 被过度承诺 | §9 明确“无动态 regrid”；`IAmrPolicy` 只做策略 |
| R7 | 插件生态碎片化 | 官方内置插件即参考实现；接口 v1 冻结；契约测试 |
| R8 | 全量迁移拖太久 | 按 M0–M4 分阶段；M1 就能交付用户价值 |

**明确不做**：
- 不做“无内核改动的纯外挂”（物理上不可能，除非复制整个 WarpX）。
- 不做“运行期替换 GPU kernel”。
- 不做“插件实现新的 AMR 机制”。
- 不做“重写 WarpX 为微内核”。

---

## 13. 工作量（人月，粗估）

| 项 | 人月 |
|---|---|
| M0 SDK + seams | 0.5–0.75 |
| M1 场/物质/材料/边界/诊断 + A/B | 1.5–2 |
| M2 PSATD/ES/Hybrid + 诊断/源/碰撞 | 3–4 |
| M3 粒子子角色 + QED + 运行期加载 | 3–4 |
| M4 AMR 策略 + 时间积分 + EB | 3–6 |
| **合计（全量，含缓冲）** | **12–18** |
| **MVP（M0+M1）** | **2–2.75** |

> 与 2026-11 的架构审阅（12–21 人月）同量级；差别是本方案把“场/物质/材料/边界/诊断”先落地，AMR/时间积分放最后，避免一开始就卡在最大重构上。

---

## 14. 待确认（进入 M0 前）

1. **接口粒度**：`IFieldSolver` 是否接受 `advance_half_b` / `advance_e` / `advance_whole_step` 三入口？还是要求“整步一个入口”（更统一，但 FDTD 的 B 半步序列要挪进插件）？
2. **物种接口**：是否接受 `ISpeciesModel` 组合器 + `IParticleModel`/`IFluidModel` 两层？还是只保留一个 `ISpeciesModel`（会把 5 矩/冷流体的异构藏进实现）？
3. **AMR 范围**：是否接受“M4 只做 tagging/load-balance/base-grid 策略，不做动态 regrid”？若要动态 regrid，需单独立项（内核特性）。
4. **运行期加载**：是否真的需要 `.so`？若只是“用户不重编内核”，编译期 `-DWarpX_EXTRA_PLUGIN_SOURCES` 已满足；运行期可作为 M3 可选项。
5. **内核开孔**：是否接受 13 个 seam、~150 行的**一次性**内核改动？这是“用户只写一个模块”的最低成本；零改动方案不存在。

---

## 15. 一句话

**内核 = 状态 + 生命周期 + 通信 + 步进骨架；插件 = 数值决策 + 物理闭包 + 离散 kernel。内核只在 13 个聚合入口开一次孔（旧体保留、默认逐位），之后冻结；用户实现 `IFieldSolver`/`IFluidModel`/`IMaterialModel`/`IFieldBoundary`/`IDiagnostic`/`IAmrPolicy` 之一，写一个 `.cpp`，用 `plugins.<role>=<name>` 接上——不改正旧源码。AMR 只插件化策略，不插件化机制；动态 regrid 是内核特性，不是插件能力。**
