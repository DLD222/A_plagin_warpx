# WarpX 插件化 Fork — 提交后正确性审计报告

- 审计对象：`76570664c6ca7c463b6aac95dec9bcf0f19d9003`
  （`studio v2: 实时视图自测覆盖 + 阶段4 文档（selftest 60/60）`，2026-11-16 07:32:37 +0800）
- 审计期间远端前进到：`9d5824c35561cdbe26413eb601feb6c073c3060e`
  （`studio v3: 中文说明全覆盖(802/802) + 按物理分类(12 类) + 输入框灰色示例（自测 65/65）`）。
  该提交只改 Studio 工具链（`Tools/warpx-studio/selftest.py`、分类/中文数据、Web 资源），
  未触及 A-01～A-07 涉及的 C++ 路径；A-08 中“缺依赖即退出”的启动逻辑没有被修改。
- 审计日期：2026-11-16
- 审计范围：过去 24 小时内的提交。本仓库历史为单根提交（squash），因此本报告覆盖该提交的完整文件树。
- 审计方式：只读静态审查 + 跨文件执行路径追踪 + 离线 deck / selftest 复现。
- 约束遵守：**未修改任何项目代码或项目文件**；本文件是唯一新增产物。

---

## 0. 结论摘要

本轮确认 8 项缺陷，其中 2 项会直接改变仿真物理结果且不产生任何报错（静默），
3 项破坏插件接口契约，其余为工具链 / 文档不一致。

| 编号 | 严重度 | 类别 | 一句话结论 |
|---|---|---|---|
| A-01 | 严重 | 物理 + 插件契约 | 碰撞阶段把任意观察者当成“核心替代者”，只读插件会让碰撞静默全部关闭 |
| A-02 | 严重 | 物理 | 张量 ε 的亚网格平均只对 z 法向界面生效，x/y 法向界面退化为阶梯 |
| A-03 | 高 | 物理 | `sigma_tensor_*` 在默认 `macroscopic_subpixel=0` 下完全无效；指数步仅按 σ 对角元开门 |
| A-04 | 高 | 插件契约 | Finalize / FieldSolve / Diagnostics 阶段未填 `dt/time/step`，第三方插件拿到全零 |
| A-05 | 高 | 插件契约 | `DataBus::alias` 只检查源字段“读”权限，却返回可变指针，可绕过写声明与冲突检测 |
| A-06 | 中 | 工具链 | Studio 的 RZ 最小 deck 必然启动失败，而 `validate()` 返回“无问题” |
| A-07 | 中 | 文档/配置 | 宏观求解器文档键名与枚举值和代码不一致，按文档配置会被静默忽略或 abort |
| A-08 | 低 | 验证 | selftest 在缺少 `uvicorn` 时直接退出，“60/60”在本环境不可复现 |

---

## 1. 项目目标与判定标准

本仓库是 WarpX（DOE 开源 PIC 代码）的非官方插件化 fork，目标是把核心与插件分离：
插件之间相互独立，只通过接口和核心总线（`Source/Core/Runtime`）接入，使用户能自行编写功能插件。
因此本报告同时使用两套判据：

1. **物理正确性**：算法是否满足 PIC / Maxwell 方程的离散一致性（电荷守恒、界面连续性、色散关系等）。
2. **插件契约**：核心是否向插件提供了完整、可验证、不可绕过的接口；观察者插件是否可能与核心算法互相污染。

---

## 2. 关键缺陷

### A-01 严重 — 碰撞阶段把观察者当成替代者，碰撞会被静默关闭

**位置**

