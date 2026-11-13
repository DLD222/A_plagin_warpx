# 插件化五条要求：原话、解读、现状、目标与实施路线

<!-- SPDX-License-Identifier: BSD-3-Clause -->

> **文档定位**：这份文件是"要求与路线"文件，不是"完成声明"。
> - 它记录用户（项目负责人）的原话、这些原话的可检验解读、当前代码的真实状态、目标形态与实施步骤。
> - **所有"现状"判断以代码为准**，不以任何设计文档的自述为准；凡本文写"未通过"，均指在代码中找不到该能力的实现或验收。
> - 它不替代 `A7_*` 系列文档。A7 系列描述"在现有架构内做模块化"的施工；本文描述"把执行权与状态权上移到核心"的路线。两者有交集，见 §4.5。
> - 状态快照时间：**2026-11-08 12:30（HEAD `b40f342`）**。本仓库有另一个会话在并行推进 A7 系列，见 §9。

---

## 1. 原话

> **（2026-11-08 12:24，本轮要求）**
> "记住我的要求：物理无关的核心；插件接口通用。插件依靠总线耦合；用户根据自身需求有时候要额外接入多个自己写的新插件，接口与总线必须支持、不需要修改核心后才能接入；插件功能完全包含warpx原版项目的功能。"

> **（2026-11-08 12:24，同一轮的施工约束）**
> "另一个会话z正在改的是A7系列，正在改"

> **（2026-11-08 11:11 前后，判定方法）**
> "注意不要被文档误导，以代码为根据，但是跟我不要讲代码"

> **（2026-11-08 11:11 前后，提出这两个问题的动机）**
> "如果会，怎么解决这两个问题（因为这背离了插件化、模块化的根本目标）？"

> **（2026-11-08 11:11，本轮之前已确认的两个阻断项）**
> "4.『Core 无 Plugins』只在 include 层成立…… 5. 插件 ABI 名为 C、实为 C++……"
> "先这两个"

---

## 2. 我对原话的解读

把你的话拆成 5 条**可检验**的要求（下称 R1–R5）。每条都给出"怎样才算通过"，避免"目录搬家"或"文档自述"被当成通过。

### R1 物理无关的核心

原文："物理无关的核心"。

- 含义：核心**不含任何具体物理对象**——没有 E/B、rho、粒子容器、Yee/Boris/Esirkepov/PSATD 这类名字，没有"推粒子并沉积""解场"这类整包物理动作。
- 判据：
  1. 核心目录中搜不到具体物理名词；
  2. 删除全部物理插件后，核心仍能构建；
  3. 核心仍能给出**明确诊断**（"没有可用内核/没有注册某能力"），而不是链接失败或空指针；
  4. 核心不读取任何插件私有状态（配置视图、时间视图都必须由核心自己拥有或由插件推入）。
- 澄清：R1 不等于"核心是空的"。核心必须**拥有执行权**（步进、调度、层级资源生命周期、检查点驱动），否则核心里没有物理、但也没有运行时，R1 会变成"核心被架空、PicEm 仍是大脑"。

### R2 插件接口通用

原文："插件接口通用"。

- 含义：同一套接口能承载**能力型扩展**，而不是只能挂"一步回调"。
- 至少覆盖六类：新场方程/新求解器、新材料模型、新边界、新源项、新诊断、新 AMR 策略；再加粒子模型、时间积分器。
- 判据：
  1. 每类能力都有一个**通用插头**（同一形态的接口 + 清单 + 能力声明），新增实现只写实现、不写"胶水"；
  2. 接口参数里只出现**核心自己的概念**（状态键、阶段、上下文、服务），不出现某个具体插件的类型；
  3. 一个插件声明自己"需要什么、提供什么"，核心在能力不满足时**明确拒绝**。

### R3 插件依靠总线耦合

原文："插件依靠总线耦合"。

- 含义：**总线是唯一数据通道**。插件之间零直接引用、零互相持有；插件对状态的一切读、写、创建都经总线，且必须事先声明。
- 判据：
  1. 任意两个插件之间没有 include、没有符号引用（构建层可检）；
  2. 内置插件也**只经总线**读写状态（当前恰恰相反，见 §3.3）；
  3. 未声明的访问**运行时报错**，而非静默成功；
  4. 总线能承载全部状态类别（场、物种、材料、标量、粒子属性），不是只有两类键。

### R4 零改核心接入多个自写插件

原文："用户……额外接入多个自己写的新插件，接口与总线必须支持、不需要修改核心后才能接入"。

- 含义：仓库外作者的两个以上插件，**不改核心**（也不改别人的插件）即可构建、加载、协同运行。
- 判据（硬门）：
  1. 仓库外插件 A：新增一个**阶段**并拥有**自己的场**，随网格重划存活；
  2. 仓库外插件 B：**替换**某个已有能力（例如解场），与 A 同时加载；
  3. 两者的耦合只经总线（A 写、B 读，或反之），顺序由声明决定；
  4. 全程 `git diff` 不涉及核心目录。
- 这条是根本目标，也是当前差距最大的一条。

### R5 功能完全包含原版 WarpX

原文："插件功能完全包含warpx原版项目的功能"。

- 含义：不是"能跑通几个算例"，而是**功能等价**：原版支持的物理、算法、维度、输入参数都在。
- 判据：
  1. 四维（1D/2D/3D/RZ）× 全部官方算例，数值等价（逐位或严格容差）；
  2. 原版输入参数继续可用（参数归属插件，但语义与默认值不变）；
  3. 组合特性（EB、QED、PSATD、隐式、碰撞、激光、AMR）逐个有验收，不以"环境缺失"当通过；
  4. 删任一插件后，其余插件仍构建并通过各自验收。

### 五条之间的关系（顺序不能反）

```
R1 ← 依赖 ← R4（执行权先在核心手里，核心才可能物理无关）
R3 ← 依赖 ← R2（状态先可注册、可分配，总线才可能成为唯一通道）
R5 ← 依赖 ← R2 + R3（拆插件时不能靠"回到原内核内部"补功能）
```

因此实施顺序必须是：**先搬执行权 → 再扩阶段 → 再让状态可注册 → 再补生命周期与能力 → 再让总线承载数据 → 最后才拆插件、做全覆盖验收。**
先拆目录（当前正在做的 A7 的一部分）能得到"目录是插件、实现仍是原内核"的结果，这正是 R1–R4 要杜绝的形态。

---

## 3. 现在项目的情况（以代码为证）

### 3.1 真实成立的（可以写"完成"的）

| 项 | 证据 |
|---|---|
| 核心可独立构建 | 核心目标单独构建通过，只编 9 个源；核心编译选项里 0 个插件路径；核心源码 0 个插件源 |
| 构建层边界 | `check_build_boundaries.py` 通过：核心 CMake 不得登记插件源，插件只能登记到自己的层 |
| include 层边界 | `check_plugin_boundaries.py` 通过：核心不 include 插件头，插件之间不互相 include |
| 内核工厂解耦 | 核心不含内核实现，只持注册表；未注册时抛出带指引的异常而不是链接失败 |
| C 注册 ABI | 插件描述符与处理器描述符为纯 C；一个纯 C 编的插件可被加载（该 .so 只有 1 个未定义宿主符号、动态依赖只有 C 运行库） |
| 处理器排序与冲突检测 | 同阶段内按"写→读"拓扑排序；同阶段双写、成环都会被拒绝 |

### 3.2 不成立的（R1–R5 逐条判定）

| 要求 | 判定 | 代码层面的理由 |
|---|---|---|
| R1 物理无关 | **未通过** | 核心目录仍有 **49 个文件**直接出现"粒子"语义（粒子工具、碰撞 tile、加速器元件、WarpX 形状的粒子接口），粒子相关的公共头也在核心（A7 正在继续把更多粒子单元移进核心，见 §4.5）；核心的配置视图仍从 WarpX 静态配置读出；核心不拥有步进循环 |
| R2 接口通用 | **未通过** | 接口只有"注册 + 一步回调 + 重划回调"三件套；没有初始化、建层、检查点、重启、收尾；没有能力集与依赖声明；六类能力中，新材料/新边界/新源项/新 AMR 策略没有可插的形态 |
| R3 总线耦合 | **未通过** | 全部内置处理器**没有一个**经总线读写状态，它们回调的是原内核的整包动作；总线只有"场 + 物种"两类键，没有材料/标量/粒子属性；不能创建场；声明了"写"但运行时不校验 |
| R4 零改核心接入 | **未通过** | 阶段是固定枚举，且全仓库**只有 5 个阶段被真正调度**（推粒子、推完、解场、诊断、碰撞），其余 9 个阶段注册了也永不触发；每层场分配是硬编码清单；物种只能"登记已有的"；C 面只有注册与只读场指针；外部插件无法提供内核 |
| R5 功能全覆盖 | **未通过** | 2D 有 255 个测试、1D/3D/RZ 有 469 个，但没有形成"四维 × 全算例 × 数值等价"的证据链；失败判定里混着环境缺失与 checksum 平台差异（当前 2D 基线有 41 项失败，其中 34 项 checksum、4 项要求 openPMD、3 项为其下游） |

### 3.3 两个结构性事实（决定路线）

**事实一：执行权不在核心。**
核心的"步进"是把步数转交给内核；每一步内部"推粒子→沉积→解场→诊断→碰撞"的顺序、以及阶段调用点，全部写在 PicEm 里。核心因此只能"宿主 + 契约 + 编目"，不能"运行"。

**事实二：数据流不经过总线。**
总线设计成"单一存储 + 名称键"（不复制、别名共享、声明式规格、注册期冲突检测），但真实处理器全部绕过它。总线实际只承担两件事：登记场规格（供外部读取/诊断/检查点引用）和登记物种。也就是说，**总线现在是"编目 + 只读窗口"，不是"耦合通道"**。

### 3.4 规模快照（2026-11-08 12:30）

