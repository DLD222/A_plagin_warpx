# 提交后正确性检查报告

- **报告日期**：2026-11-19
- **审计范围**：过去 24 小时内的提交（`HEAD = a4f05d5`，作者 DLD222，2026-11-19 08:48 +0800）
- **仓库性质**：WarpX 的非官方插件化 fork（核心 + 独立插件，经核心总线 `Source/Core/Runtime` 接入）
- **审计方式**：静态代码审查 + 追踪完整执行路径 + 项目自带模块化检查器 + 项目既有审计/比对文档交叉验证
- **重要约束**：本报告**不修改任何项目代码或文件**，仅撰写文档；所有结论均为"发现问题"，不做修复。
- **环境说明**：当前为浅克隆（`shallow = true`，仅 1 个提交），无父提交可做 diff，因此审计对象为该提交所代表的**整树当前状态**。环境中无 `conda/mamba` 与可用的 WarpX 构建，故未能编译运行复现；物理结论基于对源码控制流与计算物理理论的静态核对，以及项目自身比对文档给出的数值证据。

---

## 0. 执行摘要

在 `a4f05d5` 的整树状态中，确认 **3 个缺陷**（其中 2 个影响物理正确性/健壮性，1 个影响可复现性），并标记 **1 个项目自身未根因的未决一致性差异**。此外对插件化/模块化目标与 WARPX 功能完备性给出评估。

| 编号 | 严重度 | 类型 | 一句话 |
|------|--------|------|--------|
| F-1 | 高（物理正确性·静默） | ECT 共形 E 更新丢掉 div(E) 双曲修正项 ∇F | 开启共形 EB + divE 清理时，高斯定律修正被静默丢弃 |
| F-2 | 中高（正确性·生命周期） | 4 处物理路径仍用"冻结首个实例"的 `static WarpXRuntimeData` | 实例重建/多轮运行后访问旧实例指针（悬空） |
| F-3 | 中（可复现性） | `WARPX_PEC_EB_MASK` 环境变量暗中切换 PEC/EB 物理行为 | 不同机器/CI 环境下结果可不一致 |
| F-4 | 未决（需确认） | `dynamic_regrid` 项目自身文档记录的真实数值差异未根因 | 项目内部文档对"是否已修复"自相矛盾 |

> 说明：项目既有的 `Docs/audit_2026_11/` 与 `CODE_REVIEW_2026-11-16.md` 声称已修复全部 189 项审计发现。本报告的价值在于**独立复核**：F-1、F-2 恰恰是被该审计标记为"已修"但在当前代码中**实际未修/未修全**的项；F-3 是审计修掉了同类问题却漏掉的项；F-4 是项目比对文档内部不一致、尚未闭环的问题。

---

## 1. 项目目标理解（审计基准）

该 fork 的根本目标（来自 `FORK_NOTICE.md`、`Docs/research_notes/PLUGIN_ARCHITECTURE_FINAL.md` 与用户任务说明）：

1. **插件化/模块化**：将 WarpX 拆为独立**核心**与若干**插件**；插件之间相互独立，只通过核心接口（`Source/Core/Runtime/I*.H`）与核心总线（`DataBus` / `Scheduler` / `PluginHost`）接入；用户可按需撰写新功能插件，实现强扩展性。
2. **物理正确性**：在重构/新增物理（张量介质求解器、亚像素张量、矩阵指数电导率积分、ECT 共形 E、恒星 MHD 等）的同时，保持与上游 WarpX 的物理一致性，算法须符合计算物理正确理论。
3. **功能完备**：应完整实现上游 WarpX 支持的功能（"legacy coverage missing = 0"）。

审计即围绕这三条基准展开。

---

## 2. 详细发现

### F-1【高·物理正确性·静默】ECT 共形 E 更新路径丢弃 div(E) 双曲修正项 ∇F

