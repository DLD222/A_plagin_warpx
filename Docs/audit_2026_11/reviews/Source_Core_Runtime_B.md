# Source_Core_Runtime_B 审查

分区范围：`Source/Core/Runtime` 的后半部分（服务注册表、KernelFactory、Material/Scalar/Species/State 注册表、
Spectral/Magnetostatic 工厂、Null* 服务、PluginCAdapter/PluginCApi/C ABI、Runtime/RuntimeContext/Scheduler/
StageRegistry/KeyMatch/LegacyServices）。

上游对照说明：`_upstream/warpX-26.10` 中**不存在** `Source/Core/Runtime` 目录（`find Source -name "Plugin*" -o -name "Runtime*" -o -name "*Registry*" -o -name "ServiceFactory*"` 无输出），
因此本分区绝大多数文件是**本地新写**的契约层代码；唯一有上游对应的是 `SpectralFieldIndex.H/.cpp`（从
`Source/FieldSolver/SpectralSolver/SpectralFieldData.H` + `SpectralFieldData.cpp` 的 `SpectralFieldIndex` 构造函数机械搬移）。
所以审查方法以「契约 vs 调用点」为准：每个声明/接口都去查它是否真的被调用、返回值是否被检查、
失败分支是否被当成成功。全文只读，未修改任何源码。

## 覆盖

| 文件 | 行数 | 类型 | 结论 |
|---|---|---|---|
| ISimulationKernel.H | 56 | new | 可疑：`finalize()` 纯虚但无调用点（F3） |
| ISpeciesMetadata.H | 17 | new | OK |
| ISpectralSolver.H | 75 | new | OK（纯接口，实现侧不在本分区） |
| ImplicitKernelServices.H | 130 | new | OK（std::function 集合，空成员会抛 `bad_function_call`，不会静默） |
| KernelFactory.H | 56 | new | 可疑：last-one-wins 无冲突检测（F13） |
| KernelFactory.cpp | 73 | new | 可疑：同上 |
| KeyMatch.H | 67 | new | 可疑（轻微）：前缀匹配对称，`_` 边界之外还会把聚合名与分量名互相授权 |
| LegacyServices.H | 51 | new | 可疑：`legacy_costs()` 无人调用（F14） |
| LegacyServices.cpp | 118 | new | 同上 |
| MagnetostaticSolverFactory.cpp | 46 | new | 可疑：无冲突检测 + 缺省返回 nullptr（F13） |
| MaterialRegistry.H | 84 | new | OK（同名冲突逐字段比较后抛异常，行为正确） |
| MaterialSpec.H | 41 | new | OK |
| NullDiagnosticsService.H | 40 | new | OK（缺插件时 WarpXInitData.cpp:867-879 会 high-priority 警告，不是静默） |
| NullParticleSystem.H | 158 | new | OK（访问器抛 `logic_error` 而不是返回悬垂引用；计数为 0 是设计意图） |
| ParticleContext.H | 34 | new | OK（但未见使用者，见"复核"） |
| PluginABI.H | 22 | new | OK |
| PluginCAdapter.H | 37 | new | OK |
| PluginCAdapter.cpp | 430 | new | **缺陷**：F1、F4、F5、F11 |
| PluginCApi.H | 178 | new | 可疑：文档承诺「失败返回 0，绝不跨越 C 边界抛异常」，实现未全部兑现（F1） |
| PluginLoader.H | 27 | new | OK |
| PluginLoader.cpp | 84 | new | 可疑：`register_handlers` 结果被忽略（F11） |
| PluginManifest.H | 109 | new | **缺陷**：7 个生命周期钩子中 4 个从未被调用（F2） |
| Runtime.H | 117 | new | 可疑：`finalize()` 后的 `host()/bus()/plugins()` 无保护（F14 附注） |
| Runtime.cpp | 148 | new | **缺陷**：不调用 kernel `finalize()`（F3）；ctor 抛出时 AMReX 不 finalize（F15） |
| RuntimeConfig.H | 108 | new | 可疑：`E_ext_particle_s()` 返回 mutable 缓存引用 + 惰性写（跨分区，见"复核"） |
| RuntimeConfigSnapshot.H | 67 | new | OK |
| RuntimeContext.H | 76 | new | 可疑：`particles`/`accelerator_lattice_at` 由 `WarpX::SetRuntimeContext` 填（已接线，OK）；但上下文被 kernel 反向写入 |
| RuntimeData.H | 147 | new | OK（缺省虚函数抛 `logic_error` 而不是返回空对象） |
| ScalarRegistry.H | 62 | new | 可疑：`checkpoint` 字段无人实现（F8） |
| ScalarRegistry.cpp | 67 | new | **缺陷**：同名声明不比较初值，静默 first-wins（F7） |
| Scheduler.H | 48 | new | 可疑：文档承诺"插件插入的标准间隔之间一定能执行"，实际有执行不到的区间（F12） |
| Scheduler.cpp | 60 | new | OK（异常路径恢复 `active_handler`，嵌套调度正确） |
| ServiceFactory.H | 48 | new | OK |
| ServiceFactory.cpp | 295 | new | **缺陷**：冲突串全局化（F9）、legacy 静态持有者不随 `clear_` 重置（F10） |
| SolverVectorContext.H | 42 | new | OK（纯回调包，无实现） |
| SpeciesRegistry.H | 75 | new | 可疑：`get()` 按值返回快照（F6 一部分） |
| SpeciesSpec.H | 60 | new | **缺陷**：`numberOfParticles()` 是注册时刻的冻结快照（F6） |
| SpectralFieldIndex.H | 128 | moved（上游 SpectralFieldData.H） | OK（逐字搬移，`n_fields` 未初始化是上游既有问题，非本 fork 引入） |
| SpectralFieldIndex.cpp | 106 | moved（上游 SpectralFieldData.cpp） | OK（构造逻辑与上游逐行一致；只是去掉了 `#if WARPX_USE_FFT` 包裹） |
| SpectralSolverFactory.H | 100 | new | 可疑：last-one-wins 无冲突检测（F13） |
| SpectralSolverFactory.cpp | 49 | new | 可疑：同上 |
| StagePoint.H | 104 | new | OK（文档明确"执行引擎到不了的 ordinal 就是不执行"，诚实） |
| StageRegistry.H | 92 | new | 可疑：`reset()` 会清掉插件注册的阶段（测试专用，已注明） |
| StageRegistry.cpp | 179 | new | 可疑：允许注册永远不会执行的 ordinal 且不报警（F12） |
| StateRegistry.H | 70 | new | **缺陷**：`clear()` 契约与实现不符（F5） |
| StateRegistry.cpp | 58 | new | **缺陷**：同上 |
| TimeState.H | 33 | new | OK |