- [WarpXEvolve.cpp](file:///workspace/Source/Plugins/PicEm/Modules/Evolve/WarpXEvolve.cpp#L738-L751)（`RunCollisionStage`）
- [Scheduler.cpp](file:///workspace/Source/Core/Runtime/Scheduler.cpp#L16-L40)（区间语义：`from <= stage < to`）
- 对照修复记录：[AUDIT_RESPONSE_2026_11_12.md](file:///workspace/Docs/research_notes/AUDIT_RESPONSE_2026_11_12.md#L31-L43)

**现象与原因**

`RunCollisionStage` 用“区间内是否执行了任何处理器”来决定是否跳过核心碰撞：

```cpp
if (scheduler.run(ctx, Stage::BeforeCollisions,
                  Stage::AfterCollisions) > 0) { return; }
```

而 `Scheduler::run` 的返回值是区间内**执行过的处理器个数**，不区分“替代核心的处理器”与“只读观察者”。
同一文件中的 Push / Finalize / FieldSolve / Diagnostics 已经改成
`has_handlers(Stage, Stage+1)`（只把注册在核心 stage 点上的处理器视为替代者），
但碰撞这一处没有跟着改。修复记录明确写了“四个站点（Push / AfterPush / FieldSolve / Diagnostics）都做了同样处理”，
碰撞是第五个站点，被漏掉了。

**执行路径**

1. 用户加载一个在 `Stage::BeforeCollisions` 上注册的只读插件（例如碰撞计数、能量诊断、可视化）。
2. 每个时间步 `RunCollisionStage` 调用 `scheduler.run(ctx, BeforeCollisions, AfterCollisions)`。
3. 该只读处理器的 `stage` 落在区间内，`executed` 变为 1，`run()` 返回 1。
4. 函数在调用 `m_collision_handler` 之前 `return`，核心碰撞（Coulomb / MCC 等）**一步都不执行**。
5. 全程没有警告、没有计数、没有失败退出。

**对仿真的影响（静默）**

等离子体碰撞率直接变为 0。弛豫时间、电阻率、能量交换、束流热化等宏观输运量全部错误，
但场和粒子仍然“正常”演化，输出看起来完全合理。这是最难通过目视发现的一类物理错误。
更严重的是，该错误只在“恰好安装了观察者插件”时触发，**不装插件时结果正确**，
因此常规回归测试无法覆盖。

**为什么代码审查容易漏掉**

`> 0` 与 `has_handlers` 的差异只在“区间内含观察者、但不含替代者”这一种组合下才暴露；
仓库现有测试的插件大多是替代型（PicEm 在核心 stage 点上），没有“仅观察、不替代”的碰撞插件，
所以两条路径在测试中给出了相同结果。

---

### A-02 严重 — 张量 ε 的亚网格平均只对 z 法向界面生效，x/y 法向界面退化为阶梯

**位置**

- 邻居单元选择：[MacroscopicProperties.cpp](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicProperties/MacroscopicProperties.cpp#L596-L620)
- 法向重算与材料采样：[MacroscopicProperties.cpp](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicProperties/MacroscopicProperties.cpp#L646-L657)
- ε 张量的界面旋转：[MacroscopicProperties.cpp](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicProperties/MacroscopicProperties.cpp#L696-L714)
- 维度分支：[MacroscopicProperties.cpp](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicProperties/MacroscopicProperties.cpp#L551-L563)

**现象与原因**

代码注释声称 F-05 把“法向判定”从固定 z 推广到了 `grad(phi)` 方向，并称对 z 法向是严格推广。
但**邻居单元的选择并没有一起推广**：

- 2D/3D 分支中 `iL = iR = ii`，即沿 x 方向永远取同一个单元；
- 3D 中 `jL = jR = j`，即沿 y 方向永远取同一个单元；
- 只有 `kL/kR`（z）按 `nrmv[2]` 偏移；
- 2D 中只有 `jL/jR`（对应 z）按 `nrmv[2]` 偏移。

同时，3D 的法向重算只计算了 `gx0` 与 `gz0`，**`nrmv[1]`（y 分量）从未被赋值**，
默认值保持 `(0,0,1)`。ε 张量的界面旋转也在 `y = 0.0` 处取样，且只做 x/z 差分。

结果是：

- 2D x 法向界面：`iL == iR`，两侧材料采样落在同一单元，`eL == eR`，亚网格平均退化为“取本地材料”，
  界面变成阶梯，Kottke 平滑完全不生效。
- 2D z 法向界面：正常（现有测试只覆盖这一种）。
- 3D x / y 法向界面：同样退化；y 法向还会被误判成 z 法向（`nrmv` 保持默认 `(0,0,1)`），
  张量旋转方向错误。
- 1D z：邻居选择正确。

**执行路径**

1. 用户给出 `macroscopic.interface_function(x,y,z)`，界面为 `x = const` 或 `y = const`。
2. `InitData` 的节点循环对每个节点计算 `normal = |nrmv[c]| > 0.5`；
   对 x 法向，`nrmv[0]` 不为零，因此 Ex 被判定为“法向分量”。
3. 但 `eL/eR` 从 `iL == iR` 的同一个单元读取，`chi_inv_*` 的两个输入完全相同。
4. 节点 ε / ε⁻¹ 等于该单元的材料值，界面没有亚网格混合。
5. `MacroscopicEvolveE` 的 BE 分支与指数分支都使用该节点值，直接把阶梯误差带入场推进。

**对仿真的影响（静默）**

- 界面反射/透射系数出现一阶几何误差；短波长下 R/T 偏差可达 O(1)。
- 介质界面出现非物理的表面电荷 / 表面电流与虚假散射。
- 各向异性张量在非 z 法向界面上旋转错误，主介电轴方向与实际界面不匹配。
- 由于结果仍然“像波在传播”，没有 NaN、没有 abort，属于典型静默物理错误。

**测试盲区**

仓库内唯一带界面的宏观算例是
[inputs_1d_periodic_interface](file:///workspace/Examples/Tests/macroscopic_solver/inputs_1d_periodic_interface)（1D z 法向）。
没有任何 2D/3D 的 x 法向或 y 法向界面回归用例，因此该缺陷不会使现有 ctest 变红。

**补充说明**

[AUDIT_RESPONSE_2026_11_12.md](file:///workspace/Docs/research_notes/AUDIT_RESPONSE_2026_11_12.md#L45-L74) 已承认
“节点级 `interface_function` 只在 XZ 下坐标正确”是上游既有实现，并选择不改。
本报告指出的不是“是否继承上游”，而是**当前树里的 F-05 注释宣称已推广到任意法向，
实际实现只推广了判定、没有推广采样**。注释与实现不一致，会让后续开发者误以为 x/y 法向已被覆盖。

---

### A-03 高 — `sigma_tensor_*` 在默认配置下完全无效；指数步仅按 σ 对角元开门

**位置**

- 标量 σ 的警告：[MacroscopicProperties.cpp](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicProperties/MacroscopicProperties.cpp#L481-L490)
- 接口但无亚网格的警告：[MacroscopicProperties.cpp](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicProperties/MacroscopicProperties.cpp#L385-L392)
- `InitSubpixelRows` 的调用条件：[WarpXPushFieldsEM.cpp](file:///workspace/Source/Plugins/PicEm/Modules/FieldSolver/WarpXPushFieldsEM.cpp#L1271-L1282)
- BE 分支的 σ 读取：[MacroscopicEvolveE.cpp](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicEvolveE.cpp#L247-L263)
- 指数步的对角门：[MacroscopicEvolveE.cpp](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicEvolveE.cpp#L443-L457)

**现象与原因**

`algo.macroscopic_subpixel` 默认为 0（[MacroscopicProperties.H](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicProperties/MacroscopicProperties.H#L133-L134)）。
`InitSubpixelRows`（唯一构建 M1/M2 的地方）只在 `WarpX::macroscopic_subpixel` 为真时调用。
因此默认配置下 `has_sigma_matrices()` 为 false：

- BE 分支的 `sigma_interp` 走标量单元场 `sigma_arr`；
- 而 `sigma_tensor_*` 只写入 `m_sigma_tensor_mf`，**标量 `m_sigma_mf` 仍是 0**；
- 于是张量电导率被完全忽略，介质表现为真空。

更隐蔽的是警告逻辑：L481 的警告只检查标量 `m_sigma_mf->max(0) > 0`。
用户只给 `sigma_tensor_*` 时该条件为假，**不会出现“张量被忽略”的警告**；
反而会触发“未指定 `sigma`，使用真空默认值 0”的提示，容易被误读为“只是标量没给”。

即使打开 `macroscopic_subpixel=1`，指数步仍按**节点 σ 的对角分量**开门：

```cpp
if (sn(i, j, k, c) <= 0) { return; }
```

BE 分支也用 `sn0(...,0)` / `sn1(...,1)` / `sn2(...,2)` 判对角。
对于一个对角元为零、但非对角元非零的张量（例如纯旋转型 / Hall 型电导率），
三个分量的指数步全部被跳过；BE 分支又把 `sigma_interp` 取成对角元 0，
于是**非对角耦合被彻底丢弃**。若只有部分对角元为零，则不同分量走不同积分器，
指数步还会用未被更新的 `eold` 去计算另一分量的耦合项，破坏矩阵指数的半群一致性。

**执行路径**

1. 用户写 `macroscopic.sigma_tensor_xy = ...`，不给标量 `macroscopic.sigma`。
2. `ReadParameters` 记录 `m_has_sig_tensor = true`，但 `sigma_specified` 仍为 false。
3. 默认 `macroscopic_subpixel = 0`，`InitSubpixelRows` 不调用，`m_sigma_mat_ready = false`。
4. `MacroscopicEvolveE` 中 `has_sig_mat = false`，`sigma_interp` 取标量 `sigma_arr = 0`。
5. 电场完全没有阻尼；非对角电导率对应的电流 / 霍尔项不存在。

**对仿真的影响（静默）**

- 有损介质 / 导体被当成真空：衰减长度无穷大，波不衰减。
- 各向异性电导率（含 Hall 项）的偏振旋转、交叉耦合完全缺失。
- 用户按“张量已生效”的预期做参数扫描，得到的是真空结果，且无任何错误提示。

---

### A-04 高 — `StageContext` 在 Finalize / FieldSolve / Diagnostics 阶段未填 `dt/time/step`

**位置**

- [HandlerSpec.H](file:///workspace/Source/Core/Runtime/HandlerSpec.H#L48-L66)：`step = 0`、`time = 0.0`、`dt = 0.0`
- [WarpXEvolve.cpp](file:///workspace/Source/Plugins/PicEm/Modules/Evolve/WarpXEvolve.cpp#L612-L628)：`RunFinalizeStage` 三个字段都没设
- [WarpXEvolve.cpp](file:///workspace/Source/Plugins/PicEm/Modules/Evolve/WarpXEvolve.cpp#L636-L653)：`RunFieldSolveStage` 只设 `time`
- [WarpXEvolve.cpp](file:///workspace/Source/Plugins/PicEm/Modules/Evolve/WarpXEvolve.cpp#L702-L719)：`RunDiagnosticsStage` 只设 `step`

**现象与原因**

`StageContext` 的三个时间字段默认全零。`RunPushStage`（L580-604）与
`RunCollisionStage` / `RunAfterCollisionStage` 都完整填充了 `step/time/dt`，
但以下三个站点没有：

| 站点 | `step` | `time` | `dt` |
|---|---|---|---|
| Finalize | 0 | 0.0 | 0.0 |
| FieldSolve | 0 | 正确 | 0.0 |
| Diagnostics | 正确 | 0.0 | 0.0 |

`RunFinalizeStage` 甚至没有 `step/time/dt` 形参，无法自行补齐。

**对仿真的影响（静默）**

- 任何在 `Stage::FieldSolve` 上接管场推进的第三方插件，若按 `ctx.dt` 更新场，
  会得到 `dt = 0`：场不推进，但程序继续运行，输出看似“静态场”。
  若插件写成 `E += ctx.dt * curlH / eps`，结果就是 E 永远不变，属于最隐蔽的物理错误。
- `Diagnostics` 插件拿不到 `time`，写出的 openPMD / 文本时间戳为 0；
  后续相位、频率、传播速度分析全部偏移。
- `Finalize` 插件无法知道当前步号与时间，无法做按步/按时的收尾逻辑。

**为什么是插件契约问题**

核心自己的 `OneStep_FieldSolve` 使用内部 `dt[lev]`，不依赖 `ctx`，所以核心路径不受影响；
但项目目标恰恰是让用户用插件替换核心。契约字段为空等于把“可替换”变成了“替换后物理错误且无提示”。

---

### A-05 高 — `DataBus::alias` 只检查源字段“读”权限，却返回可变指针

**位置**

- [DataBus.cpp](file:///workspace/Source/Core/Runtime/DataBus.cpp#L136-L152)
- 对照正确实现：[DataBus.cpp](file:///workspace/Source/Core/Runtime/DataBus.cpp#L121-L134)（`field()` 的写路径检查 `write=true`）

**现象与原因**

```cpp
require_field_access(dst, /*write=*/true);
...
require_field_access(src, /*write=*/false);
return m_fields.alias_init(dst, src, ...);
```

`alias` 让 `dst` 与 `src` 共享同一份 `MultiFab` 存储，返回的是 `dst` 的**可变**指针。
但它对 `src` 只要求 `reads_fields`，不要求 `writes_fields`。
处理器完全可以声明“只读 `Efield_fp`”，然后通过 `alias` 拿到可变指针并写入源字段。

**执行路径**

1. 插件声明 `reads_fields = {"Efield_fp"}`、`writes_fields = {"my_alias"}`。
2. 调用 `bus.alias("my_alias", "Efield_fp", dir, lev)`。
3. 权限检查通过（`dst` 有写声明，`src` 有读声明）。
4. 返回的指针指向 `Efield_fp` 的真实存储。
5. 插件通过该指针改写电场，但 `HandlerSpec` 中没有对 `Efield_fp` 的写声明。

**对仿真的影响**

- 字段写冲突检测（`PluginHost::add` 中“同 stage 点两处理器写同一字段”）完全失效。
- 调度排序看不到这条写依赖，插件可能在核心推进场的过程中改写场，产生与顺序相关的非确定结果。
- 对用户而言，插件“只读”的声明是假的；一个诊断插件可能无意中破坏主仿真状态。

当前树内没有插件调用 `alias`（已全仓库检索），因此这是**面向第三方插件作者的潜在契约漏洞**，
但项目的核心目标正是支持第三方插件，故按高严重度记录。

---

### A-06 中 — Studio 的 RZ 最小 deck 必然启动失败，而 `validate()` 不报错

**位置**

- [model.py](file:///workspace/Tools/warpx-studio/warpx_studio/deck/model.py#L129-L139)：RZ 默认 `ppc = "1 1"`（2 个数）
- [model.py](file:///workspace/Tools/warpx-studio/warpx_studio/deck/model.py#L166-L175)：所有方向（含径向）设为 `periodic`，并设 `n_rz_azimuthal_modes = 2`
- [WarpX.cpp](file:///workspace/Source/Plugins/PicEm/WarpX.cpp#L1468-L1485)：RZ 径向不得周期，`r=0` 处场/粒子边界必须为 `none`
- [PlasmaInjector.cpp](file:///workspace/Source/Plugins/ParticleCore/Particles/Initialization/PlasmaInjector.cpp#L423-L439)：RZ 要求 `num_particles_per_cell_each_dim` 恰好 3 个
- [validate.py](file:///workspace/Tools/warpx-studio/warpx_studio/deck/validate.py#L105-L175)：只做数值/必填检查，没有维度相关规则

**现象与原因**

`minimal_deck("rz")` 的 docstring 声称是“该维度下真正能跑起来的最小 deck”，但实际至少有三处必然失败：

1. 径向边界设为 `periodic`，`WarpX` 在初始化时直接 abort（径向不允许周期）。
2. `r = 0` 处边界未设为 `none`，同样 abort。
3. `num_particles_per_cell_each_dim = "1 1"` 只有 2 个分量，RZ 要求 3 个，`PlasmaInjector` 断言失败。
4. 即使修好前三条，`n_rz_azimuthal_modes = 2` 而 `ppc_theta = 1`，
   低于推荐的 `2 * n_rz_azimuthal_modes = 4`，会触发高优先级警告并影响 RZ 精度。

`validate()` 对这份 deck 返回空列表（已离线复现：`1d [] / 2d [] / 3d [] / rz []`），
因此用户会看到“校验通过”，然后启动即 abort。

**对仿真的影响**

不直接产生错误物理结果，但会让用户无法通过 Studio 进入 RZ；
对一个以“降低使用门槛、支持插件扩展”为目标的项目，这属于核心可用性缺陷。
它也暴露了“校验器只覆盖通用键、不覆盖维度/几何约束”的系统性问题。

---

### A-07 中 — 文档与代码的宏观求解器键名/枚举不一致

**位置**

- [MACROSCOPIC_INPUTS.md](file:///workspace/Docs/research_notes/MACROSCOPIC_INPUTS.md#L6-L18)（最小输入）
- [MACROSCOPIC_INPUTS.md](file:///workspace/Docs/research_notes/MACROSCOPIC_INPUTS.md#L36-L42)（算法开关表）
- [WarpXAlgorithmSelection.H](file:///workspace/Source/Core/Utils/WarpXAlgorithmSelection.H#L46-L49)：只有 `BackwardEuler`、`LaxWendroff`
- [WarpX.cpp](file:///workspace/Source/Plugins/PicEm/WarpX.cpp#L1632-L1636)：实际读取的键是 `algo.macroscopic_sigma_method`

**现象与原因**

文档反复要求用户写：

```
algo.macroscopic_solver = exponential     # 唯一支持张量的路径
```

但代码中：

- 枚举 `MacroscopicSolverAlgo` 没有 `exponential` 这个值；
- 实际读取的键是 `algo.macroscopic_sigma_method`，不是 `algo.macroscopic_solver`。

因此按文档配置的结果是：键名不被识别（旧实现可能静默忽略，或在严格模式下报未知键）；
把值改成 `exponential` 则会被枚举解析拒绝并 abort。文档同时声称这是“张量必需”的路径，
而代码中张量对应的矩阵指数 M1/M2 是由 `macroscopic_subpixel=1` 触发的，与这个键无关。

**对仿真的影响**

- 用户以为启用了矩阵指数 / 张量路径，实际运行的是默认 Backward–Euler 或直接 abort。
- 参数扫描的结论与文档描述不符，且没有运行期证据指出“这个键没生效”。
- 属于典型的“配置静默失效”，会误导物理结论。

---

### A-08 低 — selftest 在缺少依赖时直接退出，“60/60”不可复现

**位置**

- [selftest.py](file:///workspace/Tools/warpx-studio/selftest.py#L44-L57)：`start_server` 在 60 次重试后 `raise SystemExit("server did not start")`
- [selftest.py](file:///workspace/Tools/warpx-studio/selftest.py#L74-L75)：`main()` 没有把服务器启动失败转成可诊断的结果

**现象与原因**

`start_server` 通过 `python -m uvicorn` 启动服务，失败时只抛出 `server did not start`，
不检查 `ModuleNotFoundError: No module named 'uvicorn'`，也不打印子进程 stderr（被重定向到 `DEVNULL`）。
在本环境复现时，`uvicorn` 未安装，脚本立即以该消息退出，后续 60 项检查一项都没有执行。

**影响**

这是验证覆盖问题，不是运行期物理缺陷。需要记录的是：
提交信息中的“selftest 60/60”（以及远端 v3 的“65/65”）在当前环境无法独立复核；
缺少依赖与真正的服务启动失败被混为一谈，容易造成“测试通过”的错觉。
远端 `9d5824c` 只在 `run_checks` 中新增了 v3 检查项，
没有修改 `start_server` 的依赖缺失处理，因此该问题在最新树上仍然成立。

---

## 3. 对三个核心问题的回答

### 3.1 代码是否违背项目的根本目标（插件化 / 模块化）？

**是，有三处直接违背。**

- A-01：观察者插件会关闭核心碰撞。插件本应“只观察、不改变”，却产生了全局物理影响。
- A-04：核心没有向插件提供完整的时间上下文，导致“可替换”的接口在替换后必错。
- A-05：`alias` 让插件可以在不声明写权限的情况下修改核心字段，破坏了总线访问控制这一核心隔离机制。

这三项都不是“某个插件写错了”，而是**核心接口/调度层的系统性缺陷**，
会影响所有未来插件，符合“系统性缺陷”的定义。

### 3.2 代码逻辑是否违背物理理论？

**是，主要是宏观介质求解器。**

- A-02：亚网格平均在 x/y 法向界面上退化为阶梯，违背 Kottke 子网格平滑的理论前提
  （节点材料应由界面两侧材料按法向加权），界面连续性条件不再被正确离散。
- A-03：张量电导率在默认配置下被完全丢弃，或在部分对角元为零时丢失非对角耦合，
  违背各向异性 Ohm 定律 `J = σE` 的完整形式。

两者都产生“看起来正常”的结果，属于用户要求的“影响物理正确性的静默性 bug”。

### 3.3 是否没有完整实现原版 WarpX 支持的功能？

- 宏观介质的张量 / 界面功能在非 z 法向与默认配置下不完整（A-02、A-03）。
- RZ 的 Studio 最小 deck 无法运行（A-06），虽然底层 WarpX 的 RZ 本身是可用的，
  但项目提供的入口没有正确实现该维度。
- 文档宣称的 `exponential` 路径在代码中不存在（A-07），属于“文档承诺 > 实现”。

需要说明：仓库内 1D/2D/3D/RZ 的核心求解器仍有大量上游功能，
本报告只列出本轮审查中**可由提交树直接证实**的缺口，不宣称这是完整的对等性清单。

---

## 4. 已核查并排除的疑点

为避免误报，以下项目经过完整路径追踪后**确认不是缺陷**：

1. **`EvolveB(0.5dt) / EvolveE(dt) / EvolveB(0.5dt)` 的顺序**与上游一致，未发现分裂误差。
2. **`m_eps_inv_node` 是否在 `subpixel=0` 时保持全零**：否。构造函数分配、`InitData` 无条件填充
   （[MacroscopicProperties.cpp](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicProperties/MacroscopicProperties.cpp#L777-L794)），
   BE 分支始终读取节点逆介电行，不会出现“电场冻结”。
3. **σ 张量的非对角元是否被无条件清零**：否。L759-L760 明确写入了 `s3.m[1] / m[2] / m[5]`；
   问题在于后续的门控与默认配置（A-03），不是写入丢失。
4. **`PluginHost::add` 的字段冲突检测**已从字符串相等改为 `key_matches`，修正确实生效。
5. **`Scheduler::run` 的嵌套处理器恢复**（保存 `previous` 并在正常/异常路径恢复）实现正确。
6. **`DataBus::field(const)` 的读权限检查**正确；问题只在 `alias`（A-05）。
7. **RZ 要求 3 个 ppc 的断言**本身是上游既有行为，不是 fork 引入的回归；
   A-06 的问题在于 Studio 的默认 deck 违反了它。

---

## 5. 复现记录

| 项目 | 操作 | 观察结果 |
|---|---|---|
| 提交窗口 | `git log --oneline -5` | 仅一个根提交 `7657066`，因此审计覆盖整棵树 |
| RZ deck 校验 | 离线构造 `minimal_deck("rz")` 并调用 `validate()` | 返回 `[]`（无问题），但 deck 含径向 `periodic`、ppc 仅 2 项 |
| 四维校验对照 | `1d / 2d / 3d / rz` 全部调用 `validate()` | 四者均返回 `[]`，说明校验器没有维度规则 |
| selftest | `python3 Tools/warpx-studio/selftest.py` | `SystemExit: server did not start`；根因为 `ModuleNotFoundError: No module named 'uvicorn'` |
| 代码路径 | 逐行阅读 `RunCollisionStage`、`RunFieldSolveStage`、`RunFinalizeStage`、`RunDiagnosticsStage` | 确认 A-01 与 A-04 的字段/返回值行为 |
| 总线路径 | 阅读 `DataBus::alias` 与 `DataBus::field` | 确认 A-05 的权限不对称 |
| 介质路径 | 阅读节点填充、ε 张量旋转、指数步门控 | 确认 A-02 与 A-03 |

本次审计**未执行编译或 ctest**（避免在只读审计中改动构建目录与可能触发的检查点重置）；
所有结论均来自源码路径与离线 Python 复现。

---

## 6. 建议的后续验证方向（仅方向，不含实现）

1. 为 A-01 增加一个“只在 `BeforeCollisions` 注册、不写任何字段”的观察者插件回归测试，
   断言碰撞计数与不装插件时一致。
2. 为 A-02 增加 2D x 法向与 3D y 法向的平面界面算例，断言沿切向不变，
   并与解析 Fresnel 系数对比 `R + T ≈ 1`。
3. 为 A-03 增加“仅给 `sigma_tensor_*`、不给标量 sigma”的算例，
   断言衰减长度与解析值一致；再增加一个对角元为零、非对角元非零的算例。
4. 为 A-04 增加一个在 `FieldSolve` 上读取 `ctx.dt` 并写回诊断的探针插件，
   断言其收到的 `dt` 与 deck 的 `dt` 一致。
5. 为 A-05 增加一个声明只读却通过 `alias` 写源的负向测试，断言总线应拒绝。
6. 为 A-06 让 `validate()` 读取 `geometry.dims` 并检查径向边界与 ppc 个数，
   同时让 RZ deck 在 CI 中真正启动一次（可用 `max_step = 1` 的 smoke test）。

以上均为验证方向；按用户要求，本报告**不提供任何修改代码或修改项目文件的补丁**。
