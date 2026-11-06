# 按逻辑/物理功能划分模块：必要性、现状与可选实现路径

> **文档定位**
>
> 本文**不给出结论**，也不对现状做合格/不合格判定。全文只做三件事：
>
> 1. 给出可度量的判据（什么算"按单一逻辑/物理功能划分"）；
> 2. 用可复现的测量陈述当前状态；
> 3. 列出若干备选实现路径，以及各自的代价、风险与验证方式。
>
> 文中所有"建议"均为**待取舍的选项**，不是行动指令。是否采用、采用哪条、按什么顺序，
> 取决于项目目标（可插拔扩展优先 / 长期可维护优先 / 短期功能交付优先）。这三种目标
> 对应的最优路径并不相同，本文不代为选择。

> **修订说明（第二版）**
>
> 第一版把两个不同层级混在一起度量：一版的 M1–M5 描述的是**原子单元**（最小组成单元），
> 而不是**模块**。本版先定义 L0–L3 四层，把判据分层（原子层 M1–M5、模块层 N1–N5），
> 再按层陈述现状。一版中依然成立的测量数据保留，但归属到正确的层级。

---

## 1. 层级定义

"模块"在不同语境下指不同粒度。本文固定如下四层，后续所有度量都标注所属层级。

| 层 | 定义 | 单一性判据 | 本项目实例 | 观察到的数量 |
|---|---|---|---|---|
| **L0 原子单元** | 一个可命名的物理/数学动作，不可再分 | 只做一件事 | `UpdateMomentumBoris`、`ChargeDeposition`、`DivBFunctor`、单个 `ReducedDiag` | 密集 |
| **L1 模块** | 一组原子单元 + 一个契约 + 一个选择/生命周期入口，对外提供**一个能力**，可独立替换 | 单一能力域、契约可校验、可替换、可独立构建、参数自包含 | 场求解、碰撞、激光注入、诊断输出 | **隐式存在，无显式层** |
| **L2 子系统** | 一组模块，按物理域聚合 | 依赖单向、无环 | `Particles/`、`FieldSolver/`、`Diagnostics/`、`BoundaryConditions/` | 18 个目录 |
| **L3 编排** | 决定模块的执行顺序与时机 | 调度确定、冲突可检 | `Runtime` 的 14 个阶段槽位 / 4 个 PluginHost handler（9 个阶段有实现） | 1 层 |

层级关系：

```
L3 编排    Runtime 阶段调度（14 个槽位，9 个阶段有实现，4 个 PluginHost handler）
              ↓ 每个槽位调用
L2 子系统  Particles / FieldSolver / Diagnostics / BoundaryConditions / Laser ...（18 个）
              ↓ 每个子系统内聚合
L1 模块    推进族 / 沉积族 / 求解器族 / 碰撞族 / 诊断族 / 激光族 ...（隐式）
              ↓ 每个模块由若干
L0 原子    一个 pusher 变体 / 一个沉积核 / 一个 functor / 一个求解 kernel
```

**粒度选择的含义**：以 L0 为边界太细（用户要自己拼装很多原子并知道顺序）；
以 L2 为边界太粗（用户必须替换整个子系统）。**L1 是"一个能力"的边界**，
也是插件契约、版本号、参数 schema 最自然的挂载点。

---

## 2. 判据

### 2.1 L0 / 原子层判据（沿用第一版）

| 编号 | 判据 | 度量方式 |
|---|---|---|
| **M1** | 单一执行单元：一次执行对应一个可命名的物理/数学操作 | 数一次调用内覆盖的操作种类数 |
| **M2** | 单一数据契约：声明读/写且声明可被机器校验 | 数契约字段的消费者（非声明处） |
| **M3** | 可独立替换：替换不需要改动其它多数文件 | 数替换需触碰的文件数 |
| **M4** | 单一所有权：资源由唯一单元持有并释放 | 数同一资源的持有者/释放者 |
| **M5** | 依赖单向：通过显式注入通信，而非全局单例 | 数 `WarpX::GetInstance()` 残留 |