| 项 | 数值 |
|---|---|
| 源码 `.cpp` | 291 |
| 核心 `Runtime` 目录文件 / 其中接口 | 59 / 27 |
| 核心 `Modules` 目录文件 | 31 |
| 核心中提到"粒子"的文件 | 49 |
| 插件目录 | 3（PicEm、Diagnostics、IO） |
| 运行期真正注册的处理器（Yee 路径） | 5 |
| 被真正调度的阶段 | 5（共 14 个已定义） |
| 测试总数（2D / 1D+3D+RZ） | 255 / 469 |

### 3.5 结论

现在准确的位置是：**可装配，不可扩展**。
"把处理器挂上去"是插件化的；"增加一种能力"目前不是插件化的。R1/R3/R4 的根因是同一个——**执行权与状态权还在插件里**。

---

## 4. 目标项目的构思

### 4.1 总图（目标形态）

```
┌──────────────────────────────────────────────────────────────┐
│ 核心（物理无关）                                              │
│  ├ 执行引擎：步进循环 / 阶段表 / 调度器 / 层级资源驱动        │
│  ├ 状态底座：状态登记表（场·物种·材料·标量·粒子属性）+ 总线  │
│  ├ 插件宿主：清单 / 能力 / 依赖 / 生命周期 / 禁用             │
│  ├ 原子服务：边界填充·归约·通信·解析器·随机数·FFT·泊松·计时  │
│  └ 契约群  ：求解器·粒子模型·材料·边界·源项·诊断·AMR·积分器   │
└───────────────▲───────────────────────────▲──────────────────┘
                │ 只经总线读写（声明式）      │ 只经清单注册
┌───────────────┴───────────────┐  ┌────────┴─────────────────┐
│ 物理插件（可多个、可替换）      │  │ 能力插件（可多个）        │
│ PicEm: 求解器/粒子/碰撞/QED/…  │  │ 新材料·新边界·新源项·     │
│ 第三方的替代内核 / 新物理       │  │ 新诊断·新 AMR 策略        │
└───────────────────────────────┘  └──────────────────────────┘
```

### 4.2 六条设计不变量

| # | 不变量 | 违反时的可检验症状 |
|---|---|---|
| I1 | 核心不含物理名词与整包物理动作 | 核心目录出现具体物理对象名 |
| I2 | 插件的唯一通道是总线与清单 | 插件 A include 了插件 B |
| I3 | 状态的生命周期归核心（创建/重划/销毁/检查点） | 插件自己 new 场、自己处理重划 |
| I4 | 新能力**不需要**改核心 | 新增阶段/新场/新物种要改核心文件 |
| I5 | 旧输入不破坏、默认路径数值不变 | 原算例 checksum 变化 |
| I6 | 能力不满足时明确拒绝 | 运行期崩溃或静默走错分支 |

### 4.3 三个必须补齐的机制

1. **可注册阶段表**（替代固定枚举）：阶段用"名字 + 序号 + 父阶段"声明，插件可注册；核心保证"同阶段按数据依赖有序、跨阶段按声明序"。这是 R4 的第一前提。
2. **可注册状态 + 核心驱动的层级生命周期**（替代硬编码场清单）：四类键各有规格（分量、中心、ghost、单位、是否进检查点、owner）；核心按登记表在建层/重划/负载均衡后创建与重建；插件只能"声明拥有"，不能自己分配。
3. **插件清单与生命周期**（替代"只有注册函数"）：清单声明名字/版本/角色/依赖/能力/阶段占用；生命周期含 schema、初始化、建层、重划后、检查点、重启、收尾；核心据此装配与校验。

### 4.4 与现架构的差异

| 维度 | 现在 | 目标 |
|---|---|---|
| 谁拥有步进 | PicEm | 核心 |
| 阶段从哪来 | 核心固定枚举，PicEm 决定调用点 | 核心阶段表，插件注册 |
| 场的分配 | PicEm 硬编码清单 | 核心按登记表 |
| 插件读写状态 | 回调原内核整包动作 | 只经总线，声明式 |
| 插件可做的生命周期 | 一步回调 + 重划回调 | 七类钩子 + 能力校验 |
| 外部插件能提供什么 | 只能是处理器 | 处理器、阶段、状态、能力实现、内核替代 |
| 功能覆盖的验收 | 部分维度 + 冒烟 | 四维 × 全算例 × 数值等价 |

### 4.5 与 A7 系列的关系（重要，涉及并行施工）

A7 系列（另一会话）在做的事：粒子能力契约（tile/iterator/reduce/属性访问）、碰撞迁移、构建层拆分（已完成）、C 注册 ABI（已完成）。这些都**可以成为**本路线 S5/S6 的输入：契约、边界检查、构建分层都是必需的零件。

但有一处**设计张力需要裁决（记为 D1）**：

> A7.2.4 的做法是把 WarpX 形状的粒子单元（tile/iterator/reduce、粒子创建工具、加速器元件）**移进核心**，以便"碰撞不必 include 粒子实现"。
> 这与 R1/R2 冲突：这些单元带具体粒子语义与具体存储布局，进核心后核心就不再物理无关，通用性也被"WarpX 的粒子形状"锁死。

D1 的两个选项（需负责人裁决，本文不擅自决定）：

- **D1-a（与 R1 一致）**：核心只保留**通用**粒子契约（"有多少粒子、遍历、归约、按名取属性"），具体 tile/SoA 访问与工具留在粒子插件；碰撞通过通用契约访问。
- **D1-b（与 A7 一致）**：承认核心内置"PIC 粒子底座"，把它作为核心的**标准扩展层**，并在 R1 的判据里显式豁免；代价是核心不再是纯物理无关。

建议：**D1-a**。因为它同时满足 R1 与 R2，且 S3（状态可注册）会自然提供"粒子属性作为可注册键"，比把 WarpX 的具体布局搬进核心更省后续返工。

---

## 5. 预期实现方法与步骤

原则：**每一步都保持可编译、可回归；每一步都有可跑的验收门；顺序不可交换。**
工作量是排序用的粗估（S/M/L），不是承诺。

### S0 规格与证据矩阵（S）

- 做法：把 R1–R5 落成"要求 → 检查 → 当前判定"的矩阵；每条要求挂一个可跑检查（能复现"未通过"）；把 D1 提交裁决。
- 产出：本文件 + `Tools/` 下的检查脚本骨架（能力矩阵、阶段覆盖、总线使用率）。
- 门：矩阵里每条都能在当前代码上跑出"未通过"。
- 风险：检查写成"文档式检查"（只查字符串）。要求：检查必须基于构建/运行证据。

### S1 执行权上移（M，拆两半）

**S1a 区间调度（完成）**：核心提供"按阶段区间执行"的调度器，PicEm 不再逐阶段点名，
只给出区间；区间内**所有**处理器（含插件插入的自定义阶段）由核心按序数顺序执行。

```text
[before_push, after_push)        推粒子（含 before_push）
[after_push, before_collisions)  沉积收尾
[before_collisions, after_collisions)  碰撞
[before_field_solve, after_field_solve)  解场
[diagnostics, finalize)          诊断
```

门（全部满足）：
1. 2D 全量失败集合与改动前**完全一致**（41/41）；
2. 仓库外语义的验收测试 `stage_dispatch_2`：插件在 `after_push` 之后注册的阶段
   在**真实一步演化**中被执行，且注册在 `before_init`（任何区间之外）的处理器
   **不会**被执行；
3. 核心与 PicEm 都不含"逐阶段点名"的分发。

**S1b 步进归属（未开始，依赖 S6）**：把区间序列与步进循环本身也交给核心，
PicEm 退化为"提供物理处理器 + 服务"，这是删除 WarpX（A7.9）的前置。
之所以放在 S6 之后：碰撞/诊断/沉积的调用点与演化格式（显式/隐式/子循环）
强耦合，在物理仍集中在 PicEm 时搬循环会把耦合一起搬进核心，反而妨碍 R1。

- 单步验收硬门：每步与改动前**失败集合一致**，不以"看起来对"为准。

### S2 阶段表可扩展（S）

- 做法：阶段从固定枚举改为可注册（名字 + 序号 + 父阶段）；核心保证同阶段内按数据依赖排序、跨阶段按声明序；插件可注册自定义阶段。
- 门：仓库外插件注册一个自定义阶段并被调度；核心零改动。
- 风险：跨阶段顺序的确定性。要求：调度表可打印、可断言。

### S3 状态可注册（L）

- 做法：场/物种/材料/标量/粒子属性统一进登记表与总线；核心按登记表在建层、重划、负载均衡后创建与重建；检查点由登记表驱动（新增状态默认持久化）；插件"声明拥有"，不得自行分配。
- 门：
  1. 仓库外插件拥有自己的场，随网格重划存活，并被写入检查点、重启后恢复；
  2. 原有场的检查点格式仍可读（向后兼容门）。
- 风险：检查点格式与旧格式兼容；重划时的数据搬移。要求：旧 checkpoint 可读、新格式可写读回，两条都要有测试。

### S4 插件清单、生命周期与能力（M）

- 做法：插件清单（名字/版本/角色/依赖/能力/阶段占用）；生命周期钩子（schema、初始化、建层、重划后、检查点、重启、收尾）；能力不足时明确拒绝；按插件禁用。
- 门：缺能力时给出可读错误而不是崩溃；任一插件可单独关闭且其余仍构建运行。
- 风险：能力声明与真实行为不一致。要求：能力声明由测试对照（声明不支持 AMR 就必须在 AMR 输入下被拒）。

### S5 总线成为唯一通道（L）

- 做法：把内置处理器从"回调原内核整包动作"改为"经总线读写状态"；总线加访问校验（只允许声明过的读写）、键的强约束（类别/分量/层号）、错误即报。
- 门：
  1. 现有 2D 全量数值逐位不变；
  2. 内置处理器不出现"绕过总线"的路径（可检查）；
  3. 未声明访问会被拒绝（负面测试）。
