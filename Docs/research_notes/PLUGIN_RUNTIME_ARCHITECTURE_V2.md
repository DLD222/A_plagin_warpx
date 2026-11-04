# 插件化运行时架构 V2：通用内核、数据总线与协作调度

> 本文是对 `PLUGIN_ARCHITECTURE_FINAL.md` 的修正版。修正的原因：上一版仍把 `WarpX` 这个“物理类”当内核，把 E/B/J/rho 写进接口；这违反“内核不绑定任何具体物理”。
> 依据：WarpX 源码逐文件核对 + 5 个成熟框架的对照（MOOSE、Cordis/DSH、Gkeyll、preCICE、AMReX）。
> 结论先行：**主程序应是一个新的 `SimulationRuntime`（运行时内核），不是 `WarpX` 类；`WarpX` 降级为“标准 PIC/EM 物理包”。内核只认“命名状态 + 阶段 + 数据依赖”，不认 E/B/粒子/材料。**

---

## 0. 直接回答三个问题

### Q1：用什么作为主程序？

**`warpx_runtime`（`SimulationRuntime`）**：一个不包含任何物理方程的可执行程序。

```
main()
 └─ Runtime::run()
      ├─ 初始化 AMReX（网格/并行/GPU）
      ├─ 读取输入（内核部分 + 插件配置）
      ├─ 构建 StateSchema（场/物种/材料/辅助量的命名表）
      ├─ 加载 PhysicsPackage（标准 PIC/EM 包 = 一组插件 + 阶段计划）
      ├─ 加载用户插件（编译期或运行期）
      ├─ PluginHost::resolve()（依赖、能力、数据键校验）
      ├─ 初始化状态（IO/restart 或初值插件）
      ├─ Scheduler::run()（阶段循环）
      └─ Finalize（checkpoint/诊断 flush）
```

`WarpX` 类不再出现在 `main` 里，而是变成 `PicEmPackage` 的实现体：它注册 `maxwell_yee`、`pic`、`medium`、`pec`、`full_diag` 等插件，并提供标准阶段序列。**用户新增能力 = 新增一个插件，挂到某个阶段；不改 `main`、不改 `Runtime`、不改 `WarpX`。**

```cpp
// Source/main.cpp（目标形态）
int main (int argc, char* argv[])
{
    warpx::runtime::Runtime runtime(argc, argv);   // AMReX + 输入 + PluginHost
    runtime.load_package<warpx::pic::PicEmPackage>();
    runtime.load_plugins_from_input();             // plugins.<role> = <name>
    runtime.initialize();                          // schema/resolve/on_init
    runtime.run();                                 // Scheduler
    runtime.finalize();
}
```

过渡期可以保留 `WarpX::GetInstance()` 作为 `Runtime` 的一个 facade（见 §9），但目标形态是上面的运行时。

### Q2：WarpX 最核心、最普适、最通用的是什么？

分三层：**L0 已存在、L1 需新建，这两层是通用底座；L2 全部是插件**。

| 层 | 内容 | 是否通用 | 说明 |
|---|---|---|---|
| **L0 执行底座** | AMReX：`AmrCore`、`BoxArray`/`DistributionMapping`、MPI 域分解、OpenMP/GPU 调度、`FillBoundary`/`SumBoundary`、`ParticleContainer`/`ParticleTile`、粒子排序/迁移、EB 几何框架、Arena 内存、plotfile | **是** | 这些是 WarpX 和所有 AMReX 应用共享的；不要重写 |
| **L1 运行时骨架** | 命名状态注册表（场/物种/材料/辅助量）、阶段调度器、数据总线、插件宿主、checkpoint 框架、服务（parser/RNG/FFT/归约/日志） | **是（但 WarpX 现在没有，需要新建）** | 这是本文的核心；它只认“名字 + 数据依赖”，不认物理 |
| **L2 物理包** | Maxwell/Yee/PSATD、PIC push/deposit/gather、冷流体/矩流体、材料 ε/σ、PEC/PML、激光、QED、诊断实现 | **否** | 全部下沉为插件；标准 PIC/EM 包只是“默认组合” |

