# Source_Core_Runtime_A 审查

审查范围：`Source/Core/Runtime/` 的 46 个文件（抽象接口层 + DataBus/HandlerSpec/PluginHost + 少量适配器头）。
这些文件**没有上游对应物**（`_upstream/warpX-26.10` 中无 `Runtime/` 目录），因此判定基准确认为：
`Docs/research_notes/PLUGIN_RUNTIME_ARCHITECTURE_V2.md`（§4 插件契约/生命周期）、
`HANDOFF.md` §1/§4（不变量与"配置快照过期"坑）、`A1.8_SERVICES_DESIGN.md`、
`A7.1_SNAPSHOT_PROGRESS.md`，以及 C++ 语言规则（虚函数/生命周期/include guard/异常跨 C ABI）。
少数接口方法逐一与上游同名函数对照（`FiniteDifferenceSolver`、`WarpXParticleContainer`、
`MultiParticleContainer`、`CollisionHandler`、`HybridPICModel`），用于确认契约与上游语义是否一致。

## 覆盖

| 文件 | 行数 | 类型 | 结论 |
|---|---:|---|---|
| Callbacks.H / Callbacks.cpp | 35 / 50 | new | 可疑（F6） |
| CollisionHandlerAdapter.H | 82 | new | 缺陷（F5、F13） |
| DataBus.H / DataBus.cpp | 103 / 169 | new | 可疑（F2 相关权限语义、F11） |
| DiagnosticsHandler.H | 45 | new | OK |
| DiagnosticsPackage.H | 29 | new | OK |
| EmptySpeciesMetadata.H | 36 | new | 可疑（见"不确定"U3） |
| FieldGatherOrders.H | 30 | new（上游 `doGatherShapeN` 的 3 参数打包） | 可疑（F12） |
| FieldRegistry.H | 89 | new | OK（元数据比较完整） |
| FieldSchema.H | 19 | new（别名） | OK |
| FieldSpec.H | 40 | new | OK |
| FrameState.H | 35 | new（`WarpX` 静态量的快照） | OK |
| HandlerSpec.H | 381 | new（HandlerSpec/StageContext/PluginHost） | 缺陷（F2、F3、F4、F10） |
| HostServices.H | 64 | new | OK（含无用 include/fwd，仅风格） |
| HybridPICParameters.H | 67 | new（自上游 `HybridPICModel` 提取的平坦参数块） | OK（逐字段核对上游默认值一致，见下） |
| IAmrPolicy.H | 40 | new | OK |
| ICheckpoint.H | 13 | new | OK |
| ICollisionHandler.H | 47 | new | OK（`doCollisions(step,cur_time,dt)` 与上游 `CollisionHandler::doCollisions` 前三参顺序一致） |
| IDiagnosticsMetadata.H | 36 | new | OK（实现逐参转发，未发现 i/b 交换） |
| IDiagnosticsService.H | 37 | new | OK（与上表大量重复，属设计冗余） |
| IElectrostaticSolver.H | 62 | new | OK（纯虚默认参数 `efield = nullopt` 是隐患，见 U4） |
| IFieldAlgebra.H | 17 | new | 可疑（U2：`computeDivB/computeDivE/computePhi` 语义歧义、缺 `<memory>`） |
| IFieldSolver.H | 47 | new | 缺陷（F9：整份无定义无使用者） |
| IFieldSolverEngine.H | 21 | new | 缺陷（F9） |
| IFilterBank.H | 33 | new | OK（经 `RuntimeData::filterBank()` 使用） |
| IFiniteDifferenceSolver.H | 124 | new（上游 `FiniteDifferenceSolver.H` 的抽象化） | 缺陷（F8：`#endif` 之后的工厂块） |
| IFluidModel.H | 37 | new | OK |
| IGeometryProvider.H | 16 | new | 缺陷（F7：guard 名抄错） |
| IHybridPICModel.H | 39 | new | OK（`ParserExecutor` 按值返回，模型存活期内有效） |
| IImplicitSolver.H | 36 | new | OK（含 `Modules/FieldSolver/ImplicitOptions.H`，该头确实在 Core 内） |
| ILegacyRuntime.H | 17 | new | OK（实现在 ServiceFactory.cpp，函数内 static 持有） |
| IMacroscopicProperties.H | 135 | new（上游 `MacroscopicProperties` 抽象化） | OK（`has_sigma_matrices()` 默认 false + `unavailable()` Abort 组合自洽） |
| IMagnetostaticSolver.H | 122 | new | 可疑（U5：`MagnetostaticConfig` 为裸指针/`std::function` 集合，无版本/尺寸校验） |
| IMovingWindow.H | 15 | new | OK |
| IPML.H | 15 | new | OK |
| IParticleBoundaryBuffer.H | 57 | new | OK（`void*` 不透明句柄是刻意的，见 U6） |
| IParticleCapability.H | 38 | new | OK |
| IParticleContainer.H | 106 | new（上游 `WarpXParticleContainer` 的抽象化） | 可疑（U1：`DepositCharge` 丢掉上游 `apply_boundary_and_scale_volume` 参数） |
| IParticleContainerAccess.H | 35 | new | OK |
| IParticleServices.H | 38 | new | OK |
| IParticleSpecies.H | 16 | new | OK |
| IParticleSystem.H | 237 | new | OK（`Evolve/PushP/gather_orders` 与上游 `PushPX` 的 `nox/galerkin/nox` 语义一致） |
| IParticleThermalizer.H | 54 | new | OK |
| IReducedDiagsService.H | 23 | new | OK（nullptr 路径由 `NullReducedDiagsService` 兜底） |
| IRunControl.H | 12 | new | OK |