- 风险：性能（每步的校验不能进入 cell 循环）。要求：校验只在阶段边界发生。

### S6 按物理功能拆插件（XL，分批）

顺序（每批都要"删得掉 + 数值不变"）：

1. 场求解器（有限差分/Yee、静电、PSATD、隐式、静磁）
2. 粒子核心（容器、推进、沉积、收集、边界、注入、重采样、排序）
3. 碰撞、QED
4. 激光、粒子 IO
5. 边界/PML/EB
6. 初始化、流体与矩、加速器元件
7. AMR 策略、时间积分器

- 门（每批）：删掉该插件，其余仍构建并通过各自验收；该插件启用时数值与拆分前一致。
- 风险：粒子/碰撞/诊断之间的共享状态最多。要求：先做 S3/S5，再动这一批。

### S7 全覆盖验收（M）

- 做法：四维 × 全部官方算例；组合特性矩阵（EB/QED/PSATD/隐式/碰撞/激光/AMR）；仓库外双插件示例（新阶段+新场；替换解场）；删插件矩阵。
- 门：全部通过；报告中不得把"环境缺失"计为通过。
- 风险：测试总量大、耗时长。要求：分"快门"（每次提交）与"全门"（每日/合并前）。

### 里程碑

| 里程碑 | 内容 | 可宣布什么 |
|---|---|---|
| M1 | S1+S2 完成 | "执行权在核心，阶段可扩展" |
| M2 | S3+S4 完成 | "状态可注册，插件有生命周期" |
| M3 | S5 完成 | "总线是唯一通道" |
| M4 | S6 完成 | "功能由插件组成，可删可换" |
| M5 | S7 完成 | "五条要求成立" |

---

## 6. 验收矩阵（本文件的核心表）

| 要求 | 验收检查（未来挂到 CI） | 当前 |
|---|---|---|
| R1 | 核心物理名词扫描 = 0；删除全部物理插件后核心构建 + 明确诊断 | 未通过 |
| R2 | 六类能力各有一个仓库外实现示例（只写实现，不写胶水） | 未通过 |
| R3 | 插件间零引用检查；内置插件"经总线访问"覆盖检查；未声明访问负面测试 | 未通过 |
| R4 | 仓库外双插件（新阶段+新场 / 替换解场）零改核心运行 | 未通过 |
| R5 | 四维全量数值等价 + 删插件矩阵 + 组合特性矩阵 | 未通过 |

**规则**：任何文档里出现"完成/已实现"，必须同时给出上表对应的检查名与最近一次运行结果；否则视为未验证。

---

## 7. 风险与"不做什么"

| # | 风险 | 缓解 |
|---|---|---|
| P1 | 把"目录移动"当成插件化 | 每个里程碑只认表 §6 的检查 |
| P2 | 核心里继续堆粒子/求解器专用单元（D1-b 的滑坡） | D1 需显式裁决并写入判据 |
| P3 | 总线上"校验"进入热循环导致变慢 | 校验只在阶段边界；性能门单列 |
| P4 | 检查点格式破坏旧数据 | 双读 + 版本号 + 旧文件回归 |
| P5 | 与 A7 并行施工互相踩 | 见 §9 协作协议 |
| P6 | 用"环境缺失"当通过 | 报告中禁止；失败要分类（代码/环境/平台） |

**不做**：不一次性重写全部物理；不在 S3 完成前拆粒子/碰撞；不做"插件直接调插件"的快捷方式；不为了通过检查而放宽检查。

---

## 8. 本文与其他文档的关系

| 文档 | 关系 |
|---|---|
| `最终的技术路线.md` | 架构愿景的来源；其中"内核不含物理、插件只经总线"与本文一致；但它的进度自述不再作为验收依据 |
| `A7_MASTER_PLAN.md` / `A7_REORDERED_PATH.md` / `A7_BUILD_ABI_AUDIT.md` | A7 施工线；本文是其上游要求。A7 的构建拆分与 C 注册 ABI 已作为 S5/S6 的零件 |
| `MODULARITY_AUDIT.md` / `ARCHITECTURE_*_AUDIT*.md` | 问题清单来源；本文不重复其内容 |

---

## 9. 与另一会话（A7 系列）的协作协议

**对方正在改的**（截至 12:30，工作区未提交）：碰撞相关的粒子访问迁移、`ParticleCreation/SmartCopy.H`、`SmartUtils.H` 移入核心、碰撞实现文件批量修改。

**协议**：

1. **文件边界**：本路线（S0–S7）不修改 A7 会话正在改的文件——`Source/Core/Runtime/IParticleContainer.H`、`Source/Core/Modules/Particles/*`、`Source/Plugins/PicEm/Modules/Particles/Collision/*`、`tests/runtime/*`、`Docs/research_notes/A7_*`。
2. **提交边界**：每次提交只带自己的路径（显式路径提交），不 `-a`、不 `-A`，避免吞掉对方未提交的改动。
3. **冲突点**：S1（执行权上移）会改 `WarpXEvolve`，A7 也会改 PicEm 内部；两者须串行——S1 开工前双方确认 PIC 侧无在途改动。
4. **D1 需裁决**：A7.2.4 的"粒子单元进核心"与本文件 R1/R2 冲突。在裁决前，A7 继续按现状推进，但**不得据此宣布 R1/R2 成立**。
5. **共同资产**：构建层拆分、C 注册 ABI、两个边界检查脚本视为双方共用，任何一方修改都要跑全量门。

---

## 10. A7 系列与本路线的映射（A7 收尾用）

按 D1-a（核心保持物理无关）执行后，A7 剩余项并入本路线，避免两次返工：

| A7 项 | 并入 | 说明 |
|---|---|---|
| A7.2.1–A7.2.3（诊断/IO 独立、边界检查） | 已完成 | 两个边界检查脚本保留为门 |
| A7.2.4（粒子能力契约） | **S3 + S5** | 只保留通用契约；具体 tile/工具留在粒子插件（D1-a）；2026-11-08 已清掉 11 个 Core/插件重复头（其中 6 个内容已分叉）与 3 个别名式伪契约 |
| A7.2.5–A7.2.7（内核契约、禁用开关、验收） | **S4** | 由插件清单/能力/生命周期统一承载 |
| A7.3 ParticleCore 插件 | **S6 第 2 批** | 拆插件在总线与调度就位之后做 |
| A7.4 FieldSolver 插件 | **S6 第 1 批** | 同上 |
| A7.5–A7.7（诊断/边界/其余模块） | **S6 第 3–7 批** | 同上 |
| A7.8 插件 ABI | **已完成（C ABI v4）** | 见 §11 |
| A7.9 删除 WarpX | **S7 的终态** | 四维全量通过后 |

---

## 11. 实施进度（滚动更新）