**关键纠正**：
- `WarpX` 类不是通用内核；它是 L2 的“标准 PIC/EM 包”。
- `FieldType`（76 项）不是通用枚举；它是 L2 的 schema。
- `MultiParticleContainer`/`WarpXFluidContainer` 不是通用容器；通用的是 AMReX 的 `ParticleContainer` + 命名物种表。
- EB 的**几何框架**（`EB2::Build`、`EBFArrayBoxFactory`、level set 查询）是 L0；**cut-cell 算法**（面积/体积分数、ECT、共形 E）是 L2 插件。
- AMR 的**机制**（层级、BA/DM、通信）是 L0；**策略**（tagging、负载均衡、基础网格）是 L2，但**动态 regrid 目前内核缺失**（见 §7）。

### Q3：多物理插件如何协作？

**插件之间零直接依赖、零直接调用。** 协作只有两条通道：

1. **共享数据总线**（DataBus）：命名场、命名物种、命名材料系数、命名标量；带 schema（分量数、stagger、ghost、单位）与读写权限。
2. **主循环阶段**（Stages）：内核定义标准阶段；插件声明“我在哪个阶段、读哪些键、写哪些键”；内核按数据依赖做拓扑排序，并检测环/冲突。

这等价于 MOOSE 的“Objects 只通过 Variables/Materials 交互 + 执行顺序由依赖决定”，也等价于 Cordis/DSH 的“`inject` 服务 + `ctx.on` 事件 + 生命周期托管”。**不是**“插件 A 拿插件 B 的指针”。

---

## 1. 为什么上一版不够：三个反例

| 反例 | 上一版的问题 | V2 的修正 |
|---|---|---|
| 用户要“介质气化” | 接口里写死 `Efield()/Bfield()`；气化需要 `T`、`phase_fraction`、`mass_source`，上一版没有通用数据键 | 通用 `FieldKey`/`ScalarKey`；气化插件自己注册 `phase_fraction`、`mass_source` |
| 用户要“化学反应” | 上一版只有 `ICollisionModel`，把化学硬塞进碰撞；反应需要读多个物种密度、写反应率/能量 | 独立 `IChemistry` 插件，声明 `consumes/produces`，挂在 `AfterDeposit` |
| 用户要“新 AMR” | 上一版说“只做策略、不做动态 regrid”，但用户要的正是动态加密 | 动态 regrid 升级为**内核特性**（L0 扩展），`IAmrPolicy` 只决定“何时/哪里” |

**根本教训**：接口不能以“物理量”命名（E/B/J/rho），只能以“数据角色 + 数据键”命名；否则每加一种物理就要改接口。

---

## 2. 研究对照：成熟框架怎么做

| 框架 | 核心机制 | 对本项目的启示 | 不照搬的地方 |
|---|---|---|---|
| **MOOSE**（INL，多物理 FE） | 所有物理对象（Kernel/BC/Material/AuxKernel）注册进 `Factory`/`Warehouse`；对象之间不直接调用，只通过 `Variable` 与 `Material` 属性交互；执行顺序由对象声明和 `execute_on` 标志决定 | **最接近本项目的目标**：角色目录 + 共享数据 + 依赖排序；`Material` 就是“材料系数总线” | MOOSE 是 FE/连续介质，没有 PIC 粒子；不能照搬其变量/残差抽象 |
| **Cordis / DSH**（DeepSeek Harness 的插件框架） | 插件声明 `inject: ['service']`；通过 `Context` 注入服务；用 `ctx.on(event)` 订阅事件；`Fiber` 负责生命周期与清理；插件之间通过注入的服务/事件交互，不持有彼此实例 | **运行时插件宿主**的范本：依赖注入 + 事件总线 + 生命周期；正好对应“插件只认内核与共享数据” | JS 动态类型/运行期加载容易；C++ 需要 C ABI + 版本校验，不能照搬热插拔 |
| **Gkeyll**（等离子体 DG） | `App` 拥有网格/物种/场；`Updater` 对象只做“给定输入数组 → 计算 RHS/advance”；`Equation` 描述方程；App 负责调用顺序 | **状态/算法/序列分离**：Updater 不拥有全局状态，只吃数组；对应“插件只吃 DataBus 视图” | Gkeyll 2.0 的 App 是手写序列，不是通用调度器；可借鉴其“Updater 纯函数化” |
| **preCICE**（多物理耦合库） | 各求解器通过 adapter 注册；数据在耦合界面做 mapping；求解器彼此不知道对方存在 | **跨包耦合**的范本：数据契约 + 适配器；对应“包与包之间也只通过数据总线” | preCICE 是跨进程/跨代码耦合，不是单进程内插件；本项目先做进程内 |
| **AMReX / WarpX** | `MultiFabRegister` 已是字符串键；`AmrCore` 提供层级/通信；`ParticleContainer` 提供物种容器 | **L0 已经存在**，不要重写；把 `FieldType` 降为包级 schema | WarpX 的 `WarpX` 类是上帝类，不能当内核 |

