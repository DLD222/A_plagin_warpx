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
