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

## 第二轮：ParticleCore_B F7/F9 与 PicEm_A F8 已修并验证（提交 ee6be11）

F9 (ParticleCore_B) fixed by ee6be11：`villasenor_mass_matrices_max_grid_crossings` 的**单一真源**。
新文件 `Source/Core/Modules/Particles/MassMatricesLimits.H`（`warpx::mass_matrices::max_grid_crossings = 3`），
`WarpX.H:385` 改为引用它，`RuntimeConfig.cpp:264` 不再硬编码 `return 3;`，
`WarpXParticleContainer.cpp` 的 8 处模板实参 `<1..4, true/false, 3>` 与 `ImplicitPushPX.cpp` 的 4 处都改用同一常量
（后者原来的文件级 `constexpr int ... = 3;` 已删）。改前同一数值有 4 份独立定义，运行期断言
（`ImplicitSolver.cpp:889`）与编译期数组容量只靠"碰巧都等于 3"绑定。
验证：`ctest -R "test_1d_theta_implicit_planar_pinch\.|test_2d_semi_implicit_pc_jacobi\.|test_2d_theta_implicit_jfnk_vandb\."`
→ 9/9 通过（含 `test_2d_theta_implicit_jfnk_vandb.run` 64 s）。
反证：`warpx.1d Examples/Tests/implicit/inputs_test_1d_theta_implicit_planar_pinch particles.max_grid_crossings=99`
→ `Assertion max_grid_crossings <= config().villasenor_mass_matrices_max_grid_crossings() failed
(ImplicitSolver.cpp:888)`，即"越界检查与核容量由同一常量绑定"这条链路是活的。
（注：该 deck 自带 `particles.max_grid_crossings = 2`，所以"把常量改成 2"并不会触发断言——用 99 才是有效反证。）

F7 (ParticleCore_B) fixed by ee6be11（cosmetic）：`MultiParticleContainer.cpp` 删掉重复的
`#include "Runtime/LegacyServices.H"` 并让 Runtime 组按字母序；`Resampling/ResamplingTrigger.cpp` 同上
（原来 `LegacyServices.H` 出现两次，第二次在 `Utils/Parser` 组里）；`MultiParticleContainer.H:383`
原来把 `UpdateInjectionPositions` 的注释挂在 `countParticlesWithSuborbits` 上方，已移到正确声明上方。
`nsuborbits` 的"缺属性保护"部分**不成立**：唯一调用链 `ImplicitSolver.cpp:31-33` 有真 `HasiAttrib` 检查，
与 26.11 `ImplicitSolver.cpp:35` 等价，故不加冗余检查。
验证：四个维度的 `app_*` 目标全部重编通过；`bash Tools/verify/run_gates.sh` 九门全 OK。

F8 (PicEm_A) fixed by ee6be11：`MomentFluidSolver::AdvanceAndDeposit` 不再丢弃 `deposit_conservative` 的
连续性残差，改为断言。参考量取 **max|rho|/dt** 而不是 |drho/dt|：本会话先用 |drho/dt| 实现，
在 dt≈1e-16 的正常 deck 上 `resid=469, scale=2021`（两者都是舍入噪声/dt）→ 误杀；
换成 max|rho|/dt 后同一 deck 残差远低于容差。
验证：`warpx.2d audit_runs/lead-scratch/fluid_moment_2d.inp`（XZ、periodic、单 box、`fluids.moment.enabled=1`）
→ 正常跑到 `AMReX finalized`。反证：把 `YeeDeposition.H` 的 `rho_np1` 乘 1.5 重编 → 同一 deck 触发
`Assertion resid <= 1e-12*resid_scale failed (MomentFluidSolver.cpp:149)`；之后已 `git diff` 确认该文件干净。

## 第三轮：PicEm_A F5/F6 收尾（提交 aa92167），窗口 18 条全部结案