**已逐字段核对、未发现偏差的强语义点**（记录以示已查）：
- `HybridPICParameters` 的默认值与上游 `HybridPICModel.H` 的同名成员（546-641 行）逐项一致：
  `n_floor=1.0`、8 个 bool 全 false（`has_per_species_eta`/`has_resistive_drag`/`add_external_fields`/
  `holmstrom_vacuum_region`/`include_hyper_resistivity_term`/`hyper_resistivity_has_B_dependence`/
  `resistivity_has_J_dependence`/`solve_electron_energy_equation`）、`VacuumSeamSwitchMode::Edge`。
- `IParticleSystem::PushP` 的 `gather_orders` 默认值与上游 `PhysicalParticleContainer.cpp:1248-1309`
  一致：E 与 B 都用 `nox`，E 带 `galerkin_interpolation`（fork 侧 `PhysicalParticleContainer.cpp:1279`
  的 `value_or({nox, galerkin, nox})` 与之一致）。
- `IParticleContainer` 的基类模板参数 `ParticleContainerPureSoA<PIdx::nattribs, 0, PolymorphicArenaAllocator>`
  与上游 `WarpXParticleContainer.H:194` 逐字相同，`nattribs` 仍是实分量个数（布局未变）。
- `IParticleContainer::sumParticleMomentum()` 的计算（含光子用 `m_e` 的特例）与上游
  `ParticleMomentum.cpp` 的 lambda 逐项一致（`w*m*ux` 等，`isPhoton() ⇔ AmIA<photon>()`）。
- `ICollisionHandler::doCollisions(int step, Real cur_time, Real dt)` 与上游
  `CollisionHandler.H:32` 的前三个参数同序（上游第 4 个 `MultiParticleContainer*` 被构造期注入取代）。

## 缺陷清单

### F1 `SpeciesHandle::numberOfParticles()` 永远返回启动时的粒子数（过期快照）
- 文件:行
  - `Source/Core/Runtime/SpeciesSpec.H:29,32,50`（`count` 一次性拷贝进 `SpeciesSpec`，`SpeciesHandle` 按值持有）
  - `Source/Core/Runtime/SpeciesRegistry.H:28`（`m_species.emplace(...)` 只存副本；同名二次注册时 `inserted==false`，
    且冲突检查只比 `charge/mass`，`count` 不会被更新）
  - `Source/Core/Runtime/DataBus.cpp:162-167`（`species()` 每次返回副本）
  - `Source/Core/Runtime/Runtime.cpp:78-81`（**唯一**一次注册，在 `initialize()` 里）
  - `Source/Plugins/PicEm/WarpXKernel.cpp:129`（`spec.count = pc.TotalNumberOfParticles(false)` 在此冻结）
- 严重度: major
- 置信度: confirmed
- 现象与依据: `IParticleSpecies` 的注释把 `numberOfParticles()` 当成 handler 可见的物种视图
  （`SpeciesSpec.H:38-42`"Opaque species view for handlers（A3.3b2）"），但 handler 只在
  `register/allocate` 阶段被调用一次，`DataBus::register_species()` 之后再无刷新路径
  （`grep -rn "register_species" Source/` 只有 `Runtime.cpp:80` 一处）。因此运行中任何
  `bus.species(name).numberOfParticles()` 得到的是**初始化后第一步之前**的粒子数，
  且同名重复注册也不会纠正（`emplace` 不覆盖）。这与 `HANDOFF.md` §4 的"配置快照过期"是同一族缺陷，
  只是这里连"刷新点"都没有。
- 建议修法: 让 `SpeciesHandle` 持有一个查询回调（`std::function<std::int64_t()>`）或让
  `DataBus::species()` 转发到 `IParticleContainerAccess::GetParticleContainer(name).totalNumberOfParticles()`；
  若保留值语义，则必须在 `Runtime::sync_state()` 里重注册，并在文档里明确"该值是快照"。
- 验证方法:
  ```bash
  grep -rn "register_species" Source/            # 只应看到 Runtime.cpp:80
  # 反例用例：注册后 step 100 时打印 bus.species("electrons").numberOfParticles()，
  # 与 speciesMetadata().NumberOfParticlesInGrid(0) 求和对不上即为确认。
  ```

