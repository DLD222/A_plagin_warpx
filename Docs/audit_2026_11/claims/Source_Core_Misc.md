# Source_Core_Misc 工作日志

claim: Source_Core_Misc F2,F4,F5,F6,F7 @lead 2026-11-15T19:40

F4 fixed: `Modules/Particles/FilterFunctors.H` 与 `Modules/Particles/Filter/FilterFunctors.H` 是两份
  **逐字节相同**的副本、且 include guard 同名（`WARPX_FILTERFUNCTORS_H`）。只有 `Filter/` 下那份被
  `WarpXOpenPMD.cpp` / `FlushFormatPlotfile.cpp` 引用，`Modules/Particles/` 下那份是孤儿；同名 guard 让
  "只改孤儿副本"变成静默不生效（且它在全局作用域定义 `SuperParticleType`，两份不同步就是静默 ODR 违规）。
  已删除孤儿副本。
F5 invalid（非缺陷，风险已记录）: `Fields.H` 的 `FieldType` 在枚举中段插入了 4 个新值、其后数值整体平移；
  但没有任何代码按整数索引/序列化该枚举（键是名字），`Runtime/FieldRegistry.H` 也不引用它 ⇒ 平移安全。
  review 自己也写"已核查无消费者，仅记录风险"。
F6 fixed（注释级）: `WarpXSolverVec/DOF` 把 `"vector_potential_fp_nodal"` 映射到**nodal** 版本，与上游的
  非 nodal 不同——但这是 fork 修好了上游的不一致（磁静力学解实际填的就是 nodal 场），且该分支**无调用者**。
  保留行为，在分支上方写明"当前为死分支 + 为什么是 nodal"。
F7 fixed: `main` 里 `Runtime` 构造抛异常会逃出 `main` 触发 `std::terminate`（用户看到
  "terminate called after throwing…" 而不是"没有内核插件"／插件加载失败的真实原因）。现在 `main` 用
  try/catch 包住构造，用 `std::cerr` 打印 `e.what()` 并返回 1（用 std::cerr 而非 amrex::Print：
  异常可能发生在 AMReX 初始化**内部**）；main 主体抽成 `run(Runtime&)`。

验证: 增量构建通过；`ctest -R 'runtime_snapshot|plugin_|filter|magnetostatic|openpmd|plotfile'` 28/28，
  `test_1d_langmuir_multi|nonpic|runtime_snapshot` 6/6。
反证: 用一个不含内核插件的构建跑 `warpx.2d`，现在打印 `ERROR: Runtime: no PIC kernel plugin is
  registered...` 且 exit=1（此前是 terminate + abort）。