| 步骤 | 状态 | 证据 |
|---|---|---|
| S0 规格与证据矩阵 | 进行中 | 本文件；检查脚本待补 |
| S1a 区间调度 | **完成** | 核心 `Scheduler` 按区间执行；PicEm 只给区间；`stage_dispatch_2` 证明插件自定义阶段在真实演化中执行、区间外的处理器不执行；全量失败集合仍为 41/41 |
| S1b 步进归属（删除 WarpX 前置） | 未开始（依赖 S6） | — |
| S2 阶段表可扩展 | **完成** | 阶段为整数序数 + `StageRegistry`（按名注册、锚点定位、幂等、空闲段中点插入）；未注册阶段注册处理器会被拒绝；新增 `stage_registry_2` 测试；C ABI 升到 v4（阶段序数改为 0/1000/…/13000） |
| S3a 场（插件声明、核心分配/重划/检查点） | **完成** | 核心 `StateRegistry` + `AllocPluginFields`：插件声明拥有的场由核心在每层分配，动态加密后在新层自动创建，并进入 schema 驱动的检查点；`plugin_field_2` 三项断言通过 |
| S3b 标量可注册 | **完成** | `ScalarRegistry` + `StateRegistry::request_scalar`；总线 `scalar`/`set_scalar` 受声明约束；标量参与处理器排序与同阶段双写检测；C ABI v5 增加标量声明与读写；`scalar_state_2` 七项断言通过 |
| S3c 物种/材料/粒子属性 | 未开始 | 物种已可读（既有）；材料与粒子属性随其插件（S6） |
| D1-a 粒子单元归位（第 1 批） | **完成** | Core 与 PicEm 各存一份同名粒子头的问题已消除：Core 保留通用副本，插件副本删除、33 个文件 include 改写；`SmartCopy/SmartUtils/InitializationPolicy` 移回插件；`IParticleIterator` 随具体类型移回；删除 `IParticleTile/IParticleCollisionTile/IParticlePositionAccess` 三个别名式契约（对应编译检查随之更新）；`ParticleUtils.cpp` 实现改用通用容器类型；边界检查器新增「Core 与插件不得同名重复头」规则 |
| A7.4 粒子插件第 1 单元（ParticleThermalizer 迁出内核） | **完成** | 新增独立层 `warpx_particlecore_<dim>` + 开关 `WarpX_PLUGIN_PARTICLECORE`（默认 ON）；核心契约 `Source/Core/Runtime/IParticleThermalizer.H`（`defined()` / `apply(ParticleContext const&)`）+ 工厂（未链接返回 `nullptr`）；插件静态注册 + 锚点 `warpx_particlecore_anchor`；内核 `WarpX.H/.cpp`、`WarpXEvolve.cpp` 只经契约调用，不再包含插件头。插件实现内 **0** 处 `WarpX::`、**0** 个 `WarpX.H`、**0** 个具体容器类型（物种经 `IParticleContainerAccess`，tile 用 AMReX SoA 迭代器，几何取自 `IParticleContainer::Geom`）。行为保持：同一概率公式/阈值语义/RNG 顺序；无 `species` 过滤时遍历 species+lasers（与旧 `for (auto& pc : mpc)` 同一集合）。1D A/B 证明插件真的执行：无热化 `2.677077e6 → 2.677052e6 J`，有热化 `2.677077e6 → 1.924147e6 J`（step1）→ `1.380356e6 J`（step3）。`WarpX_PLUGIN_PARTICLECORE=OFF` 的 2D 构建通过并跑通普通算例；此时若输入要求热化，启动即报错并提示开关（不静默改变物理）。四检查器 0/0/0/OK；2D 全量 ctest 263 项：43 项失败 = 32 项 openPMD 环境项 + 4 个用例（11 项）在并行负载下 1500 s 超时，这 4 个用例单独重跑 12/12 通过；与上一次记录的 52 项相比**新增 0**。1D/3D/RZ 全量 ctest 469 项：216 项失败 = 204 项与接手基线逐条一致 + 4 个用例（12 项）并行负载超时，单独重跑 12/12 通过后失败集回到 **204**、**新增 0**。其余粒子单元的量化阻塞（31 cpp / 18747 行 / 17 个 `WarpX.H` / 71 行 `WarpX::` / 256 行具体容器类型）见 `A7.4_PARTICLE_SYSTEM_PLAN.md` §10 |
| S4/R5（第 14 步·插件状态声明闭环） | **完成** | 碰撞的 `*_particle_production` 场改为**在 `on_schema` 阶段声明、由核心随层分配**（新增 `ICollisionHandler::declare_state` + 生命周期桥 `CollisionStatePlugin`），插件里只保留"核心没分配才补"的回退路径。修复 3D/RZ 的 D-T 聚变诊断（此前诊断初始化时字段还不存在）；多维失败集 207 → **205**（剩余全部为 openPMD 环境项；`test_3d_particles_in_pml_mr` 单独运行 98 s 通过，是并行下的偶发超时而非回归） |
| R5 多维基线（第 13 步·再修一个真实缺陷） | **完成** | `computeRho` 的"分量数必须等于诊断请求数"校验在 RZ 下必然失败（沉积 rho 有 `2*nmodes-1` 个分量，诊断要 1 个）；改为显式 `allow_extra` 参数，仅当诊断做 RZ→笛卡尔转换时允许——其余情形保持严格校验。`test_rz_laser_acceleration` 的 **run 与 checksum 双双通过**（checksum 证明数值正确），多维失败集 209 → **207**，2D 仍 33 项 |
| R5 多维基线（第 12 步·修真实缺陷） | **完成** | 多维基线暴露的第一个真实缺陷已定位并修复：`WarpX::getdt()` **按值返回**，而 `RuntimeData::dtAll()` 以 `const&` 转发它——调用方拿到的是**悬垂引用**，在 3D 加速器晶格路径（每粒子构造外场收集器）上读到释放的内存而崩溃。改为返回 `const&` 后：4 个晶格/等离子体透镜用例与 1D 宏观求解器用例全部恢复，多维失败集 221 → **209**；2D 仍为 33 项不变 |
| R2/R4 外部插件验证（第 11 步） | **完成** | 同一缺陷模式继续排查：`BeforeInit`/`Init`/`AfterInit`/`Finalize` 四个阶段点**同样没有调度**。已补初始化三段与 Finalize 点的调度；外部插件演示现在同时验证 init、after_field_solve、finalize 三处都执行；`stage_dispatch_2` 原先断言的正是这个缺陷（"不在区间内的处理器不应执行"），已改为断言修复后的语义 |
| R2/R4 外部插件验证（第 10 步） | **完成** | 在**仓库外**用普通 gcc 写并编译了一个插件（只包含公共 C ABI 头、不链接任何 WarpX 代码），用 `WARPX_PLUGIN_PATH` 加载进真实算例：清单（名称/版本/角色）与处理器注册成功、处理器在 `AFTER_FIELD_SOLVE` 被调度执行（dt 正确）；全程**零改核心**。该验证暴露出一个真实缺陷：**注册在阶段边界点（AfterFieldSolve/AfterCollisions/Finalize）的处理器永远不会被调度**（调度区间是半开区间，缺少以这些点开头的区间）——已补两条区间调度并验证生效；同时修正可执行文件未导出符号导致外部插件 `dlopen` 失败的问题（`ENABLE_EXPORTS`） |
| S6 第 13 步·回调总线 | **完成** | 新增核心**回调总线** `Runtime/Callbacks.H`（命名事件的注册/查询/触发/移除），Python 绑定改为向总线注册、场求解器改为向总线触发；跨插件违规 26 → **20**（诊断适配器头、静电流体/回调/粒子容器、FD 遗留适配器头均已清除） |
| S6 第 12 步·跨插件依赖棘轮 | **完成** | 把 `Fields.H`（场类型枚举）与 `EmbeddedBoundary/Enabled.H` 移入核心（它们本就是通用概念），再把通用的粒子创建工具 `InitializationPolicy.H`/`SmartCopy.H`/`SmartUtils.H` 移入核心；**跨插件违规 87 → 26**（含 `_fwd.H` 豁免：前向声明头只命名类型、不产生依赖）；检查器新增 `--dump` 模式用于重新生成清单 |
| S6 第 2 批·场求解器（第 11 步·**目录迁移**） | **完成** | 有限差分与静电子树迁入 `Source/Plugins/FieldSolver/`，新增独立层目标 `warpx_fieldsolver_<dim>` 与开关 `WarpX_PLUGIN_FIELDSOLVER`；内核只经核心工厂创建，锚点随开关生效。同时把插件边界检查器改为**按实际解析路径判定**（此前按字符串前缀，会漏报无前缀的跨插件依赖）——立刻暴露 87 处真实跨插件 include，已用允许清单（棘轮）记录，新增即失败 |
| S6 第 2 批·场求解器（第 10 步·材料与静电契约） | **完成** | 新增核心 `IMacroscopicProperties` 契约（含 `materials()`），材料模型实现它并从核心工厂创建（插件注册 + 锚点），内核不再持有具体材料类型；`IFiniteDifferenceSolver::MacroscopicEvolveE` 的参数从具体材料类改为契约引用（**核心接口不再出现具体材料类型**），具体类型只在插件内部显式窄化；静电侧新增 `setPotentialBoundaryString` 契约，删掉内核的 `GetElectrostaticSolverConcrete` 通道。内核里剩余的求解器包含只剩：隐式组（已文档化暂不拆）、`WarpX_FDTD.H`（内核自己的 FDTD 辅助）、静磁头（待补接口） |
| S6 第 2 批·场求解器（第 9 步） | **完成** | 有限差分求解器也改为**注册制**（核心注册表 + 插件静态注册 + 锚点），内核只调用 `create_finite_difference_solver`；`mlmgOptions()` 提升到静电契约，宿主服务不再需要「取具体求解器」的通道；内核里 3 个具体静电头文件被删除 |
| S6 第 2 批·场求解器（第 8 步） | **完成** | 内核不再 `new` 具体静电求解器：新增核心工厂 `create_electrostatic_solver(algo, max_level, context)`，静电插件侧静态注册并加链接锚点；内核侧只调用工厂，未链接时给出可操作错误；这是「内核向核心要插件」模式的第三个实例（诊断、碰撞、静电） |
| S6 第 2 批·场求解器（第 7 步·可行性定界） | **完成** | 可拆分部分（有限差分/静电/PSATD/静磁）内核耦合棘轮 **清零**；**隐式求解器暂不拆**：它通过 `Define(WarpX*)` 驱动内核的场推进、粒子保存/恢复、边界与诊断，共 198 处成员使用、约 11 类内核操作，必须先有「内核服务层」才能离开内核插件；该结论与缺口已写入路线文档，棘轮范围显式排除隐式并注明原因（避免把「暂时不拆」伪装成「已拆」） |
| S6 第 2 批·场求解器（第 6 步） | **完成** | 静电子树**清零**（核心提供 `legacy_host_services()`，两个类不再构造内核适配器）；隐式组把 7 类配置静态量（`nox`、`current_deposition_algo`、`field_gathering_algo`、`use_filter`、`max_grid_crossings`…）改读 `RuntimeConfig` 访问器，**`WarpX::` 引用清零**；棘轮 16 → **8**（只剩 8 个文件仍包含 `WarpX.H`，它们通过 `Define(WarpX*)` 使用内核对象，属于下一步的接口改造） |
| S6 第 2 批·粒子系统（第 2 步） | **完成** | 守护单元掩码与负载代价已全部走 `IRuntimeData` 契约：`data().currentBufferMasks(lev)`、`data().gatherBufferMasks(lev)`、`data().costs(lev)`（`PushX` 里最后一处 `WarpX::getCosts` 已切换）。契约对"不提供缓冲的核"给出可操作异常。验证：2D 264 个测试失败集**逐条一致** |
| S6 第 2 批·粒子系统（第 1 步收尾） | **完成（按结构可及范围）** | 容器类对内核静态量的读取已全部改走 `config()`。剩下的 3 处**结构性不可迁移**并逐条记录：①`MultiParticleContainer.cpp:348` 是把输入**解析进**静态量（写入方，不是读取）；②③`WarpXParticleContainer.cpp:141,147` 的 `ParticleTileIter` 构造函数——该类的 `ContainerType` 是 AMReX 的 `ParticleContainer_impl`（基类），没有 `config()`，尝试改写成 `pc.config()` 会编译失败（已实测并回退） |
| S6 第 2 批·粒子系统（第 1 步） | **完成** | 容器类对内核静态量的读取改走 `config()`：边界类型（与已迁移的 hi 侧对齐）、求解器 id、共享内存沉积开关、插值阶数、方位模数，共 13 处。真正无法用 `config()` 的位置保留静态量并记录原因：非容器类的成员初始化列表（初等过程）、tile 迭代器、`ParticleIO` 的自由函数、以及一处把输入解析进静态量的地方。剩余静态读取 5 处。验证：2D 264 个测试失败集与迁移前**逐条一致** |
| S6 第 2 批·粒子系统（可行性定界） | **暂不拆（已量化）** | 实测：容器家族共约 11 700 行、30 余文件，且**互相继承**（`PhotonParticleContainer` ← `PhysicalParticleContainer` ← `WarpXParticleContainer`），必须整体搬迁。对内核的依赖面其实很窄：`WarpX::` 静态量共 22 处（`particle_boundary_lo` 5、`nox` 3、`galerkin_interpolation` 3、`moving_window_v`/`do_dynamic_scheduling` 各 2，其余各 1），**全部可由已有的 `RuntimeConfig` 提供**；另有 3 处 `GetInstance()` 是 legacy 回退（构造 `WarpXRuntimeData`/`WarpXHostServices`），以及 `CurrentBufferMasks`/`GatherBufferMasks`（内核守护单元数组）与 `getCosts` 需要新契约。结论：**前置契约只有两三项**（守护单元掩码、负载代价），先例（隐式求解器的服务袋）已证明可行；真正的工作量在"一次搬 1.2 万行 + 对应数百个测试"的验证，不适合在剩余轮次内启动 |
| S6 第 2 批·场求解器（第 11 步·隐式出场） | **完成** | 隐式求解器（theta/Strang/semi-implicit/Darwin 四种）整体移入 `Source/Plugins/FieldSolver/Implicit/`，归 `warpx_fieldsolver_<dim>`，仅 `WarpX_PLUGIN_IMPLICIT=ON` 时编译。前置工作：①`NonlinearSolvers`（28 文件）移入核心；②内核侧三处未使用的 `ImplicitSolver.H` 包含删除；③`Define` 改收 `ImplicitKernelServices` 服务袋（50+ 项），求解器内已无任何 `WarpX*`；④最后的 Python `afterdeposition` 钩子改为服务袋项。校验：检查器全 0、27 个隐式测试通过 |
| S6 第 2 批·场求解器（第 10 步·隐式服务面量化） | **阻塞（已量化）** | 隐式求解器的公开入口是 `Define(WarpX*, bool)`，即直接拿内核对象驱动整步。逐文件实测耦合：`ImplicitSolver.cpp` 70 处、`ThetaImplicitEM.cpp` 34、`SemiImplicitDarwin.cpp` 60、`SemiImplicitEM.cpp` 25、`StrangImplicitSpectralEM.cpp` 8；按操作归并共约 30 个内核操作，其中 `m_WarpX->m_fields` 占 109 次（已可由 `RuntimeData` 提供），其余为粒子容器（9）、`Geom`（9）、`getngEB`（6）、`GetPECInsulator_IsESet`（5）、`SetElectricFieldAndApplyBCs`/`FinishElectricFieldAndApplyBCs`（7）、`FillBoundaryB`/`EvolveB`（8）、`SaveParticlesAtImplicitStepStart`/`ResetImplicitParticleData`/`HandleParticlesAtBoundaries`（7）、`reduced_diags`（3）、`get_pointer_fdtd_solver_fp`（3）等。**结论**：需要一层「内核服务」契约（约 20 个方法）才能离开内核插件；在此之前不拆，避免把「暂时不拆」伪装成「已拆」 |
| S6 第 2 批·场求解器（第 9 步·静磁契约） | **进行中** | 核心已定义 `IMagnetostaticSolver` + `MagnetostaticConfig` + 工厂（配置携带场注册表、0 层几何、层数、dt、四个求解容差，以及三项仍属内核的操作：逐物种沉积电流、跨层同步、RZ 体积缩放）；内核实现尚未改为走契约，行为未变（下一步写适配器） |
| S6 第 2 批·场求解器（第 8 步·PSATD 出场） | **完成** | PSATD 子树整体移入 `Source/Plugins/FieldSolver/PSATD/`，源码归入 `warpx_fieldsolver_<dim>`；内核只持有 `ISpectralSolver` 并经核心工厂 `create_spectral_solver(SpectralSolverConfig)` 取得实例（`WarpX.cpp`/`PML.cpp` 中已无 `make_unique<SpectralSolver>`）；关闭 `WarpX_PLUGIN_FIELDSOLVER` 时 PSATD 不参与编译，PSATD 输入报"field-solver 插件未提供谱求解器"的可操作错误；110 个 PML/PSATD/Ohm/隐式测试全绿 |
| S6 第 2 批·场求解器（第 5 步） | **完成** | PSATD 子树**清零**：`SpectralFieldDataRZ` 通过构造注入 `RuntimeData`（`upperCorner`/`costs`），`SpectralSolverRZ` 一路转发；材料模块自己解析 `algo.macroscopic_subpixel`（参数归模块）；两个已无引用的 `WarpX.H` 包含被删除；棘轮 20 → **16**（剩静电 3、隐式 13） |
| S6 第 2 批·场求解器（第 4 步） | **完成** | 求解器的 `data()` 回退不再自己构造内核适配器：核心提供 `legacy_runtime_data()`（惰性创建、核心持有生命周期），PSATD 与有限差分求解器因此完全不提 `WarpX`；棘轮 22 → **20** |
| S6 第 2 批·场求解器（第 3 步） | **完成** | 把 `HybridPICModel`（含外部矢势）整体移出求解器树 → `Modules/HybridPIC/`：它是混合 PIC 物理模型（电子压力闭合、QDSMC、粒子耦合），不是麦克斯韦求解器，32/36 处单例引用随之离开拆分范围；棘轮 30 → **22**（注释行不再计入） |
| S6 第 2 批·场求解器（第 2 步） | **完成** | 棘轮范围收窄为真正的求解器子树（编排层留在 PicEm）；逐文件尝试去掉 `#include "WarpX.H"`，26 个文件确认不需要内核头、14 个仍需（编译验证后精确保留）；棘轮条目 48 → **30**（14 个 include + 16 个仍有 `WarpX::` 的文件） |
| S6 第 2 批·场求解器（第 1 步） | **进行中** | 把「WarpX 成员函数写在求解器目录里」的两处搬回编排层（`Implicit/WarpXImplicitOps.cpp`、`Magnetostatic/MagnetostaticSolver.cpp` → `FieldSolver/`）；磁静耦合 22 → **1**、隐式 42 → **19**；新增 `check_kernel_coupling.py` + 允许清单（当前 56 条，只能减不能增）作为拆分棘轮 |
| S6 第 1 批·碰撞插件 | **完成** | 碰撞成为独立插件：目录 `Source/Plugins/Collision/`、独立层目标 `warpx_collision_<dim>`、开关 `WarpX_PLUGIN_COLLISION`；内核通过核心注册的工厂取处理器（未链接时返回空，碰撞静默跳过并保持可运行）；`WarpX_PLUGIN_COLLISION=OFF` 时核心+内核+诊断+IO 仍构建并跑通算例；顺带把 QED 虚光子生成移回粒子插件（它被宿主服务调用，不属于碰撞） |
| S6 第 1 批·碰撞去内核（前半） | **完成** | 碰撞模块（16 个 cpp）现在对内核**零依赖**：0 个 `WarpX.H`、0 处 `WarpX::`。做法是给 `RuntimeData` 补 `loadBalanceCostsUpdateAlgo`，把负载均衡代价/几何/下角点/场寄存器/体量统计全部改走运行时数据契约，并让碰撞模型构造时接收 `RuntimeData const&`、`AllocData` 也带上它；全量失败集不变 |
| D1-a 收口（核心纯度 **0**） | **完成** | `WarpXParIter` 经核实**不含任何具体 WarpX 类型**（继承 AMReX 的 SoA 迭代器、用核心的属性索引常量），唯一的 WarpX 痕迹是名字；将其重命名为 `ParticleTileIter`（65 个文件、145 处），作为通用 AMReX 级 tile 迭代器保留在核心；纯度允许清单**清空**，检查器等价于零容忍 |
| D1-a 第 5 批（加速器元件 + 检查器修正） | **完成** | `LatticeElementFinder` 的两个入口对迭代器类型模板化，具体实例化留在插件；修正纯度检查器：`WarpX::` 这类以 `::` 结尾的 token 之前因词边界写法**从未匹配**，修正后核心仍为 0 处单例引用；核心纯度清单 2 → **1**（只剩迭代器自身定义） |
| D1-a 第 4 批（静电求解器契约） | **完成** | 新增核心 `ParticleContext`（通用访问 + 粒子服务）与 `IParticleServices::depositCharge`；`IElectrostaticSolver::ComputeSpaceChargeField` 不再接收具体多粒子容器，改为接收 `ParticleContext`；四个求解器实现同步；具体容器只在插件内部做一次显式窄化（带断言）；核心纯度清单 3 → **2**（只剩加速器元件与迭代器自身） |
| D1-a 第 3 批（契约去具体类型） | **完成** | `IParticleContainerAccess` 不再暴露具体容器迭代器/容器向量，改为按索引访问 + `initializesSelfFields(i)` 通用查询；两处 range-for 消费方改为通用循环；`RuntimeData` 删除无用的具体容器前向声明；核心纯度清单从 5 降到 **3**（剩余：加速器元件、迭代器自身、静电求解器参数） |
| D1-a 第 2 批（纯度棘轮） | **完成** | `GetExternalEBField` 构造改为对迭代器类型模板化（核心不再指名具体迭代器）；`FilterFunctors` 的 `SuperParticleType` 改用通用容器类型；新增 `check_core_purity.py` + 允许清单：核心中「具体粒子类／迭代器／WarpX 单例」的引用必须登记，新增即失败；当前真实引用从 8 降到 **5**，且清单只能减不能增 |
| D1-a 第 1 批的收尾 | **完成** | 首次提交只完成了一部分（`git mv` 链在 `ShapeFactors` 处中断），`IParticleIterator` 与三个别名契约实际仍留在 Core；本次补齐并核对：Core 侧 18 个粒子头中，仅 4 个仍引用具体类型（`WarpXParIter`、`ParticleIO`、`GetExternalFields`、`FilterFunctors`），这 4 个被诊断插件使用，需先换通用契约才能搬迁 |
| S4a 清单 + 生命周期 + 依赖校验 | **完成** | `PluginManifest`（名字/版本/角色/依赖/能力）+ `IPluginLifecycle`（schema/init/建层/重划/检查点/重启/收尾）；`on_schema` 在 `initialize()` 开头执行（构造后加入的插件也能拿到）、`on_init` 在内核建层后、`on_finalize` 在收尾前且幂等；依赖未满足或重名插件在启动时被拒；PicEm 以 kernel 角色登记清单；`plugin_lifecycle_2` 九项断言通过 |
| S4d 插件可移除 | **完成** | 新增 `WarpX_PLUGIN_DIAGNOSTICS` / `WarpX_PLUGIN_IO` 构建开关；关闭诊断+IO 后核心与内核仍构建，并可跑完整算例（无诊断输出、无崩溃）；`WarpX::InitFromCheckpoint`/`GetRestartDMap` 从 IO 插件移回 PicEm（它们是 WarpX 成员而非 IO 服务）；默认构建 255 项失败集合不变 |
| S4c 服务工厂注册表 | **完成** | 诊断/约简诊断/遗留运行时三组工厂改为「插件静态注册、核心查找」：核心没有服务时退回空实现（诊断）或给出可读错误（遗留路径），**插件之间不再互相引用符号**；`service_registry_2` 验证注册、缺失退化、清除后行为 |
| S4b 能力校验 + C 清单 | **完成** | 内核上报本次运行的启用特性（AMR/EB/QED/PSATD/RZ），插件声明了能力却不包含启用特性时**在首步前被拒**并指出插件与缺失特性（未声明能力的插件不校验，保留兼容）；C ABI v6：插件描述符带角色/依赖/能力/`on_schema`/`on_init`/`on_finalize`，并有状态声明入口；`plugin_lifecycle_2` 增至 13 项断言，`plugin_loader_2d` 校验 C 插件清单 |
| S5a 声明式访问校验 | **完成** | 处理器运行时总线只允许其声明的读写；未声明访问在调用点抛错并给出声明指引；聚合名/分量名共用同一匹配规则（`KeyMatch.H`）；内置处理器仍走宿主服务（S5b 待做）；`bus_access_2` 覆盖允许/拒绝/内核路径/聚合名 |
| S5b 内置处理器改经总线 | 未开始 | — |
| S6 按物理功能拆插件 | **进行中** | 粒子第 1 单元已拆（见 A7.4 行）；碰撞/诊断/场求解器已是独立插件 |
| S7 全覆盖验收 | 未开始 | — |