## 缺陷清单

### F1 `warpx_stage_field()`（写入口）把访问检查异常抛过 C ABI，违反自己的「返回 0、绝不抛」契约
- 文件:行：`Source/Core/Runtime/PluginCAdapter.cpp:422-430`（契约在 `PluginCApi.H:140-141`、`PluginCApi.H:153-157`）
- 严重度: critical
- 置信度: confirmed（宿主侧）/ likely（具体插件端到端后果，需跑一条 star 用例）
- 现象与依据:
  - `DataBus::field(name, lev)`（非 const 重载，`DataBus.cpp:108-112`）先调 `require_field_access(name, /*write=*/true)`，
    该函数在 handler 未在 `writes_fields` 中声明该字段时**抛 `std::runtime_error`**（`DataBus.cpp:17-33`）。
  - 同一个文件里其它六个入口全部用 `try/catch` 兜住并在失败时返回 0：`warpx_stage_read_field`(325-333)、
    `warpx_stage_read_field_dir`(350-358)、`warpx_stage_field_dir`(367-374)、`warpx_stage_scalar`(401-407)、
    `warpx_stage_set_scalar`(414-419)。**只有写入口 `warpx_stage_field` 没有**——这是典型的漏改一处。
  - 该文件 317-320 行的注释正好记录了这个崩溃模式（"the exception crossed the C boundary and terminated the
    process"），说明修复只做到了 read 入口。
  - 现成的触发链（跨分区，供 lead 路由）：`Source/Plugins/NewtonGravity/StarGravityPlugin.cpp:202`
    用**写入口**读只读字段 `star_rho`，而该插件把 `star_rho` 只声明在 `reads_fields`（同文件 280-281 行）。
    于是 `warpx_stage_field` 内部抛异常；插件自带的 `try{...}catch(...)`（203-206 行）把它吞掉并 `return -1.0`，
    `fill_all_levels`（240-247 行）随即 `break`，**没有一层被填充**，`star_rho` 恒为 0，自引力势解成常数 0。
    也就是说：这里既可能"进程直接 terminate"（真正的 C 插件），也可能"静默地不做任何引力"（本插件）。
    插件作者自己在 200-203 行写了注释承认"the bus throws ... leaving it uncaught terminates the run"，
    本质是在给宿主这个 bug 打补丁。
