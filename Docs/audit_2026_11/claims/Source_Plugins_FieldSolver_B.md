# Source_Plugins_FieldSolver_B 工作日志

claim: Source_Plugins_FieldSolver_B F2,F3,F4,F5,F7,F8,F9,F10,F11 @lead-mid 2026-11-15T18:30

逐条判定（基准 = 26.11，不是 26.10）：

- F2 `WeightedJacobiImpl::Update()` 覆盖自适应 omega：**invalid**。
  `diff` 26.11 `Source/NonlinearSolvers/WeightedJacobiImpl.H` 的 `Update()` 与 fork 的同名函数：
  **逐字节相同**（含 `m_omega = m_matrix_ops->EstimateOmega();`）。上游 26.11 自己把
  JacobiPC 抽成了 WeightedJacobiImpl，语义就是这样。
- F3 Newton 发散判据：**invalid**。26.11 `NewtonSolver.H:271-278` 与 fork `:276-283` 相同
  （`m_exit_status = -9` + `divergence_tolerance` 默认 1e4）。
- F4 `SpectralFieldData.H` 重复前置声明：确认成立，已删掉重复行并把声明移到 include 组之后。
- F5 Darwin `chi_dd` 同下标直取：**invalid**。26.11
  `Source/FieldSolver/ImplicitSolvers/SemiImplicitDarwin.cpp:646-673` 与 fork `:510-537`
  是同一段代码（同样的 `fac = 2 mu0/dt`、同样的 row-sum、同样的注释），只有字段查表从
  `m_WarpX->m_fields` 改成 `m_services.fields`。不是 fork 的偏离。
- F7 PETSc 分支未编译：**invalid（不是 fork 缺陷）**。本机没有 PETSc，审计构建
  `AMReX_PETSc=OFF` 是事实；但 26.11 的 `.github/workflows/` 里同样没有任何 PETSc job
  （`grep -rl "PETSC\|petsc" _upstream/warpX-26.11/.github/workflows` 结果为空），
  且这段代码在 26.11 里同样整体在 `#ifdef AMREX_USE_PETSC` 内 ⇒ 覆盖缺口是上游继承的，
  不是本 fork 引入的；要补只能在 CI 加 `-DAMReX_PETSC=ON` 的 job（本机无法验证）。
- F8 `SpectralFieldData` 持 `RuntimeContext*`、`SpectralFieldDataRZ` 持 `RuntimeData*`：确认成立，
  统一成 `RuntimeContext*`（见下）。
- F9 `pc_petsc.verbose` 直接 abort：确认成立（26.11 已没有这个旧键，fork 自己加了硬失败）。
  改为：读到旧键时先采用其值（若新键未给）再 `WMRecordWarning` 提示改键，不再 abort。
- F10 `ThetaImplicitEM::ResetStep()` 不恢复场：**invalid**，审查自己也判定为良性（记录项）。
- F11 `defineVectorPotentialBCs(geom)` 用默认初始化的边界类型：确认成立（当前无调用者，属潜在）。
  增加 `m_boundary_types_set` 标记（在 `computeField(config)` 里置位），提前调用则
  `WARPX_ALWAYS_ASSERT_WITH_MESSAGE` 给出可诊断错误。

F8 改动：`SpectralFieldDataRZ` / `SpectralSolverRZ` 的构造参数与成员统一为
`const warpx::runtime::RuntimeContext*`（与笛卡尔 `SpectralFieldData` / `SpectralSolver` 一致），
`data()` 里回退到 `legacy_runtime_data()`；注册点改传 `config.context`。

验证：见"验证记录"。