**基线修正（2026-11-08）**：早期用 `tail -45` 截取失败列表，开头若干条被截掉，基线不完整。改用 ctest 的 `LastTestsFailed.log` 后，2D 构建的真实失败集是 **33 项**：全部为「测试输入要求 openPMD」的运行/分析/校验组（本构建 OPENPMD=OFF），加上既有的 `runtime_snapshot_particles_2` 输入缺项。后续一律以此 33 项为比对基准。

**基线（接手时，2026-11-08，HEAD `76f24d1`，2D 构建）**：255 个测试中 41 项失败 =
35 checksum（平台相关）+ 3 个要求 openPMD 的 run + 其 3 个下游 analysis；无系统性失败。
每次改动后与此集合比对，新增失败必须解释。

**跨维度（S1a/S2 之后）**：1D/3D/RZ 全量**编译通过**（0 错误）；
运行时抽查 `test_1d_collision_z`、`test_1d_macroscopic_solver`、`test_3d_embedded_boundary_cube`
通过，`test_3d_electrostatic_sphere`、`test_1d/3d_langmuir_multi` 失败均为
`OPENPMD=OFF` 构建下测试输入要求 openPMD（既有问题）。四维运行时全量验收
需在 openPMD 构建上进行，列入 S7。

---

## 12. 变更记录