**共同模式**（5 个框架都成立）：

1. **注册表/工厂**：对象按名字注册，输入选名字。
2. **依赖注入**：对象只声明“我需要什么服务/数据”，不自己 new 别的对象。
3. **共享数据**：对象之间不直接调用，通过命名数据/属性交互。
4. **生命周期**：初始化、每步、regrid、finalize 都有统一回调。
5. **顺序由框架定**：要么固定阶段，要么依赖排序；绝不由插件互相调用决定。

**结论**：用户提出的“共享数据总线 + 标准钩子”方向是对的；需要补的是**数据 schema/依赖声明**和**包级阶段计划**，而不是一张固定死的钩子表。

---

## 3. 内核（Runtime）的最终组成

```
SimulationRuntime
├─ MeshSubstrate        // AMReX AmrCore 包装：level/BA/DM/geom/refRatio/periodicity
├─ StateSubstrate
│   ├─ FieldRegistry    // 命名场：FieldKey -> MultiFab (ncomp, nodal, ngrow, checkpoint)
│   ├─ SpeciesRegistry  // 命名物种：ParticleSpecies / FluidSpecies（不透明句柄 + 能力）
│   ├─ MaterialRegistry // 命名系数：MaterialKey -> MultiFab/标量
│   └─ AuxRegistry      // 命名标量/小数组（T、phase_fraction、reaction_rate…）
├─ DataBus              // 上述注册表的只读/可写视图 + schema + 访问审计
├─ Scheduler            // Stage + Handler + 数据依赖拓扑排序 + 冲突检测
├─ PluginHost           // 注册/选择/依赖注入/版本/生命周期
├─ Services             // FillBoundary/SumBoundary/Reduce/Parser/RNG/FFT/Poisson/Log/Profile
├─ IOSubstrate          // checkpoint/restart（按 schema 遍历）+ plotfile/openPMD 调度
└─ EbSubstrate          // AMReX EB 几何查询 + cut-cell 数据接口（算法在插件）
```

### 3.1 内核**只**知道的类型

```cpp
using FieldKey    = std::string;   // "E_x", "rho", "phi_gravity", "mass_source"
using SpeciesKey  = std::string;   // "electron", "ion", "gas"
using MaterialKey = std::string;   // "eps", "sigma", "mu"
using ScalarKey   = std::string;   // "T_e", "phase_fraction"

struct FieldSpec {
    int ncomp;
    amrex::IntVect nodal;     // stagger
    amrex::IntVect ngrow;
    bool checkpoint = true;
    std::string unit;         // 仅元数据
};

struct SpeciesSpec {
    enum class Kind { Particles, Fluid };
    Kind kind;
    int ncomp;                // 粒子 SoA 分量 / 流体守恒量分量
    std::vector<std::string> components; // "rho","mx","my","mz","E"
    bool checkpoint = true;
};
```

**内核里没有 `Efield`、没有 `rho`、没有 `Boris`、没有 `Yee`。** 这些名字由标准 PIC/EM 包注册。

### 3.2 数据总线

```cpp
class DataBus {
public:
    //! 只读视图；不存在或类型不匹配 -> abort（带键名与请求者）
    [[nodiscard]] const amrex::MultiFab& field (FieldKey, int lev) const;
    [[nodiscard]] amrex::MultiFab&       field (FieldKey, int lev);       // 写
    [[nodiscard]] const amrex::MultiFab& material (MaterialKey, int lev) const;
    [[nodiscard]] amrex::Real            scalar (ScalarKey) const;
    [[nodiscard]] SpeciesHandle          species (SpeciesKey);   // 不透明句柄
    [[nodiscard]] const FieldSpec&       spec (FieldKey) const;  // schema 查询

    //! 注册（仅 on_schema 阶段允许）
    void register_field (FieldKey, FieldSpec);
    void register_species (SpeciesKey, SpeciesSpec);

    //! 访问审计：谁读了谁、谁写了谁；用于依赖校验与调试
    [[nodiscard]] const AccessLog& access_log () const;
};
```

