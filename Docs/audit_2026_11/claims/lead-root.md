# lead-root 工作日志（BACKLOG 尾部窗口）

claim: Source_Plugins_ParticleCore_A F5; Source_Plugins_ParticleCore_B F1,F2,F5-F10; Source_Plugins_PicEm_A F1,F2,F5-F10,F12 @lead-root 2026-11-15T18:26

范围说明：BACKLOG 189 条中「倒数第 40～第 21 位」的未认领窗口（条目 71-90）。
- 已跳过 `@lead-tools` 认领的尾部 20 条（Tools_Tests F9,F15-F31；Star F11；PicEm_B F4）；
- 已跳过 `@lead` 认领的 ParticleCore_B F3/F4 与 Runtime_B F6。
- 本窗口 20 条中，ParticleCore_B F3/F4 属 @lead，故实际认领 18 条。

约定：每条修完在此追加一行（提交号 + 验证命令 + 结果 + 反证方式）。构建目录与 @lead 共用
`audit_build/fork`，故**先确认没有别的会话正在跑 ctest/构建**再增量构建（见下方"资源互斥"）。

## 资源互斥记录

- 2026-11-15T18:24 观测到 `ctest --test-dir audit_build/fork -R implicit|mass_matrices -j 6`（@lead）在跑，
  本会话先做只读分析，等其结束后再构建。
F5 (ParticleCore_A) invalid: 守卫**已存在**于 `PhysicalParticleContainer::BackwardCompatibility()`
（`Source/Plugins/ParticleCore/Particles/PhysicalParticleContainer.cpp:476-492`：momentum_distribution_type=maxwell_juttner 时对
theta / theta_distribution_type / theta_function(x,y,z) 全部 abort 并给出 temperature_in_eV 迁移提示），且由初始快照 7bcb44a 带入。
证据：`audit_build/fork/bin/warpx.3d audit_runs/lead-scratch/theta_legacy.inp`（maxwell_juttner + theta=1.0）→
`<species>.theta is no longer supported. Maxwell-Juttner thermal spread is now specified with temperature_in_eV...`。
上游 26.10 的 theta 也只在 maxwell_juttner 分支解析（`_upstream/warpX-26.10/Source/Initialization/TemperatureProperties.cpp:34-62`），覆盖完整。
## ParticleCore_B（8 条）复核：6 条 invalid，2 条待做（F7 清洁、F9 常量单一真源）

F1 invalid：`GetExternalFields.cpp:73-88` 的 parser 分支已有 `ps != nullptr` 断言（5d71c45），
三个曾被判"不传 ctx"的调用点 `Ionization.cpp:63`、`QEDPairGeneration.cpp:51`、`QEDPhotonEmission.cpp:54`
都已是 `GetExternalEBField(a_pti, a_offset, a_ctx)`（deefbf6），且 `WarpX.cpp:4114` 填 `context->particles`。
报告写的是 deefbf6 之前的快照。验证：`test_1d_fel`（走 parser 分支）+ 反证＝注释掉 `WarpX.cpp:4114` 该断言必触发。

F2 invalid：`MultiParticleContainer.cpp:621-651` 的 DepositCharge 无 RZ 缩放**与 26.11 逐行同构**
（`_upstream/warpX-26.11/Source/Particles/MultiParticleContainer.cpp:613-639`，上游在 26.10→26.11 把缩放移到
`FinalizeRhoOnLevel`）；逐物种替代 finalizer 存在于 `WarpXPushFieldsHybridPIC.cpp:344-345`。
现有覆盖：`test_rz_ohm_solver_resistive_drag_species_fields_picmi`（逐物种 rho 与诊断 rho 的 1e-10 判据）。
若删掉该 FinalizeRhoOnLevel，该 analysis 必红（反证）。

F5 invalid：`WarpXParticleContainer.H:303` 的 doc 与实现不符，但 26.11 的 `.H:346` 是**同一句**下同一实现
（26.10 的 `DepositCurrent` 才真的 ApplyJfieldBoundary）——上游自己没跟着改，非 fork 缺陷。

F6 invalid：三个隐式辅助函数的 `const int lev = 0;`（含被注释的 for 循环）与 26.11
`Source/FieldSolver/ImplicitSolvers/SemiImplicitDarwin.cpp:314/360/398/464/532` 逐字相同；上游就是单层实现。
本构建不含 RSPHERE/PETSc，无法也不需要验证。

F8 invalid：RSPHERE 质量矩阵角向分支是 fork 新功能（26.11 只有 RZ/RCYLINDER，`ImplicitPushPX.cpp` 的 RSPHERE 出现在
5 处与质量矩阵无关的位置）。按 `MassMatricesDeposition.H:143-196` 的 r̂/θ̂/φ̂ 定义，当前取样的三个基向量**共享同一
缩放因子** `(|r_mid|/rp_mid)²=1+O(dt²)`，正交性完好——与上游 RZ 分支同性质，不构成"耦合错误"。
本构建 `WarpX_DIMS=1;2;3;RZ` 不含 RSPHERE，RZ 隐式用例又被 `if(AMReX_PETSc)` 挡住 ⇒ 零覆盖，已记在案。

F10 invalid：`WarpXParticleContainer.cpp:1973-1978` 的 `!local` 断言与 Python 绑定 `finalize` 默认 true
在 26.11 逐字一致（`26.11/.../WarpXParticleContainer.cpp:1974-1979`、`26.11/.../Python/Particles/WarpXParticleContainer.cpp:148`），
26.10 才是两参数签名。

F7（待做，cosmetic）：`nsuborbits` 保护部分不成立（调用方 `ImplicitSolver.cpp:31-33` 的真 HasiAttrib 检查，与上游等价）；
真·fork 残留是重复 include（`MultiParticleContainer.cpp:48-49`、`Resampling/ResamplingTrigger.cpp:11+14`）与
错位注释（`MultiParticleContainer.H:383` 的注释挂错对象）。注意 `WarpXParticleContainer.cpp:33` 的重复 include
已在 @lead 的未提交改动里，不要碰。

F9（待做，real minor）：`villasenor_mass_matrices_max_grid_crossings` 有 4 份独立定义
（`WarpX.H:384`、`RuntimeConfig.cpp:262-265` 硬编码 3、`ImplicitPushPX.cpp:28` 另起符号、
`WarpXParticleContainer.cpp` 8 处模板实参字面量），上游只有 1 处且 8 处模板实参都引它。
反证：把新常量改成 2 后 `test_1d_theta_implicit_planar_pinch` 必须报 `ImplicitSolver.cpp:889` 的断言。
⚠️ 与 @lead 正在改的 `WarpXParticleContainer.cpp` 同文件 ⇒ 等其提交后再做。