| 日期 | 变更 |
|---|---|
| 2026-11-08 | 首次建立：记录原话（5 条要求 + 3 条约束）、解读 R1–R5、代码现状体检、目标构思、S0–S7 路线、验收矩阵、与 A7 的协作协议 |
| 2026-11-08 | 接替 A7 与 S 系列；补 §10 A7→S 映射、§11 实施进度；S2 完成（阶段表可扩展，ABI v4） |

## 15b. S6 现状：已拆出的插件与仍在内核的模块（2026-11-09）

| 插件 | 独立层目标 | 开关 | 状态 |
|---|---|---|---|
| Diagnostics | `warpx_diagnostics_<dim>` | `WarpX_PLUGIN_DIAGNOSTICS` | 已拆，可关闭构建 |
| IO | —— | —— | **已并入核心**：唯一内容是通用的场打包/平均工具（无具体 WarpX 类型），诊断需要它，因此移入 `Source/Core/IO/`；层目标与开关随之删除 |
| Collision | `warpx_collision_<dim>` | `WarpX_PLUGIN_COLLISION` | 已拆，关闭后运行正常 |
| FieldSolver | `warpx_fieldsolver_<dim>` | `WarpX_PLUGIN_FIELDSOLVER` | 有限差分 + 静电已拆；关闭后构建成功、运行时给出可操作错误 |

**仍在内核插件内、且已确认需要"内核服务层"才能拆的模块**：

| 模块 | 阻塞原因（量化） |
|---|---|
| 隐式求解器 | 通过 `Define(WarpX*)` 驱动场推进/粒子快照/边界/诊断，约 198 处成员使用 |
| PSATD | 内核直接持有 `SpectralSolver`/`SpectralSolverRZ` 并使用其 `ForwardTransform`/`ApplyFilter`/`m_spectral_index`，缺少谱求解器契约 |
| 静磁 | `WarpX` 持有 `VectorPoissonBoundaryHandler` 成员 |
| 混合模型 | 混合 E 更新需要模型的 23 个参数（见下） |
| 粒子容器/沉积/推动 | 尚未拆分（A7.3 剩余部分） |

**跨插件 include 棘轮**：87 → 3。剩余 3 处（全部是真实深层耦合，已记录在
`plugin_boundary_allowlist.txt`）：

1. `EvolveEPML.cpp` → `PML.H`、`PML_current.H`（PML 系数容器 `SigmaBox`/`MultiSigmaBox`
   与 PML 电流核，需 PML 服务契约）
2. `HybridPICSolveE.cpp` → `HybridPICModel.H`（23 个参数：`eta`、`eta_h`、`n_floor`、
   各场 IndexType、电阻/超电阻开关、真空缝模式等——应做成参数块契约）

## 15f. 四维核验环境与最终状态（2026-11-09）

核验脚本与说明：`Tools/verify/`（`build_parallel_hdf5.sh`、`build_and_run_4d.sh`、`README.md`）。

**关键环境事实**：openPMD 需要**并行 HDF5** 才能启用 HDF5 后端；本机 conda 的 HDF5 是串行版，
openPMD 会静默关闭该后端。用系统 OpenMPI 自建 HDF5 1.14.6 后：

| 构建 | 失败数 |
|---|---|
| 2D（OPENPMD=OFF） | 33（openPMD 用例 + 1 已知输入缺项） |
| 1D/3D/RZ（OPENPMD=OFF） | 204（72 个 openPMD 用例） |
| 1D/3D/RZ（OPENPMD=ON，串行 HDF5） | 112 |
| 1D/3D/RZ（OPENPMD=ON，**并行 HDF5**） | 42 |
| 同上 + 修复"配置快照过期"四处 | **≈25**（仅剩环境/构建前提） |

**"配置快照过期"缺陷族（本系列最重要的一类）**：`RuntimeConfigSnapshot` 在 `InitData` 之前生成，
而 `ncomps`（`2*nmodes-1`）要到层分配时才计算；任何在 `InitData` 期间使用它的代码都拿到默认值 1。
四处中招并已修复：RZ 场数据集的模式维、后处理字段映射、BTD 缓冲区尺寸、后处理核的模式循环。
**规则**：`InitData` 期间的代码必须读实时 `RuntimeConfig`，不得读快照。

**剩余前提**：`*_boosted_opmd` 需 `-DWarpX_FFT=ON`；`load_external_field*` 需
`openPMD-example-datasets`；`test_3d_particles_in_pml_mr` 并行下易超时。

## 15e. 插件删除矩阵（S7，2026-11-09 实测）

2D 构建，同一份源码，逐个关闭插件；`cfg` = 配置成功，`build` = 编译链接成功，`run` = 实跑结果。

| 关闭的插件 | 开关 | cfg | build | 实跑行为 |
|---|---|---|---|---|
| 诊断 + IO | `-DWarpX_PLUGIN_DIAGNOSTICS=OFF -DWarpX_PLUGIN_IO=OFF` | ✅ | ✅ | `inputs_2d` **跑完**，**不产出任何诊断文件**（核心的空服务兜底），无崩溃 |
| 碰撞 | `-DWarpX_PLUGIN_COLLISION=OFF` | ✅ | ✅ | `inputs_2d` 跑完；碰撞算例（`collision_xz`）也**跑完**，碰撞被静默跳过（参数照常被解析并打印） |
| 场求解器 | `-DWarpX_PLUGIN_FIELDSOLVER=OFF` | ✅ | ✅ | 静电算例给出可操作错误：`No electrostatic solver plugin is linked: build with WarpX_PLUGIN_FIELDSOLVER=ON ...` |

结论：三个插件都可**独立关闭并保持构建**；缺失时行为分两类——
"可选能力"（诊断、碰撞）退化为空/跳过，"必需能力"（场求解器）给出明确可操作错误。
这正是"功能由插件提供、核心不依赖任何插件"的验收形态。

## 15d. S7 证据现状与 openPMD 环境限制（2026-11-09，重要）

**做了什么**：建了 `OPENPMD=ON` 的四维构建（`build_multi_hdf5`），跑完 469 个测试。

**结果**：112 条失败，其中 21 个用例在运行阶段失败，报：

```
openPMD::error::WrongAPIUsage
Wrong API usage: openPMD-api built without support for backend 'HDF5'.
```