- 所有键在 `on_schema` 阶段注册完毕；之后只读/写，不再增删。
- 写权限按 handler 的 `produces` 集合校验：没声明写却写 → abort。
- 单位/分量在 schema 里，诊断/IO 可自动生成输出。

### 3.3 阶段（Stages）与处理器（Handlers）

内核定义**标准阶段名**（不是固定顺序执行；顺序由包/依赖决定）：

```
OnSchema, OnInitialize, BeforeStep, BeforeDeposit, AfterDeposit,
BeforeFieldSolve, AfterFieldSolve, BeforeParticlePush, AfterParticlePush,
BeforeDiagnostics, AfterStep, OnRegrid, OnCheckpoint, OnFinalize
// 包可以定义子阶段（如 Collisions、Chemistry、Sources）；
// 内核只保证标准阶段的先后，不解释子阶段的物理含义
```

每个插件在注册时声明：

```cpp
struct HandlerSpec {
    std::string name;          // "vaporization"
    Stage stage;               // AfterDeposit
    int priority = 0;          // 同阶段、无数据依赖时的次序
    std::vector<FieldKey>    reads_fields,  writes_fields;
    std::vector<SpeciesKey>  reads_species, writes_species;
    std::vector<MaterialKey> reads_material, writes_material;
    std::vector<ScalarKey>   reads_scalars,  writes_scalars;
};
```

调度器在 `resolve()` 时：

1. 按阶段分组；
2. 同阶段内按“写 → 读”建边（A 写 X，B 读 X ⇒ A 先于 B）；
3. 拓扑排序；有环 → abort，打印环上的 handler；
4. 两个 handler 同阶段写同一个键且无先后声明 → abort（冲突）；
5. `priority` 只用于无依赖的并列项，保证确定性。

**为什么不是一张固定钩子表**：化学既可能要在碰撞前（电离平衡）也可能在碰撞后（非平衡弛豫），气化既可能在沉积前也可能在沉积后；固定顺序无法覆盖所有组合。**标准阶段给“粗粒度顺序”，数据依赖给“同阶段内的细粒度顺序”**，两者合起来既稳定又可变。

### 3.4 服务（Services）

```cpp
class Services {
public:
    virtual void FillBoundary (FieldKey, int lev, int ng) = 0;
    virtual void SumBoundary  (FieldKey, int lev, int scomp, int ncomp) = 0;
    virtual amrex::Real ReduceSum (FieldKey, int lev, int comp) = 0;
    virtual amrex::Parser Parser (const std::string&) = 0;
    virtual Rng& Rng () = 0;                       // 可复现
    virtual void SolvePoisson (FieldKey rho, FieldKey phi, int lev) = 0;  // 线性求解服务
    virtual void Fft (...);                        // PSATD 用
    virtual void Log (Level, const std::string&) = 0;
    virtual void Profile (const std::string&) = 0;

    //! 临时内存：底层是 AMReX Arena；内核不新造内存池
    virtual amrex::MultiFab* alloc_temp (int lev, int ncomp, amrex::IntVect ng) = 0;
    virtual void free_temp  (amrex::MultiFab*) = 0;
    virtual void Abort (const std::string&) = 0;
};
```

服务是**内核能力**，不是物理；插件用服务，不自己起 MPI/FFT。**内存不需要新造池**：AMReX 的 `Arena` 已经管设备/主机内存；内核只提供 `alloc_temp` 的薄封装与统一错误处理。

---

## 4. 插件契约 V2

### 4.1 元数据