- 建议修法：给 `warpx_stage_field` 加与 `warpx_stage_read_field` 完全相同的 `try { ... } catch (const std::exception&) { return 0; }`；
  同时把 `StarGravityPlugin.cpp:202/248/271` 改用 `warpx_stage_read_field`（或把 `star_rho` 加进 `kWrites`）。
  建议在 `PluginCAdapter` 里为所有 `extern "C"` 入口加一个统一包装，避免下次再漏。
- 验证方法：
  ```bash
  grep -n "try\|catch" Source/Core/Runtime/PluginCAdapter.cpp   # 看 422-430 段确实无 try
  # 端到端：跑一条 star 用例，正常应打印 "[gravity] level N: ... mass ="，现在不会打印
  grep -rn "\[gravity\] level" build/bin/*/*.out 2>/dev/null
  ctest --test-dir build -R star
  ```

### F2 `IPluginLifecycle` 的 7 个钩子里有 4 个从未被调用（幻觉式集成）
- 文件:行：`Source/Core/Runtime/PluginManifest.H:98,100,101,102`（`on_level_created` / `on_regrid` / `on_checkpoint` / `on_restart`）
- 严重度: major
- 置信度: confirmed
- 现象与依据:
  - `PluginHost` 只提供 `run_schema`/`run_init`/`run_finalize` 三个分发器被实际调用
    （`HandlerSpec.H:222-243`，调用点 `Runtime.cpp:72,85,108`）。
  - `run_level_created`、`run_checkpoint`、`run_restart`（`HandlerSpec.H:230,234,238`）**全树零调用点**；
    `IPluginLifecycle::on_regrid` 连分发器都没有：`notify_regrid`（`HandlerSpec.H:331-334`）只调
    `IHandler::on_regrid`，不调 `entry.plugin->on_regrid`。
  - 现有测试也没覆盖：`tests/runtime/test_runtime_snapshot.cpp:230-251` 只测了 `IHandler::on_regrid`，
    注释却写成 "plugin on_regrid callback invoked"，掩盖了这个空档。
  - 后果：一个在 `on_level_created` 里按层分配缓冲、在 `on_checkpoint/on_restart` 里写读自有状态的插件
    会**永远拿到空/过期数据**，而且没有任何报错——amr 加密、checkpoint/restart 场景下最危险。
- 建议修法：在 `WarpX::MakeNewLevelFromScratch`/`RemakeLevel`/`Regrid` 之后调 `run_level_created`/`on_regrid`，
  在 checkpoint 写/读路径调 `run_checkpoint`/`run_restart`；或在真正接线前把这 4 个钩子从接口里删掉，
  避免插件作者以为它们有效。
- 验证方法：
  ```bash
  for f in run_level_created run_checkpoint run_restart; do echo "-- $f"; grep -rn "$f" Source/ ; done
  grep -rn "plugin->on_regrid" Source/    # 无输出 => 生命周期钩子确实没有分发器
  ```

### F3 内核契约 `ISimulationKernel::finalize()` 从未被调用
- 文件:行：`Source/Core/Runtime/ISimulationKernel.H:27`；`Source/Core/Runtime/Runtime.cpp:102-119`
- 严重度: major
- 置信度: confirmed
- 现象与依据:
  - `Runtime::finalize()` 只调 `m_kernel->clear()`，全树没有任何 `kernel->finalize()` 调用
    （`grep -rn "kernel->finalize" Source/` 无输出）。
  - 上游 `Source/main.cpp:91` 是 `WarpX::Finalize()`；本 fork 把 teardown 挪进了
    `WarpXKernel::clear()`（`Source/Plugins/PicEm/WarpXKernel.cpp:141-149`：`m_warpx.reset(); WarpX::ResetInstance();`），
    而 `WarpXKernel::finalize()` 是空实现（同文件 169 行）。
  - 后果：任何按契约实现 `finalize()`（写收尾输出、关文件、释放外部资源）的**第三方内核插件**会被静默跳过。
    当前 PicEm 内核侥幸无害，因为它的 finalize 是空的——这正是"接口存在但没人调用"的典型形态。