F6 (PicEm_A) fixed by aa92167：`rho_medium` 的更新盒不再对每个 FAB 两端各缩 1（那样在每条内部
FAB 交界上留一行既不被 seed 也不被更新的"洞"）。新规则 `rho_medium_shrink_box()`：
内部 FAB 的低端节点交给前一个 FAB（前者的高端保留），只在非周期域的端点收缩——每条 nodal 线
恰好被一个 FAB 写。seed 时加了一次**并集覆盖自检**（BoxArray + 1 分量 mask，`covered.min(0)==1`），
它在该规则被改坏时确实会失败。
过程中发现并一起修掉的两个真实问题（都只有在新规则下才暴露）：
  1. 相邻 nodal FAB 共享节点被两个 FAB 各写一次，后写者读到未填的 ghost → 实测 `rho_medium`
     一步后变成 `1e193`（旧写法因为"洞"而不写该节点，反而看不出来）；
  2. `Jcond`（`MacroscopicProperties::m_jcond`）只有 0 层 ghost，周期端点 `jx(i-1,j)` 越界 →
     现在分配 1 层 ghost，`FillBoundary(periodicity)` 才有实际作用。
验证：`warpx.2d Examples/Tests/macroscopic_solver/inputs_test_2d_macroscopic_solver warpx.verbose=1`
（PEC、2×2=4 个 FAB）→ `[GAUSS] step 100 max|divD-(rho_p+rho_m)| = 2.0e-16`，`max|rho_m| = 0`。
反证：把规则换回 `bx.grow(-1)` 重编 → 同一 deck 报
`Assertion covered.min(0) == amrex::Real(1.0) failed ... (a one-node hole is left at an internal FAB boundary)`。
回归：`ctest -R "macroscopic"` → 12/12 通过（含 1D/2D/3D EB 的 checksum），说明 ghost 层与盒规则
不影响任何被检查的输出。

F5 (PicEm_A) fixed by 93afb3b + aa92167：`eps2` 未使用已删、`Ey2->FillBoundary` 已补（93afb3b）；
同一 `[GAUSS]` 块里 **3D 分支此前仍用 2D 散度**（`(Dx_i-Dx_{i-1})/dx + (Dz_j-Dz_{j-1})/dz`，与已修好的
3D seed 不一致），aa92167 补上 `Dy3` 与 k 方向的 z 差分。
验证：`warpx.3d .../inputs_test_3d_embedded_boundary_cube_macroscopic warpx.verbose=1` 跑通并打印
`[GAUSS]`；`ctest -R "macroscopic"` 12/12。
**反证未完成（如实记录）**：现有 3D 宏观 deck 在最坏节点的 y 项贡献为 0（把残差换回 2D 形式，
打印数值逐位相同），所以"运行时用残差区分 2D/3D 形式"这条路走不通；这次改动目前只有
"与 seed 同构 + 编译/回归通过"这一级证据。若后续要补，需要一个 D 沿 y 变化的 3D deck
（注意默认的 div-B/projection cleaner 会把外部 E 清掉，需要关掉或另设初值）。

## 给集成者的汇总（窗口 18 条）

fixed（8）：`PicEm_A F1`(93afb3b)、`PicEm_A F5`(93afb3b+aa92167)、`PicEm_A F6`(aa92167)、
`PicEm_A F7`(93afb3b)、`PicEm_A F8`(ee6be11)、`PicEm_A F10`(93afb3b)、
`ParticleCore_B F7`(ee6be11)、`ParticleCore_B F9`(ee6be11)。
invalid（10）：`ParticleCore_A F5`；`ParticleCore_B F1,F2,F5,F6,F8,F10`；`PicEm_A F2,F9,F12`。
建议集成者写进 REPORT 的一句话：尾部窗口的 10 条 invalid 全部是"报告当时以 26.10 为基准/或代码
已被 deefbf6、5d71c45 覆盖"，不是回避；8 条 fixed 里 F6 与 F8 各带出一个上游/本 fork 的隐藏缺陷
（nodal FAB 共享节点双写、dt→0 时残差参考量退化），都已在本窗口内一并修掉或记录。

