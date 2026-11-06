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

---

## 1. 判据：怎样才算"按单一逻辑/物理功能划分"

为避免"模块化"变成主观词，先定义 5 条可测量的判据。

| 编号 | 判据 | 含义 | 度量方式 |
|---|---|---|---|
| **M1** | 单一执行单元 | 模块的一次执行对应**一个可命名的物理/数学操作**（如"推进粒子"、"沉积电流"、"解 Maxwell 方程"） | 数一次 `apply()` 内调用的物理操作种类数 |
| **M2** | 单一数据契约 | 模块声明自己读哪些场/物种、写哪些场/物种，且声明**可被机器校验** | 数契约字段的**消费者**（非声明处） |
| **M3** | 可独立替换 | 替换该模块不需要改动其它模块的源码 | 数替换一个模块需触碰的文件数 |
| **M4** | 单一所有权 | 资源（场、PML、缓冲、诊断状态）由**唯一**模块持有并释放 | 数同一资源的持有者/释放者数量 |
| **M5** | 依赖单向 | 模块之间通过显式注入的引用通信，而非全局单例横向访问 | 数 `WarpX::GetInstance()` 等全局入口的残留 |

这 5 条互相独立：一个项目可以在 M3 达标而 M1 不达标（例如"整个阶段可以换人实现，但那一个实现里塞了三个物理操作"），
反之亦然。后文的现状陈述按这 5 条分开度量，不做合并打分。

---

## 2. 必要性：三种目标下的收益与代价

"必要性"本身是目标函数，不是属性。下表把三种常见目标分开列出，
**收益与代价同时给出**，不判断哪一列更重要。

| 目标 | 若按单一功能划分，获得的收益 | 需要付出的代价 |
|---|---|---|
| **A. 用户可插拔扩展**（第三方不改内核接入） | M3 达标后，替换一个物理操作只需实现一个窄接口；插件清单可枚举、可版本化 | 需要把现有"一次调用覆盖多操作"的地方拆开；拆分点即是 API 稳定性承诺点，早期拆错会长期锁死接口 |
| **B. 长期可维护** | M1/M4 达标后，单个缺陷的定位范围收敛到一个操作；小型 PR 不易触碰大文件 | 拆分产生更多文件与转发层，跨模块改动需要多处协同；审阅成本前移 |
| **C. 短期功能交付** | 保持现有粗粒度，改动面小，物理验证成本低 | 每新增一个变体（新求解器/新沉积方案）都要在原文件里加分支，分支间共享可变状态，回归风险随时间上升 |

一个常见的中间态是：**先满足 M2/M5（可验证、无全局横向依赖），暂缓 M1/M3（不强拆执行单元）**。
这条中间路线在 A 目标下只完成了一半，但在 B 目标下能较早收获收益。是否接受这种"半程"取决于 A 目标的时间压力。

---

## 3. 现状观察（全部可复现，见附录命令）

### 3.1 同时存在三套切分轴

| 层 | 切分依据 | 单元数量 | 观察到的边界性质 |
|---|---|---|---|
| 顶层目录 | **物理域** | 18 个目录 | 是"域"（如 `Particles/` 同时含推进、收集、沉积、碰撞、IO） |
| Runtime 新层 | **职责** | 31 个 `.H/.cpp`，共 2833 行 | 文件级普遍单一职责；接口级存在两个宽接口（见 3.4） |
| 执行调度 | **生命周期阶段** | 14 个阶段枚举，**4 个**实际注册 | 粒度是"阶段"，非"物理操作" |

三套轴并不对齐：同一个物理操作可能跨目录、跨阶段、跨接口三处定义。

### 3.2 顶层目录规模

```
Source/ 下 .H/.cpp 共约 171,121 行
```

实现文件数（`.cpp`）：`FieldSolver` 60、`ReducedDiags` 24、`ComputeDiagFunctors` 18、`Particles` 10。

### 3.3 最大文件（单一文件内混合的职责数量是 M1 的间接信号）