```cpp
struct PluginInfo {
    const char* name;         // "vaporization"
    const char* version;      // "1.0.0"
    Role role;                // Chemistry / Material / FieldSolver / ...
    std::vector<std::string> requires;   // 依赖的服务或角色，例如 {"poisson"}
    std::vector<HandlerSpec> handlers;   // 可挂多个阶段
    int api_version = 1;
};

class IPlugin {
public:
    virtual ~IPlugin () = default;
    [[nodiscard]] virtual PluginInfo info () const = 0;

    virtual void on_schema   (SchemaBuilder&) {}   // 注册数据键
    virtual void on_init     (InitContext&)   {}   // 分配私有缓存
    virtual void on_stage    (StageContext&)  {}   // 由 handler 绑定
    virtual void on_regrid   (int lev)        {}
    virtual void on_checkpoint (CheckpointWriter&) {}
    virtual void on_restart  (CheckpointReader&)   {}
    virtual void on_finalize () {}
};
```

- **没有 `WarpX&` 参数**；只有 `StageContext`（DataBus + Services + Time + Geometry）。
- 插件不持有别的插件指针；需要的数据从 DataBus 按 key 取。
- 插件私有缓存（如 stencil、FFT plan）在 `on_init`/`on_regrid` 建。

### 4.2 阶段上下文

```cpp
struct StageContext {
    int step;
    amrex::Real time, dt;
    int finest_level;
    DataBus& data;
    Services& services;
    const MeshView& mesh;
    // 只给“本阶段、本 handler”声明的视图；越权访问 abort
};
```

### 4.3 生命周期与顺序

```
register（静态/动态）
  → on_schema（注册键）
  → resolve（查依赖/排序/校验）
  → on_init（字段已分配）
  → [每步] 每个 stage：按拓扑序调用 handler 的 on_stage
  → on_regrid（RemakeLevel 后）
  → on_checkpoint / on_restart
  → on_finalize
```

### 4.4 加载

- **编译期（默认）**：`WARPX_REGISTER_PLUGIN` + `warpx_runtime` 链接；GPU/AMR functor 必须走这条。
- **运行期（可选）**：`.so` 导出 C ABI `warpx_plugin_entry_v2`，运行时校验 `api_version`、`WARPX_VERSION`、`sizeof(Real)`、`WARPX_DIMS`、GPU 标志；只允许 host 侧 handler（chemistry/material/source/diagnostic），不允许 device kernel。
- 两种方式共用同一个 `PluginHost` 与同一套 schema 校验。

### 4.5 能力与校验

```cpp
struct Caps {
    bool amr = false;          // 支持多 level
    bool regrid = false;       // 支持 on_regrid
    bool checkpoint = false;   // 支持序列化
    bool gpu = false;
    bool whole_step = false;   // 场求解器整步
};
```

`resolve()` 检查：声明 `amr=false` 但 `max_level>0` → abort；声明 `checkpoint=true` 但没实现 `on_checkpoint` → abort；同阶段写冲突 → abort。

---

## 5. 标准 PIC/EM 包（把现有 WarpX 装进去）

**包 = 一组插件 + 一份阶段计划 + 一份 schema**；内核只认 `IPhysicsPackage`：

```cpp
class IPhysicsPackage {
public:
    virtual ~IPhysicsPackage () = default;
    virtual void declare_schema (SchemaBuilder&) = 0;   // 注册 E/B/J/rho、species、material
    virtual void declare_stages (StagePlan&) = 0;       // 选阶段顺序（可多家包叠加）
    virtual void register_plugins (PluginHost&) = 0;    // 注册内置插件
};
```

多个包（例如 PIC/EM + 化学包）时，`StagePlan` 合并；同阶段顺序由显式 priority 与数据依赖共同决定；包之间同样只通过 DataBus 交互。

`PicEmPackage` 是一个“包”，不是内核：

```cpp
class PicEmPackage : public IPhysicsPackage {
public:
    void declare_schema (SchemaBuilder&) override {
        // 注册 E_x..B_z, J_x.., rho, rho_medium, F, G, ...
        // 注册 species: electron/ion/laser/...
        // 注册 material: eps, sigma, mu
    }
    void declare_stages (StagePlan&) override {
        // 标准显式 Strang 序列：
        // BeforeDeposit → AfterDeposit → BeforeFieldSolve → AfterFieldSolve → ...
    }
    void register_plugins (PluginHost&) override {
        host.add<YeeFieldSolver>("yee");
        host.add<PicSpecies>("pic");
        host.add<MediumTensor>("medium_tensor");
        host.add<PecBoundary>("pec");
        host.add<FullDiagnostic>("full_diag");
        ...
    }
};
```