### F2 `declare()` 的规格会参与 `add()` 的写冲突检查 → "先声明计划、后注册执行 handler"这一受支持模式会直接抛异常
- 文件:行
  - `Source/Core/Runtime/HandlerSpec.H:85-122`（`add()` 遍历 **`m_specs`** 判重写冲突，包含 `declare()` 进来的条目）
  - `Source/Core/Runtime/HandlerSpec.H:128-132`（`declare()` 只做 `validate` 后 push 进 `m_specs`，不做任何冲突检查）
  - `Source/Plugins/PicEm/Modules/PicEm/PicEmPackage.cpp:41-67` 定义了与
    `PicEmPackage.cpp:23-38` 注册的 handler **同名、同 StagePoint、同 writes_fields** 的声明
    （`particle_push` {Push,0} 写 `current_fp/rho_fp`；`deposition.charge_current` {AfterPush,0}；
    `field_solve.yee` {FieldSolve,0} 写 `Efield_fp/Bfield_fp`）
  - `Source/Plugins/PicEm/RuntimeBridge/Runtime/CollisionPackage.H:31-57` 与
    `Source/Core/Runtime/CollisionHandlerAdapter.H:29-36` 是同一组合（`binary_collision`/`mcc_collision`/
    `dsmc_collision` 声明 vs `collision_handler` 实注册，species 名在 {Collisions,0} 上重叠）
- 严重度: major
- 置信度: confirmed
- 现象与依据: `declare()` 的注释是"Declarations only; the bus owns no storage"（`HandlerSpec.H:32`），
  语义上应当只是计划/占位；但 `add()` 在判重写时把 `m_specs` 全量当作竞争者。因此一旦有人调用
  `PicEmPackage::register_specs(host, ...)`（该函数与 `register_handlers` 成对存在、命名即"注册计划"），
  随后的 `host.add(std::make_shared<ParticlePushHandler>())` 会抛
  `PluginHost: handlers 'particle_push' and 'particle_push' both write field 'current_fp' at the same StagePoint`，
  进程启动失败。当前是**潜伏**缺陷（`grep -rn "register_specs" Source/` 无调用点），
  但两个包都把这对 API 摆在了那里，任何使用者都会踩。
- 建议修法: `add()` 只与 `m_handler_specs`（真正有执行体的）比较写冲突；`declare()` 条目单独存
  `m_declared`，供 `ordered_specs()`/计划视图使用。这样"声明 = 计划，add = 实现"才自洽。
- 验证方法:
  ```bash
  # 在 Tests 或临时 main 中，先 PicEmPackage::register_specs(host, "field_solve.yee") 再
  # PicEmPackage::register_handlers(host, "field_solve.yee")：预期抛 runtime_error。
  ```

### F3 生命周期钩子 `on_level_created` / `on_checkpoint` / `on_restart` 从未被调用（死 API）
- 文件:行
  - `Source/Core/Runtime/HandlerSpec.H:230-241`（`run_level_created/run_checkpoint/run_restart` 三个转发函数）
  - `Source/Core/Runtime/PluginManifest.H:96-102`（接口承诺："A new mesh level exists (initial hierarchy or
    dynamic refinement)"、"Release resources…"）
  - 设计契约 `Docs/research_notes/PLUGIN_RUNTIME_ARCHITECTURE_V2.md:296-305`（§4.3 生命周期里
    `on_regrid` → `on_checkpoint` / `on_restart` 都在 "")
- 严重度: major
- 置信度: confirmed
- 现象与依据: 全仓库检索只有**定义**、没有任何调用点：
  `grep -rn "run_level_created\|run_checkpoint\|run_restart" .` → 仅 `HandlerSpec.H:230/234/238`。
  对比 `on_regrid` 是有真实驱动的（`WarpX::RunRegridStage` → `notify_regrid`，
  `Source/Plugins/PicEm/Modules/Evolve/WarpXEvolve.cpp:660-670`），说明这三个不是"故意留空"而是漏接。
  后果：插件的私有状态（每层缓存、随机数流、累积量）在 checkpoint/restart 时被**静默丢弃**；
  动态网格加密新建的层不会通知插件，`on_level_created` 里分配的东西不存在。
  另外 §4.5 承诺的"声明 `checkpoint=true` 但没实现 `on_checkpoint` → abort"也没有实现
  （`validate_capabilities()` 只比字符串能力，`HandlerSpec.H:197-220`，无法探测某个虚函数是否被 override）。
- 建议修法: 在 `WarpX::InitFromCheckpoint`/`WarpXCheckpoint` 与 `AmrCore::remake_level`/新建 level 处
  补 `run_level_created/run_checkpoint/run_restart` 调用；或在头文件里把这三个明确标注
  "NOT WIRED YET (TODO)"，避免插件作者误以为会被调用。