## 第四轮认领：Collision_A F2/F3/F4 + Collision_B F2/F3/F5/F6/F7 @lead-root 2026-11-15T19:36

## 第五轮：Collision_A F2/F3/F4（fc98f8d）与 Collision_B F5（220d9a4）

F2/F3/F4 fixed by fc98f8d（都是重构期残留，纯清理）：
- F2：`DSMCFunc.H`、`SplitAndScatterFunc.H`、`BremsstrahlungFunc.H` 里三个"接受
  `MultiParticleContainer const*`"的构造函数只有声明没有定义（`.cpp` 已改成
  `CollisionSpeciesTable` 版本），删除并把 doxygen 参数名改对。
  验证：`grep -rn "MultiParticleContainer const \*" Source/Plugins/Collision` 剩余两条都是
  有定义、带 `/*mypc*/` 形参的委托构造；链接 + 77 个 collision 用例全绿。
- F3：`SplitAndScatterFunc.H` / `PhotonCreationFunc.H` 第 1 行（版权头与 include guard 之前）的
  `DefaultInitialization.H` 移入正常 include 块，顺带删掉重复的 `IParticleIterator.H` /
  `Runtime/IParticleContainer.H` 和 `.cpp` 里连续空行。
- F4：`BinaryCollisionUtils` 的两个 `IParticleContainerAccess` 转发重载（树内无调用者，每次还会
  构造一份 `CollisionSpeciesTable` 快照再丢弃）删除。
验证：`cmake --build --target app_{1d,2d,3d,rz}` 全通过；`ctest -R "collision|dsmc|bremsstrahlung"`
→ **77/77 通过**（含 129 s 的 `test_3d_beam_beam_collision`）；九门 `run_gates.sh` 全 OK。

F5 fixed by 220d9a4：`NuclearFusionFunc::executor()` 在 `save_particle_production` 分支解引用
`m_data` 前加 `WARPX_ALWAYS_ASSERT_WITH_MESSAGE(m_data != nullptr, ...)`——`m_data` 只在可选钩子
`AllocData()` 里赋值，任何没走 `CollisionHandler::AllocData` 的路径都会空指针。
**报告里"`fields().get()` 也要判空"不成立**：`MultiFabRegister::internal_get` 在名字不存在时
`throw std::runtime_error`（`Source/Core/ablastr/fields/MultiFabRegister.cpp:424-447`），
指针不可能为 nullptr，加判空就是死代码。
验证：`ctest -R "test_3d_deuterium_tritium_fusion\."`（唯一使用 `save_particle_production` 的 deck）
→ 3/3 通过。反证：临时在 `AllocData()` 末尾把 `m_data` 置空重编 → 同一 deck 报
`NuclearFusionFunc: save_particle_production needs AllocData() to have been called before executor()
(no particle-production field is registered).`；之后已 `git diff` 确认该文件无残留。

剩余（仍是 wip:lead-root，下一轮）：Collision_B F2（`pulsed_decay` 采样器语义）、F3（`CollisionSpeciesTable`
的 `SpeciesHandle` 死代码 + 构造时全局归约）、F6（`save_particle_production` 两套解析机制）、
F7（`declare_state` 只在 kernel 路径生效）。

## 第六轮：Collision_B F2（invalid）、F3/F6（104a379）

F2 **invalid**：报告的三个"本 fork 改动"（每宏粒子 Bernoulli、整片删除母粒子、`w=w_parent/weight_ratio`）
与 26.11 **逐字相同**——`_upstream/warpX-26.11/Source/Particles/Collision/PulsedDecay/PulsedDecay.cpp:202-288`
就是同一段代码，`PulsedDecay.H:22-31` 的 docstring 也逐字一致（含 `1-exp(-nu*t)` 的期望式写法）。
所以"方差结构与上游不同""注释未更新"都不是 fork 引入的；`weight_ratio` 的自减链也是上游设计。