- 现有 `WarpX::Evolve` 的 250 行序列，搬进 `PicEmPackage::declare_stages` + 各插件的 handler；
- 现有 `WarpX` 的成员（`m_fields`、`mypc`、`m_fdtd_solver_*`、`m_macroscopic_properties`）逐步变成 DataBus 里的键 + 插件私有缓存；
- 过渡期 `WarpX` 仍可作为 `PicEmPackage` 的实现体（见 §9），但不再出现在 `main`。

**这样“原有源码尽量不动”**：第一版包可以直接在 handler 里调用现有 `WarpX::EvolveB/E` 等函数；内核和插件接口先立起来，物理实现逐步搬。

---

## 6. 五个场景的插件组合（证明“互不认知”也能协作）

### 6.1 介质内部场

```
MaterialPlugin "medium_tensor"
  produces: material.eps, material.sigma
  stage: OnInitialize / BeforeFieldSolve

FieldSolverPlugin "yee"
  consumes: material.eps, material.sigma, field.J
  produces: field.E, field.B
  stage: BeforeFieldSolve / AfterFieldSolve
```

无直接调用；内核保证 material handler 先于 field handler（写→读依赖）。

### 6.2 介质气化（相变）

```
MaterialStatePlugin "phase"
  produces: scalar.T, scalar.phase_fraction, material.eps_eff
  stage: AfterDeposit

VaporizationPlugin "vaporization"
  consumes: scalar.T, scalar.phase_fraction
  produces: field.mass_source, scalar.phase_fraction (写), scalar.latent_heat
  stage: AfterDeposit

GasSpeciesPlugin "gas"
  consumes: field.mass_source
  produces: species.gas (density), field.rho
  stage: AfterDeposit
```

- 气化插件不知道“气体物种插件”存在，只写 `mass_source`；
- 气体插件不知道“气化插件”存在，只读 `mass_source`；
- 内核按 `mass_source` 的写→读依赖排序；
- 相变需要的 `T` 由材料状态插件提供，不需要改场求解器。

### 6.3 化学反应

```
ChemistryPlugin "chem"
  consumes: species.electron, species.ion, species.neutral, scalar.T
  produces: field.reaction_rate, scalar.heat_source, species.new_species
  stage: Chemistry (AfterDeposit)
```

- 反应速率写进 `reaction_rate`，源项插件/物种插件按需读取；
- 若反应改变电荷，写 `field.rho`/`field.J`，场求解器下一阶段自然读到；
- 不需要 `ICollisionModel` 与化学互相认识。

### 6.4 自适应网格

```
AmrPolicyPlugin "tag_E"
  consumes: field.E
  produces: tags
  stage: OnRegrid (decision)
```

但**动态 regrid 本身是内核特性**（当前缺失）：

- 内核新增 `Runtime::regrid()`：`AmrCore::regrid` + `RemakeLevel` 通用化 + 所有注册场的重分配 + 插件 `on_regrid`；
- `IAmrPolicy` 只回答“哪些 cell 加密、何时、负载均衡怎么分”；
- 插件必须在 `on_regrid` 里重建自己的 level-dependent 缓存；
- 这是“内核扩展”，不是“插件能凭空实现 AMR”。见 §7。

### 6.5 新边界算法

```
BoundaryPlugin "my_wall"
  consumes: field.E, field.B
  produces: field.E, field.B (ghost/halo)
  stage: AfterFieldSolve
```

- 通用边界只碰命名场，不需要知道求解器；
- 求解器内部边界（PML/Silver-Mueller）作为场求解器插件的一部分，不暴露给通用边界；
- 边界插件声明 `caps.amr/coarse_patch`，内核校验。

---

## 7. AMR：内核特性 vs 策略插件

### 7.1 现状（源码事实）

- `regrid_int` 解析后**无人读**；`MakeNewLevelFromCoarse` **abort**；`RemakeLevel` 只接受同 `BoxArray` 的换 DM；子循环只支持 2 层。
- 所以今天**没有动态 regrid**；任何“自适应网格插件”都是空话。