- 建议修法：`Runtime::finalize()` 里在 `run_finalize(ctx)` 之后调 `m_kernel->finalize()`，再 `clear()`；
  或者删掉纯虚 `finalize()` 并在注释里指明 `clear()` 即 teardown。
- 验证方法：`grep -rn "finalize ()" Source/Core/Runtime/ Source/Plugins/PicEm/WarpXKernel.cpp`；
  写一个 `finalize()` 里 `++calls` 的 fake kernel 走 `Runtime` 生命周期（参考 `tests/runtime/test_nonpic_kernel.cpp`）。

### F4 `warpx_stage_geometry()` 无法对"不存在的层"返回 0，且没有上界检查
- 文件:行：`Source/Core/Runtime/PluginCAdapter.cpp:287-304`
- 严重度: major
- 置信度: likely
- 现象与依据:
  - 文档（`PluginCApi.H:163`）写的是 "Returns 1 on success, 0 if the level does not exist"。
  - 实现只判 `lev < 0`，然后直接 `ctx->data->geom(lev)`；`lev >= finestLevel` 时栈位上
    `WarpXRuntimeData::geom(lev)` → `w().Geom(lev)`（`Source/Plugins/PicEm/RuntimeBridge/Runtime/WarpXRuntimeData.cpp:76`）
    是 `amrex::Vector<Geometry>::operator[]`，release 构建无边界检查 → 越界读（垃圾几何 / 段错误），
    而不是返回 0。该函数也没有 `try/catch`。
  - 插件侧确实按"返回 0 = 层不存在"来探测层数（StarGravityPlugin 的 `for lev<8` 循环），
    只是因为先读字段拿到 0 才 break，才没有踩到。
- 建议修法：`if (lev < 0 || (ctx->data != nullptr && lev > ctx->data->finestLevel())) { return 0; }`，
  并把函数体包进 `try/catch(...) { return 0; }`。
- 验证方法：
  ```bash
  # 用一个声明了 handler 的测试插件，在 lev=finestLevel+1 上调 warpx_stage_geometry，
  # 期望返回 0；当前会读越界（-fsanitize=address 或 amrex DEBUG 下直接 assert）
  cmake --build build -j 8 --target test_runtime 2>/dev/null; ctest --test-dir build -R runtime
  ```

### F5 `StateRegistry::clear()` 不清理 scalar（契约与实现不符）
- 文件:行：`Source/Core/Runtime/StateRegistry.cpp:52-56`（契约 `StateRegistry.H:60-61`）
- 严重度: major
- 置信度: confirmed
- 现象与依据:
  - 头文件写 "Drop every request (tests only)"，实现只 `m_fields.clear()`，`m_scalars` 原封不动；
    而 `ScalarRegistry` 自己明明有 `clear()`（`ScalarRegistry.cpp:60-65`）。
  - 后果：同进程跑第二个算例（本仓库 Python 接口与 `tests/runtime` 都是这个模式）时，
    上一算例的 scalar 声明和值都还在；一个插件重新 `request_scalar` 同名但不同 `unit`/`checkpoint` 会
    抛 "conflicting declaration"，或者更糟——静默沿用上一算例的值（配合 F7）。
- 建议修法：`clear()` 里加 `m_scalars.clear();`。
- 验证方法：`grep -n "clear" -A5 Source/Core/Runtime/StateRegistry.cpp`；补一条 unit test：
  `state.request_scalar(...); state.clear(); CHECK(!state.scalars().has(...));`

### F6 `SpeciesHandle::numberOfParticles()` 是注册时刻的冻结快照，且全树无人调用
- 文件:行：`Source/Core/Runtime/SpeciesSpec.H:43-56`（尤其 47、50 行）；`SpeciesRegistry.H:42-49`
- 严重度: major
- 置信度: confirmed
- 现象与依据:
  - `SpeciesHandle` 按**值**持有一份 `SpeciesSpec`（含 `count`），`count` 在
    `WarpXKernel::register_species`（`Source/Plugins/PicEm/WarpXKernel.cpp:120-131`）时由
    `pc.TotalNumberOfParticles(false)` 填充，而 `register_species` 只在 `Runtime::initialize` 里调用一次
    （`Runtime.cpp:78-81`）。`DataBus::species()` 也按值返回（`DataBus.cpp:162-166`）。
  - 于是 handler 拿到的粒子数永远是**第 0 步之前**的数量；`SpeciesHandle` 的类注释却写
    "The underlying WarpXParticleContainer is hidden in the .cpp; handlers only see name/count"，
    暗示是一个 view，实际是一个冻结快照——注释与语义不符。
  - `grep -rn "numberOfParticles" Source/` 只有定义本身，无调用者：这是一个既没人用、语义又错的 API。