- 验证方法:
  ```bash
  grep -rn "run_level_created\|run_checkpoint\|run_restart" . --include=*.cpp --include=*.H
  # 期望：除 HandlerSpec.H 定义外为空 → 确认无驱动
  ctest --test-dir build -R "external_plugin" -V   # 插件里打日志即可观测到钩子从未触发
  ```

### F4 `StageContext::config` 只给 `RuntimeConfigSnapshot`，且该 snapshot 在一次 run 里不会刷新
- 文件:行
  - `Source/Core/Runtime/HandlerSpec.H:55`（`const RuntimeConfigSnapshot* config = nullptr;`，无"这是快照、勿读 ncomps"的告警注释）
  - `Source/Core/Runtime/Runtime.cpp:82-86`、`106-109`（`StageContext{..., &m_config_snapshot}`）
  - `Source/Plugins/PicEm/Modules/Evolve/WarpXEvolve.cpp:578-582, 611-615, 635-639, 660-664, 672-676, 684-688, 701-705, 720-724, 736-740`
    （所有 stage 上下文都传 `m_runtime_context->config_snapshot`）
  - `Source/Core/Runtime/Runtime.cpp:136-146`（`sync_config()` 只在 `initialize()` 与**整段 `evolve()` 结束后**调用）
  - `Source/Core/Runtime/RuntimeConfigSnapshot.H:45-58`（`ncomps/n_rz_azimuthal_modes/filter_npass_each_dir` 都是值拷贝）
- 严重度: minor（当前无消费者，属潜伏；但正是 `HANDOFF.md` §4 点名的坑族）
- 置信度: confirmed（机制）／needs-verification（是否会真的被读到）
- 现象与依据: runtime 同时持有**实时** `m_config`（`RuntimeConfig* config` 在 `RuntimeContext` 里）
  与**冻结**的 `m_config_snapshot`，但发给 handler 的只有后者。`RuntimeConfigSnapshot::FromLegacy()`
  在 `Runtime` 构造函数里就已经调用一次（`Runtime.cpp:34`，早于 `m_kernel->initialize()`），
  因此构造期到 `initialize()` 之间该快照的 `ncomps` 是默认值 1（不是 RZ 的 `2*n_modes-1`）；
  `initialize()` 之后会覆盖一次，此后整个 `evolve()` 期间不再更新。`HANDOFF.md:42-43` 明确要求
  "`InitData` 期间的代码必须读实时 `RuntimeConfig`，不能读 `RuntimeConfigSnapshot`（`ncomps` 要到层分配时才计算）"。
  目前 `grep -rn "ctx.config->" Source/` 为空（无 handler 读它），所以尚未造成数值错误。
- 建议修法: `StageContext` 同时给出 `const RuntimeConfig* live_config`，并在 `HandlerSpec.H:55`
  上写明"快照值可能过期，敏感性判断请用 `ctx.data/config()` 的实时入口"；或在每个 stage 之前
  `refresh_config()` 之类别名。
- 验证方法:
  ```bash
  grep -rn "config_snapshot" Source/Plugins/PicEm/Modules/Evolve/WarpXEvolve.cpp  # 全部传快照
  grep -rn "ctx\.config->\|ctx->config->" Source/                                  # 目前无消费者
  ```

### F5 `CollisionHandlerAdapter::apply` 的 null 检查是不可达的死代码，异常分支永远进不去
- 文件:行: `Source/Core/Runtime/CollisionHandlerAdapter.H:41-44`
- 严重度: minor
- 置信度: confirmed
- 现象与依据:
  ```cpp
  if (m_handler == nullptr) { return; }                       // 41-42
  if (m_handler == nullptr) { throw std::runtime_error(...); } // 42-44 不可达
  ```
  第二个 `if` 永远为假 → 注释/意图（"null handler 要报错"）没有实现，实际行为是**静默跳过碰撞**。
  虽然 `CollisionPackage::register_handler` 已在 `handler == nullptr` 时不注册
  （`Source/Plugins/PicEm/RuntimeBridge/Runtime/CollisionPackage.cpp:18-20`），属于双保险，
  但这正是"提前 return 吞错"的典型模式。
- 建议修法: 删掉第一个 `if`，保留 throw；或删掉 throw 并在构造时断言。
- 验证方法: 静态阅读即可；`clang-tidy -checks=-*,clang-diagnostic-unreachable-code` 会给出告警。

### F6 命名回调总线：重名注册静默覆盖、缺失名字静默 no-op
- 文件:行: `Source/Core/Runtime/Callbacks.cpp:24-27`（`registry()[name] = std::move(callback)`）、`34-39`（`find(name)==end` 直接 return）
- 严重度: minor
- 置信度: confirmed
- 现象与依据: `Callbacks.H:17-24` 把它定位成"插件之间互不认识的挂接点"，那么
  "两个插件注册同名事件"是**正常会发生**的情况；`operator[]` 覆盖时没有任何诊断，
  先注册者的回调被无声丢弃（`has_callback` 仍返回 true）。`execute_callback("typo")`
  也不会报错，打错名字的插件永远不执行。
