# Source_Plugins_Diagnostics_B 工作日志

claim: Source_Plugins_Diagnostics_B F5,F7 @lead-mid 2026-11-15T18:30

F5（`WarpXOpenPMDPlot` 直接解引用 `m_runtime_context`）：
- 新增私有访问器 `WarpXOpenPMDPlot::config_snapshot()`（`WarpXOpenPMD.H/.cpp`）：
  `WARPX_ALWAYS_ASSERT_WITH_MESSAGE(m_runtime_context != nullptr &&
  m_runtime_context->config_snapshot != nullptr, ...)` 后返回 `*m_runtime_context->config_snapshot`。
- 把 `.cpp` 里 19 处 `m_runtime_context->config_snapshot->` 全部改成 `config_snapshot().`；
  另一处把 `*m_runtime_context->config_snapshot` 传参改成 `config_snapshot()`。
  效果：legacy/Python 路径缺 context（或 snapshot）时给出可诊断的 abort，而不是段错误。
  注：`:1378` 的 `m_runtime_context->config->ncomps()` 本来就有判空条件，保持不动。

F7（基类 `unique_ptr` 成员与 defaulted 拷贝）：
- `ReducedDiags.H`：把 `= default` 的拷贝构造/拷贝赋值显式改成 `= delete`（并加注释），
  保留 move 的 `= default`。原来那两行会被编译器隐式定义为 deleted，注释却写"可拷贝"。

验证：见"验证记录"。