- 建议修法：要么把 `SpeciesHandle` 改成持有 `const IParticleContainerAccess*` + 名字、每次调用现查；
  要么在文档里明确"count 是注册时的值"，并在 `register_species` 每次 `InitData`/重分布后刷新。
- 验证方法：
  ```bash
  grep -rn "numberOfParticles" Source/    # 只有 SpeciesSpec.H:50 一处定义
  # 反例用例：init 0 个粒子、第 1 步注入 100 个，handler 读 species("<name>").numberOfParticles() 仍为 0
  ```

### F7 `ScalarRegistry::declare()` 不比较初值：同名 scalar 冲突被静默吞掉
- 文件:行：`Source/Core/Runtime/ScalarRegistry.cpp:23-32`
- 严重度: minor
- 置信度: confirmed
- 现象与依据:
  - 冲突检查只比 `checkpoint` 与 `unit`（24 行），**不比 `value`**；命中已存在时直接 `return`，
    保留先注册者的初值（`m_values.emplace` 只在首次插入）。两个插件对 `"temperature"` 分别声明
    1.0 与 1e6，第二个被静默忽略。
  - 同类的 `MaterialRegistry::declare`（`MaterialRegistry.H:37-48`）和 `StateRegistry::request_field`
    （`StateRegistry.cpp:35-41`）都比了全部字段，唯独 scalar 漏了 `value`——不一致。
- 建议修法：`if (it->value != spec.value || ...) throw`，或明确语义为"首次声明赢"并写进注释。
- 验证方法：`grep -n "it->checkpoint" -A3 Source/Core/Runtime/ScalarRegistry.cpp`；
  写 unit test 声明两次不同 value，断言抛异常。

### F8 `ScalarSpec::checkpoint` 被接受但从未实现（声明成功 = 静默失效）
- 文件:行：`Source/Core/Runtime/ScalarRegistry.H:24`；`StateRegistry.H:29-30`（"the core decides when to create, recreate and checkpoint it"）
- 严重度: minor
- 置信度: confirmed
- 现象与依据:
  - 全树只有两处读 `checkpoint`：`ScalarRegistry.cpp:24`（冲突比较）和 `StateRegistry.cpp:36`（field 的比较）。
    没有任何 checkpoint 写出/读回路径处理 scalar；相比之下 field 有 `FieldRegistry::checkpointNames()`
    （`FieldRegistry.H:63-68`）与 `AllocPluginFields` 的 `spec.checkpoint` 传递。
  - 也就是：`request_scalar({{... .checkpoint = true}})` 返回成功，restart 后值丢失，无警告。
- 建议修法：接入 checkpoint 读写，或在 `declare` 遇到 `checkpoint == true` 时 `WMRecordWarning`/抛异常。
- 验证方法：`grep -rn "checkpoint" Source/Core/Runtime/ScalarRegistry.* Source/Plugins/PicEm/Modules/*/Checkpoint* `。

### F9 服务注册表的"冲突"是**全局单串**：任一服务种类冲突会让所有无关服务创建失败
- 文件:行：`Source/Core/Runtime/ServiceFactory.cpp:42,53-69`（`require_no_conflict` 被 `create_*` 全部调用：115/135/161/169/192/212/239/249/260/286）
- 严重度: minor
- 置信度: confirmed
- 现象与依据:
  - `Registry::conflict` 只有一个 `std::string`，`assign_factory` 把任意种类的冲突写进它；
    `require_no_conflict()` 在每个 `create_*()` 开头无条件抛出。于是"两个插件都注册了 collision-handler"
    会让 `create_diagnostics_service()` 抛 `runtime_error("two plugins registered a collision-handler factory...")`——
    错误信息指向另一类服务，失败点也与真实问题无关。调试成本高，且可能掩盖真正的首个失败。