- 建议修法: 重名时 throw（或至少 `WARPX_ALIGNED_WARNING`），`execute_callback` 对未注册名字
  给出一次警告（可带 `has_callback` 供调用者自查）。
- 验证方法:
  ```bash
  grep -rn "register_callback" Source/   # 观察是否存在多个注册点使用同一名字
  ```

### F7 `IGeometryProvider.H` 的 include guard 是从 `ISimulationKernel` 抄来的
- 文件:行: `Source/Core/Runtime/IGeometryProvider.H:1-2`（`WARPX_RUNTIME_ISIMULATIONKERNEL_H_`）
- 严重度: minor
- 置信度: confirmed
- 现象与依据: 当前不冲突（`ISimulationKernel.H:1` 用的是 `WARPX_RUNTIME_IPICKERNEL_H_`，
  `grep -h '^#ifndef WARPX' Source/Core/Runtime/*.H | sort | uniq -d` 为空），
  但两个头文件的 guard 名与自身文件名/类名不匹配，任何一方"顺手改正"就会互相屏蔽，
  现象是 `IGeometryProvider` 未定义（或更糟：`RuntimeData` 里只剩前向声明而编译仍通过）。
  这是 AI 生成代码典型的复制粘贴残留。
- 建议修法: 改成 `WARPX_RUNTIME_IGEOMETRYPROVIDER_H_`；顺手把 `ISimulationKernel.H` 的 guard 也改成
  `WARPX_RUNTIME_ISIMULATIONKERNEL_H_`（并保持 F7 的新名字不撞）。
- 验证方法: `grep -h '^#ifndef WARPX' Source/Core/Runtime/*.H | sort | uniq -d`（现在空，改名后仍须空）。

### F8 `IFiniteDifferenceSolver.H` 的工厂块写在 `#endif` 之后（不在 include guard 保护内）
- 文件:行: `Source/Core/Runtime/IFiniteDifferenceSolver.H:105-106`（`} // namespace` 与 `#endif` 提前结束），
  `108-124`（重新 `namespace warpx::runtime {` 声明 `FiniteDifferenceSolverFactory` /
  `register_/create_finite_difference_solver` / `has_finite_difference_solver_factory`，末尾仅一个 `}`）
- 严重度: minor
- 置信度: confirmed
- 现象与依据: `#ifndef` 在 1 行、`#endif` 在 106 行，因此 108-124 行在**每次** `#include` 时都会被
  重新处理（guard 已定义也挡不住）。这些是纯声明/`using`，重复声明在 C++ 合法，所以现在能编过；
  但一旦有人在这段里加内联定义或 `static` 变量，就会变成"每个包含它的 TU 各有一份"，
  与 include guard 的初衷相反。同时这段里 `struct FieldSolverRequest;` 的前向声明指向的结构体
  定义在 `IFieldSolver.H:11-18`（另一个头文件），本身也说明这段是拼接残留。
- 建议修法: 把 `#endif` 移到最后，工厂声明并入主体；`FieldSolverRequest` 若需要就在这里
  `#include "Runtime/IFieldSolver.H"` 或干脆删除该前向声明。
- 验证方法: `sed -n '100,124p' Source/Core/Runtime/IFiniteDifferenceSolver.H`；或
  ```bash
  printf '#include "Runtime/IFiniteDifferenceSolver.H"\n#include "Runtime/IFiniteDifferenceSolver.H"\n' \
    | g++ -std=c++20 -I Source/Core -I Source/Core/ablastr -fsyntax-only -x c++ -
  ```

### F9 `IFieldSolver.H` / `IFieldSolverEngine.H` 是完全没有实现、没有使用者的死接口
- 文件:行: `Source/Core/Runtime/IFieldSolver.H:21-44`（`IFieldSolver`、`NullFieldSolver`、`FieldSolverRequest`、
  `create_field_solver`）、`Source/Core/Runtime/IFieldSolverEngine.H:9-19`（`IFieldSolverEngine`、
  `create_field_solver_engine`）
- 严重度: minor
- 置信度: confirmed
- 现象与依据:
  ```bash
  grep -rn "IFieldSolver\b\|create_field_solver\b" Source/   # 除定义处外 0 命中
  grep -rn "IFieldSolverEngine" Source/                       # 除定义处外 0 命中
  ```
  两个 `create_*` 只有声明、**没有定义**：任何第三方插件照着这两个头写代码，会在链接期拿到
  undefined reference（不是编译期错误，很难定位）。更值得注意的设计风险是 `NullFieldSolver`
  的注释——"Accepts every request without changing state (disabled-plugin sentinel)"：
  若将来把"插件缺失"接到它身上，场求解会被**静默跳过**（不推进 E/B，也不报错），
  这与本仓库其它地方的"缺失就走 `amrex::Abort` 并给出可操作错误"（`IMacroscopicProperties.H:95-121`）
  策略相反。