**位置**
- 调度入口：[Source/Plugins/FieldSolver/FiniteDifference/EvolveE.cpp:96-98](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/EvolveE.cpp#L96-L98)
- ECT 共形核函数：[Source/Plugins/FieldSolver/FiniteDifference/EvolveE.cpp:245-421](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/EvolveE.cpp#L245-L421)
- 对照（标准求解器正确应用 ∇F）：[Source/Plugins/FieldSolver/FiniteDifference/EvolveE.cpp:211-233](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/EvolveE.cpp#L211-L233)
- 调用方（场推进主循环无条件演化 F）：[Source/Plugins/PicEm/Modules/Evolve/WarpXEvolve.cpp:947,957,966](file:///workspace/Source/Plugins/PicEm/Modules/Evolve/WarpXEvolve.cpp#L947-L966)
- 触发条件默认值：`ect_conformal_E` 默认 0（[WarpX.H:268](file:///workspace/Source/Plugins/PicEm/WarpX.H#L268)），`do_dive_cleaning` 默认 false（[WarpX.cpp:129](file:///workspace/Source/Plugins/PicEm/WarpX.cpp#L129)）

**原因**
`FiniteDifferenceSolver::EvolveE` 按算法分派：
```
Yee/CKC/Nodal/Cylindrical/Spherical  →  EvolveECartesian/...(..., Ffield, ...)   // 传入 Ffield
ECT + ect_conformal_E>0              →  EvolveECartesianECT(..., nullptr)         // 末参传 nullptr
```
标准路径在核函数内显式执行（EvolveE.cpp:211-233）：
```cpp
if (Ffield) {  Ex += c2*dt*UpwardDx(F);  Ey += ...;  Ez += ...; }
```
这是 div(E) 双曲（hyperbolic）修正：F 由 `EvolveF` 演化（dF/dt = -c² div(E) + 源项），再用 `E += c² dt ∇F` 把 E 拉回满足高斯定律 `div(E) ≈ ρ/ε₀`，用于抑制离散电荷守恒误差。

而 `EvolveECartesianECT`（245-421）的三个 `Ex/Ey/Ez` 更新（335-412）只做 `curl(B) - μ₀J`（以及宏观介质的 α/β 加权），**既不接收 `Ffield` 参数，也不从 fields 寄存器取 F，更不计算 ∇F**。`EvolveE` 在 ECT 分支传的是 `nullptr`。

同时主循环 `OneStep_FieldSolve` 在 FDTD 路径下**无条件**调用 `EvolveF`（WarpXEvolve.cpp:947、966），且没有任何"ECT 共形模式下关闭 dive cleaning"的保护逻辑。因此只要用户同时开启：
- `algo.maxwell_solver = ect` 且 `algo.ect_conformal_E > 0`（fork 的招牌共形 EB 功能），
- `warpx.do_dive_cleaning = 1`（F 场激活），

F 就会被分配、被演化，但其对 E 的修正被**静默丢弃**。

**影响（对仿真的影响）**
- 高斯定律 `div(E) - ρ/ε₀` 的误差在共形 EB 区域内**不被清理**，随时间累积；电荷守恒漂移。这是**静默物理错误**——仿真不报错、不崩溃，输出看似正常，但电场在 EB 附近的长期演化与正确结果系统性偏离。
- 这是 fork 招牌功能（ECT 共形 E 更新）与另一标准功能（divE 清理）的组合缺陷。两者均为 opt-in，CI 示例（`embedded_boundary_rotated_cube` 等 2 例用 ECT，但未同时开 `ect_conformal_E>0` + `do_dive_cleaning`）未覆盖该组合，故长期未被发现。
- 该问题已被项目自身审计（`Docs/audit_2026_11/CODE_AUDIT.md` #37 / 对应 `BACKLOG.md` F12）标记为 **"已修"**，但当前代码证明**实际未修**——属于"已修声明与代码不符"，会给使用者造成错误的安全感。

**建议**（仅描述，不实施）：在 `EvolveECartesianECT` 内补回 `E += c² dt ∇F`（与 `EvolveECartesian` 一致，取 `fields.get(FieldType::F_fp/cp, lev)`），或在分派处对 ECT 共形传入 `Ffield` 并在核函数内应用；并增加一个同时开启共形 E + divE 清理的算例进入回归。

---

### F-2【中高·正确性/生命周期】4 处物理路径仍使用"冻结首个 WarpX 实例"的 `static WarpXRuntimeData`

**位置**
- [Source/Plugins/PicEm/Modules/BoundaryConditions/PML.cpp:1519](file:///workspace/Source/Plugins/PicEm/Modules/BoundaryConditions/PML.cpp#L1519) （`PML::data()` 回退路径）
- [Source/Plugins/PicEm/Modules/Initialization/DivCleaner/ProjectionDivCleaner.cpp:412](file:///workspace/Source/Plugins/PicEm/Modules/Initialization/DivCleaner/ProjectionDivCleaner.cpp#L412)
- [Source/Plugins/PicEm/Modules/Fluids/QdsmcParticleContainer.cpp:663](file:///workspace/Source/Plugins/PicEm/Modules/Fluids/QdsmcParticleContainer.cpp#L663)
- [Source/Plugins/PicEm/Modules/Fluids/WarpXFluidContainer.cpp:1620](file:///workspace/Source/Plugins/PicEm/Modules/Fluids/WarpXFluidContainer.cpp#L1620)

对照（已修复的正确写法）：
- `WarpXRuntimeData::Legacy()` 每次调用刷新指针：[Source/Plugins/PicEm/RuntimeBridge/Runtime/WarpXRuntimeData.cpp:28-44](file:///workspace/Source/Plugins/PicEm/RuntimeBridge/Runtime/WarpXRuntimeData.cpp#L28-L44)（关键在第 42 行 `legacy->m_warpx = &WarpX::GetInstance();`）
- `WarpX.cpp` 已改用 `Legacy()`：[Source/Plugins/PicEm/WarpX.cpp:405-408](file:///workspace/Source/Plugins/PicEm/WarpX.cpp#L405-L408)
- `WarpXRuntimeData` 持有的是非拥有裸指针 `WarpX* m_warpx`：[WarpXRuntimeData.H:119](file:///workspace/Source/Plugins/PicEm/RuntimeBridge/Runtime/WarpXRuntimeData.H#L119)

**原因**
上述 4 处的模式为：
```cpp
static const warpx::runtime::WarpXRuntimeData legacy(WarpX::GetInstance());
```
该 `static` 局部对象在**首次调用时构造一次**，把当时 `WarpX::GetInstance()` 返回的指针存入 `m_warpx`，之后**永不刷新**（只有 `Legacy()` 会在每次调用时刷新，见上）。`WarpXRuntimeData::w()`（WarpXRuntimeData.cpp:47-50）在 `m_warpx != nullptr` 时直接解引用该缓存指针，不会回退到最新的 `GetInstance()`。

项目审计（`BACKLOG.md` / `CODE_AUDIT` #50，PicEm_B/F4）已将此"函数局部 static 冻结首个实例"判定为缺陷并修复——但修复**只覆盖了 `WarpX.cpp`/`ReducedDiags` 等站点**，上述 4 处物理关键路径（PML、投影 div 清理、两类流体容器）被遗漏，仍保留冻结模式。属于**同类缺陷修复不一致/未修全**。

**影响（对仿真的影响）**
- 单次、单实例运行时 `GetInstance()` 指针不变，缺陷不可见——这正是它能通过全部单实例对齐测试的原因（静默）。
- 一旦 WarpX 实例被销毁并重建（PICMI/pywarpx 多轮 `run()`、共仿真、检查点重启后新建实例、单元测试连跑），`WarpX::GetInstance()` 返回新实例，但这 4 个 `static` 仍指向**已销毁的旧实例** → `w()` 解引用悬空指针 → **use-after-free / 读到陈旧状态**（PML 几何、div 清理场、流体状态）。表现为第二轮起物理状态错误或随机崩溃，极难定位。
- 同时违背插件化目标：核心/插件应通过 `RuntimeContext`（活体上下文）取数，而不是回退到一个冻结的单例快照；这 4 处的"无 context 时回退到冻结 legacy"削弱了"上下文注入"的设计契约。

**建议**：将 4 处 `static const ... legacy(WarpX::GetInstance())` 统一改为 `WarpXRuntimeData::Legacy()`（或在构造处保存 context、优先使用 context），与已修复站点保持一致。

---

### F-3【中·可复现性】`WARPX_PEC_EB_MASK` 环境变量暗中切换 PEC/EB 物理行为

**位置**
- [Source/Plugins/PicEm/Modules/BoundaryConditions/WarpXFieldBoundaries.cpp:66-67](file:///workspace/Source/Plugins/PicEm/Modules/BoundaryConditions/WarpXFieldBoundaries.cpp#L66-L67)

```cpp
static bool const pec_eb_mask = (std::getenv("WARPX_PEC_EB_MASK") == nullptr
                                 || std::string(std::getenv("WARPX_PEC_EB_MASK")) != "0");
```

**原因**
是否对嵌入边界（EB）阶梯覆盖格执行 `E_tangential = 0`（理想导体 PEC 条件）由**环境变量** `WARPX_PEC_EB_MASK` 决定，且在函数首次调用时以 `static` 读取一次后固化。该开关：
- 不出现在输入卡组（`inputs`）里，不进 `RuntimeConfig`/`ConfigSnapshot`，不进检查点；
- 仅在源码注释中提及（用于"临时判断 hybrid 求解器是否被阶跃干扰"），无文档、无默认值声明；
- 不同机器/CI 上只要该环境变量被设置或继承，同一份 inputs 就会跑出**不同的 EB/PEC 物理**。

项目审计 F-11 已把同类的"环境变量控制数值路径"问题（`WARPX_YEE_CELL_EPS`）修掉、改为输入卡组显式参数，但**漏掉了 `WARPX_PEC_EB_MASK`**——又是一次同类修复未覆盖全。

**影响（对仿真的影响）**
- **可复现性破坏**：同输入卡组、同二进制，在设置了 `WARPX_PEC_EB_MASK=0` 的环境中，EB 表面切向 E 不再置零 → PEC 边界条件失效 → 电磁波在导体表面的反射/吸收完全改变，物理结果系统性不同。
- 静默：无任何运行期提示表明该环境变量生效；用户难以察觉为何"同一算例两台机器结果不一样"。

**建议**：将 `WARPX_PEC_EB_MASK` 改为显式输入参数（如 `warpx.pec_eb_mask`，默认开启），纳入 `RuntimeConfig` 与检查点，移除环境变量读取；至少在启动时打印其状态。

---

### F-4【未决·需确认】`dynamic_regrid` 项目自身记录的真实数值差异未根因，且文档自相矛盾

**位置**
- 未决差异记录：[Docs/audit_2026_11/PARITY_FINDINGS.md](file:///workspace/Docs/audit_2026_11/PARITY_FINDINGS.md)（"未解决：dynamic_regrid（真实差异）"，给出 iter 1 与 iter 8 的数值证据）
- 声称已通过：[Docs/audit_2026_11/PARITY_RESULTS.md](file:///workspace/Docs/audit_2026_11/PARITY_RESULTS.md)（第 12 轮）、[ACCEPTANCE_FINAL.md](file:///workspace/Docs/audit_2026_11/ACCEPTANCE_FINAL.md)（第 14/16 轮）
- 重网格路径（fork 重写）：[Source/Plugins/PicEm/Modules/Parallelization/WarpXRegrid.cpp](file:///workspace/Source/Plugins/PicEm/Modules/Parallelization/WarpXRegrid.cpp) 与 [Source/Plugins/PicEm/Modules/Evolve/WarpXEvolve.cpp:404-428](file:///workspace/Source/Plugins/PicEm/Modules/Evolve/WarpXEvolve.cpp#L404-L428)
- 算例配置：[Examples/Tests/dynamic_regrid/inputs_test_2d_dynamic_regrid](file:///workspace/Examples/Tests/dynamic_regrid/inputs_test_2d_dynamic_regrid)（`warpx.regrid_int = 1`，`warpx.refine_start_time = 1.0e-14`，`amr.max_level = 1`）

**已知证据（来自项目自身文档）**
- `PARITY_FINDINGS.md`（同样标注 2026-11-19）记录：fork 在 iter 1 无 level-1 输出、level-0 场幅小约 4600 倍；iter 8 两者都有 lvl1，但电流 j 的相对差达 **7%（level-0）/ 97%（level-1）**。文档自述排查到 `RemakeLevel` 保存并回填了 33 个字段（`old_max == new_max`），排除"remake 丢数据"，怀疑**重网格时机**或**level-0 场在 regrid 前后被覆盖**，结论为"待查"。
- 后续 `PARITY_RESULTS`/`ACCEPTANCE_FINAL` 声称"可比较算例全部逐字段一致/全通过"，但**没有任何文档给出 iter-8 残余差异的根因与修复点**——即项目内部对"dynamic_regrid 是否已修复"给出了**矛盾结论**。

**我的静态核对**
- `WarpX::ErrorEst` 在 t=0 的标记已正确：`deferred_refine` 默认 0（[WarpX.H:1724](file:///workspace/Source/Plugins/PicEm/WarpX.H#L1724)），故 `refine_start_time` 默认被忽略，初始网格在 t=0 即被标记——这解释了 PARITY_FINDINGS 中 iter 1 "无 lvl1"的旧现象应已随默认值修复而消失。
- 但 **iter 8 的 j 差异（7%/97%）是否仍存在，仅凭静态阅读无法排除**；regrid 块（WarpXEvolve.cpp:404-428）是 fork 新写路径，涉及 `multi_diags->RemakeLevel/ResizeLevels`、ParGDB/粒子 box 更新、`Redistribute`、`defineAllParticleTiles`、边界缓冲 `redistribute`、`reduced_diags->LoadBalance` 等一连串收尾，任何一步的时序或遗漏都会在 AMR 细水平上产生大误差（level-1 上 97% 的电流差尤其可疑，通常指向粗-细插值/同步或细水平场重采样缺失）。
- 因环境无法构建运行，**本项未能独立确认是"已修复"还是"仍存在"**。

**影响**
- 若仍存在：AMR 动态重网格下细水平电流/场系统性错误（level-1 达 97%），属**严重物理正确性问题**，且静默（仿真正常完成）。
- 无论是否仍存在：项目比对文档自相矛盾、关键差异未根因即宣布"全通过"，本身是**验证流程可信度问题**——会掩盖潜在的 AMR 回归。

**建议**：以固定随机种子在同一二进制下重跑 `dynamic_regrid` 三件套（`inputs_test_2d_dynamic_regrid`、`..._remake`、`..._remove`），与上游逐字段对比 j/E/B，明确 iter-8 差异是否仍在；若在，按文档已给的两条线索（regrid 时机 / level-0 场覆盖）定位；并在文档中统一结论。

---

## 3. 插件化/模块化目标合规性评估

针对"插件相互独立、仅经核心接口/总线接入"的目标，做了如下客观核查：

1. **插件间互不 `#include`**：对 7 个插件（Collision / Diagnostics / FieldSolver / FluidMoments / NewtonGravity / ParticleCore / PicEm）两两检查，**交叉包含数为 0**。插件在包含层面相互独立，符合目标。
2. **核心纯净性**：`Tools/Modularization/check_core_purity.py` 报告 **0 已知、0 新增** 核心→插件反向依赖（核心不依赖具体插件实现），符合"核心冻结、插件可插拔"。
3. **构建分层**：`check_build_boundaries.py` 报告 7 层插件，无回归。
4. **总线访问管控**：`DataBus` 提供 `require_field_access/require_species_access/require_scalar_access`（[DataBus.H:88-91](file:///workspace/Source/Core/Runtime/DataBus.H#L88-L91)），并由 `Scheduler` 在调度时设置 `active_handler`，理论上能强制"插件只能访问声明过的场"。`has_handlers(stage, stage+1)` 的"精确阶段接管"判定在 Push/AfterPush/FieldSolve/Diagnostics/Collisions 各点一致（[WarpXEvolve.cpp:580-772](file:///workspace/Source/Plugins/PicEm/Modules/Evolve/WarpXEvolve.cpp#L580-L772)），审计曾发现的"只读观察者误关核心物理"缺陷已在所有站点统一修复。
5. **需注意的偏离**：插件对核心的包含中，有 **1361 处**包含 `Core/` 下 `Runtime/`、`ablastr/` 之外的内部模块（`Core/Modules`、`Core/Numerics`、`Core/Utils` 等）。这在过渡性重构中可接受（核心提供共享数值模块），但意味着插件与核心实现的耦合面仍较大，距离"插件只依赖抽象接口、核心可整体替换"的终态目标还有差距。建议长期把这些共享模块明确划分为"核心公开 API"并在接口层收敛。

**结论**：插件化的**骨架目标基本达成**（独立、可插拔、总线管控存在），但 F-2 表明"经活体上下文取数、不回退冻结单例"这一契约在部分物理路径未被遵守；1361 处核心内部包含说明接口收敛尚未完成。

---

## 4. WARPX 功能完备性评估

- `Tools/Modularization/check_legacy_coverage.py` 报告：legacy 文件 829，**missing = 0**（另 109 个在不同目录下找到、3 个合并入其它文件）。即声明已完整覆盖上游 WarpX 源文件。
- 需谨慎：该检查器曾被项目自身审计（CODE_AUDIT #62）判定为不可靠（曾把 554 项误报为 missing），现已重写。"missing = 0"是覆盖**文件存在性**，不等价于**功能行为等价**。
- 行为等价性由 `parity_all.py`/`parity_run.py`（本次提交新增）做全量 446 算例对齐验证：当前进度 289/446（通过 159、跳过 124、失败 6）。`parity_run.py` 用 1e-10 相对容差、固定随机种子、能正确识别 fork 崩溃（判 FAIL）与参考版崩溃（判跳过），逻辑总体可信；但其 `compare()` 对**无 fields 组的算例只比顶层结构、不比粒子数组值**（[parity_run.py:114-119](file:///workspace/Tools/warpx-studio/tests/parity_run.py#L114-L119)），且只比对**最后一个** openPMD 文件——存在覆盖盲区，纯粒子诊断或中间迭代的差异可能漏报。
- `known_divergence.json` 声明了 3 处"设计性差异"（EB 阶梯掩码、恒星插件、宏观介质 F-04/F-08）。其中 F-04（张量电导整矩阵门控）/F-08（切向电导算术平均）是 fork **有意改变**的物理——已超出"对齐上游"范畴，应作为 fork 的显式物理特性文档化并提供可关闭开关，否则用户无法复现上游行为。

**结论**：文件级覆盖声称完整；行为对齐在可比较算例上大体可信，但存在验证盲区（F-4 的文档矛盾、parity 只比末帧/不比无场算例粒子值）。建议补盲并对"设计性物理偏离"提供输入开关。

---

## 5. 本次未发现但已确认"已正确修复"的项（供参考，排除误报）

为避免把已修复项误报为缺陷，独立核对了以下曾被标记的问题，确认当前代码已正确修复，**不计入缺陷**：
- `applyElectromagneticSources` 取错 E 分量 `(Ex,Ex,Ex)`（CODE_AUDIT #11）——已改为按 `d < AMREX_SPACEDIM` 取 `Ev[d] = ea[d](...)`，缺省方向保持 0（[FluidMoments/Electromagnetic.H:140-147](file:///workspace/Source/Plugins/FluidMoments/Electromagnetic.H#L140-L147)）。
- 宏观介质 `macroscopic_solver = laxpw` 被静默改回 Euler（CODE_AUDIT #36）——`MacroscopicEvolveE.cpp` 现按 `LaxWendroff/BackwardEuler` 分派，α/β 取自 `T_MacroAlgo`（[MacroscopicEvolveE.cpp:75-102](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicEvolveE.cpp#L75-L102)）。
- 亚像素张量正定性检查用可被编译期消去的 `AMREX_ASSERT`（CODE_AUDIT #139）——已改为 `AMREX_ALWAYS_ASSERT_WITH_MESSAGE`（[SubpixelTensor.H:190,354,411,419](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicProperties/SubpixelTensor.H#L190)）。
- `deferred_refine` 默认生效导致 dynamic_regrid 初始无细网格（PARITY_RESULTS 第 12 轮）——默认已改为 0（[WarpX.H:1724](file:///workspace/Source/Plugins/PicEm/WarpX.H#L1724)）。
- 调度器"只读观察者误关核心物理"——各 `Run*Stage` 已统一用 `has_handlers(stage, stage+1)` 精确判定。

---

## 6. 结论

1. 发现 **3 个确认缺陷**（F-1 物理正确性、F-2 生命周期/正确性、F-3 可复现性），均为**静默**型，且 F-1、F-2 与项目自身"已全部修复"的声明相矛盾——说明既有审计/回归存在漏报或"已修"标注失真，建议把这两项重新打开。
2. 标记 **1 个未决差异**（F-4 dynamic_regrid）：项目文档自相矛盾、关键数值差异未根因即宣布通过，需以固定种子重跑确认。
3. 插件化骨架目标基本达成，但接口收敛未完成（F-2 是契约违背的具体表现）；功能覆盖声称完整但验证有盲区。
4. **未修改任何项目代码或文件**；本报告为纯文档输出。

> 因环境无法构建复现，F-1/F-2/F-3 为源码级确认（控制流与物理意图明确不符），F-4 为基于项目自身证据的高风险提示。建议在可构建环境中优先复现 F-1（构造 `ect_conformal_E>0` + `do_dive_cleaning=1` 算例，监控 div(E)-ρ/ε₀）与 F-4（重跑 dynamic_regrid），以锁定修复优先级。