- 建议修法：把 `conflict` 改成 `std::array<std::string, N>` 或 `std::map<const char*, std::string>`，按种类检查。
- 验证方法：注册两个不同的 collision 工厂，再调 `create_diagnostics_service(nullptr)`，观察异常文本。

### F10 `legacy_runtime_data()` / `legacy_host_services()` 的函数级静态持有者不随 `clear_service_factories()` 重置
- 文件:行：`Source/Core/Runtime/ServiceFactory.cpp:269-281`（对比 `clear_service_factories()` 217-221）
- 严重度: minor
- 置信度: confirmed
- 现象与依据:
  - `static const std::unique_ptr<RuntimeData> holder = create_legacy_runtime_data();` 一旦初始化，
    `clear_service_factories()`（只重置 `registry()`）无法让它失效；之后重新注册的 factory 永远不会被用，
    测试里表现为"第二次注册没生效 / 顺序相关"。
  - 这与 `RuntimeConfig.H:100-102` 自己写的理由直接矛盾：「a function-level static would leak one
    simulation's input into the next one in the same process」——同一个坑在这个文件里又踩了一次。
  - 另外 `Runtime::[~Runtime]` 与 `Runtime::finalize()` 之后 `Runtime::host()/bus()/plugins()`
    （`Runtime.cpp:129-134`）会解引用已 `reset()` 的 unique_ptr，无任何保护。
- 建议修法：提供一个 `reset_legacy_services()` 与 `clear_service_factories()` 绑定；或在 `clear_` 时
  把 holder 改成 `std::optional`/指针并在 `clear_` 里置空。`Runtime` 的访问器加 `if (m_finalized) throw`。
- 验证方法：两个 unit test 顺序执行（先 `legacy_runtime_data()`，再 `clear_service_factories()` 并重新注册），
  断言新工厂被调用；`ctest --test-dir build -R runtime` 配合 `--repeat until-fail:5` 也能暴露顺序依赖。

### F11 C ABI 把 handler 注册失败全部折叠成返回码 4，loader 又完全忽略它
- 文件:行：`Source/Core/Runtime/PluginCAdapter.cpp:260-266`；`Source/Core/Runtime/PluginLoader.cpp:77`
- 严重度: minor
- 置信度: confirmed
- 现象与依据:
  - `warpx_host_add_handler` 的 `catch (const std::exception&) { return 4; }` 把"重复名字 / 重复写者 /
    未注册的 stage / null apply"全部压成同一个 4，插件拿不到原因；
  - `PluginLoader::load` 调 `desc->register_handlers(...)`（void 返回）**完全不看**注册结果，
    于是"整个插件的所有 handler 都被拒绝"也会被当作加载成功，`host.add_plugin` 照常加入 manifest。
    进程随后正常跑，只是这个插件的物理**从来没生效**——正是最坏的一类静默降级。
  - `PluginCApi.H:123-126` 的注释也没说 4 代表什么。
- 建议修法：`PluginLoader` 在 `register_handlers` 前后比较 `host.size()`，为 0 时至少
  `WMRecordWarning`/抛异常；返回码改为按失败类型区分，并在 C 头文件里写明。
- 验证方法：写一个 `apply == nullptr` 的 C 插件，`PluginLoader::load` 之后 `runtime.plugins().size()` 与
  handler 数量对比；期望报错，当前静默。

### F12 `register_stage()` 可以注册执行引擎永远到不了的 ordinal，且没有任何提示
- 文件:行：`Source/Core/Runtime/StageRegistry.cpp:130-170`；文档承诺 `Scheduler.H:19-23`
- 严重度: minor
- 置信度: confirmed
- 现象与依据:
  - `register_stage(name, "finalize", After)` → 取到 13001..13999 之间的 ordinal；引擎最后一次调度是
    `run_stage(ctx, Stage::Finalize)`（`Runtime.cpp:111-112`，精确匹配），`RunDiagnosticsStage` 只跑到
    `[Diagnostics, Finalize)`（`WarpXEvolve.cpp:708-709`）。→ 该 stage 永不执行。
  - `register_stage(name, "before_init", Before)` → 得到**负** ordinal（lo=-999, hi=-1 → -500），同样永不执行，
    而 `CApiHandler` 的构造校验 `StageRegistry::contains(-500)` 会返回 true，handler 被安静接受。
  - `Scheduler.H` 却宣称 "A plugin that registers a stage between two standard stages is therefore executed
    without any change to the core or to the kernel"：只在部分间隔成立，最终阶段之后/最前阶段之前不成立。
  - `StagePoint.H:27-28` 倒是诚实（"declaring a handler at an ordinal that the engine never reaches simply
    means it never runs"），但没有任何运行时反馈。