| 文件 | 行数 | 文件内可辨认的职责 |
|---|---|---|
| [WarpX.cpp](file:///workspace/Source/WarpX.cpp) | 3844 | 输入解析、几何、场管理、边界、诊断、AMR、重启 |
| [WarpXParticleContainer.cpp](file:///workspace/Source/Particles/WarpXParticleContainer.cpp) | 3234 | 容器、排序、通信、沉积入口、边界、IO |
| [HybridPICModel.cpp](file:///workspace/Source/FieldSolver/FiniteDifferenceSolver/HybridPICModel/HybridPICModel.cpp) | 3002 | 求解器、粒子容器、10+ 场分配 |
| [PhysicalParticleContainer.cpp](file:///workspace/Source/Particles/PhysicalParticleContainer.cpp) | 2287 | 推进、收集、沉积、初始化、边界 |
| [MultiParticleContainer.cpp](file:///workspace/Source/Particles/MultiParticleContainer.cpp) | 2031 | 物种容器、碰撞、边界、物理属性 |

### 3.4 两个宽接口（M1 在接口层的观察）

| 接口 | `virtual` 声明条数 | 覆盖的域 |
|---|---|---|
| [HostServices.H](file:///workspace/Source/Runtime/HostServices.H) | 53 | 场准备、沉积、静电、滤波、EB、RZ、推进、求解、碰撞、时间、moving window、PML、BTD、诊断元数据、in-situ、输出 |
| [RuntimeData.H](file:///workspace/Source/Runtime/RuntimeData.H) | 51 | 场、物种、几何、时间、网格、PML、粒子边界、诊断 |

唯一实现 [WarpXHostServices.cpp](file:///workspace/Source/Runtime/WarpXHostServices.cpp) 为 308 行、实现全部 53 项。
按 M1 的度量方式，这两个接口的"单次调用对应一个物理操作"成立，但**一个实现者必须同时承担全部域**，
即接口层面的单一职责不成立。这是"门面模式"的典型特征：调用侧收敛，实现侧发散。

### 3.5 执行单元：14 个阶段，4 个实现

[StagePoint.H](file:///workspace/Source/Runtime/StagePoint.H) 定义 14 个阶段：
`BeforeInit / Init / AfterInit / BeforePush / Push / AfterPush / BeforeCollisions / Collisions /
AfterCollisions / BeforeFieldSolve / FieldSolve / AfterFieldSolve / Diagnostics / Finalize`。

实际注册的 handler（[Runtime.cpp:62-64](file:///workspace/Source/Runtime/Runtime.cpp#L62-L64)、[:83](file:///workspace/Source/Runtime/Runtime.cpp#L83)）：

| Handler | 阶段 | 一次 `apply()` 覆盖的物理操作 | 操作种类数 |
|---|---|---|---|
| [PicPushHandler](file:///workspace/Source/Runtime/PicPushHandler.H#L40-L46) | `Push` | 粒子推进 + 电流/电荷沉积（`pushParticlesAndDeposit`） | 2 |
| [PicFinalizeHandler](file:///workspace/Source/Runtime/PicFinalizeHandler.H) | `AfterPush` | 沉积收尾（同步、滤波、边界和） | 1（内部含多步） |
| [PicFieldSolveHandler](file:///workspace/Source/Runtime/PicFieldSolveHandler.H) | `FieldSolve` | 按运行期枚举分发到 Yee / CKC / ECT / Hybrid / PSATD / 隐式 / 静电 / 静磁 / Moments | 6+ 类算法 |
| [CollisionHandlerAdapter](file:///workspace/Source/Runtime/CollisionHandlerAdapter.H) | `Collisions` | 按 `CollisionType` 分发到全部碰撞模型 | 多类算法 |

按 M1：4 个执行单元中，1 个为单操作，3 个为多操作；剩余 10 个阶段无实现。

### 3.6 契约声明与实际消费（M2）

[HandlerSpec](file:///workspace/Source/Runtime/HandlerSpec.H#L29-L38) 声明 4 个契约字段：
`reads_fields / writes_fields / reads_species / writes_species`。

全仓库搜索（排除声明文件本身）：**20 处出现，全部位于各 handler 的 `spec()` 字面量中，0 处消费逻辑**。
`PluginHost`（[HandlerSpec.H:83-105](file:///workspace/Source/Runtime/HandlerSpec.H#L83-L105)）仅按
`StagePoint` + `priority` 做稳定排序，未使用任何契约字段。

按 M2：契约"已声明、未校验"。A3.3c（拓扑排序）与 A5.2（读写冲突检测）在任务图中仍为 pending。

### 3.7 全局入口残留（M5）

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
属于过渡期设计；其余 63 处位于物理模块内部。

### 3.8 M1/M3 表现最好的区域

| 区域 | 观察 |
|---|---|
| `Diagnostics/ReducedDiags/` | 24 个实现文件，多数一个类对应一个物理量（能量、动量、最大场强、粒子数……） |
| `Diagnostics/ComputeDiagFunctors/` | 18 个实现文件，一个 functor 对应一个诊断算子（DivB、DivE、J、Rho、温度……） |
| `FieldSolver/` 子目录 | 按算法族分目录：`FiniteDifferenceSolver/`、`SpectralSolver/`、`ElectrostaticSolvers/`、`ImplicitSolvers/`、`MagnetostaticSolver/` |

即：**"按物理功能划分"在诊断层已大体成立，在求解器层是"按算法族"成立，
在 Runtime 执行层是"按阶段"成立，在顶层是"按域"成立。** 四种粒度共存。

### 3.9 资源所有权（M4）

| 资源 | 持有者 | 释放者 | 持有者数量 |
|---|---|---|---|
| 场注册表 | `ablastr::fields::MultiFabRegister` | Runtime / AMR 回调 | 1（被多方借用） |
| PML | `LevelResources` | `ClearLevel` | 1 |
| EB 借用因子、costs、dotMask、加速器晶格 | `LevelResources` | `ClearLevel`/`RemakeLevel` | 1 |
| 碰撞处理器 | `Runtime` | `Runtime` 析构 | 1 |
| 其余若干运行时状态 | `WarpX` 静态成员 | `WarpX` | 1（但通过全局单例访问） |

按 M4：主要资源已收敛到唯一持有者；M4 的剩余缺口体现在"访问路径"（M5），而非"所有权"。

---

## 4. 现状与判据的对应关系（只陈述度量结果）

| 判据 | 度量结果 | 备注 |
|---|---|---|
| M1 单一执行单元 | 4 个执行单元中 1 个单操作、3 个多操作；接口层 53/51 条声明覆盖 16 个域 | 数值本身不构成评价 |
| M2 契约可校验 | 20 处声明 / 0 处消费 | 机制已存在，校验逻辑未接入 |
| M3 独立替换 | 替换 `Push` 阶段需同时替换推进与沉积；替换单一求解器需改 `PicFieldSolveHandler` 或内核 | 取决于是否把"求解器"视为一个功能 |
| M4 单一所有权 | 主要资源 1 个持有者 | 访问路径另计 |
| M5 依赖单向 | 111 处全局入口 | 63 处在物理模块内 |

---

## 5. 可能的实现方法（平列，未排序，未取舍）

以下 5 条为**互不排斥的选项**。每条给出：做法、触及面、主要风险、验证信号、工作量等级（S/M/L，
仅表示相对规模，不含日历时间）。

### P1 — 宽接口按域拆分

- **做法**：把 53 项的 `HostServices` 与 51 项的 `RuntimeData` 按域拆为若干窄接口
  （候选：沉积 / 场求解 / 静电 / PML / BTD / 诊断元数据 / in-situ / 时间与几何）。
  保留一个组合门面供过渡期调用方使用（适配器只做转发）。
- **触及面**：`Source/Runtime/HostServices.H`、`RuntimeData.H`、两个适配器实现、
  以及约 71 个使用 `RuntimeContext` 的消费文件中的调用点（可按接口分组分批迁移）。
- **风险**：拆分点一旦被外部插件依赖即为 API 承诺；拆得过细会产生大量转发样板。
- **验证**：编译矩阵全绿 + 每个窄接口有独立的最小实现（可用于单测 mock）。
- **工作量**：M（接口定义轻，调用点迁移重）。

### P2 — 执行单元按物理操作拆分

- **做法**：把 `Push` 拆为 `PushParticles` 与 `DepositCurrent` 两个 handler；
  把 `FieldSolve` 改为"每类算法一个 handler"，由算法枚举选择激活哪一个
  （注册表按名字选择，而非在一个 handler 内分支）。
- **触及面**：`Runtime.cpp` 注册段、`PicPushHandler.H`、`PicFieldSolveHandler.H`、
  `WarpXEvolve.cpp` 的四个调用点。
- **风险**：`pushParticlesAndDeposit` 当前是内核里的一次融合调用；拆开可能改变
  每步的调用次序与临时暂存生命周期，**逐位一致性需要 A/B 验证**。
  另：推进与沉积在部分算法中共享中间量，硬拆会引入额外内存往返。
- **验证**：`WARPX_DISABLE_PLUGIN_HANDLERS` 开关下的逐位对比；
  若无法逐位一致，退而求其次做容差对比并记录差异来源。
- **工作量**：M–L（拆 `FieldSolve` 比拆 `Push` 风险低，可单独做）。

### P3 — 契约落地

- **做法**：让 `HandlerSpec` 的 `reads/writes` 真正被消费：
  同阶段内做拓扑排序；检测两个 handler 写同一字段且无先后声明；未声明却访问的字段在调试构建下报警。
- **触及面**：`HandlerSpec.H` 的 `PluginHost`、新增校验模块、各 handler 的 `spec()`（补全声明）。
- **风险**：声明不完整时会误报；需区分"可选读"与"必读"。
- **验证**：构造冲突用例的单测（两个写同一字段的 handler）；对现有 4 个 handler 跑一遍无报警。
- **工作量**：S（在现有数据结构上增量）。

### P4 — 大文件局部抽取

- **做法**：以"可独立命名 + 可独立测试"为准，从 5 个最大文件中抽出成组的自由函数/类，
  例如把沉积核、收集核、粒子边界处理从 `PhysicalParticleContainer` 移出。
  **不做整体拆分**，只抽边界清晰、无成员耦合的部分。
- **触及面**：`WarpX.cpp`、`WarpXParticleContainer.cpp`、`PhysicalParticleContainer.cpp`、
  `MultiParticleContainer.cpp`、`HybridPICModel.cpp`。
- **风险**：这类文件的历史耦合密度高，抽取常伴随性能回归（内联边界变化）；
  GPU 构建下的 `AMREX_GPU_DEVICE` 可用性需逐函数确认。
- **验证**：抽取前后同一算例逐位对比 + 内核启动次数对比；优先抽"纯函数"。
- **工作量**：L（且可分文件独立评估，不必一次做完）。

### P5 — 只增不改的旁路方案

- **做法**：不动现有结构，只增加**可观测性与验证**：
  为每个阶段记录实际执行的物理操作序列；为 `HandlerSpec` 增加只读一致性检查（失败仅告警）；
  增加"模块边界快照"测试，防止边界在后续 PR 中被无意破坏。
- **触及面**：`Runtime` 的调度循环、测试目录。
- **风险**：不改善 M1/M3，只是让现状可度量、可回归；可能形成"测了但没改"的长期负债。
- **验证**：度量报告本身即可作为验证（阶段→操作映射表随每次构建产出）。
- **工作量**：S。

### P6（可选补充）— 按"物理量 schema"划分

- **做法**：以数据而非代码为划分单位：为每个物理量（`rho`、`J`、`E`、`B`、粒子属性）
  定义 schema（分量、节点性、guard cell、checkpoint 语义），由 schema 驱动分配、通信、输出、重启。
- **触及面**：`DataBus`/`FieldSchema` 已有雏形；需扩展到 checkpoint（A4）与输出。
- **风险**：与 AMReX 的 `IndexType`/`IntVect` 语义要对齐；RZ 与 1D/2D 的分量映射差异较大。
- **验证**：同一输入在 schema 驱动与硬编码两条路径下逐位一致。
- **工作量**：L。
- **备注**：这条与 M1 无关，但与 M2 强相关——它把"模块契约"从字段名提升到字段语义。

---

## 6. 选项之间的关系（只陈述约束，不排序）

| 组合 | 关系 |
|---|---|
| P1 + P3 | 可并行；P1 改变接口，P3 只改声明消费，无冲突 |
| P2 + P3 | **P3 是 P2 的前置**：先把契约消费起来，再拆执行单元，才能验证拆分后读写关系是否仍自洽 |
| P1 + P2 | 可并行，但都触及 `Runtime.cpp` 注册段，需协调一次 |
| P4 + 任意 | 独立；但 P4 的抽取若改变内联边界，会干扰 P2 的逐位对比，宜错开 |
| P5 + 任意 | 独立；可作为其它选项的验收基线 |
| P6 + P2 | P6 是数据侧重构，若先做，P2 的求解器拆分可按 schema 而非硬编码字段名声明读写，减少返工 |

若只允许做一件事，`P3` 的投入产出比最高（S 级工作量，直接让 M2 从 0 变为 1）。
但这一判断仍取决于是否把"契约可校验"视为优先目标——本文不代为决定。

---

## 7. 任一选项都适用的验证手段

| 手段 | 适用判据 | 说明 |
|---|---|---|
| `WARPX_DISABLE_PLUGIN_HANDLERS` A/B 逐位对比 | M1、M3 | 现有开关，可验证"改结构不改结果" |
| `ctest -R '^runtime_'` | 全部 | 现有 6 个算例；注意当前构建树路径与 ctest 注册不一致，需重新 configure |
| 契约冲突单测 | M2 | 构造"两个 handler 写同一字段"的最小用例 |
| 构建矩阵（1D/2D/3D/RZ × CPU/GPU） | 全部 | 拆分常引入维度相关的编译分支 |
| 内核启动计数 / 内存峰值 | M1、M3 | 拆分可能增加或减少一次 kernel；不逐位时用它解释差异 |
| `grep` 计数快照 | M1、M5 | 见附录，可作为"边界未被侵蚀"的回归阈值 |

---

## 8. 待决问题（选择路径前需要先回答）

1. **"功能"的粒度以什么为准**：一次内核调用？一个物理量？一个可替换算法？三者对应的模块数差异很大。
2. **API 稳定性承诺何时开始**：一旦外部插件依赖某个窄接口，该接口的签名即成为兼容性负担。是否先做内部拆分、后对外承诺？
3. **是否接受"半程"模块化**（M2/M5 达标、M1/M3 暂缓）？若不接受，短期交付目标如何调整？
4. **逐位一致是否是不可让渡的要求**？若允许容差，P2 的可行性显著上升。
5. **`Push` 的推进与沉积是否允许拆开**？两者在部分算法中共享中间量，拆开需要额外存储或额外一次遍历。
6. **各求解器是否应成为独立可替换单元**？若目标只是"新增一种求解器"，扩展现有分支可能比拆分更经济。
7. **资源所有权是否还有未收敛的项**？本文只抽样检查了 5 类资源。
8. **顶层 18 个目录是否也需要按功能重划**，还是只调整 Runtime 层与执行层？两者代价差一个数量级。
9. **文档与代码的边界如何同步**：本仓库已有大量设计文档，若模块重划，文档中的路径引用需要同步更新。
10. **谁承担"边界守护"职责**：是否需要 CI 中的结构回归检查（目录/接口/契约计数），防止拆分成果被后续 PR 回退。

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
grep -rn "WarpX::GetInstance()" Source --include='*.cpp' --include='*.H' | cut -d/ -f2 | sort | uniq -c | sort -rn

# 最大文件
find Source \( -name '*.cpp' -o -name '*.H' \) | xargs wc -l | sort -rn | head -12

# 接口宽度
grep -c "virtual" Source/Runtime/HostServices.H
grep -c "virtual" Source/Runtime/RuntimeData.H

# Runtime 层规模
wc -l Source/Runtime/*.H Source/Runtime/*.cpp | tail -1
```

> 以上测量基于本文撰写时的仓库状态。数字会随提交变化，判据（M1–M5）不随变化。
</content>