### 2.2 L1 / 模块层判据（本版新增）

| 编号 | 判据 | 含义 | 度量方式 |
|---|---|---|---|
| **N1** | 单一能力域 | 模块对外只提供一个能力（一"种"推进 / 一"种"沉积 / 一"族"求解） | 模块公开入口数量 |
| **N2** | 契约 | 声明：需要什么、产出什么、参数 schema 是什么 | 契约对象是否存在且被消费 |
| **N3** | 可替换 | 新增一个实现不需要改内核源码或分派 if/else | 新增实现的 diff 行数与触及文件数 |
| **N4** | 可独立构建/测试 | 模块能脱离内核编译并单独测试 | 是否有独立 target + 单测 |
| **N5** | 参数自包含 | 模块参数由自己的参数对象提供，不读全局 parser | 模块内全局配置读取点 |

### 2.3 L2 / L3 判据

| 编号 | 判据 | 度量方式 |
|---|---|---|
| **K1** | L2 子系统依赖无环、方向单一 | 目录级 include 图 |
| **K2** | L3 调度确定性：同输入同顺序、冲突可检 | 调度结果可复现 + 冲突检测是否存在 |

四条判据组互不替代：L0 达标不代表 L1 达标（一堆原子可以很干净而模块边界不存在），
L1 达标也不代表 L3 达标（模块可以很干净而调度仍是硬编码）。

---

## 3. 必要性：三种目标下的收益与代价

"必要性"本身是目标函数，不是属性。下表把三种常见目标分开列出，
**收益与代价同时给出**，不判断哪一列更重要。

| 目标 | 若按 L1 模块划分，获得的收益 | 需要付出的代价 |
|---|---|---|
| **A. 用户可插拔扩展**（第三方不改内核接入） | N3 达标后，新增一个物理能力 = 新增一个模块 + 一行注册；模块清单可枚举、可版本化 | 需要为现有"枚举 + if/else"式分派补上模块抽象；模块边界一旦被插件依赖即为长期 API 承诺 |
| **B. 长期可维护** | N1/N2 达标后，缺陷定位范围收敛到一个能力；小型 PR 不易触碰大文件 | 模块化产生更多文件与转发层，跨模块改动需多处协同；审阅成本前移 |
| **C. 短期功能交付** | 保持现有枚举分支，改动面小，物理验证成本低 | 每新增一个变体都要在原分派点加分支；分支间共享可变状态，回归风险随时间上升 |

常见的中间态是：**先满足 N2/N5（契约与参数自包含），暂缓 N1/N3（不强拆能力边界）**。
这条路线在 A 目标下只完成一半，但在 B 目标下能较早收获收益。是否接受这种"半程"取决于 A 目标的时间压力。

需要说明的是：L0 层（原子单元）在本项目中已经相对干净，继续在 L0 上做细粒度拆分
对上述三种目标的边际收益较低；**收益主要来自补齐 L1**。

---

## 4. 现状观察（全部可复现，见附录命令）

### 4.1 三套切分轴并存，各自对应不同层级

| 层 | 切分依据 | 单元数量 | 观察到的边界性质 |
|---|---|---|---|
| 顶层目录（L2） | 物理域 | 18 个目录 | 是"域"：`Particles/` 同时含推进、收集、沉积、碰撞、IO |
| Runtime 文件（L3 及其服务） | 职责 | 31 个 `.H/.cpp`，共 2833 行 | 文件级普遍单一职责；接口级存在两个宽接口（见 4.3） |
| 执行调度（L3） | 生命周期阶段 | 14 个阶段枚举，**4 个**实际注册 | 粒度是"阶段"，非"能力" |

### 4.2 L0：原子单元密集且可辨认