- 建议修法: 删掉这两个头（实际契约是 `IFiniteDifferenceSolver` / `IElectrostaticSolver`），
  或在 `ServiceFactory` 里补真实工厂 + "缺失即 Abort"；`NullFieldSolver` 应改成 abort 版本。
- 验证方法: `grep -rn "IFieldSolver\|IFieldSolverEngine" Source/ Python/`；`nm -C build/lib/libwarpx* | grep create_field_solver`。

### F10 `ordered_specs()` 在"只有声明没有 handler"时退化为纯 priority 排序（跳过依赖拓扑排序与环检测）
- 文件:行: `Source/Core/Runtime/HandlerSpec.H:313-329`（`if (!m_handler_specs.empty()) {...} out = m_specs; stable_sort(by point/priority)`）
  对比 `HandlerSpec.H:251-311`（`order_indices`：读写依赖建边 + 拓扑排序 + 环检测）
- 严重度: minor
- 置信度: confirmed
- 现象与依据: 同一个"给我排序后的规格"查询，因为内部走了两个分支而给出**两种不同语义的答案**：
  有 handler 时按 `writes→reads` 依赖排序，只有 `declare()` 时只按 (point, priority) 排序，
  完全不看依赖、也不检测环。`declare()` 正是给"计划视图"用的（见 F2 的用法），
  于是"计划"与"实现"的执行顺序可能不一致，且无人报警。
- 建议修法: `out = m_specs;` 之后也调用 `order_indices(out)` 取排序结果，而不是 `stable_sort`。
- 验证方法: 声明 A{writes={"E"}}、B{reads={"E"}} 且 A.priority > B.priority，观察
  `ordered_specs()` 返回 B 在 A 前（应报错或按依赖排 A 在前）。

### F11 `DataBus::alias` 对"已存在的目标名"不做任何声明校验
- 文件:行: `Source/Core/Runtime/DataBus.cpp:136-148`（139-141 行先 `has(dst)` 直接 return，145-146 行才 `require_field_access`）
- 严重度: minor
- 置信度: confirmed
- 现象与依据: S5 的设计意图是"handler 只能碰自己声明的 key"（`HandlerSpec.H:32`、
  `DataBus.H:88-91`）。但一旦某个更早的 handler 创建了别名，后来的 handler 用同一个 `dst`
  名字就能**绕过**读写声明直接拿到 `MultiFab*`——权限检查被"名字已存在"短路。
  这与 `field()` 的严格检查不对称。
- 建议修法: 在 `has(dst)` 早退之前先 `require_field_access(dst, true)`。
- 验证方法: 单测：在两个 handler 里先后 `alias("E_alias","Efield_fp",0,0)`，
  第二个 handler 不声明 `E_alias` 也应抛异常（当前不抛）。

### F12 `FieldGatherOrders` 的成员没有默认初始化器
- 文件:行: `Source/Core/Runtime/FieldGatherOrders.H:21-26`（`int e_shape; bool e_galerkin; int b_shape;`）
- 严重度: minor
- 置信度: likely
- 现象与依据: 这是被 `std::optional<FieldGatherOrders>` 打包进虚函数签名的 POD
  （`IParticleSystem.H:95`）。当前所有构造点都用聚合初始化填满三个成员
  （`PhysicalParticleContainer.cpp:1279-1280`、`SemiImplicitDarwin.cpp:191,255`），所以现在没有实际 UB；
  但任何 `FieldGatherOrders o;` 或 `= {}` 之外的默认构造都会读到未初始化的 gather 阶数，
  表现为"随机精度"的粒子推进，且完全静默。此外 `bool` 夹在两个 `int` 之间会引入 padding，
  若将来经 C ABI 传给 `.so` 插件，`sizeof` 会随编译器变化。
- 建议修法: `int e_shape = 1; bool e_galerkin = false; int b_shape = 1;`（值取"一阶形状因子"，
  与 WarpX 默认 `nox=1` 一致）。
- 验证方法: `grep -rn "FieldGatherOrders" Source/` 复核构造点；打开 `-Wmaybe-uninitialized`。

### F13 碰撞适配器把物种名硬编码成 `electrons/ions/gas/photons`
- 文件:行: `Source/Core/Runtime/CollisionHandlerAdapter.H:34-35`（`reads_species`/`writes_species` 四个固定名字）
- 严重度: minor
- 置信度: confirmed
- 现象与依据: WarpX 的物种名完全由输入决定（`MultiParticleContainer::GetSpeciesNames()`），
  `ICollisionHandler`（`ICollisionHandler.H:15-24`）也没有查询"我碰哪些 species"的接口，
  所以这 4 个名字对绝大多数输入都是**错的**。后果有两层：
  (a) `HandlerSpec` 的依赖边/写冲突检查（`HandlerSpec.H:261-289`、`112-121`）对真实物种名失效，
  同 StagePoint 上写同一 species 的竞争不会被发现；
  (b) 若有人的物种恰好叫 `ions`，而同一 StagePoint 上另一 handler 也写 `ions`，会**误报**
  冲突并 abort。`DataBus::require_species_access` 目前对该 handler 不生效（它拿的是
  `IParticleContainerAccess`，不走 bus），所以 (a) 的直接影响限于排序元数据。
