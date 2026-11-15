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
