# Source_Core_Runtime_A 工作日志

claim: Source_Core_Runtime_A F4,F5,F6,F7,F8,F9,F10,F11,F12,F13 @lead 2026-11-15T18:56

## 已修（按 BACKLOG 顺序）

F5 fixed: `CollisionHandlerAdapter::apply` 里第二个 `if (m_handler == nullptr) throw ...` 是不可达死代码
  （前一行已经 return）⇒ 删掉死分支，保留"没有碰撞处理器就跳过"这个既有语义并在注释里写清。
F7 fixed: `IGeometryProvider.H` 的 include guard 从 `ISimulationKernel` 抄来（两个头互相遮蔽）；
  改名为 `WARPX_RUNTIME_IGEOMETRYPROVIDER_H_`。
F8 fixed: `IFiniteDifferenceSolver.H` 的工厂块（`FiniteDifferenceSolverFactory` +
  `register_/create_/has_` 三个函数）原本写在 `#endif` **之后**，即不在 include guard 内 ⇒
  任何二次包含的 TU 都会重复声明。已移入 guard 内并补 `class RuntimeContext;` 前置声明。
F9 fixed: `IFieldSolver.H` / `IFieldSolverEngine.H` 全树零实现零使用者（`create_*` 只有声明，
  第三方插件照写必然链接失败）。删除两个头，并删除只验证它们"是抽象类"的
  `tests/runtime/compile_only_field_solver_engine_interface.cpp` 及其 CMake 注册（连同遗留的
  `target_link_libraries` 行——删可执行文件时漏删会直接 configure 失败）。

验证: 增量构建通过；`ctest -R 'compile_only|runtime_snapshot|service_registry|nonpic'` 8/8。
反证: 把上面任一改动回退，F8 的重复声明/Gates 的 catalog drift 会立刻报出（见下一轮 run_gates 输出）。

F11 fixed: `DataBus::alias` 原先先 `has(dst)` 早退、之后才 `require_field_access`，于是第一个创建
  别名的 handler 之后，所有后来的 handler 都能用同名 `dst` 绕过声明检查拿到 MultiFab*。
  改成先检查声明再走"已存在"短路（与 `field()` 对称）。
F12 fixed: `FieldGatherOrders` 三个成员补默认值（1 / false / 1）：该 POD 在虚函数签名里以
  `std::optional` 传递，`FieldGatherOrders o;` 曾会把未初始化的 gather 阶数带进内核。

验证: 增量构建通过；`ctest -R 'runtime_snapshot|plugin_|compile_only|gather|particle'` 133/133。
反证: 把 F11 的回退（先 has 再检查）恢复后，新增的"两个 handler 先后 alias 同名"用例会通过而不报错。

F6 fixed: 命名回调总线 `register_callback` 用 `registry()[name] = ...` **静默覆盖**先注册者
  （`has_callback` 仍为 true），现在重名直接 throw 并提示给事件名加命名空间前缀；
  `execute_callback` 对未注册名字原本静默 no-op（打错名字的插件永远不执行），现在按名字
  只警告一次（保持"可选事件可以不注册"的语义）。
F10 fixed: `ordered_specs()` 在"只有 declare() 没有 handler"时退化成 (point, priority) 排序、
  跳过 read/write 依赖拓扑排序与环检测 —— 与有 handler 时的语义不一致，且"计划"与"实现"的顺序
  可能不同而无人报警。现在两支都走 `order_indices()`。
F4 fixed（文档级）: `StageContext::config` 是**冻结快照**（`Runtime::evolve()` 只在整段 evolve 前后
  `sync_config()`，无法逐 step 刷新，因为 step 循环在内核里）。在字段上写明"这是快照；
  需要实时值请走 `context->config`（转发到内核）"，把 F4 点名的坑族从"隐含"变成"写在类型上"。

验证: 增量构建通过；`ctest -R 'runtime_snapshot|plugin_|compile_only|service_registry|stage_dispatch'`
  18/18、`ctest -R 'runtime_snapshot|plugin_|compile_only|stage_dispatch|task'` 17/17。
反证: 回退 F6 的 throw 后，两个插件注册同名回调只会在后者的行为里体现（前者的回调消失，无任何输出）。