**根因已定位，是环境而非代码**：本机的 HDF5（conda）是**串行版**，而 openPMD 是按 MPI
构建的；openPMD 的 CMake 在"有 MPI 但只有串行 HDF5"时**自动关闭 HDF5 后端**（配置日志里
有明确警告），也不存在 ADIOS2（`ADIOS2_DIR-NOTFOUND`）。测试输入里显式写着
`openpmd_backend = h5`，因此这些用例在本机**无论如何都跑不起来**。

**因此**：

| 项 | 数值 | 结论 |
|---|---|---|
| 1D/3D/RZ 全部测试 | 469 | — |
| 需要 openPMD 的用例 | 72 | 本机无法运行（缺 HDF5 后端） |
| **不需要 openPMD 的用例** | **397** | **全部通过（含数值校验和）** |
| 2D 全部测试 | 255 | 失败 33 = openPMD 用例 + 1 个已知输入缺项 |
| **2D 不需要 openPMD 的用例** | **222** | **全部通过（含数值校验和）** |

**这正是 S7"四维全算例数值等价"当前能给出的最强证据**：所有能在本机运行的用例
（397 + 222 项，含数值校验）都通过；不能运行的 72 类用例全部卡在同一环境依赖上，
且失败信息可直接归因（openPMD 缺 HDF5 后端）。

**要用官方配置真正核验，需要**：带 MPI 的 HDF5（例如 conda-forge 的
`hdf5=*=mpi_openmpi*`）→ 让 openPMD 编出 HDF5 并行后端，然后重跑四维套件。
这是核验阶段的第一项前置条件。

## 15c. 四维 × openPMD 对照（2026-11-09）

两个构建、同一份源码、同一批 469 个测试：

| 构建 | OPENPMD | 失败条目 | 失败用例 | 说明 |
|---|---|---|---|---|
| `build_multi2` | OFF | 204 | 72 | 全部为"需要 openPMD 输出"的用例（运行/分析/校验） |
| `build_multi_hdf5` | ON | **112** | — | 21 运行 + 50 分析 + 41 校验 |

openPMD 打开后 204 → 112：**证明约 92 条失败确实是环境（缺少 openPMD）造成的**。

但**仍有 21 个用例在运行阶段失败**，全部报同一个错：

```
openPMD::error::WrongAPIUsage
```

也就是说：这 21 个用例不是"环境缺依赖"，而是 **openPMD 写出路径的 API 用法/后端不匹配**问题，
必须在核验阶段单独定位（是 openPMD 版本与后端不匹配，还是本项目写出代码的用法错误）。

**结论：在 OPENPMD=ON 的官方配置下，四维仍有 21 个真实运行失败待修**；
这是当前"功能完整覆盖原版"里最大的未知项，核验阶段第一件事就是它。

## 16. 多维基线（1D/3D/RZ，2026-11-08 首次建立）

配置：`build_multi2`（DIMS=1;3;RZ，OPENPMD=OFF，QED=ON，OMP，conda python），
共 469 个测试，**221 项失败**。按成因分类：

| 类别 | 数量（测试条目） | 说明 |
|---|---|---|
| openPMD 依赖 | 46 个用例 × 3 ≈ 190 | 输入要求 openPMD 输出，本构建 OPENPMD=OFF；环境性，非代码缺陷 |
| 其它 | 7 个 `.run` | **真实失败**，需逐个修 |
| 其余 | — | 通过 |

### 真实失败清单（7 个）

| 测试 | 现象 | 初判 |
|---|---|---|
| `test_3d_hard_edged_quadrupoles` / `_boosted` / `_moving` | 段错误，栈在 `LatticeElementFinder::GetFinderDeviceInstance<ParticleTileIter>` | 加速器晶格路径，3D 独有；2D 无对应测试故未暴露 |
| `test_3d_plasma_lens_hard_edged` | 同上 | 同上 |
| `test_1d_macroscopic_solver` | 段错误 | 1D 宏观材料路径 |
| `test_rz_laser_acceleration` | `computeRho: requested ncomp does not match the deposited rho` | RZ 激光/rho 多分量 |
| `test_3d_deuterium_tritium_fusion` 等 3 个 | 诊断初始化时查不到 `*_particle_production` 场 | **顺序缺口**：诊断在 `InitData` 解析字段名，而该场在碰撞处理器 `AllocData` 里才分配。架构上插件应在 `on_schema` 声明自有状态、由核心在分配层时创建；碰撞的生产场尚未走这条路径 |
| `test_3d_load_external_field_particle_time_restart`、`test_rz_load_external_field_{grid,particles}_restart` | 无错误信息，需逐个排查 | 重启路径 |

### 结论

- 2D 的"33 项失败集不变"**不足以证明其它维度正确**：多维基线暴露了 6 处 2D 覆盖不到的真实失败。
- 这些失败需要逐一定位是"本项目引入"还是"上游既有"；方法是构建上游 `7bcb44a` 的 3D 版跑同样 6 个用例，或在分支内部二分。
- 诊断无法输出动态注册的场，是插件化后的**接口缺口**（自定义字段名不在诊断的字段名映射里），需要一张"运行期字段名 → 诊断可输出"的注册表。

## 15. S6 待解决的内核服务缺口（隐式求解器）

隐式求解器不是"再清几处静态量"就能拆的模块，它真正依赖内核的这些能力：

| 内核能力 | 使用处（隐式树内） |
|---|---|
| 场寄存器 `m_fields` | 109 |
| 粒子容器 `GetPartContainer` | 9 |
| 几何 `Geom(lev)` | 9 |
| EB 掩膜 `getngEB` / `getFieldDotMaskPointer` | 6 |
| 绝缘体 PEC 标记 `GetPECInsulator_IsESet` | 5 |
| 场推进 `EvolveB` / `FillBoundaryB` | 8 |
| 边界应用 `SetElectricFieldAndApplyBCs` | 4 |
| 粒子保存/恢复 `SaveParticlesAtImplicitStepStart` | 4 |
| 诊断 `reduced_diags` | 3 |
| FDTD 求解器指针 `get_pointer_fdtd_solver_fp` | 3 |

结论：要拆隐式求解器，先要在核心定义一组"内核服务"契约（场推进、边界、粒子快照、EB 掩膜、诊断），
再由内核插件实现。这是 S5b 的正式内容，排在可拆分求解器落地之后。

### P3（粒子系统出场）停止点与理由（2026-11-11）

**已完成**：①容器类对内核静态量的读取全部改走 `config()`（3 处结构性例外已逐条记录）；
②守护单元掩码与负载代价走 `IRuntimeData`；③新增服务定位器 `warpx::runtime::legacy_data/host/config`，
初等过程（电离、QED 光子发射、QED 对产生）与三个粒子文件的 `WarpX::` 归零；
④`RuntimeConfig` 补齐 `E_ext_particle_s`/`B_ext_particle_s`。

**停止点**：整体搬迁（`git mv` 到 `Source/Plugins/ParticleCore/` + 开关）**未执行**。

**理由（实测）**：内核侧对粒子类型仍有 42 处引用（`MultiParticleContainer` 21 个文件、
`WarpXParticleContainer` 12 个、`ParticleBoundaryBuffer` 9 个），且内核调用了
`mypc->` 上约 20 个不属于 `IParticleContainerAccess` 的方法（`Redistribute`、`DepositCurrent`、
`PushP`、`DepositTemperatures`、`defineAllParticleTiles`、`m_external_particle_fields_metadata` 等）。
要把它们全部搬进契约并切换，再搬 1.17 万行、30 余文件，是 10-20 轮的工程。

**同时**：并发会话正在同一批文件里做同样的重构（`IParticleContainerAccess` 已由
`MultiParticleContainer` 实现、掩码/代价/配置 getter 都是它加的）。**我在同一批文件上继续编辑
已两次误删它的未提交改动**（见 fa80b21、54e617c），继续并行改动只会互相破坏。

**结论**：P3 的剩余部分交由该会话（它有完整的上下文与正在进行的工作），本轮起我不再改动
`Source/Plugins/PicEm/Modules/Particles/` 下的文件，转而为整个目标做**独立验证与证据固化**。

### P3 交接单：把容器家族搬进 `Source/Plugins/ParticleCore/` 的具体步骤

现状：`Source/Plugins/ParticleCore/` **已存在且已接入构建开关**（`WarpX_PLUGIN_PARTICLECORE`），
目前装着 `ParticleThermalizer`（346 行 + 注册 + 链接锚点）。**容器家族仍在
`Source/Plugins/PicEm/Modules/Particles/`（12 个 .cpp + 头文件，约 1.17 万行）。**

**为什么不能"一次搬一个文件"**：`PhysicalParticleContainer` 调用 `AddParticles`，
`PhotonParticleContainer` 继承 `PhysicalParticleContainer`，后者的基类 `WarpXParticleContainer`
又被内核的 12 个文件引用。搬任何一个，都立刻产生"内核/插件互相 include"的边界违规
（`check_plugin_boundaries.py` 会直接报错）。**必须整体搬。**

**步骤（每步都要能编译、并跑 2D 基线比对失败集）**

1. **补契约**（`Source/Core/Runtime/`）：内核用到但契约里没有的 `MultiParticleContainer` 方法：
   `Redistribute`、`DepositCurrent`、`DepositCharge`、`PushP`、`DepositTemperatures`、
   `defineAllParticleTiles`、`nContainers`、`nSpeciesGatherFromMainGrid`、
   `m_external_particle_fields_metadata`、`m_E_ext_particle_s`/`m_B_ext_particle_s`。
   加到 `IParticleContainerAccess` 或新建 `IParticleSystem`，由 `MultiParticleContainer` 实现。
2. **内核改走契约**：把 12 个内核文件里的 `mypc->X(...)` 改为 `data().particleAccess().X(...)`
   （或 `particleSystem()`），逐个编译验证。