| 区域 | 原子单元数量 | 形态 |
|---|---|---|
| `Particles/Pusher/` | 4 个推进变体（Boris / Boris+辐射反作用 / Higuera-Cary / Vay）+ 位置更新 | header-only 模板 |
| `Particles/Deposition/` | 4 个沉积族（电荷 / 电流 / 质量矩阵 / 温度） | header-only 模板 |
| `Diagnostics/ComputeDiagFunctors/` | 18 个 `.cpp`，一个 functor 一个算子（DivB、DivE、J、Rho、温度……） | 有基类 `ComputeDiagFunctor` |
| `Diagnostics/ReducedDiags/` | 24 个 `.cpp`，多数一个类一个物理量 | 有基类 `ReducedDiags` |
| `FieldSolver/` | 60 个 `.cpp`，分布在 6 个算法族子目录 | 类 + 工厂 |

按 M1：L0 层可辨认度高，多数单元一次执行对应一个动作。

### 4.3 L1：模块层是**隐式**的

项目里实际存在一层模块，但它由「枚举选择器 + 实现族 + 分支点」隐式构成，
而不是由「抽象接口 + 注册表」显式构成：

| 子系统 | 模块选择机制 | 抽象基类 | 注册/工厂 | 观察到的形态 |
|---|---|---|---|---|
| 推进 | `WarpX::particle_pusher_algo`（[WarpX.H:215](../../Source/WarpX.H#L215)）+ [PushSelector.H:89](../../Source/Particles/Pusher/PushSelector.H#L89) 的 if/else 链 | ✗（header-only 模板） | ✗ | **L0 集合**，未升为模块 |
| 沉积 | `current_deposition_algo` / `charge_deposition_algo`（[WarpX.H:209-211](../../Source/WarpX.H#L209-L211)） | ✗（模板 + 枚举分派） | ✗ | **L0 集合** |
| 场求解 | 6 个子目录 + 工厂枚举 | ✓ | ✓ | 接近 L1 |
| 碰撞 | `CollisionType`（[BinaryCollisionUtils.H:21](../../Source/Particles/Collision/BinaryCollision/BinaryCollisionUtils.H#L21)）+ `CollisionBase` 派生 | ✓ | ✓（具名解析） | 接近 L1 |
| 诊断 | 名字解析 → 派生类 | ✓ | ✓（名字 → 类） | 接近 L1 |
| 激光注入 | `LaserProfile` 派生 + 工厂 | ✓ | ✓ | 接近 L1 |

按 N1–N5 的观察结果（只列度量，不做评价）：

| 判据 | 推进 | 沉积 | 场求解 | 碰撞 | 诊断 | 激光 |
|---|---|---|---|---|---|---|
| N1 单一能力域 | 一个变体一个能力 ✓ | 一个族一个能力 ✓ | 一族一个能力 ✓ | 一类一个能力 ✓ | 一个量/算子 ✓ | 一种剖面 ✓ |
| N2 契约存在且被消费 | ✗ | ✗ | 部分（工厂签名） | 部分（具名参数） | 部分（名字 + 参数） | 部分 |
| N3 新增实现不改内核 | ✗（改枚举 + if/else） | ✗（改枚举 + 模板分支） | 部分（改工厂注册） | 部分 | 部分（加类 + 解析项） | 部分 |
| N4 可独立构建/测试 | ✗（随内核编译） | ✗（随内核编译） | ✗（同 target） | 部分 | 部分 | ✗ |
| N5 参数自包含 | ✗（全局 parser 静态） | ✗ | 部分 | 部分（具名） | 部分 | 部分 |

**观察结论仅限数字层面**：L1 层的抽象程度在整个项目中不均——有基类和工厂的子系统
（场求解、碰撞、诊断、激光）已接近模块；只有枚举和模板的子系统（推进、沉积）仍停留在原子集合。
这不是"有没有模块"的二值问题，而是"模块成熟度分布"的问题。

### 4.4 L2：18 个子系统，边界清晰但粒度是"域"

```
Source/ 下 .H/.cpp 共约 171,121 行
```

实现文件数（`.cpp`）：`FieldSolver` 递归 60、`ReducedDiags` 24、`ComputeDiagFunctors` 18、`Particles` 顶层 10（递归 44）。
一个子系统内通常有 3–8 个 L1 模块；因此以目录做插件边界偏粗。

最大文件（L2 内部仍混合多个 L1 模块的间接信号）：

| 文件 | 行数 | 文件内可辨认的职责 |
|---|---|---|
| [WarpX.cpp](../../Source/WarpX.cpp) | 3844 | 输入解析、几何、场管理、边界、诊断、AMR、重启 |
| [WarpXParticleContainer.cpp](../../Source/Particles/WarpXParticleContainer.cpp) | 3234 | 容器、排序、通信、沉积入口、边界、IO |
| [HybridPICModel.cpp](../../Source/FieldSolver/FiniteDifferenceSolver/HybridPICModel/HybridPICModel.cpp) | 3002 | 求解器、粒子容器、10+ 场分配 |
| [PhysicalParticleContainer.cpp](../../Source/Particles/PhysicalParticleContainer.cpp) | 2287 | 推进、收集、沉积、初始化、边界 |
| [MultiParticleContainer.cpp](../../Source/Particles/MultiParticleContainer.cpp) | 2031 | 物种容器、碰撞、边界、物理属性 |

### 4.5 L3：编排层已存在，但槽位不是模块

[StagePoint.H](../../Source/Runtime/StagePoint.H) 定义 14 个阶段：
`BeforeInit / Init / AfterInit / BeforePush / Push / AfterPush / BeforeCollisions / Collisions /
AfterCollisions / BeforeFieldSolve / FieldSolve / AfterFieldSolve / Diagnostics / Finalize`。

实际注册的 handler（[Runtime.cpp:62-64](../../Source/Runtime/Runtime.cpp#L62-L64)、[:83](../../Source/Runtime/Runtime.cpp#L83)）：

| Handler | 阶段 | 一次 `apply()` 覆盖的物理操作 | 操作种类数 |
|---|---|---|---|
| [PicPushHandler](../../Source/Runtime/PicPushHandler.H#L40-L46) | `Push` | 粒子推进 + 电流/电荷沉积 + 碰撞入口（内部调用 `RunCollisionStage`） | 3 |
| [PicFinalizeHandler](../../Source/Runtime/PicFinalizeHandler.H) | `AfterPush` | 沉积收尾（同步、滤波、边界和） | 1（内部含多步） |
| [PicFieldSolveHandler](../../Source/Runtime/PicFieldSolveHandler.H) | `FieldSolve` | 按运行期枚举分发到 Yee / CKC / ECT / Hybrid / PSATD / 隐式 / 静电 / 静磁 / Moments | 6+ 类算法 |
| [CollisionHandlerAdapter](../../Source/Runtime/CollisionHandlerAdapter.H) | `Collisions` | 按 `CollisionType` 分发到全部碰撞模型；由 `RunCollisionStage` 在 `Push` 内及显式碰撞点查找，不由 `StagePlan` 直接调度 | 多类算法 |

按 M1：PluginHost 注册 4 个 handler，其中 1 个单操作、3 个多操作。14 个阶段中，5 个由 `InitData()` 内的 `StagePlan` 分发（`BeforeInit/Init/AfterInit/FieldSolve/Diagnostics`）；`Push/AfterPush/FieldSolve` 由 `Run*Stage` 直接调度；`Collisions` 不是独立 `StagePlan` 项，而是 `RunCollisionStage` 在 `Push` 内及显式碰撞点查找。6 个阶段未使用（`BeforePush/BeforeCollisions/AfterCollisions/BeforeFieldSolve/AfterFieldSolve/Finalize`）。
按 K2：`HandlerSpec` 的读/写字段共 **44 处出现（4 处成员声明 + 40 处 spec 初始化，含已注册 handler 与 PicEm/Collision package 声明）、0 处消费**（[HandlerSpec.H:29-38](../../Source/Runtime/HandlerSpec.H#L29-L38)；
`PluginHost` 仅按 `StagePoint` + `priority` 排序，见 [:83-105](../../Source/Runtime/HandlerSpec.H#L83-L105)），
调度顺序可复现，但冲突不可检。

值得注意的一点：`HandlerSpec` 描述的是"槽位"的读写，属于**L3 编排**的语义，
却被用作模块契约的载体；而 L1 模块本身没有契约对象。两个层级共用同一个数据结构，
这也是它难以被消费的原因之一。

### 4.6 宽接口（L1/L3 之间的服务边界）

| 接口 | `virtual` 声明条数 | 覆盖的域 |
|---|---|---|
| [HostServices.H](../../Source/Runtime/HostServices.H) | 53 | 场准备、沉积、静电、滤波、EB、RZ、推进、求解、碰撞、时间、moving window、PML、BTD、诊断元数据、in-situ、输出 |
| [RuntimeData.H](../../Source/Runtime/RuntimeData.H) | 51 | 场、物种、几何、时间、网格、PML、粒子边界、诊断 |

唯一实现 [WarpXHostServices.cpp](../../Source/Runtime/WarpXHostServices.cpp) 为 308 行、实现全部 53 项。
按 N1：单次调用对应一个能力，但**一个实现者必须同时承担全部域**——调用侧收敛、实现侧发散（门面模式特征）。

### 4.7 全局入口残留（M5）

`WarpX::GetInstance()` 共 **111** 处，分布：

| 目录 | 处数 |
|---|---|
| `Runtime/` | 48 |
| `FieldSolver/` | 36 |
| `Particles/` | 13 |
| `Diagnostics/` | 6 |
| `Python/` | 4 |
| `Fluids/` | 2 |
| `Initialization/` | 1 |
| `BoundaryConditions/` | 1 |

`Runtime/` 内的 48 处主要集中在两个适配器（`WarpXHostServices`、`WarpXRuntimeData`），
属过渡期设计；其余 63 处位于物理模块内部。

### 4.8 资源所有权（M4）

| 资源 | 持有者 | 释放者 | 持有者数量 |
|---|---|---|---|
| 场注册表 | `ablastr::fields::MultiFabRegister` | Runtime / AMR 回调 | 1（被多方借用） |
| PML | `LevelResources` | `ClearLevel` | 1 |
| EB 借用因子、costs、dotMask、加速器晶格 | `LevelResources` | `ClearLevel`/`RemakeLevel` | 1 |
| 碰撞处理器 | `Runtime` | `Runtime` 析构 | 1 |
| 其余若干运行时状态 | `WarpX` 静态成员 | `WarpX` | 1（但通过全局单例访问） |

按 M4：主要资源已收敛到唯一持有者；剩余缺口体现在"访问路径"（M5），而非"所有权"。

---

## 5. 现状与判据的对应关系（只陈述度量结果）

| 层 | 判据 | 度量结果 | 备注 |
|---|---|---|---|
| L0 | M1 单一执行单元 | 4 个执行槽位中 1 个单操作、3 个多操作 | 数值本身不构成评价 |
| L0 | M2 契约可校验 | 44 处出现（4 声明 + 40 初始化）/ 0 处消费 | 机制已存在，校验逻辑未接入 |
| L0 | M3 独立替换 | 替换单一 `Push` 需同时换推进与沉积 | 取决于是否把二者视为一个能力 |
| L0 | M4 单一所有权 | 主要资源 1 个持有者 | 访问路径另计 |
| L0 | M5 依赖单向 | 111 处全局入口 | 63 处在物理模块内 |
| **L1** | **N1 单一能力域** | 6 个子系统中 4 个有基类/工厂、2 个（推进、沉积）仅 enum+模板 | 成熟度不均 |
| **L1** | **N2 契约** | 无模块级契约对象；`HandlerSpec` 属 L3 |  |
| **L1** | **N3 可替换** | 推进/沉积新增实现需改 `PushSelector.H` 或枚举；求解器需改工厂 |  |
| **L1** | **N4 独立构建** | 推进/沉积为 header-only，随内核编译 |  |
| **L1** | **N5 参数自包含** | 多数模块读全局 parser 静态成员 |  |
| L2 | K1 依赖无环 | 18 个目录，未见环（未做完整 include 图分析） | 未验证项，需补 |
| L3 | K2 调度可检 | 顺序可复现；冲突检测缺失 |  |

---

## 6. 可能的实现方法（平列，未排序，未取舍）

以下选项互不排斥。每条给出：所属层级、做法、触及面、主要风险、验证信号、
工作量等级（S/M/L，仅表示相对规模，不含日历时间）。

### P1 — 宽接口按域拆分（层级：L1/L3 边界）

- **做法**：把 53 项的 `HostServices` 与 51 项的 `RuntimeData` 按域拆为若干窄接口
  （候选：沉积 / 场求解 / 静电 / PML / BTD / 诊断元数据 / in-situ / 时间与几何）。
  保留一个组合门面供过渡期调用方使用（适配器只做转发）。
- **触及面**：`Source/Runtime/HostServices.H`、`RuntimeData.H`、两个适配器实现，
  以及约 71 个使用 `RuntimeContext` 的消费文件中的调用点（可按接口分组分批迁移）。
- **风险**：拆分点一旦被外部插件依赖即为 API 承诺；拆得过细会产生大量转发样板。
- **验证**：编译矩阵全绿 + 每个窄接口有独立最小实现（可用于单测 mock）。
- **工作量**：M（接口定义轻，调用点迁移重）。

### P2 — 执行单元按物理操作拆分（层级：L3）

- **做法**：把 `Push` 拆为 `PushParticles` 与 `DepositCurrent` 两个 handler；
  把 `FieldSolve` 改为"每类算法一个 handler"，由注册表按名字选择激活哪一个。
- **触及面**：`Runtime.cpp` 注册段、`PicPushHandler.H`、`PicFieldSolveHandler.H`、
  `WarpXEvolve.cpp` 的四个调用点。
- **风险**：`pushParticlesAndDeposit` 当前是内核里的一次融合调用；拆开可能改变
  每步的调用次序与临时暂存生命周期，**逐位一致性需 A/B 验证**。
  另：推进与沉积在部分算法中共享中间量，硬拆会引入额外内存往返。
- **验证**：`WARPX_DISABLE_PLUGIN_HANDLERS` 开关下的逐位对比；
  若无法逐位一致，退而做容差对比并记录差异来源。
- **工作量**：M–L（拆 `FieldSolve` 比拆 `Push` 风险低，可单独做）。
- **备注**：此项解决 L3 粒度，**不解决 L1 缺失**。

### P3 — 契约落地（层级：L1 + L3）

- **做法**：让读/写声明真正被消费：同阶段内做拓扑排序；检测两个 handler 写同一字段且无先后声明；
  未声明却访问的字段在调试构建下告警。同时把"模块能力契约"从 `HandlerSpec` 中拆出，
  成为 L1 模块自身的对象（模块提供什么、需要什么、参数 schema）。
- **触及面**：`HandlerSpec.H` 的 `PluginHost`、新增模块契约类型、各 handler 与模块的声明。
- **风险**：声明不完整时会误报；需区分"可选读"与"必读"；拆契约类型会造成一次批量迁移。
- **验证**：构造冲突用例单测（两个写同一字段的 handler）；对现有 4 个 handler 跑一遍无报警。
- **工作量**：S（仅调度侧）/ M（含模块契约拆分）。

### P4 — 大文件局部抽取（层级：L2 内部）

- **做法**：以"可独立命名 + 可独立测试"为准，从 5 个最大文件中抽出成组的自由函数/类，
  例如把沉积核、收集核、粒子边界处理从 `PhysicalParticleContainer` 移出。
  **不做整体拆分**，只抽边界清晰、无成员耦合的部分。
- **触及面**：`WarpX.cpp`、`WarpXParticleContainer.cpp`、`PhysicalParticleContainer.cpp`、
  `MultiParticleContainer.cpp`、`HybridPICModel.cpp`。
- **风险**：历史耦合密度高，抽取常伴随性能回归（内联边界变化）；
  GPU 构建下的 `AMREX_GPU_DEVICE` 可用性需逐函数确认。
- **验证**：抽取前后同一算例逐位对比 + 内核启动次数对比；优先抽"纯函数"。
- **工作量**：L（可分文件独立评估，不必一次做完）。

### P5 — 只增不改的旁路方案（层级：全层观测）

- **做法**：不动现有结构，只增加可观测性与验证：为每个阶段记录实际执行的物理能力序列；
  增加只读一致性检查（失败仅告警）；增加"模块边界快照"测试，防止边界在后续 PR 中被无意破坏。
- **触及面**：`Runtime` 调度循环、测试目录。
- **风险**：不改善 N1/N3，只让现状可度量、可回归；可能形成"测了但没改"的长期负债。
- **验证**：度量报告本身（阶段 → 能力映射表随每次构建产出）。
- **工作量**：S。

### P6 — 按"物理量 schema"划分（层级：L1 数据契约）

- **做法**：以数据而非代码为划分单位：为每个物理量（`rho`、`J`、`E`、`B`、粒子属性）
  定义 schema（分量、节点性、guard cell、checkpoint 语义），由 schema 驱动分配、通信、输出、重启。
- **触及面**：`DataBus`/`FieldSchema` 已有雏形；需扩展到 checkpoint（A4）与输出。
- **风险**：与 AMReX `IndexType`/`IntVect` 语义需对齐；RZ 与 1D/2D 的分量映射差异较大。
- **验证**：同一输入在 schema 驱动与硬编码两条路径下逐位一致。
- **工作量**：L。
- **备注**：与 N1 无关，与 N2 强相关——把"模块契约"从字段名提升到字段语义。

### P7 — 把 L0 集合升为 L1 模块（层级：L1，本版新增）

- **做法**：对"只有枚举 + 模板"的两个子系统给出模块化路径：
  1. 引入抽象基类（如 `IPusher` / `IDepositionScheme`），把 header-only 模板包成类；
  2. 引入按名字的注册表（静态注册或编译期注册），枚举退化为"内置实现的名字表"；
  3. 引入参数对象（`PusherParams` / `DepositionParams`），模块只读自己的参数；
  4. 给每个模块一个独立 target 与最小单测。
  可选：只对其中一个子系统做试点（推进或沉积），另一个保持现状作为对照。
- **触及面**：`Particles/Pusher/`、`Particles/Deposition/`、`WarpX.H` 的两个静态枚举、
  `PushSelector.H`、`PhysicalParticleContainer` 的调用点、构建脚本。
- **风险**：把模板包成类会引入每粒子的虚调用；GPU 构建下虚函数与模板内联的选择需要实测；
  `PushSelector.H` 的分派会把算法枚举带入内核签名，改写会波及 GPU kernel 参数列表。
  **这是唯一直接触及内核热路径的选项**，风险高于其它选项。
- **验证**：逐位对比 + 每粒子耗时对比（Boris/Vay 各一组）+ GPU 构建的 kernel 参数合法性。
- **工作量**：L。
- **备注**：若目标是"用户可新增一种推进/沉积而不改内核"，此项是 N3 的直接对因；
  若目标只是"新增一种求解器"，此项不必要。

---

## 7. 选项之间的关系（只陈述约束，不排序）

| 组合 | 关系 |
|---|---|
| P1 + P3 | 可并行；P1 改接口，P3 改声明消费，无冲突 |
| P2 + P3 | P3 是 P2 的前置：先让契约可校验，再拆执行单元，才能验证拆分后读写关系仍自洽 |
| P1 + P2 | 可并行，但都触及 `Runtime.cpp` 注册段，需协调一次 |
| P4 + 任意 | 独立；但 P4 若改变内联边界，会干扰 P2/P7 的逐位对比，宜错开 |
| P5 + 任意 | 独立；可作为其它选项的验收基线 |
| P6 + P7 | P6 若先做，P7 可按 schema 而非硬编码字段名声明模块参数/产出，减少返工 |
| P7 + P2 | 独立层级：P2 是 L3 槽位粒度，P7 是 L1 能力粒度；都做才同时满足"可编排"与"可替换" |

若只允许做一件事，按工作量/覆盖面衡量，`P5` 成本最低（S 级，且能产出全层度量基线），
`P3` 在 L3 侧可直接让"契约从 0 到 1"。但这一判断仍取决于目标优先级——本文不代为决定。

---

## 8. 任一选项都适用的验证手段

| 手段 | 适用层级 | 说明 |
|---|---|---|
| `WARPX_DISABLE_PLUGIN_HANDLERS` A/B 逐位对比 | L1/L3 | 现有开关，可验证"改结构不改结果" |
| `ctest -R '^runtime_'` | L1/L3 | 现有 5 个算例（默认）/ 7 个（FFT）；注意当前构建树路径与 ctest 注册不一致，需重新 configure |
| 契约冲突单测 | L1/L3 | 构造"两个 handler 写同一字段"的最小用例 |
| 构建矩阵（1D/2D/3D/RZ × CPU/GPU） | 全部 | 拆分常引入维度相关编译分支 |
| 内核启动计数 / 内存峰值 | L0/L1 | 拆分可能增减一次 kernel；不逐位时用它解释差异 |
| 每粒子耗时（Boris/Vay 两组） | L1 | 专用于 P7 的虚调用/内联代价评估 |
| `grep` 计数快照 | 全部 | 见附录，可作为"边界未被侵蚀"的回归阈值 |

---

## 9. 待决问题（选择路径前需要先回答）

1. **"模块"的粒度以什么为准**：一个物理能力（推进/沉积/求解）？一个算法族？还是一个可替换实现？
2. **L1 模块是否必须可独立编译**？若只要求"可独立替换"而不要求"独立 target"，工作量差异显著。
3. **API 稳定承诺何时开始**：是否先内部模块化、后对外承诺？
4. **是否接受"半程"模块化**（N2/N5 达标、N1/N3 暂缓）？若不接受，短期交付目标如何调整？
5. **逐位一致是否为不可让渡要求**？若允许容差，P2 与 P7 的可行性显著上升。
6. **`Push` 的推进与沉积是否允许拆开**？两者在部分算法中共享中间量。
7. **推进/沉积是否值得模块化**，还是"扩展现有分派分支"更经济？
8. **模块参数由谁持有**：模块自带的参数对象，还是继续沿用全局 parser 的静态成员？
9. **L2 的 18 个目录是否也要按功能重划**，还是只调整 L1 与 L3？
10. **是否需要结构回归门禁**（目录/接口/契约计数快照）防止拆分成果被后续 PR 回退？

---

## 附录：本文数据的复现命令

```bash
# 阶段枚举
sed -n '/enum class Stage/,/};/p' Source/Runtime/StagePoint.H

# 实际注册的 handler
grep -n "add(\|make_shared<" Source/Runtime/Runtime.cpp

# 契约字段的声明处与消费者
grep -rn "reads_fields\|writes_fields\|reads_species\|writes_species" Source

# 全局入口分布
grep -rn "WarpX::GetInstance()" Source --include='*.cpp' --include='*.H' | wc -l
grep -rn "WarpX::GetInstance()" Source --include='*.cpp' --include='*.H' \
  | cut -d/ -f2 | sort | uniq -c | sort -rn

# 算法选择器（L1 模块的隐式入口）
grep -n "deposition_algo\|particle_pusher_algo" Source/WarpX.H
grep -n "pusher_algo ==" Source/Particles/Pusher/PushSelector.H

# 最大文件
find Source \( -name '*.cpp' -o -name '*.H' \) | xargs wc -l | sort -rn | head -12

# 接口宽度
grep -c "virtual" Source/Runtime/HostServices.H
grep -c "virtual" Source/Runtime/RuntimeData.H

# 层级内原子单元数量
ls Source/Particles/Pusher/ Source/Particles/Deposition/
ls Source/Diagnostics/ComputeDiagFunctors/*.cpp | wc -l
ls Source/Diagnostics/ReducedDiags/*.cpp | wc -l
find Source/FieldSolver -name '*.cpp' | wc -l

# Runtime 层规模
wc -l Source/Runtime/*.H Source/Runtime/*.cpp | tail -1
```

> 以上测量基于本文撰写时的仓库状态。数字会随提交变化，判据（M1–M5、N1–N5、K1–K2）不随变化。
</content>