- 建议修法: 让 `ICollisionHandler` 增加 `virtual std::vector<std::string>
  readsSpecies()/writesSpecies() const`（或由 `register_handler` 的调用方传入物种名列表），
  适配器不再猜名字。
- 验证方法: 用物种名不是这四个的输入（如 `particles = e_beam`）跑一个带碰撞的算例，
  在 `PluginHost::add` 后打印 `specs()` 观察声明为空转。

## 不确定但值得复核的点

### U1 `IParticleContainer::DepositCharge` 丢掉上游 `apply_boundary_and_scale_volume` 参数 —— 逐调用点核对后判定为"有意重构"，但仍是高危改动
上游 `WarpXParticleContainer.H` 的签名是
`DepositCharge(rho, local=false, reset=false, apply_boundary_and_scale_volume=false, interpolate_across_levels=true, icomp=0)`，
fork 的契约（`IParticleContainer.H:87-90`）把第 4 个参数删了，第 4 位变成 `interpolate_across_levels`。
**参数顺序一旦有任一处调用点按上游 6 参风格机械移植，就会把 `bool` 平移到错误位置**（同类型可编译，静默改变 rho 的归一化/边界处理）。
我已核对两条主通路无碍：上游 `MultiParticleContainer::DepositCharge` 传的是
`local=true, reset=false, apply_boundary_and_scale_volume=false, interpolate=false`（`_upstream/.../MultiParticleContainer.cpp:632-641`），
fork 对应处（`MultiParticleContainer.cpp:640-647`）为 `local=true, reset=false, interpolate_across_levels=false`，语义等价；
`finalize` 语义由 `GetChargeDensityOnLevel(lev, local, finalize)`（`WarpXParticleContainer.H:353`）+ `IParticleServices::finalizeRhoOnLevel` 承接。
**其余调用点（尤其 `DepositChargeOnLevel`、ECG/静电 rho、PSATD rho）必须逐个核对是否补了 `finalize`**，漏一处就是 RZ 下漏 `1/r` 体积权重。
```bash
grep -rn "DepositCharge\s*(" Source/ | grep -v "virtual\|\.H:"
grep -rn "finalizeRhoOnLevel" Source/
```

### U2 `IFieldAlgebra` 的 `computeDivB/computeDivE/computePhi` 语义与命名不一致，且缺 `<memory>`
`Source/Core/Runtime/IFieldAlgebra.H:10-16`：`computeDivB(MultiFab&, int, VectorField const&, const std::array<Real,3>&)` 的第 2 个 `int`
没有名字也没有注释（是 `lev`？`icomp`？），`computeDivE(MultiFab&, int)` 根本不返回或接收 E 场，
`computePhi(int)` 只给一个 `int` 就返回 `unique_ptr<MultiFab>`。与 `IFiniteDifferenceSolver::ComputeDivE(Efield, divE)` 的
显式契约（`IFiniteDifferenceSolver.H:52-53`）相比信息量差太多，唯一实现
（`DiagnosticsHandler` 调 `fieldAlgebra().fieldSolve(ctx.time)` 就够说明这组接口还没用起来）。
另外 `std::unique_ptr` 用到了却没 `#include <memory>`（靠 `AMReX_MultiFab.H` 传递）。
需求：本分区内只有声明，没有调用点，**无法判定是否真有功能缺口**，建议由 `HostServices`/诊断分区确认实际使用者。

### U3 `EmptySpeciesMetadata::NumberOfParticlesInGrid` 返回空 vector，调用方按下标访问就是越界读
`Source/Core/Runtime/EmptySpeciesMetadata.H:28-31` 返回 `{}`；`Source/Plugins/Diagnostics/ComputeDiagFunctors/PartPerGridFunctor.cpp:39`
拿 `const auto& npart_in_grid = ...NumberOfParticlesInGrid(m_lev);` 之后按网格下标取。
`amrex::Vector::operator[]` 不做边界检查，因此在"没有粒子插件 + 请求了 ParticlePerGrid 诊断"的组合下是 UB 读。
`NullParticleSystem.H:121-124` 同样返回 `{}`。是否还能被触发取决于诊断插件是否已被正确禁用
（`WarpXInitData.cpp:870-884` 会警告但**不阻止**继续）。验证：
```bash
grep -rn "NumberOfParticlesInGrid" Source/ | grep -v Runtime/
ctest --test-dir build -R nonpic_kernel_2 -V
```