- 建议修法：`register_stage` 在 `[Stage::BeforeInit, Stage::Finalize]` 之外时抛异常；
  或注册成功后记录并在 `run_finalize` 时对从未执行的 handler 发 warning。
- 验证方法：`register_stage("post_finalize", "finalize", StagePosition::After)` 后注册 handler，
  跑一步，断言计数为 0；`grep -n "lo = anchor_ordinal" -A8 Source/Core/Runtime/StageRegistry.cpp`。

### F13 三处工厂仍是"最后一个赢"且缺省静默返回 nullptr（与 ServiceFactory 的 F-09 修复不一致）
- 文件:行：`Source/Core/Runtime/KernelFactory.cpp:28-32`；`Source/Core/Runtime/SpectralSolverFactory.cpp:24-28`；`Source/Core/Runtime/MagnetostaticSolverFactory.cpp:21-25,33-38`
- 严重度: minor
- 置信度: confirmed
- 现象与依据:
  - 三个注册器都是裸 `registry() = factory;`，没有 `ServiceFactory::assign_factory` 那样的冲突记录。
    两个 field-solver 插件同时链接时，谁生效取决于静态初始化/链接顺序——这正是 `ServiceFactory.cpp:45-52`
    注释里明确要消灭的 F-09 缺陷模式，只在一半的注册表上修了。
  - `create_magnetostatic_solver()`（36 行）和 `create_spectral_solver()`（39 行）在无插件时返回 nullptr；
    `SpectralSolverFactory.H:88-90` 声明"内核会在输入要求 PSATD 时报错"，需要调用点保证——
    属于需要复核的隐式契约（见下）。
- 建议修法：把 `assign_factory`/`require_no_conflict` 提取成一个公用模板（例如 `FactorySlot<F>`），三处复用。
- 验证方法：`grep -rn "registry() =" Source/Core/Runtime/*Factory*.cpp`；两次注册不同工厂后调 `create_*`，断言报错。

### F14 `legacy_costs()`：注册了 provider，却没有任何调用者（死契约）
- 文件:行：`Source/Core/Runtime/LegacyServices.H:41-43`；`LegacyServices.cpp:67-76`；注册点 `Source/Plugins/PicEm/WarpX.cpp:442-444`
- 严重度: minor
- 置信度: confirmed
- 现象与依据:
  - `grep -rn "legacy_costs" Source/` 只有 setter、getter 定义和 `WarpX.cpp:442` 的注册；
    getter 零调用。真正的负载均衡代价数组走的是 `RuntimeData::costs(lev)`（见
    `Source/Plugins/Diagnostics/ReducedDiags/LoadBalanceCosts.cpp:117`），而 `legacy_data()` 那条路是活的。
  - 即：`set_legacy_costs` 的 lambda 注册后永不触发；`LegacyServices.H` 的注释（"null when it does not keep one"）
    描述的机制其实不存在调用者。属于机械搬迁留下的残骸，容易误导后续修改负载均衡的开发者。
- 建议修法：删除 `set_legacy_costs`/`legacy_costs` 与 `WarpX.cpp:442-444` 的注册，或让某个真实调用点用它。
- 验证方法：`grep -rn "legacy_costs" Source/ tests/`。

### F15 `Runtime` 构造函数抛出时 AMReX 不会被 finalize（资源泄漏 + AMReX 退出断言）
- 文件:行：`Source/Core/Runtime/Runtime.cpp:26-50`
- 严重度: minor
- 置信度: likely
- 现象与依据:
  - 顺序是 `initialize_runtime_libraries()`（→ `initialize_external_libraries` = `amrex::Initialize`）
    → `create_solver_kernel()`；后者在无内核插件时**抛 `runtime_error`**（`KernelFactory.cpp:43-47`）。
    构造函数抛异常时析构函数不会执行，`finalize_runtime_libraries()` 永远不调用 → AMReX 全局状态未收尾。
  - 正常 Core-only 构建里两个钩子同源（都没注册），不会走到这里；但"注册了 create 却没注册
    initialize_libraries"的插件、或 `make_warpx_kernel` 内部 OOM/解析失败时就会命中。
