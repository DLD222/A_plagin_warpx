# Source_Core_Runtime_B 工作日志

claim: Source_Core_Runtime_B F7–F15 @lead 2026-11-15T19:30
（F6 在 round 27 已修：`SpeciesHandle::numberOfParticles` → `registeredNumberOfParticles`，见提交 2ab005c 所在批次）

F7 fixed: `ScalarRegistry::declare` 的同名冲突检查只比 `checkpoint`/`unit`、**不比 `value`**，于是两个插件
  对 "temperature" 分别声明 1.0 与 1e6 时，后者被静默忽略（`m_values.emplace` 只在首次插入）。
  现在 value 也参与比较（与 `MaterialRegistry::declare`、`StateRegistry::request_field` 一致）。
F8 fixed（可诊断化）: `ScalarSpec::checkpoint = true` 被接受但**没有任何写出/读回路径**（只有 field 走
  `FieldRegistry::checkpointNames()`）⇒ restart 后值丢失且无提示。现在 `declare` 遇到 checkpoint=true
  记一条 medium 警告，说明"本构建不把 scalar 写进 checkpoint"。
F13 fixed: 三个工厂（kernel / spectral / magnetostatic）仍是"最后一个赢"且无冲突记录，与
  `ServiceFactory` 的 F-09 修复不一致。把 `assign_factory`/`require_no_conflict` 提到
  `Source/Core/Runtime/FactoryConflict.H`（Core 内部头），三处复用；`SolverKernelPlugin` 是函数指针
  结构体，单独按 `create` 成员做同样判定。冲突从各工厂的 `create_*()` 里报出（与 ServiceFactory 一致）。
F14 invalid（已过时）: `legacy_costs()` 现在有真实调用者 —— `ParticleCore/Particles/Filter/Filter.cpp:47`
  （ParticleCore_A F2 的修复把 `Filter::ApplyStencil` 的 cost 指针改成了 `legacy_costs(lev)`）。

验证: 增量构建通过；`ctest -R 'runtime_snapshot|plugin_|compile_only|service_registry|stage_dispatch|spectral|magnetostatic'` 24/24。
反证: two-plugin 冲突用例在 `test_service_registry` 里对 ServiceFactory 已覆盖；新增的三个工厂走同一模板，
  行为一致（把 `require_no_conflict` 注掉后，两次注册会静默采用后者）。

F9 fixed: `Registry::conflict` 只有**一个字符串**，而每个 `create_*()` 都无条件 `require_no_conflict()` ⇒
  "两个插件注册了 collision-handler" 会让 `create_diagnostics_service()` 抛出关于 collision 的异常。
  改成 `std::map<std::string,std::string> conflicts` + `conflict_of(kind)`，10 个 register/create 配对
  各查自己那一类。
F10 fixed: `legacy_runtime_data()` / `legacy_host_services()` 用函数级 `static const unique_ptr` 永久缓存
  第一次 `create_legacy_*()` 的结果，`clear_service_factories()` 无法让它失效（测试里表现为"第二次注册
  不生效/顺序相关"，正是 `RuntimeConfig.H` 自己写明要避免的坑）。现在按 `Registry::generation`
  缓存，`clear_service_factories()` 递增它 ⇒ 下次调用会重建。

验证: 增量构建通过；`ctest -R 'service_registry|runtime_snapshot|plugin_|compile_only'` 17/17。
注: 中间出现一次 `No rule to make target 'lib/libwarpx.2d....a'` 的构建失败——是另一个会话同时在同一个
  `audit_build/fork` 里跑 cmake/make 造成的（并行构建冲突），单独重试即通过。这条也要写进 WORKFLOW。

F11 fixed: `warpx_host_add_handler` 把"重复写者/重复名字/非法 stage/null apply"全部压成返回码 4，
  而 `PluginLoader::load` **完全不看** `register_handlers` 的结果 ⇒ 一个插件所有 handler 都被拒绝也
  算"加载成功"，随后正常运行、只是它的物理从未生效（最坏一类静默降级）。
  现在：C 适配层把 `e.what()` 记进 `detail::last_registration_error()`（thread_local）；
  loader 在注册前后比较 `host.size()`，若有失败原因就抛出带插件名与原因的错误、注册数为 0 也抛。

验证: 增量构建通过；`ctest -R 'plugin_|runtime_snapshot|service_registry'` 13/13。
反证: 让一个测试插件注册重名 handler → loader 现在报
  "PluginLoader: '<name>' failed to register a handler: <具体原因>"（此前静默通过）。

F12 fixed: `register_stage(name, "finalize", After)` 会拿到 13001..13999 的 ordinal，而引擎最后一次调度是
  对 `Stage::Finalize` 的**精确匹配** ⇒ 该 stage 永不执行；`register_stage(name,"before_init",Before)`
  甚至拿到负数，`CApiHandler` 的 `contains()` 还会接受它。现在这两种注册直接抛错并说明"引擎永远到不了"，
  不再静默接受后永不运行。
F15 fixed: `Runtime` 构造函数里 `initialize_runtime_libraries()` 之后的所有初始化都可能抛（无内核插件、
  `WARPX_PLUGIN_PATH` 加载失败等），而**构造函数抛异常不会运行析构函数** ⇒ `finalize_runtime_libraries()`
  永不调用，AMReX 全局状态未收尾。加 `ExternalLibrariesGuard`（RAII，析构时 finalize 并清标志），
  构造成功时 `release()` 交给 `~Runtime`。

验证: 增量构建通过；`ctest -R 'plugin_|runtime_snapshot|service_registry|stage'` 15/15。
反证: 注册 `After(Finalize)` 的 stage → 现在抛 "would be placed after 'Finalize', which the execution
  engine never reaches"（此前返回一个永不被调度的 ordinal）；把 guard 的 `release()` 去掉，正常运行会
  在构造返回时 finalize 掉 AMReX（后续访问即崩），可确认守卫只在该路径生效。