### 7.2 内核必须补的三件事

1. **动态 regrid 事务**：`AmrCore::regrid` + 新 level 创建/销毁 + 所有注册场按 schema 重分配 + 粒子重分布 + 插件 `on_regrid`。
2. **通用 `RemakeLevel`**：不再假设同 BoxArray；按 `FieldSpec` 遍历 `FieldRegistry` 重建；EB/PML/谱求解器由对应插件在 `on_regrid` 重建。
3. **schema 驱动的 checkpoint**：新 level/新场能正确读写。

### 7.3 `IAmrPolicy` 只做决策

```cpp
class IAmrPolicy : public IPlugin {
public:
    virtual void tag_cells (int lev, amrex::TagBoxArray&, amrex::Real time) = 0;
    virtual bool should_regrid (int step, amrex::Real time) { return false; }
    virtual void costs (int lev, amrex::LayoutData<amrex::Real>&) {}
    virtual std::optional<amrex::DistributionMapping>
    distribute (int lev, const amrex::LayoutData<amrex::Real>&,
                const amrex::DistributionMapping&) { return std::nullopt; }
};
```

内核在 regrid 事务里调用它；它不碰 `RemakeLevel`。

---

## 8. 与上一版（FINAL）的差异

| 维度 | FINAL（V1） | V2 |
|---|---|---|
| 主程序 | `WarpX::Evolve` | `SimulationRuntime`；`WarpX` 降为包 |
| 内核接口 | `Efield()/Bfield()/Jfield()` | 通用 `FieldKey`/`DataBus`；E/B 由包注册 |
| 物种 | `MultiParticleContainer` 直接进 Context | `SpeciesRegistry` + 不透明句柄 |
| 协作 | 固定阶段 + 插件各自调用 | 标准阶段 + **数据依赖拓扑排序** |
| AMR | 只做策略，无动态 regrid | 动态 regrid 升级为内核特性；策略插件只决策 |
| 加载 | 编译期 + 运行期 | 同，但增加 schema/ABI 校验 |
| 内核改动 | 13 个 seam（仍以 WarpX 为内核） | 新建 Runtime + 逐步迁移（见 §9） |

---

## 9. 迁移路线：从 WarpX 到 Runtime（不推倒重来）

### 9.1 R0：Runtime facade（1–2 个月）

- 新增 `Source/Runtime/`：`SimulationRuntime` 包装现有 `WarpX`；
- `main.cpp` 改为 `Runtime::run()`；`Runtime` 内部仍调 `WarpX::InitData/Evolve`；
- 同时引入 `DataBus` 的**只读视图**（把 `m_fields` 映射成命名键）、`PluginHost`、`Stage` 枚举；
- 现有行为逐位不变；插件可以先挂 `BeforeStep/AfterStep` 做诊断类扩展。

### 9.2 R1：状态 schema（3–6 个月）

- `FieldType` 降为 `PicEmPackage` 的 schema；`FieldRegistry` 支持任意字符串键；
- checkpoint/restart 改为按 schema 遍历（先兼容旧格式）；
- 插件可以注册自定义场/标量。

### 9.3 R2：阶段与依赖调度（6–12 个月）

- 把 `WarpX::Evolve` 的 250 行拆成标准阶段；
- `PicEmPackage` 声明阶段序列；现有物理函数变成 handler；
- 插件按 `HandlerSpec` 排序；A/B 逐位验证。

### 9.4 R3：物种/材料泛化（6–12 个月）

- `SpeciesRegistry` 替代 `MultiParticleContainer` 直连；
- PIC/冷流体/矩流体成为 `SpeciesPlugin`；
- `MacroscopicProperties` 成为 `MaterialPlugin`。

### 9.5 R4：动态 regrid + IO 泛化（6–12 个月）

- 补 `Runtime::regrid`、通用 `RemakeLevel`、schema checkpoint；
- AMR 策略插件可用。

**总工作量**：全量 2–4 人年（单人 2–4 年；与“重写 WarpX 为微内核”同量级），但 R0/R1 就能交付“用户写一个诊断/源/材料插件”的价值；在 V2 路线上 R2 后才谈“用户写场求解器”。

