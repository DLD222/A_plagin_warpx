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

## PicEm_A（9 条）：3 条 invalid、3 条 fixed（已验证）、3 条 wip（代码已随 93afb3b 落地，反证待补）

**事故记录**：本会话 18:3x 的未提交改动（F1/F5/F6/F7/F8/F10）被另一个会话的
`git add -A` 卷进 `93afb3b`（提交信息是 Tools_Tests F9），随后被推送到 origin。
代码内容正确、编译通过、`test_2d_dynamic_regrid` 12/12 全绿，但提交信息与内容不符；
`WORKFLOW.md §5.5` 已由 @lead 补上硬规则。**之后本会话只 add 自己的具体路径。**

F2 invalid：`Source/Plugins/PicEm/WarpX.cpp:773-776` 在 `ReadParameters()` 里已有
`#if !defined(WARPX_DIM_XZ)` 的 `WARPX_ABORT_WITH_MESSAGE("algo.ect_conformal_E is only implemented
for 2D (XZ) builds")`，早于 `InitData`；`InitializeEBGridData` 的调用点另被 `ect_conformal_E > 0` 包住。
实测：`warpx.3d Examples/Tests/embedded_boundary_cube/inputs_base_3d algo.maxwell_solver=ect algo.ect=1
algo.ect_conformal_E=1` → 正是该 abort。报告"入口没挡住"是误判（`ect_conformal_E` 在 26.10/26.11 都不存在）。

F9 invalid：`AddExternalFields(lev)` 对**不存在的层**会抛（`MultiFabRegister` 未注册时 throw），
而 `WarpX` 对 `amr.max_level>0` 强制要求 `warpx.fine_tag_lo/hi`，所以 `InitData` 时 `finest_level`
已经等于 `max_level`；实测 `amr.max_level=1 + B_ext_grid_init_style=parse_B_ext_grid_function`
正常跑到 step 1（Level 1 已建、AddExternalFields(1) 正常）。改回 `max_level` 是 no-op，且对
`test_2d_dynamic_regrid_uniform_b`（`refine_start_time=1e-14`，初始无 Level_1）会抛异常。
本会话已把该改动**回退**，HEAD 保持 `finestLevel()`。该用例 12/12 仍绿。

F12 invalid：`MaxwellEqn.H:79-83` 对纵向分量 `speed=0`、横向 `speed=c`，逐项等于线性 Maxwell 的
`0.5|A|(UR-UL)`：两个横向对 `(Ey,Bz)`、`(Ez,By)` 的 `|A| = c·I`，纵向特征值为 0。报告的反例把
E/B 耗散当成分别放大，不成立。注释与实现一致。

F1 fixed（93afb3b）：`Evolve()` 的 regrid 收尾删掉 `particleSystem().AllocData()`
（`resizeData()` 会在 `Redistribute()` 之前把被删层的粒子截断销毁），补
`m_particle_boundary_buffer->redistribute()` 与 `reduced_diags->LoadBalance()`。
验证：`ctest -R "test_2d_dynamic_regrid"` → 12/12 通过（4 个用例的 run/analysis/checksum）。
缺口：现有用例没有 `save_particles_at_*`（periodic 域 buffer 不采集），也没有 FieldProbe，
所以"buffer/reduced_diags 的陈旧 tile"这一半仍无回归覆盖（反证需新 deck，下一轮补）。

F7 fixed（93afb3b）：`MomentFluidSolver::InitData` 的 `AMREX_ALWAYS_ASSERT(AMREX_SPACEDIM==2)`
（RZ 下也成立）换成 `#if !defined(WARPX_DIM_XZ)` 的 `WARPX_ABORT_WITH_MESSAGE`；`m_eps0` 默认值
改为 `PhysConst::epsilon_0`（只用于 Gauss 残差诊断）。
验证：`warpx.3d ... fluids.moment.enabled=1` → `### ERROR : fluids.moment: only 2D XZ is supported
by this prototype (MomentFluidSolver.cpp:58)`。无 ctest 覆盖（`fluids.moment` 没有任何 deck）。

F10 fixed（93afb3b）：`WARPX_DISABLE_DIAGNOSTICS` 的两段合并为一段，两个服务都创建完再替换，
不再对将被丢弃的服务调 `InitData`，并加 `WMRecordWarning(..., high)`。
验证：`WARPX_DISABLE_DIAGNOSTICS=1 warpx.3d ... diagnostics.diags_names=d1 ...` → 输出
`WARPX_DISABLE_DIAGNOSTICS is set: every full and reduced diagnostic is disabled ...`，退出码 0。
反证：删掉 warning 该行即消失。

F5 wip（代码在 93afb3b）：`eps2` 未使用已删、`Ey2->FillBoundary` 已补；**顺带发现**同一 `[GAUSS]`
块里 3D 仍用 2D 散度（`(Dx diff)/dx + (Dz(i,j)-Dz(i,j-1))/dz`），已补 3D 分支（`Dy3`/`dz`→k、`dy`）。
验证（部分）：2D PEC 4-FAB 用例 `test_2d_macroscopic_solver` + `warpx.verbose=1` → 残差 1.6e-16；
3D EB 用例跑通并打印 `[GAUSS]`。反证（把 3D 分支换回 2D 形式看残差跳变）下一轮补。

F6 wip（代码在 93afb3b）：三处 `mfi.tilebox(...).grow(-bnd_shrink)` 换成 `rho_medium_valid_box()`
（`convert(mfi.validbox(), nodal)` + 仅对域边界收缩），内部 FAB 交界不再留一层"洞"。
验证（部分）：2D PEC 4-FAB 残差 1.6e-16（与单 FAB 同量级）；覆盖率反证（旧代码少 (nx/2-1) 个节点）
下一轮用临时断言补。

F8 wip（代码在 93afb3b）：`deposit_conservative` 的残差不再丢弃，按
`1e-12 * max(max|drho/dt|, 1)` 断言（与 `YeeDeposition` 的单测口径一致）。
无 deck 覆盖；反证（把 `Jhi` 换成 `Flo` 应触发断言）下一轮补。