3. **整体搬迁**：`git mv Source/Plugins/PicEm/Modules/Particles/* Source/Plugins/ParticleCore/`，
   连同 `MultiParticleContainer`；把 `ParticleCore/CMakeLists.txt` 的源文件列表补齐；
   `PicEm` 侧只保留对 `warpx_particlecore_${SD}` 的链接与工厂调用。
4. **工厂**：新建 `create_particle_system(...)` 注册点（照 `create_particle_thermalizer` 的写法），
   内核通过它拿到 `IParticleSystem`，不再 include 任何容器头。
5. **关闭开关**：`WarpX_PLUGIN_PARTICLECORE=OFF` 时，
   - 带粒子输入的运行给出可操作错误（照 `particle_thermalizer` 那条的写法）；
   - **无粒子输入仍可运行**（这条用例要新加，例如一个纯场算例）。
6. **回归**：`build_takeover` 264 条失败集逐条不变；`build_2d_full` 335/336 不变；
   四个检查器仍 0/0/0/OK。

**风险**：这批文件正在被另一会话并行重构（它加了 `IParticleContainerAccess`、掩码/代价与
`RuntimeConfig` 的 getter）。**动之前先 `git status` 看清未提交改动**，不要用
`git checkout -- <file>` 撤销自己的编辑——那会连带销毁别人的工作（本项目已发生两次）。

## 最终状态（2026-11-12）

| 条目 | 状态 |
|---|---|
| 诊断插件 | 出场，可关闭，关闭时对请求输出的输入给高优先级警告 |
| 碰撞插件 | 出场，可关闭，关闭时对请求碰撞的输入给高优先级警告 |
| 场求解器插件（FD/静电/PSATD/静磁/宏观） | 出场，可关闭，缺失时**可操作错误**（曾经是段错误） |
| 隐式求解器（含 PETSc/PC） | 并入场求解器插件，`WarpX_PLUGIN_IMPLICIT` 受 `FIELDSOLVER` 约束（曾经的链接失败已修） |
| ParticleCore 插件 | 出场，承载 ParticleThermalizer；**容器家族未搬入** |
| 审计修复 | C ABI 读场、冲突检测、stage 接管、嵌套访问控制、pytest 收集 —— 5 项已修并有回归 |

判据 2 因此是**主体完成**，不是完成：最后一块（容器家族搬迁）的计划与验收标准在上面的交接单里。

### 阶段 B 进展与"结构性保留"（2026-11-13）

**已达成的**：内核目录里 `mypc->` 与 `GetPartContainer().` 形式的调用**归零**，
粒子 include 从 39 降到 **16**。内核与粒子之间的交互全部经由
`IParticleSystem` / `IParticleContainerAccess` / `IParticleBoundaryBuffer` 三个核心契约。

**剩下的 16 个 include 里，结构性的部分**（不是"还没做"，而是"抽象不了"）：

| 文件 | 需要具体类型的原因 |
|---|---|
| `WarpXRegrid.cpp`、`WarpXMovingWindow.cpp`、`WarpXImplicitOps.cpp`、`WarpX_ImplicitParticles.cpp` | 直接构造 `ParticleTileIter pti(pc, lev)` 遍历粒子 tile —— 这是 AMReX `ParticleContainer` 的内部数据布局，任何"接口化"都必须把 tile 迭代本身搬进契约，等于把容器的内存模型暴露给内核 |
| `WarpXRuntimeData.cpp`、`WarpX.cpp` | 运行时数据/构造点：前者要把 `MultiParticleContainer&` 转成 `IParticleContainerAccess&`（需要完整类型），后者是**构造点**（Phase C 由工厂替代） |

**结论**：阶段 B 的实际目标应表述为"内核不再通过具体类型**调用**粒子系统"
（已达成），而不是"内核目录里一个 `Particles/` include 都不剩"——后者与 AMReX 的
tile 迭代模型冲突。这个修正会影响阶段 B 的验收措辞，阶段 C 之后内核唯一需要具体类型的
地方应该只剩**工厂注册**那一处。

### 阶段 C 的依赖清单（2026-11-13，实测）

把 `Modules/Particles` 暂移到 `Plugins/ParticleCore/` 试跑后，边界检查器报出**反向**违规：
粒子树仍在 include 内核头。逐条清点后，完整清单是 **18 处**：

| 内核子目录 | 处数 | 内容 | 性质 |
|---|---|---|---|
| `Modules/Initialization` | 10 | `PlasmaInjector`、`InjectorDensity/Momentum/Position`、`SampleGaussianFluxDistribution` | 注入器，属于粒子族 |
| `Modules/EmbeddedBoundary` | 3 | `ParticleBoundaryProcess`、`ParticleScraper`、`DistanceToEB` | EB 的**粒子**部分 |
| `Modules/Laser` | 2 | `LaserProfiles` | 激光是粒子容器的一种 |
| `Modules/Filter` | 1 | `NCIGodfreyFilter` | 粒子降噪滤波器 |
| `Modules/AcceleratorLattice` | 1 | `AcceleratorLattice` | 粒子属性推进 |
| `WarpX.H` | 1 | `Gather/GetExternalFields.cpp`（解析器成员 + 加速器晶格） | 需契约访问器 |

**已清零的部分（本轮之前几轮的战果）**：内核桥接头 8/8、`WarpX.H` 13→1、
`WarpX::GetInstance()` 在粒子树内 4→0、`WarpX::` 静态量 19→0。

**结论**：粒子树与内核的耦合不是"几处漏网"，而是**这 18 处全部是"粒子族的相邻模块"**。
干净的搬迁只有一种做法：把上表这些模块**随粒子一起**移入 `Plugins/ParticleCore/`
（注入器、激光剖面、EB 粒子过程、滤波器、晶格查找器），而不是留一半在内核。
剩下 `GetExternalFields` 的 `WarpX.H` 还需要两个契约访问器：外部场解析器、加速器晶格查找器。

这决定了阶段 C 的真实工作量：**不是移动 78 个文件，而是移动 78 个文件 + 6 个相邻模块**
（约再多 30–40 个文件）。在最终移动前，先按上表逐项把"相邻模块"的边界理清，
比一次性大搬迁安全。

### 阶段 C 完成（2026-11-13）

`Source/Plugins/PicEm/Modules/Particles` → **`Source/Plugins/ParticleCore/Particles`**，
CMake 目标改为 `warpx_particlecore_<dim>`，内核通过**声明的单向依赖** `PicEm -> ParticleCore`
访问（`Tools/Modularization/plugin_dependencies.txt`，检查器同时拒绝环）。

搬迁成功的判据：四个检查器全绿（plugin boundaries / core purity / kernel coupling /
build boundaries），且 `build_takeover` **232/232**。此前一次搬迁失败，原因是当时粒子树仍
反向 include 内核 18 处；那些已在本阶段逐项清零。

过程中两次"顺手改成更干净"都触发了真实回归，值得记账：

| 改动 | 症状 | 正确做法 |
|---|---|---|
| 把 `use_fdtd_nci_corr` 的解析移进 config 并做 `static` 缓存 | NCI corrector 两个用例 checksum 相对误差 6.5e-5 / 2.1e-4 | 由内核在**与原先相同的时点**解析（建好粒子系统之后），行为逐位一致 |
| 把 `Filter` 里的负载均衡代价数组直接置空 | 负载均衡计时被静默丢弃 | 新增 `legacy_costs()` 定位器，内核注册真实实现 |

**遗留**：阶段 D（`WARPX_PLUGIN_PARTICLECORE=OFF` 的两条负向测试）尚未做；
另有并发会话遗留的 `Source/Plugins/Diagnostics/WarpXOpenPMD.cpp` 未提交改动，本轮未触碰。

### 阶段 D 的实测阻塞点（2026-11-13）

`WARPX_PLUGIN_PARTICLECORE=OFF` 的完整配置与编译**已经做过一次**（`/tmp/b_nopc`，配置 18 分钟、
编译 20 分钟），结果是**链接失败**，20 条未定义符号，全部来自 `ParticleTileIter` 与
`ParticleUtils::findParticlesInEachCell`。这说明：

> **"include 归零"不等于"不链接具体实现"。** 阶段 C 的验收判据低估了真实依赖，
> 检查器看不到代码级的 tile 遍历。

逐个清点后，内核里仍有 3 个文件在用粒子内部结构：

| 文件 | 用法 | 处置 |
|---|---|---|
| `FieldSolver/ImplicitKernelServicesBuilder.cpp` | `count_suborbits` 逐 tile 统计 | ✅ 已搬进粒子插件（`countParticlesWithSuborbits`） |
| `HybridPIC/HybridPICModel.cpp` | `QDSMCApplyIonHeating` 逐 tile 的 Ornstein-Uhlenbeck 更新（216 行） | ❌ 尝试搬移后**回滚**，见下 |
| `RuntimeBridge/IO/WarpXCheckpoint.cpp` | 仅一个未使用的 include | ✅ 删除 |
| `Python/Particles/*`、`pyWarpX.cpp` | Python 粒子绑定 | 只在 `WarpX_PYTHON=ON` 时编译；OFF 配置不涉及 |

**`QDSMCApplyIonHeating` 为什么不能简单搬走**：它虽然遍历粒子，但依赖 `HybridPICModel` 的
私有成员（`m_n_floor`、`m_nu_ei`、`CalculateIonFluidVelocity` 等）以及 `warpx.m_fields` 里的
混合模型诊断场。搬到 `MultiParticleContainer` 后编译失败（`CalculateIonFluidVelocity` 未声明等）。
它本质是**混合 PIC 模型的算法**，不是通用粒子机械。

**因此阶段 D 的正确做法是**：`WARPX_PLUGIN_PARTICLECORE=OFF` 时**不构建 HybridPIC 模块**
（混合 PIC 本身就需要粒子），并在内核里对相关调用点加编译期保护或给出可操作错误；
而不是把混合模型物理硬塞进粒子插件。这是一次**构建裁剪**，不是又一次搬迁。