> 如果目标是“6 个月内让用户写新场求解器”，则不能走完整 Runtime 路线；只能走上一版的“WarpX 内开孔”路线。两条路线的取舍见 §11。

---

## 10. 两条路线的取舍（必须明确）

| | 路线 A：WarpX 内开孔（V1） | 路线 B：Runtime 重写（V2） |
|---|---|---|
| 用户写新场求解器 | 2–3 个月可做到 | 12–18 个月 |
| 内核是否“无物理” | 否（仍有 E/B/粒子） | 是 |
| 旧源码改动 | 13 个 seam，~150 行 | 大规模重构 |
| 多物理协作 | 固定阶段 | 数据依赖调度 |
| 风险 | 接口被物理污染 | 工期/回归风险极高 |
| 适合 | 近期交付、用户是物理开发者 | 长期平台、多团队/多物理 |

**建议**：**A 做近期，B 做目标**。先把“场/物质/材料/边界/诊断”用 V1 的 seam 插件化（R0+M1 约 2–3 个月交付），同时按 V2 建 `DataBus`/`Stage`/`PluginHost` 的**最小骨架**；当第二个真实多物理需求（气化/化学/动态 AMR）出现时，再决定是否推进 R2/R3。不要一开始就重写 Runtime。

---

## 11. 风险与不做什么

| # | 风险 | 缓解 |
|---|---|---|
| R1 | 数据总线变成“全局命名空间泥潭” | schema 注册 + 访问审计 + 写权限校验；键名规范 `<domain>.<name>` |
| R2 | 依赖排序引入不确定性 | 同阶段同优先级按注册顺序；拓扑排序稳定；打印调度表 |
| R3 | 运行期 `.so` ABI | 只允许 host handler；版本/编译器/维度校验；默认关闭 |
| R4 | 动态 regrid 回归 | 先做静态 MR 的 schema/checkpoint，再做 regrid；每步 A/B |
| R5 | 过度设计 | 只为“第二个真实需求”加抽象；没有第二个用例的接口不做 |
| R6 | 性能 | 数据总线只在阶段边界；kernel 内零虚调用；`FieldView` POD |

**不做**：
- 不做“插件之间直接调用”的快捷方式（会退化成上帝类）。
- 不做“运行期替换 GPU kernel”。
- 不做“插件实现 AMR 机制”（机制在内核）。
- 不做“一次性重写全部 WarpX”。

---

## 12. 待确认（决定走 A 还是 B）

1. **时间尺度**：是否接受“6–8 周交付场/材料/边界插件（路线 A）”，还是要求“先建 Runtime 再谈插件”（路线 B，12–18 个月）？
2. **内核定义**：是否接受“内核不含 E/B/粒子物理，只有命名状态 + 阶段 + 服务”？
3. **协作机制**：是否接受“标准阶段 + 数据依赖排序”，而不是一张固定钩子表？
4. **AMR**：是否接受“动态 regrid 先作为内核特性立项”，而不是插件能力？
5. **运行期加载**：是否真的需要 `.so`，还是编译期 `-DWarpX_EXTRA_PLUGIN_SOURCES` 足够？

---

## 13. 一句话

**主程序应是 `SimulationRuntime`：只拥有 AMReX 底座、命名状态、数据总线、阶段调度、服务与插件宿主；`WarpX` 是“标准 PIC/EM 包”。插件只声明“我在哪个阶段、读哪些键、写哪些键”，由内核做依赖排序；插件之间永不互认。AMR 的机制在内核、策略在插件；动态 regrid 是内核必须补的课，不是插件能变出来的。**

---

## 附录：参考

- MOOSE：<https://www.sciencedirect.com/science/article/pii/S2352711019302973>、<https://github.com/idaholab/moose>
- Cordis（DSH 使用的插件框架）：`@deepseek-ai/cordis` README（本地 `app.asar` 解包，`inject`/`Service`/`ctx.on`/`Fiber`）
- Gkeyll：<https://github.com/gkeyll/gkeyll>（`App` + `Updater` + `Equation` 分离；本地 `core/apps`、`vlasov/zero/gkyl_dg_updater_vlasov.h`）
- preCICE：<https://precice.org/>
- AMReX：<https://joss.theoj.org/papers/10.21105/joss.01370>
- WarpX：<https://warpx.readthedocs.io/>