- 建议修法：用 RAII guard（局部对象在栈展开时调 `finalize_runtime_libraries`），或把
  `m_external_libraries_initialized = true;` 之后的所有初始化放进 `try/catch` 里复位。
- 验证方法：临时设置 `WARPX_PLUGIN_PATH=/nonexistent.so` 让 ctor 抛（`PluginLoader` 在 ctor 末尾，
  会命中），在 `~Runtime` 之前用 `amrex::Finalize()` 的调试打印确认它没被调用。

## 不确定但值得复核的点

1. **`RuntimeConfig::E_ext_particle_s()` 的懒缓存**（`Source/Plugins/PicEm/RuntimeBridge/Runtime/RuntimeConfig.cpp:316-331`，
   该文件属其它分区，但契约在 `RuntimeConfig.H:100-104`）：`mutable std::string` 缓存 + 返回 `const std::string&`。
   若两个线程首次并发调用 → 对 `std::string` 的 `mutable` 写是数据竞争（`-fsanitize=thread` 可验）；
   若调用方持有该引用而 `Runtime::sync_config()`（`Runtime.cpp:136-140`）把 `m_config` 整体重新赋值，
   引用仍有效但内容可能被替换。建议确认这条路径只在串行初始化阶段被调用。

2. **`MagnetostaticSolverFactory` 的缺省 nullptr 是否被调用点转成"没配置"**：
   `create_magnetostatic_solver()` 返回 nullptr 时，调用方若只是 `if (solver)` 就静默跳过，则输入
   `algo.magnetostatic=...` 会被无声忽略。需要看 `WarpXInitData`/`FieldSolver` 侧是否报错（不在本分区）。

3. **`KeyMatch::key_matches` 的对称前缀规则**：`key_matches("jfx","jfx_external")` 为 true，
   声明读 `jfx` 即获得任何 `jfx_*` 的写权限（反向亦然）。对 WarpX 现有的 `*_fp_x`/`*_cp_x` 命名是想要的，
   但 `_` 边界不是白名单，将来出现 `Efield_fp_old` 之类同前缀字段时会出现越权。建议加一个"只允许
   `_x/_y/_z`（RZ 再加 `_r/_t`）后缀或聚合名↔分量名的显式映射表"。

4. **`ParticleContext.H`**：结构体本身没问题，但 `grep -rn "ParticleContext" Source/` 除定义外几乎无人使用，
   属于与 `IParticleContainerAccess`/`IParticleServices` 重叠的第三套抽象；建议确认是否有意保留。

5. **`SpectralFieldIndex` 的 `int n_fields;` 未初始化**（`SpectralFieldIndex.H:88`）：这**不是本 fork 引入**的，
   上游 `Source/FieldSolver/SpectralSolver/SpectralFieldData.H:101` 同样如此（构造函数是唯一赋值点，
   默认构造出来的对象 `n_fields` 是 indeterminate）。既然本 fork 把它提升成了一个公开的
   runtime 类型（`ISpectralSolver::spectralIndex()` 返回它），默认构造 + 使用 `n_fields` 的风险变大了：
   建议给默认成员初始化 `= -1` 或删除默认构造函数。

6. **`IPluginLifecycle::on_init` 的时序**：`Runtime::initialize` 在 `m_kernel->initialize()` 之后调
   `run_init`（`Runtime.cpp:76-86`），注释说"Called after the kernel has allocated levels"（`PluginManifest.H:95`），
   一致；但 `WarpXInitData` 的 InitData 阶段还会再 `RunInitStages`，两套初始化路径并存，
   建议确认从 Python 走 legacy 路径时 `on_init` 也不会被调用两次。

7. **`warpx_stage_geometry` 的 `lo/hi` 语义**：`domain.smallEnd(d)/bigEnd(d)` 返回的是 **index space** 的
   `IntVect`（int），而 `prob_lo/prob_hi/dx` 是 `double`；维度 < 3 时未用维度填 0/1.0。
   对 RZ 构建，插件若把 `lo[0], lo[1]` 当 x/y 处理而实际是 r/z，会算错域——建议在 C 头文件里明确
   "index 0 is AMReX x (= r in RZ), index 1 is AMReX y (= z in RZ)"，目前只有一句 "the index bounds"。
