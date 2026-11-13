# 提交后正确性检查报告

- 报告日期：2026-11-13
- 检查对象：`/workspace`（WarpX 插件化非官方 fork）
- 提交范围：`27030e31a1f3eef59bd029d558d9611d91231154`（2026-11-13 09:41:49 +0800，root commit，`git rev-list --count HEAD = 1`）
- 提交标题：`refactor(p3-c): the particle tree no longer includes a single kernel header`
- 提交声称：粒子树不再包含任何 kernel 头文件；boost factor 与 azimuthal-mode 数改由 `config()` 提供，cell size/lower corner 改由 `data()`；负载均衡成本数组与成本更新算法改由 core locator；external-field gather 去掉一处 `dynamic cast to WarpX`；"build green, 2D suite 232/232"。

本报告只做缺陷分析，**未修改任何项目代码或项目文件**，也不提出可落地的补丁，仅描述位置、原因与对仿真的影响。

---

## 0. 结论摘要

| 编号 | 严重度 | 类型 | 一句话结论 |
| --- | --- | --- | --- |
| **F1** | 严重 | 静默物理错误 + 崩溃 | 外部粒子场（parser / repeated_plasma_lens / accelerator lattice）在"上下文化"重构后**全线失效**：gather 取到的粒子系统指针恒为空。parser 模式必崩，lens 模式越界/未定义行为，lattice 模式静默丢场。 |
| **F2** | 高 | 静默物理错误 / 死代码 | `RuntimeContext::accelerator_lattice_at` 钩子从未被注册，`MultiParticleContainer::acceleratorLattice()` 恒返回 `nullptr`；`WarpX::get_accelerator_lattice()` 成为无调用者的死代码。 |
| **F3** | 中高 | 静默配置错误 | `RuntimeConfig::E_ext_particle_s()/B_ext_particle_s()` 用函数级 `static` 缓存，同一进程内的第二个仿真实例会沿用第一个仿真的外部场模式。 |
| **F4** | 中 | 违背项目根本目标 | "粒子树不再包含任何 kernel 头"的说法不成立：Core 头仍包含 `<WarpX.H>` 与 PicEm 的 QED 头；Collision 插件头直接包含 `<WarpX.H>`；粒子插件仍直接写 kernel 静态成员 `WarpX::use_fdtd_nci_corr`，仅靠 Core→plugin 的传递包含才"侥幸"编译。 |

核心警示：F1/F2 是**静默性（silent）物理缺陷**——在 `none`/`constant` 用例下完全正常，只有在真正启用外部场/加速器晶格时才失效。现有 CI 只声明 2D 套件 232/232 通过，而受影响的用例全部是 1D/3D，因此绿灯不能证明外部场物理正确。

---

## F1（严重）外部粒子场 gather 因"上下文"未接通而全线失效

### F1.1 问题位置