F3 fixed by 104a379：删掉 `CollisionSpeciesTable` 的 `SpeciesHandle` 机制——
`m_handles` / `SpeciesSpec` 填充、`CollisionSpeciesView::handle()`、`names()`、
`CollisionHandler::speciesTable()` 在全树无调用者（`grep -rn "\.handle()\|names()\|speciesTable"` 只命中定义处），
而构造时那次 `pc.TotalNumberOfParticles(false)` 是发生在 InitData 之前、结果恒为 0 且永不被读的 MPI 归约。
验证：四维度重编 + `ctest -R "collision|dsmc|bremsstrahlung|fusion"` → **104/104 通过**。

F6 fixed by 104a379：`declare_state` 改用与 `NuclearFusionFunc` 相同的
`utils::parser::queryWithParser(pp, "save_particle_production", bool)`，并且只对
`collision_types[i] == "nuclearfusion"` 的条目处理该键（别的 collision 不拥有它）。
**反证的局限（如实记录）**：`1.0` 在旧实现里被 int query 当 0 ⇒ schema 不声明该场，但
`AllocData()` 的迟到分配把它兜住了，运行不报错；所以没有可失败的运行时断言可做，
只能做到"两处解析一致"这一级证据。附带确认：`true` 在**两套**实现里都报
`ParmParse: failed to parse true due to unknown symbol true`（parser 不认字面量 true），
所以报告举的 `true` 例子不是差异来源，真正的差异是 `1.0` / 解析式写法。

F7 仍 wip：legacy/Python 路径的 collision handler 是 `RunCollisionStage` 首步懒创建，schema 阶段
不存在 ⇒ `declare_state` 在这条路径上是死代码。修复需要把 handler 的创建前移到 `InitData`
（`AllocPluginFields` 从 `ctx->state->requested_fields()` 取声明，见 `WarpX.cpp:2770-2790`），
属跨模块时序改动，下一轮单独做并配 legacy 路径的 deck 验证。

## 第七轮：Core_Misc F2 复核为 **invalid**（与 26.11 逐字相同）

F2 说"split-gather 路径把 B 的 Galerkin 降阶丢了、与 fused 路径语义不一致"。按 26.11 基准核对：

- 26.11 的 `_upstream/warpX-26.11/Source/Particles/PhysicalParticleContainer.cpp:1356-1364` 就是
  `doDirectGatherVectorField(orders.e_shape, orders.e_galerkin, ...)` + 
  `doDirectGatherVectorField(orders.b_shape, false, ...)`——**B 传 false 与 fork 逐字相同**；
- 26.11 的 `FieldGatherOrders`（`26.11/Source/Particles/WarpXParticleContainer.H:180-185`）同样只有
  `e_shape / e_galerkin / b_shape`，也**没有** `b_galerkin` 字段；
- 26.11 的 `SemiImplicitDarwin.cpp:193` 构造参数也与 fork 的 `SemiImplicitDarwin.cpp:191` 逐字相同
  （`e_galerkin = m_predictor_use_galerkin`，`b_shape = m_magnetostatic_particle_shape`）。

所以"fork 自己的回归"不成立：这是 26.10→26.11 上游自己引入的 split-gather API 的语义。
另外复核了内核本身：`FieldGather.H:500-560` 的 `sx_by/sx_bz/sz_bx/sz_by` 是按
"该分量在该方向是否 NODE"选 galerkin 数组的（`galerkin_interpolation=1` 时正确），
并不存在报告担心的"槽位错配"——真正的问题只是调用点传 false，而那正是上游的行为。
若要"比上游更好"，需要两边一起改（否则 400 例更不可能一致），故本条按基线判 invalid，
并把"B-only split gather 未降阶"作为上游已知限制记在此处，不改代码。