### U4 `IElectrostaticSolver::computePhi` 的默认参数写在纯虚函数上
`Source/Core/Runtime/IElectrostaticSolver.H:28-33`：`std::optional<...> efield = std::nullopt`。
默认实参在虚函数里是**静态绑定**的：通过基类指针调用时用基类默认值，通过派生类调用时用派生类默认值。
若某实现把默认值改成 `std::optional<...>{}`（等价）没问题，但若改成"传空即用内部 E 场"，两处调用点就会分叉。
建议改为两个重载或用 `const std::optional&` 显式传参。验证：对比
`Source/Plugins/FieldSolver/Electrostatic/*.H` 中 `computePhi` 的默认实参是否与基类一致。

### U5 `MagnetostaticConfig` 是 20+ 个裸指针/`std::function` 的聚合体，无尺寸或版本字段
`Source/Core/Runtime/IMagnetostaticSolver.H:38-84`：`fields`、`geom`、`level_*`、`eb_factory`（`std::function` 返回**引用**）、
`field_centering_coeffs_{x,y,z}`（`Real const*`，指向内核拥有的 device 内存）、`deposit_current`/`sync_current`/`finalize_current`
（`std::function` 捕获内核对象）。这不是 ABI 结构体（只在同一进程内按引用传递），所以没有版本号是可以接受的；
风险在**生命周期**：`eb_factory` 返回 `FabFactory const&`，若工厂在 solve 期间被 remake（regrid）则悬垂；
`field_centering_coeffs_*` 指向的系数数组由谁保活没有注释。建议在该结构体上明确写"只在 `computeField()` 调用期间有效"。
验证：`grep -rn "MagnetostaticConfig" Source/` 检查填充点是否与 `computeField` 同寿命。

### U6 `IParticleBoundaryBuffer::getParticleBufferPointer` / `IParticleSystem::acceleratorLattice` 用 `void*` 传递类型
`Source/Core/Runtime/IParticleBoundaryBuffer.H:37-39`、`IParticleSystem.H:201-204`。注释承认这是刻意的
（"opaque on purpose/casts it to the core particle container type"），但它把类型正确性的责任完全推给调用方，
且 `const void*` 与 `void*` 两处不一致（前者 const、后者 non-const），一个 const 丢失就可能就地改粒子数据。
建议至少提供 `const` 版本并在注释里点名允许的转换目标类型。

### U7（跨分区线索，不在本分区文件清单内）`PluginCAdapter.cpp` 有异常会穿过 C ABI 边界
`Source/Core/Runtime/PluginCAdapter.cpp:422-431` 的 `warpx_stage_field()` 直接
`ctx->bus->field(name, lev)`（非 const 重载 → 要求 **write** 声明，`DataBus.cpp:109-113`），
且**没有 try/catch**。而未声明写权限时 `require_field_access` 会抛 `std::runtime_error`
（`DataBus.cpp:18-31`），跨过 `extern "C"` 边界 → `std::terminate`。
同文件 `warpx_stage_read_field`（314-333）的注释恰好记录了踩过这个坑
（"the exception crossed the C boundary and terminated the process"，319 行），
`warpx_stage_scalar`（397-408）也写了"Exceptions must not cross the C boundary"（400 行）——
唯独 `warpx_stage_field` 漏了（相邻的 `warpx_stage_field_dir`（361-377）反而有 try/catch）。
同类问题：`warpx_stage_geometry`（287-307）调 `ctx->data->geom(lev)` 只检查了 `lev < 0`，
没检查上界，越界同样抛出去。
建议把这两个函数补上 try/catch 并返回 0（交由负责 `PluginCAdapter.cpp` 的分区确认/修复）。

### U8（跨分区线索）`has_handlers(stage, stage+1)` 忽略 `sub`，`{Stage::Push, sub>0}` 会静默顶掉芯内 push
`Source/Plugins/PicEm/Modules/Evolve/WarpXEvolve.cpp:591-595`：
`core_replaced = scheduler.has_handlers(Stage::Push, Stage::Push+1)` 只看 `spec.point.stage`，
而 `Scheduler::has_handlers`（`Scheduler.cpp:50-60`）不比较 `sub`。
`StagePoint::sub` 明确是"同一 stage 内的排序位"（`StagePoint.H:56-70`），
且已有插件在用（`Source/Plugins/FluidMoments/StarFluidPlugin.cpp:354` 用 `desc.sub = 1` 表达"排在另一个插件之后"）。
于是第三方插件若注册 `{Stage::Push, 1}`（意图"在芯 push 之后"），芯的 `OneStep_PushParticles` 会被**整段跳过**，
粒子不推进且无任何诊断。是否属"设计如此"取决于作者意图（注释写的是"registered *at* the core point replaces it"，
`{Push,1}` 是否算 "at the core point" 有歧义），因此标 needs-verification：
```bash
# external_plugin_demo 里把 stage 改成 WARPX_STAGE_PUSH 且 sub=1，跑 test_3d_particle_push 类算例，
# 观察粒子是否位移 → 位移为 0 即确认。
```