| 位置 | 关键代码/事实 |
| --- | --- |
| [RuntimeContext.H](file:///workspace/Source/Core/Runtime/RuntimeContext.H#L38-L45) | 构造函数只接收 config/time/frame/data/bus/host/refresh_time/config_snapshot/state，**没有 `particles` 形参** |
| [RuntimeContext.H](file:///workspace/Source/Core/Runtime/RuntimeContext.H#L61-L67) | `const IParticleSystem* particles = nullptr;`、`std::function<const void*(int)> accelerator_lattice_at{};` |
| [Runtime.cpp](file:///workspace/Source/Core/Runtime/Runtime.cpp#L42-L45) | 构造 `m_context` 时未赋值 `particles` / `accelerator_lattice_at` |
| [WarpX.cpp](file:///workspace/Source/Plugins/PicEm/WarpX.cpp#L4024-L4029) | `SetRuntimeContext()` 只把 context 交给 MPC（`particleSystem().setRuntimeContext(context)`），**从不写回 `context->particles`** |
| [MultiParticleContainer.H](file:///workspace/Source/Plugins/PicEm/Modules/Particles/MultiParticleContainer.H#L262) | `setRuntimeContext()` 只存自己的 `m_runtime_context`，**不向 species 转发** |
| [WarpXParticleContainer.H](file:///workspace/Source/Plugins/PicEm/Modules/Particles/WarpXParticleContainer.H#L758) | `const RuntimeContext* m_runtime_context = nullptr;`，全仓库无任何对 species 的赋值 |
| [WarpXParticleContainer.H](file:///workspace/Source/Plugins/PicEm/Modules/Particles/WarpXParticleContainer.H#L181) | `runtime_context() const override { return m_runtime_context; }` → 对 species 恒为 `nullptr` |
| [GetExternalFields.cpp](file:///workspace/Source/Plugins/PicEm/Modules/Particles/Gather/GetExternalFields.cpp#L29-L31) | `IParticleSystem const* ps = (ctx != nullptr) ? ctx->particles : nullptr;` |

**全仓库检索证据**：对 `Source/` 搜索 `->particles =`、`.particles =`、`accelerator_lattice_at =`，结果均为 0 处；搜索 `setRuntimeContext` 仅有 4 处（接口声明、WarpX 转发、MPC 自存，无一处给 species 或 `context->particles` 赋值）。

**结构性阻断**：[IParticleCapability.H](file:///workspace/Source/Core/Runtime/IParticleCapability.H#L24-L34) 只暴露 `register_species()` 与 `allocate_collision_data()`，**没有任何接口能把 `IParticleSystem*` 交给 RuntimeContext**。也就是说，即便把 `SetRuntimeContext` 改成想填 `context->particles`，当前分层也没有合法的获取途径——这是"上下文化"重构留下的系统性缺口，而非单点笔误。

### F1.2 产生原因

提交把 external-field gather 从"直接 `dynamic_cast` 到 `WarpX` 拿实现"改为"通过 context 上的 `particles` 指针拿实现"（见 [GetExternalFields.cpp](file:///workspace/Source/Plugins/PicEm/Modules/Particles/Gather/GetExternalFields.cpp#L27-L31) 注释："The container arrives through the context"）。但只完成了一半：

1. `RuntimeContext::particles` 只在构造函数默认初始化为 `nullptr`，没有任何生产者写入它；
2. species 级 `m_runtime_context` 从未被赋值，导致连 `ctx` 本身都是空指针；
3. 即便 `ctx` 非空，`ctx->particles` 仍为空，`ps` 依旧恒为 `nullptr`。

于是 [GetExternalFields.cpp:29](file:///workspace/Source/Plugins/PicEm/Modules/Particles/Gather/GetExternalFields.cpp#L29) 的 `ps` 在**所有**调用路径下都是 `nullptr`（旧式 legacy 路径与运行时路径皆然），而下游代码按"`ps` 有效"来编写。

所有相关 gather 构造点传入的都是 species 的 `runtime_context()`（恒空）或 species 成员 `m_runtime_context`（恒空）：

- [PhysicalParticleContainer.cpp:1263](file:///workspace/Source/Plugins/PicEm/Modules/Particles/PhysicalParticleContainer.cpp#L1263)、[:1459](file:///workspace/Source/Plugins/PicEm/Modules/Particles/PhysicalParticleContainer.cpp#L1459)
- [PhotonParticleContainer.cpp:149](file:///workspace/Source/Plugins/PicEm/Modules/Particles/PhotonParticleContainer.cpp#L149)
- [RigidInjectedParticleContainer.cpp:351](file:///workspace/Source/Plugins/PicEm/Modules/Particles/RigidInjectedParticleContainer.cpp#L351)
- [ImplicitPushPX.cpp:466](file:///workspace/Source/Plugins/PicEm/Modules/Particles/Pusher/ImplicitPushPX.cpp#L466)、[:803](file:///workspace/Source/Plugins/PicEm/Modules/Particles/Pusher/ImplicitPushPX.cpp#L803)
- [WarpXParticleContainer.cpp:1204](file:///workspace/Source/Plugins/PicEm/Modules/Particles/WarpXParticleContainer.cpp#L1204)
- [ParticleExtrema.cpp:297](file:///workspace/Source/Plugins/Diagnostics/ReducedDiags/ParticleExtrema.cpp#L297)、[ColliderRelevant.cpp:469](file:///workspace/Source/Plugins/Diagnostics/ReducedDiags/ColliderRelevant.cpp#L469)

另外，QED/电离三处构造**连 `ctx` 都不传**（默认 `nullptr`），属于同类遗漏的叠加：

- [Ionization.cpp:62](file:///workspace/Source/Plugins/PicEm/Modules/Particles/ElementaryProcess/Ionization.cpp#L62)
- [QEDPairGeneration.cpp:50](file:///workspace/Source/Plugins/PicEm/Modules/Particles/ElementaryProcess/QEDPairGeneration.cpp#L50)
- [QEDPhotonEmission.cpp:53](file:///workspace/Source/Plugins/PicEm/Modules/Particles/ElementaryProcess/QEDPhotonEmission.cpp#L53)

### F1.3 对仿真的影响（按外部场模式）

因为 `cfg` 仍能正确读到外部场模式（`legacy_config()` / context 的 config 都有效），`m_Etype`/`m_Btype` 会被正常置位，于是：

**模式 A — `parse_e/b_ext_particle_function`：必崩溃。**
[GetExternalFields.cpp:67-81](file:///workspace/Source/Plugins/PicEm/Modules/Particles/Gather/GetExternalFields.cpp#L67-L81)：

```
m_Exfield_partparser = ps->externalFieldParser("Ex")->compile<4>();
```

`ps == nullptr` → 构造 gather functor 时立即空指针解引用（主机端，进入主循环前）。
受影响用例（真实存在）：
- `Examples/Physics_applications/thomson_parabola_spectrometer/inputs_test_3d_thomson_parabola_spectrometer:151/156`（`parse_E/B_ext_particle_function`，3D）
- `Examples/Physics_applications/free_electron_laser/inputs_test_1d_fel:87`（`parse_B_ext_particle_function`，1D）

**模式 B — `repeated_plasma_lens`：未定义行为 / 越界崩溃，或静默丢场。**
[GetExternalFields.cpp:83-98](file:///workspace/Source/Plugins/PicEm/Modules/Particles/Gather/GetExternalFields.cpp#L83-L98) 从 `lens`（因 `ps` 为空而取默认构造的 `RepeatedPlasmaLens{}`，见 [IParticleSystem.H:186-194](file:///workspace/Source/Core/Runtime/IParticleSystem.H#L186-L194)）读取几何：`period = 0`、`n_lenses = 0`、`starts/lengths/strengths_* = nullptr`。
随后 [GetExternalFields.H:143-147](file:///workspace/Source/Core/Modules/Particles/Gather/GetExternalFields.H#L143-L147)：

```
const auto i_lens = static_cast<int>(std::floor(zl / m_repeated_plasma_lens_period)); // 除 0
if (i_lens < m_n_lenses) {                                                            // 0
    ... m_repeated_plasma_lens_starts[i_lens] ...                                      // nullptr 下标
}
```

- `zl / 0.0` → `±inf`；`static_cast<int>(inf)` 是未定义行为（x86 常见为 `INT_MIN`）。
- 若得到负值，`i_lens < 0` 成立 → `starts[i_lens]` 对 `nullptr` 做负下标 → 越界访问，典型表现是段错误。
- 若被编译器/平台"优化"成正的合法值，则 `i_lens < 0` 不成立 → 整段聚焦场被跳过，**外部透镜场静默消失**。
无论哪种，物理结果都不可接受。
受影响用例：
- `Examples/Tests/plasma_lens/inputs_test_3d_plasma_lens:39-45`
- `Examples/Tests/plasma_lens/inputs_test_3d_plasma_lens_boosted`
- `Examples/Tests/plasma_lens/inputs_test_3d_plasma_lens_short`
- `Examples/Tests/plasma_lens/inputs_test_3d_plasma_lens_hard_edged`

**模式 C — accelerator lattice：静默丢场（最危险）。**
[GetExternalFields.cpp:37-41](file:///workspace/Source/Plugins/PicEm/Modules/Particles/Gather/GetExternalFields.cpp#L37-L41)：

```
auto const* lattice_ptr = (ps != nullptr) ? ... : nullptr;
if (lattice_ptr != nullptr && lattice_ptr->m_lattice_defined) { d_lattice_element_finder = ...; }
```

`lattice_ptr == nullptr` → `d_lattice_element_finder` 永不初始化。此时 `m_Etype/m_Btype` 为 `None`（该用例未设 `E/B_ext_particle_init_style`），[GetExternalFields.H:63](file:///workspace/Source/Core/Modules/Particles/Gather/GetExternalFields.H#L63) 的 `isNoOp()` 返回 `true`，gather 被整体跳过。
后果：四极铁/晶格场**完全不存在**，但仿真照常跑完、正常输出，无任何报错或警告——**这是最典型的静默物理错误**。
受影响用例：
- `Examples/Tests/accelerator_lattice/inputs_test_3d_hard_edged_quadrupoles`
- `Examples/Tests/accelerator_lattice/inputs_test_3d_hard_edged_quadrupoles_boosted`
- `Examples/Tests/accelerator_lattice/inputs_test_3d_hard_edged_quadrupoles_moving`

**模式 D — `read_from_file`：相对安全但依赖旁路。**
该模式下 `m_Etype/m_Btype` 被置为 `None`（[GetExternalFields.cpp:106-111](file:///workspace/Source/Plugins/PicEm/Modules/Particles/Gather/GetExternalFields.cpp#L106-L111)），外部场由 `WarpXInitData` 另行加进 `Efield_aux/Bfield_aux`，不经过 `ps`。因此本模式大概率不受 F1 影响，但仍与 F3 的静态缓存耦合（见下）。

### F1.4 为什么"2D 套件 232/232"仍能通过

提交只声称 2D 套件全绿。上述所有受影响用例的维度分布为：

- plasma_lens：**3D**
- accelerator_lattice：**3D**
- thomson_parabola_spectrometer：**3D**
- free_electron_laser（`parse_B_ext_particle_function`）：**1D**

2D 构建不会编入/调度这些 1D/3D 测试，所以 2D 全绿与"外部场在 1D/3D 上失效"完全自洽。这不是偶发漏测，而是**维度矩阵造成的系统性盲区**：本提交恰好只验证了不受影响的那个维度。

---

## F2（高）`accelerator_lattice_at` 钩子从未注册 + `get_accelerator_lattice` 死代码

### 位置

- [RuntimeContext.H:67](file:///workspace/Source/Core/Runtime/RuntimeContext.H#L67)：`std::function<const void*(int lev)> accelerator_lattice_at{};` 默认空。
- [MultiParticleContainer.cpp:1308-1314](file:///workspace/Source/Plugins/PicEm/Modules/Particles/MultiParticleContainer.cpp#L1308-L1314)：`acceleratorLattice()` 先检查 `m_runtime_context->accelerator_lattice_at`，为空则返回 `nullptr`。
- [WarpX.H:1137](file:///workspace/Source/Plugins/PicEm/WarpX.H#L1137)：`const AcceleratorLattice& get_accelerator_lattice(int lev)` —— 全仓库**零调用者**（死代码）。

### 原因

把 lattice 查询改为经 `RuntimeContext::accelerator_lattice_at` 回调，但全仓库搜索 `accelerator_lattice_at =` 为 0 处：WarpX 从未把自己注册进这个回调（`SetRuntimeContext` 只设置 MPC 的 context 指针，不安装回调）；同时原本可直接调用的 `WarpX::get_accelerator_lattice()` 被搁置成无人调用的死代码。

### 影响

即使把 F1 修好（让 `ps` 非空），`MultiParticleContainer::acceleratorLattice(lev)` 仍会因回调为空返回 `nullptr`，accelerator lattice 继续**静默丢失**。F1 与 F2 是两个相互独立的断点，只修一个都不够——这正是"系统性缺陷"的体现。

### 附带（latent，随 F2 修复后显现）

[WarpX.cpp:429-432](file:///workspace/Source/Plugins/PicEm/WarpX.cpp#L429-L432)：

```cpp
warpx::runtime::set_legacy_frame([]() -> warpx::runtime::FrameState& {
    static warpx::runtime::FrameState frame = warpx::runtime::FrameState::FromLegacy();
    return frame;
});
```

函数级 `static` 在**首次调用时**即拷贝 `WarpX` 的加速系参数（gamma_boost/beta_boost/do_moving_window/...，见 [FrameState.cpp](file:///workspace/Source/Plugins/PicEm/RuntimeBridge/Runtime/FrameState.cpp#L18-L32)）并永久冻结。单进程内若先后运行多个 `WarpX` 实例（Python 接口常见），第二个仿真的 `FrameState::FromLegacy()` 分支会读到第一个仿真的旧值。目前因 F1（species context 恒空）走 fallback `FrameState::FromLegacy()`，此问题被掩盖；一旦接通 context 并注册 frame provider，该陈旧快照就会直接进入 boosting 物理（`m_uz_boost`/`lab_time` 全部错位）。属于必须与 F2 一并处理的隐患。

---

## F3（中高）`RuntimeConfig::E/B_ext_particle_s()` 函数级 static 跨仿真冻结

### 位置

[RuntimeConfig.cpp:313-341](file:///workspace/Source/Plugins/PicEm/RuntimeBridge/Runtime/RuntimeConfig.cpp#L313-L341)：

```cpp
const std::string& RuntimeConfig::E_ext_particle_s () const
{
    static const std::string value = [] { ... pp.query("E_ext_particle_init_style", s); ... }();
    return value;
}
```

`B_ext_particle_s()` 同构。

### 原因

把"从 `MultiParticleContainer` 读取"改为"直接从 ParmParse 读取"时（注释也写明是出于 Core/plugin 分离的动机），使用了函数级 `static` 做缓存。`static` 初始化只发生一次，之后无论 `ParmParse` 如何变化都返回首次的值。

### 影响

- 同一进程内创建第二个 `WarpX`/`Runtime`（Python 参数扫描、进程内重启、同一 Python 脚本跑多组算例）：第二个仿真的外部场模式沿用第一个仿真，可能**静默丢失或误用**外部场。
- 该函数被多处关键判断复用：`WarpXInitData.cpp:1065/1933/1986/2013`、`WarpXComm.cpp:514/542`、`WarpX.cpp:3163/3173/3207/3230`。其中 `read_from_file` 的加载/通信分支与 gather 分支共享同一个冻结字符串，意味着"是否加载外部场文件"也会被跨仿真污染。
- 单进程单算例（大多数 ctest）不受影响，因此常规 CI 无法发现。

这与 F2 的 `legacy_frame` 静态快照是同一类"函数级 static 缓存把每次仿真的状态误当成全局常量"的系统性反模式。

---

## F4（中）所谓"粒子树不再包含任何 kernel 头 / Core 纯净"不成立，违背项目根本目标

### 位置与证据

1. **Core 头直接包含 kernel 头（且未使用其中符号）**
   [DefaultInitialization.H:11](file:///workspace/Source/Core/Modules/Particles/ParticleCreation/DefaultInitialization.H#L11)：`#include <WarpX.H>`（这是 kernel 头）。该文件位于 `Source/Core/...`，按项目的核心/插件分离目标，Core 不应命名具体 kernel 类型。

2. **Core 头反向依赖 PicEm 插件内部头**
   [DefaultInitialization.H:14-17](file:///workspace/Source/Core/Modules/Particles/ParticleCreation/DefaultInitialization.H#L14-L17) 在 `WARPX_QED` 下包含 `Particles/ElementaryProcess/QEDInternals/BreitWheelerEngineWrapper.H`、`QuantumSyncEngineWrapper.H`；这些文件实际位于 `Source/Plugins/PicEm/Modules/Particles/...`。Core 依赖了具体插件实现，方向与"插件只通过接口接入核心"相反。

3. **Collision 插件头直接包含 kernel 头**
   下列文件第 2 行均为 `#include <WarpX.H>`：
   - [ParticleCreationFunc.H:2](file:///workspace/Source/Plugins/Collision/BinaryCollision/ParticleCreationFunc.H)
   - [SplitAndScatterFunc.H:2](file:///workspace/Source/Plugins/Collision/BinaryCollision/DSMC/SplitAndScatterFunc.H)
   - [PhotonCreationFunc.H:2](file:///workspace/Source/Plugins/Collision/BinaryCollision/Bremsstrahlung/PhotonCreationFunc.H)
   - [NuclearFusionFunc.H:2](file:///workspace/Source/Plugins/Collision/BinaryCollision/NuclearFusion/NuclearFusionFunc.H)
   多数并未真正使用 `WarpX` 符号，只为蹭传递包含。

4. **粒子插件直接写 kernel 静态成员，靠 Core→plugin 的传递包含才编译**
   [MultiParticleContainer.cpp:347](file:///workspace/Source/Plugins/PicEm/Modules/Particles/MultiParticleContainer.cpp#L346-L354)：
   ```cpp
   pp_particles.query("use_fdtd_nci_corr", WarpX::use_fdtd_nci_corr);
   #ifdef WARPX_DIM_RZ
       WARPX_ALWAYS_ASSERT_WITH_MESSAGE(config().use_fdtd_nci_corr()==0, ...);
   ```
   该 `.cpp` 自身没有包含 `WarpX.H`，能编译的**唯一**来源链是：
   `MultiParticleContainer.cpp:24 → FilterCopyTransform.H:11 → DefaultInitialization.H:11 → <WarpX.H>`。
   一旦按项目目标清掉 Core 头里的 `<WarpX.H>`（第 1 点），此处会**立刻编译断裂**。所以"粒子树不再包含任何 kernel 头"不仅不成立，而且当前"能编"本身就是一种脆弱的隐藏依赖。

### 影响

- 直接违背项目的核心目标："插件之间相互独立，仅通过接口接入核心，通过核心总线"。现状是 Core↔plugin 双向包含、插件直接写 kernel 静态成员、MPC 构造函数还有 `dynamic_cast<WarpX*>`（[MultiParticleContainer.cpp:102-106](file:///workspace/Source/Plugins/PicEm/Modules/Particles/MultiParticleContainer.cpp#L102-L106)）。
- 对"用户可以自行编写功能插件"的扩展性目标是实质性打击：用户若按文档另建一个粒子插件，将无法复用 Core 的 `DefaultInitialization.H`（它硬绑 `WarpX.H` 与 PicEm 的 QED 引擎类型），也无法保证在不引入 kernel 的情况下编译。
- 这类问题不直接产生错误的仿真数值，但会阻塞插件生态，属于"没完全实现原版功能/可扩展性"的结构性缺陷。

（说明：`WarpX::use_fdtd_nci_corr` 与之配套的断言读取的是 `config().use_fdtd_nci_corr()`，而该方法在 [RuntimeConfig.cpp:160-162](file:///workspace/Source/Plugins/PicEm/RuntimeBridge/Runtime/RuntimeConfig.cpp#L160-L162) 直接返回同一个 kernel 静态，故此处**不是**数值不一致 bug，仅为架构越界；本报告不把它夸大为物理错误。）

---

## 5. 覆盖与复现路径小结（供复核）

| 缺陷 | 最短复核路径 | 观察到的现象 |
| --- | --- | --- |
| F1-A | 3D 构建，跑 `thomson_parabola_spectrometer` 或 1D `free_electron_laser` | gather functor 构造期空指针崩溃 |
| F1-B | 3D 构建，跑 `Examples/Tests/plasma_lens/*` | 除零 → `INT_MIN` 下标越界崩溃，或聚焦场静默消失 |
| F1-C | 3D 构建，跑 `Examples/Tests/accelerator_lattice/*` | 正常结束、无报错，但晶格场完全缺位 |
| F2 | 检查 `accelerator_lattice_at` 赋值点 / `get_accelerator_lattice` 调用点 | 0 处赋值、0 处调用 |
| F3 | 同一进程内连续构造两个 `Runtime`/`WarpX`，第二次更换外部场模式 | 第二次沿用第一次的模式 |
| F4 | 搜索 Core 头对 `<WarpX.H>`/PicEm 头的包含；搜索插件对 `<WarpX.H>`/kernel 静态的引用 | 均有命中 |

说明：本次检查环境缺少 AMReX/conda 构建依赖，无法实际编译运行；以上结论基于完整的头文件包含链、调用点与全仓库符号检索交叉验证。

---

## 6. 已排除的疑点（避免误报）

- **`m_uz_boost = sqrt(gamma²-1)·c`**：与上游 WarpX `development` 一致，非 Bug。
- **`FieldGather.H` 的 RSPHERE 球→直角坐标变换**：与上游新版 `NodalFieldGather` 公式一致，未发现物理错误。
- **`AddParticleCosts` 的成本循环**：`nSpecies()` 与 weight 语义与上游一致。
- **Ionization/QED 新增构造参数顺序**（`a_nox, a_n_rz_azimuthal_modes, a_galerkin_interpolation, a_offset`）：初始化列表赋值正确，未发现参数错位。
- **`use_fdtd_nci_corr` 断言时机**：`WarpX.cpp:616-620` 明确要求在 MPC 构造之后断言，时机正确。

---

## 7. 声明

本报告仅基于只读分析，**未修改、未新增、未删除任何项目代码或项目文件**（除本报告 `BUG_REPORT_2026-11-13.md` 本身）。`git status --porcelain` 在分析期间保持为空。报告不包含任何可直接应用的补丁或代码改动。
