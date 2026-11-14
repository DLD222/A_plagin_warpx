# Source_Plugins_PicEm_A 审查

分区：`Source/Plugins/PicEm/Modules/{BoundaryConditions,EmbeddedBoundary,Evolve,FieldSolver,Fluids,HybridPIC,Initialization,MomentsSolver}`（52 个文件，24 272 行）。
参考：`_upstream/warpX-26.10/Source/`（按文件名对照，逐文件 `diff -u`）。只读审计，未改动源码。

## 覆盖

类型：`moved` = 上游搬移 + 纯改名（`WarpX::`→`data()/config()`、include 路径、`ISpectralSolver` 虚接口）；`modified` = 有语义变化；`new` = 上游无对应实现。

| 文件 | 行数 | 类型 | 结论 |
|---|---|---|---|
| BoundaryConditions/PML.H | 183 | moved | OK（`MultiSigmaBox` 外移、EB flag 拆 fp/cp） |
| BoundaryConditions/PML.cpp | 1513 | moved+modified | OK（新增 coarse-patch EB flag；`PML::data()` 静态回退） |
| BoundaryConditions/PML_RZ.H | 62 | moved | OK |
| BoundaryConditions/PML_RZ.cpp | 211 | moved | OK |
| BoundaryConditions/PML_fwd.H | 21 | moved | OK |
| BoundaryConditions/WarpXEvolvePML.cpp | 365 | modified | OK（`DampJPML` 改用当前 patch 的 EB flag，是修正） |
| BoundaryConditions/WarpXFieldBoundaries.cpp | 418 | modified | OK（`ApplyRhofieldBoundary` 加 scomp/ncomp + alias 视图） |
| BoundaryConditions/WarpX_PEC.cpp | 1195 | modified | OK（新增 `is_rho_field`：rho 在反演体积缩放**之前**做镜面，故去掉 dV 比修正，推导自洽） |
| EmbeddedBoundary/EmbeddedBoundaryInit.H | 171 | moved+new API | OK |
| EmbeddedBoundary/EmbeddedBoundaryInit.cpp | 1021 | modified+new | **缺陷 F2/F3**（`ComputeDualGeometry`） |
| EmbeddedBoundary/Enabled.cpp | 39 | moved | OK |
| EmbeddedBoundary/WarpXFaceExtensions.cpp | 1070 | modified | OK（XZ 三方向面分类；`GetNeigh`/`SetNeigh` 降维一致） |
| Evolve/WarpXComputeDt.cpp | 344 | modified | OK（无 subcycling 时各级 dt 相同，属修正） |
| Evolve/WarpXEvolve.cpp | 1975 | modified | **缺陷 F1**；其余（阶段调度、FinalizeJ/Rho 拆分、sub1/sub2 的 “先 raw 后缩放” 设计）自洽 |
| FieldSolver/ImplicitKernelServicesBuilder.H | 25 | new（bridge） | OK |
| FieldSolver/ImplicitKernelServicesBuilder.cpp | 172 | new（bridge） | OK（全部回调转发到 WarpX/particleSystem） |
| FieldSolver/WarpXImplicitOps.cpp | 295 | modified | OK（E_old/B_old 拆分、`dt_scale`） |
| FieldSolver/WarpXPushFieldsEM.cpp | 2166 | modified+new | **缺陷 F4/F5/F6**（macroscopic medium 新代码） |
| FieldSolver/WarpXPushFieldsHybridPIC.cpp | 607 | modified | 基本 OK（见“不确定”U3） |
| FieldSolver/WarpXSolveFieldsES.cpp | 61 | modified | OK（新增 `particleContext()`） |
| FieldSolver/WarpX_ImplicitParticles.cpp | 41 | new（bridge） | OK |
| FieldSolver/WarpX_Magnetostatic.cpp | 139 | rewritten | OK（`current_fp` 清零在插件侧做，已核对 MagnetostaticSolverImpl.cpp:68-78） |
| Fluids/MomentFluidSolver.H | 79 | new | 见 F7 |
| Fluids/MomentFluidSolver.cpp | 186 | new | 见 F7/F8 |
| Fluids/MultiFluidContainer.H/.cpp | 95/75 | moved | OK |
| Fluids/QdsmcParticleContainer.H | 186 | modified | 文档 cell→node，与 .cpp 一致 |
| Fluids/QdsmcParticleContainer.cpp | 665 | modified | OK（cell→node 改写 + 节点控制体；tile 上表面 grow 只在畴上边界命中，未见重复） |
| Fluids/WarpXFluidContainer.H | 208 | modified | OK（DepositCharge 约定改为 “未缩放”，文档同步） |
| Fluids/WarpXFluidContainer.cpp | 1622 | modified | OK（`RadialVolumeFactor` 与 `ApplyInverseVolumeScalingToChargeDensity` 逐项对应） |
| HybridPIC/HybridPICModel/ExternalVectorPotential.H/.cpp | 104/379 | moved | OK |
| HybridPIC/HybridPICModel/HybridPICModel.H | 710 | modified | OK（IHybridPICModel 实现、ctx 注入） |
| HybridPIC/HybridPICModel/HybridPICModel.cpp | 3015 | modified | 基本 OK；`QDSMCFillElectronPressureFromTe` 内部补了边界+FillBoundary，故 PushFieldsHybridPIC 里删除的调用不丢 |
| Initialization/DivCleaner/ProjectionDivCleaner.H/.cpp | 142/487 | moved | OK（`const_cast` 取 fields，可读但有效） |
| Initialization/WarpXAMReXInit.cpp | 271 | moved | OK |
| Initialization/WarpXInit.H | 63 | modified | OK |
| Initialization/WarpXInit.cpp | 225 | modified | OK（PETSc 先查 `PetscInitialized`，尾调用对称） |
| Initialization/WarpXInitData.cpp | 2117 | modified | **缺陷 F9 / F10** |
| MomentsSolver/Coupling/FluidFriction.H | 187 | new | OK（Braginskii ν 与标准式一致；动量/能量交换自洽） |
| MomentsSolver/Coupling/MomentCoupling.H | 187 | new | **缺陷 F11** |
| MomentsSolver/Coupling/TenMomentClosure.H | 67 | new | OK（k0 指数松弛与注释公式一致） |
| MomentsSolver/Coupling/YeeDeposition.H | 114 | new | 设计自洽（RK 权重 1/6,1/6,2/3 与 RK3 update 精确对应，已验算），残差返回值见 F8 |
| MomentsSolver/Equations/EquationOfState.H | 140 | new | OK |
| MomentsSolver/Equations/Euler5Eqn.H | 123 | new | OK（通量/波速为标准式） |
| MomentsSolver/Equations/MaxwellEqn.H | 89 | new | **缺陷 F12**（注释与实现不符） |
| MomentsSolver/Equations/MomentEquation.H | 57 | new | OK |
| MomentsSolver/Equations/MomentsConfig.H | 26 | new | OK |
| MomentsSolver/Equations/TenMomentEqn.H | 126 | new | OK（`p_index` 打包、E_ij 通量 `u_d E_ij + u_i P_jd + u_j P_id` 已验算） |
| MomentsSolver/FV/MomentFVDriver.H | 120 | new | OK（MC 限制器、LLF、SSP-RK3） |
| MomentsSolver/FV/MomentFVNDDriver.H | 313 | new | OK（轴循环/线循环索引、RK 加权通量已验算） |

## 缺陷清单

### F1 AMR 时间步中的 regrid 路径是新写的，缺两步收尾
- 文件:行 Source/Plugins/PicEm/Modules/Evolve/WarpXEvolve.cpp:404-422
- 严重度: major
- 置信度: likely
- 现象与依据: 参考上游 `WarpXEvolve.cpp` 在 `Evolve()` 循环里**没有**任何 regrid 逻辑（`grep -n "regrid" _upstream/.../Evolve/WarpXEvolve.cpp` 无匹配；`regrid_int` 只在 `WarpX.cpp:697` 被解析），所以这段是本地新写的、没有上游实现可对照。最接近的官方 regrid 收尾是 `WarpX::LoadBalance()`（`_upstream/.../Parallelization/WarpXRegrid.cpp:161-172`）：
  `mypc->Redistribute(); mypc->defineAllParticleTiles(); m_particle_boundary_buffer->redistribute(); reduced_diags->LoadBalance();`
  本块做了前两项（外加 `multi_diags->RemakeLevel/ResizeLevels`，与 `RemakeLevel` 只调 `InitializeFieldFunctors` 的事实互补），但**没有** `m_particle_boundary_buffer->redistribute()` 与 `reduced_diags->LoadBalance()`。粒子边界缓冲是 `LayoutData` 按 level 的 BoxArray 分配的，regrid 后不重建就继续用旧 box：被刮削粒子的容器内容/写出会错位（buffer 采集发生在 `HandleParticlesAtBoundaries`）。
  另外它多调了一次 `particleSystem().AllocData()`（`WarpXParticleContainer::AllocData()` = `reserveData(); resizeData();`，`Particles/WarpXParticleContainer.cpp`），而官方“remake 之后”的路径从不重调 `AllocData`——`resizeData()` 会按新的 ParticleBoxArray 重建 per-tile SoA，重复调用有清空/重排粒子数据的风险。
- 建议修法: 在 `particleSystem().defineAllParticleTiles()` 之后补 `m_particle_boundary_buffer->redistribute(); reduced_diags->LoadBalance();`；去掉 `particleSystem().AllocData()`（若确需重建 per-tile 容量，改调 `resizeData()` 前先确认 amrex 语义，或直接依赖 `Redistribute()`）。
- 验证方法: `ctest -R test_2d_...` 之外，跑一个 `amr.max_level=1 warpx.regrid_int=2` 且带 `particle_boundary_*` 与 reduced diag 的输入；对比 `regrid_int=-1` 的结果，并检查 regrid 步之后 `m_particle_boundary_buffer` 的粒子计数是否守恒（或在 `redistribute()` 前加断言）。

### F2 `ComputeDualGeometry` 只在 XZ 下真正实现，却在所有维度被调用/断言
- 文件:行 Source/Plugins/PicEm/Modules/EmbeddedBoundary/EmbeddedBoundaryInit.cpp:882-1018（函数体 903-1016 全在 `#if defined(WARPX_DIM_XZ)` 内；1018 行的 `amrex::Print` 在 `#endif` 之外）
- 严重度: major
- 置信度: likely
- 现象与依据: 函数签名/调用点（`Initialization/WarpXInitData.cpp` 的 `InitializeEBGridData` 里 `if (WarpX::ect_conformal_E > 0) ComputeDualGeometry(...)`）在所有维度都可达，3D/1D 构建里该函数体被预处理器整段删掉，只剩 `BL_PROFILE` + 断言 + 两次 `amrex::Print()`（892、1018 行，含 `dual_A_y.min(0)` 等）。而 `dual_A_y/dual_L_x/dual_L_z` 在 3D/1D 下从未被填充（`InitializeEBGridData` 的注释自述 “The three MultiFabs are initialised to 1.0”），ECT conformal 内核却要**除以/乘以**它们 → 3D 下打开 `algo.ect_conformal_E` 会以 A_y=1、Lx=Lz=1 静默算错，没有任何断言阻止（`InitializeEBGridData` 只断言了 `maxLevel()==0`，没断言维度）。对比 `MarkUpdateECellsECT` 里 dual_A_y 分支只在 `WARPX_DIM_XZ` 生效，说明作者知道这是 2D 特性，但入口没挡住。
- 建议修法: 在 `ComputeDualGeometry` 里对非 XZ 维度 `WARPX_ABORT_WITH_MESSAGE("algo.ect_conformal_E is XZ only")`（或把调用点包在 `#if defined(WARPX_DIM_XZ)`），并删除两处 `[DIAG]` 打印。
- 验证方法: `cmake -DWarpX_DIMS=3` 构建后以 `algo.ect_conformal_E=1`（+`algo.ect=1`、EB 几何）启动，观察 stdout 的 `[DIAG] dual geom: A_y=1..1` 且场解无意义；或直接 `grep -n "WARPX_DIM_XZ" EmbeddedBoundaryInit.cpp` 确认函数体范围。

### F3 `ComputeDualGeometry` 留下无条件调试打印与自相矛盾的断言文案
- 文件:行 Source/Plugins/PicEm/Modules/EmbeddedBoundary/EmbeddedBoundaryInit.cpp:892、1018、896-899
- 严重度: minor
- 置信度: confirmed
- 现象与依据: `amrex::Print() << "[DIAG] ComputeDualGeometry called..."` 与逐次调用的 min/max 打印没有任何 verbose 门控（同文件其它诊断都用 `WarpX::verbose > 0`）；断言文本写 “only scheme 1/2/4 are implemented”，而断言条件与后续分支实际接受 1、2、4、6、7、8、9（scheme 6/7/8 在 1010-1040 行有实现，scheme 9 归入 2 的分支）。
- 建议修法: 打印改到 `WarpX::verbose > 2`；断言文本与实际支持集合对齐。
- 验证方法: 直接阅读；或运行任一 `algo.ect_conformal_E>0` 用例看 stdout。

### F4 3D 张量介质中 `rho_medium` 种子的 Dy 用了错的 eps 分量
- 文件:行 Source/Plugins/PicEm/Modules/FieldSolver/WarpXPushFieldsEM.cpp:1315-1318（`Dy3` lambda），使用处 1329
- 严重度: minor（`rho_medium_fp` 目前只被本文件的 `[GAUSS]` 诊断消费；但种子错 ⇒ 诊断值无意义）
- 置信度: confirmed
- 现象与依据: eps 张量分量顺序在 `FieldSolver/FiniteDifference/MacroscopicProperties/MacroscopicProperties.cpp:684-685` 明确为 `(xx,yy,zz,xy,xz,yz)`（`en(...,0..5)`）。`D = eps·E` 的三个分量应为
  `Dx = xx·Ex + xy·Ey + xz·Ez`（代码 1306-1310：comps 0,3,4 ✓）
  `Dy = xy·Ex + yy·Ey + yz·Ez`（代码 1316-1318 用了 comps 3,**4**,5 ✗，中间项把 xz 当成 yy）
  `Dz = xz·Ex + yz·Ey + zz·Ez`（代码 1311-1315：comps 4,5,2 ✓）
  正确写法是 `en1(...,3)*ex + en1(...,1)*ey + en1(...,5)*ez`。因为 `Dy` 在非对角张量（`epsilon_tensor_*` 给了 xy/xz）下参与 `rh(i,j,k)` 的 `(Dy3(i,j)-Dy3(i,j-1))/dy` 项，3D 各向异性介质的种子电荷 `rho_medium(0)=div_h D` 直接算错。
- 建议修法: 把 1317 行的 `en1(ii,jj,k,4)` 改成 `en1(ii,jj,k,1)`。
- 验证方法: 3D + `macroscopic.epsilon_tensor_*=...`（含非零 xy/xz）+ `macroscopic_subpixel=1`，对比 seed 后的 `max|divD - rho_m|`（应≈0，现有代码不为 0）。

### F5 3D 各向异性下另一处同类风险（`eps2` 变量未使用，`Ex2/Ez2` 的 FillBoundary 缺 `Ey2`）
- 文件:行 Source/Plugins/PicEm/Modules/FieldSolver/WarpXPushFieldsEM.cpp:1421-1460
- 严重度: minor
- 置信度: needs-verification
- 现象与依据: `[GAUSS]` 残差复算里 `Dx/Dz` 与种子同式（因此沿用了同一套分量，不含 Dy 问题），但 `auto& eps2 = m_macroscopic_properties->getepsilon_mf();` 声明后未使用；`Ex2->FillBoundary`/`Ez2->FillBoundary` 之后 `Ey2` 的 ghost 未更新，而 `Dx`/`Dz` 都读 `ey`，所以在多 box 时残差会在 box 边界读到旧的 `Ey^{n}` ghost。
- 建议修法: 补 `Ey2->FillBoundary(...)`；删掉 `eps2`。
- 验证方法: 3D 各向异性 + `max_grid_size` 小到产生多 box，比较 `[GAUSS]` 残差与单 box 结果。

### F6 `bnd_shrink` 用域周期判断却作用于每块 tilebox ⇒ CPU 分块时 rho_medium 少更新一层
- 文件:行 Source/Plugins/PicEm/Modules/FieldSolver/WarpXPushFieldsEM.cpp:1271-1289、1383-1393、1439-1453
- 严重度: minor
- 置信度: likely
- 现象与依据: 代码意图（注释）是排除**畴边界**节点：`for d: if (!Geom(lev).periodicity().isPeriodic(d)) bnd_shrink[d]=1;` 然后 `mfi.tilebox(rho->ixType().toIntVect()).grow(-bnd_shrink)`。`MFIter(*rho, TilingIfNotGPU())` 在 CPU 上会开 tiling，`tilebox` 是 tile 而非 validbox，对每个 tile 的四条非周期边都收缩 1 格 ⇒ tile 与 tile 之间（以及多 grid 之间）出现一层“洞”，那层的 `rho_medium` 既不被 seed 也不被 `+= -dt div<J_c>` 更新。GPU（无 tiling）下才会退化为“仅畴边界”。
- 建议修法: 用 `mfi.validbox()`，并且只对与 `Geom(lev).Domain()` 重合的边收缩：
  `Box bx = mfi.validbox(); for d: if (bx.smallEnd(d)==dom.smallEnd(d) && !periodic) bx.growLo(d,-1); ...`
- 验证方法: 单 rank、`amrex.max_grid_size=16` 与 `=1024` 两种设置跑同一 3D/2D macroscopic 用例，`[GAUSS]` 打印的 `max|divD-rho_m-rho_p|` 在分块时应出现 O(1) 跳变（或直接把 `rho_medium` 写 plotfile 看棋盘格缺口）。

### F7 `fluids.moment` 原型只断言 `AMREX_SPACEDIM==2`，RZ 构建同样满足 ⇒ 静默按直角算
- 文件:行 Source/Plugins/PicEm/Modules/Fluids/MomentFluidSolver.cpp:52-55（断言），调用点 Evolve/WarpXEvolve.cpp:854-866
- 严重度: minor
- 置信度: confirmed
- 现象与依据: `WARPX_DIM_RZ` 下 `AMREX_SPACEDIM==2`，`AMREX_ALWAYS_ASSERT(AMREX_SPACEDIM == 2)` 照样通过，于是 `FVND<Euler5,2>` 以 x 当直角坐标跑，`dep.Jlo` 以 `scale=q/m` 直接 `+=` 到 `current_fp`。而该调用发生在 `OneStep_FinalizeDeposition()` 里 `FinalizeJ()` **之后**（`WarpXEvolve.cpp:854` 在该函数尾部），RZ 下 `current_fp` 已做过 `ApplyInverseVolumeScalingToCurrentDensity`，流体电流没有对应的 `2πr` 权重 ⇒ 量纲错。断言文本自称 “only 2D XZ”，与实际检查项不符。
- 建议修法: `#if !defined(WARPX_DIM_XZ) #error/#abort`；并把 `m_eps0` 的默认值与 `PhysConst::epsilon_0` 对齐（`GaussResidual` 用它做诊断）。
- 验证方法: `cmake -DWarpX_DIMS=RZ` 后 `fluids.moment.enabled=1` 启动，观察它并不 abort。

### F8 电荷守恒残差返回值被丢弃
- 文件:行 Source/Plugins/PicEm/Modules/Fluids/MomentFluidSolver.cpp:127
- 严重度: minor
- 置信度: confirmed
- 现象与依据: `deposit_conservative` 的唯一输出价值是返回 `max|(rho^{n+1}-rho^n)/dt + div J|`（`Coupling/YeeDeposition.H:58-59、99-109`），调用处直接丢弃；头文件 `MomentFluidSolver.H:10-14` 宣称的离散恒等式因此**从未被检查**（在 RZ 误用时也不会报警）。
- 建议修法: 接住返回值，`if (resid > tol) WMRecordWarning(...)` 或 `AMREX_ALWAYS_ASSERT`。
- 验证方法: 打印该残差，正常配置下应为 ~1e-15。

### F9 `AddExternalFields` 的层循环由 `max_level` 改为 `finestLevel()`
- 文件:行 Source/Plugins/PicEm/Modules/Initialization/WarpXInitData.cpp:1021-1024
- 严重度: minor
- 置信度: needs-verification
- 现象与依据: 上游同一循环是 `for (int lev = 0; lev <= max_level; ++lev)`；`InitData` 期间 amrex 的 `finest_level` 仍为 0（各级字段是按 `max_level` 分配的），所以改完后初始外场只加在 level 0。同一函数里 `AddExternalFields(lev)` 内部也把 `get_mr_levels_alldirs(..., max_level)` 改成 `..., lev)`（这是对的），但循环上界这一改会让 `max_level>0` 运行中 lev>0 的初始外场缺失，除非后续 regrid 用 level 0 插值补上（`Evolve` 里只有静电分支才重调 `AddExternalFields`，见 `WarpXEvolve.cpp:340-344`）。
- 建议修法: 恢复 `max_level`（`AddExternalFields` 已按 `lev` 取字段，不会越界）。
- 验证方法: `amr.max_level=1` + `warpx.B_ext_grid_init_style=parse_ext_grid_function` 的输入，在 `InitData` 后 dump `Bfield_fp` 的 lev=1 看 `B_ext` 是否存在。

### F10 `WARPX_DISABLE_DIAGNOSTICS` 环境变量会静默替换已构造的诊断服务
- 文件:行 Source/Plugins/PicEm/Modules/Initialization/WarpXInitData.cpp:873-893
- 严重度: minor
- 置信度: confirmed
- 现象与依据: 先 `multi_diags = create_diagnostics_service(...)`，若环境变量存在则**先** `InitData(particleSystem())` 再 `multi_diags = make_unique<NullDiagnosticsService>()`；`reduced_diags` 则是先替换后不再 InitData。行为完全由环境变量决定、无输入参数、无 warning，且两条路径不对称（一个 Init 了再丢弃，一个直接丢弃）。上游无此机制。
- 建议修法: 改成显式输入参数 + `WMRecordWarning`，两处按同一顺序处理（或直接删）。
- 验证方法: `WARPX_DISABLE_DIAGNOSTICS=1` 跑任一带 `diagnostics.diags_names` 的输入，观察无输出且无任何提示。

### F11 流体–Maxwell 耦合源的 “内能不变” 能量更新多乘了一个 rho
- 文件:行 Source/Plugins/PicEm/Modules/MomentsSolver/Coupling/MomentCoupling.H:84-89（尤其 88）
- 严重度: major
- 置信度: confirmed
- 现象与依据: 约定 `U = [rho, rho·u, E_tot]`（同文件 41-42 行注释、以及 65 行用 `Jx += (q/m)*u[1]` 说明 `u[1]` 是**动量密度**）。动量更新 `u[1] = ux0 + dt*(qm*rho*Ex + qm*(uy0*Bz - uz0*By))` 与 `∂(rho u)/∂t = (q/m)rho(E+u×B)` 一致 ✓。但动能功应为
  `ΔKE = u·Δm = dt (q/m) rho (u·E) = dt (q/m) (m_vec·E) = dt J·E`，
  代码写的是 `u[4] += dt*qm*rho*(uxa*Ex + uya*Ey + uza*Ez)`，其中 `uxa = 0.5*(ux0+u[1])` 已是**动量密度**，再乘 `rho` 等于多了一个密度因子 ⇒ 焦耳加热被放大 `rho` 倍（SI 等离子体里 rho≪1 ⇒ 内能几乎不涨；rho>1 ⇒ 过加热），违背 45-49 行“update the total energy so the internal energy is conserved”的设计目的，总能量不守恒。
- 建议修法: `u[4] += dt * qm * (uxa*Ex + uya*Ey + uza*Ez);`（去掉 `rho`）。
- 验证方法: 单 cell 双物种静止等离子体、纯 E 场、无 B：解析 `ΔKE_density = dt·J·E`，比较代码输出；或直接检查 `u[4]` 增量与 `0.5*|m_new|^2/rho - 0.5*|m_old|^2/rho` 是否相等（应严格相等）。

### F12 `Maxwell6` 数值通量注释声称精确 Roe/upwind，实现是分量式 Rusanov
- 文件:行 Source/Plugins/PicEm/Modules/MomentsSolver/Equations/MaxwellEqn.H:66-83
- 严重度: minor
- 置信度: confirmed
- 现象与依据: 注释说 “exact upwind (Roe) flux … |A| is 0 on the longitudinal components and c on the transverse components”，但 79-83 行对 E、B 各自独立加 `0.5*c*(UR-UL)` 耗散。SI 下横向特征量是 `E ± cB`，独立的 `0.5c ΔE`、`0.5c ΔB` 并不是 `|A|`：对右行纯横向波（`ΔE = cΔB`）仍会引入 `0.5c(ΔE - cΔB)` 级耗散（不精确），对左行波则过度耗散。物理通量本身（`flux()`）已逐项验算正确（`∂E/∂t=c²curlB`、`∂B/∂t=-curlE`）。
- 建议修法: 要么按 `E±cB` 特征量实现真 Roe，要么把注释改成 “Rusanov/LLF with λ=c, longitudinal components undamped”。
- 验证方法: 一维周期真空横向电磁脉冲（`Maxwell6` + `FV1D`）测衰减率：真 upwind 无耗散，现实现随步数衰减。

## 不确定但值得复核的点

- **U1（rho 缩放搬迁是否处处成立）** `ApplyInverseVolumeScalingToChargeDensity` 现在只有 `WarpXEvolve.cpp:1257`（`FinalizeRhoOnLevel`）一个调用点，且 `MultiParticleContainer::DepositCharge`（`Particles/MultiParticleContainer.cpp:627-653`）确实不再缩放。我逐条核过显式路径（`OneStep_FinalizeDeposition`）、JRhom（每次 deposit 都 reset→SyncRho→FinalizeRho）、sub1/sub2（“先把 raw 做完 restrict/add/sum，再对结果缩放一次”，含 `rho_buf` 不缩放——在这套设计下**是自洽的**，我最初怀疑 rho_buf 漏缩放，核对 `AddRhoFromFineLevelAndSumBoundary` 的调用顺序后撤回）。但**其它调用 `DepositCharge` 的消费者**（诊断 JFunctor/rho functor、Python 接口、`WarpXFluidContainer` 的非 Evolve 调用、QDSMC 的 `SetK(rho)`）是否都落在缩放之后，属于 Particles/Diagnostics 分区，建议交叉核对：`grep -rn "DepositCharge(" Source/` 逐个确认下游是否经过 `FinalizeRho*`。
- **U2（PEC 的 `is_rho_field` 与缩放顺序）** `WarpXFieldBoundaries.cpp:257-283` 在缩放**之前**对 raw rho 应用反射并关掉 dV 比修正（`WarpX_PEC.cpp:398、418、490`）。我推导 raw 表示下 `raw(i)=±raw(mirror)`、缩放后得 `ρ(i)=±ρ(mirror)·r_mirror/r_i`，与上游对已缩放 rho 用 `rscale=rmirror/rvalid` 等价——但这是**隐式耦合**（谁先谁后必须与 `FinalizeRhoOnLevel` 的语句顺序保持一致），任何后续调整顺序都会静默变成双重修正。
- **U3（hybrid PIC 的 QDSMC 压力调用）** `WarpXPushFieldsHybridPIC.cpp` 在两处 `BfieldEvolve` 前新增 `QDSMCFillElectronPressureFromTe(rho_fp_temp)`，在 `CalculatePlasmaCurrent` 前又用 `rho_fp`。`QDSMCFillElectronPressureFromTe` 现在自带 `ApplyElectronPressureBoundary + FillBoundary`（`HybridPICModel.cpp` 末尾），所以原来 PushFieldsHybridPIC 里删掉的边界调用不丢；但用 `rho_fp_temp`（不是 `rho_fp`）填 Pe 时，`rho_fp_temp` 是否已做过 RZ 体积缩放/边界反射，需要和 `HybridPICSolveE` 的 `rho` 口径对照。
- **U4（`RunCollisionStage` 的接管语义）** `WarpXEvolve.cpp:700-780`：只要调度器在 `[BeforeCollisions, AfterCollisions)` 内有任意 handler（含**只读观察者**），core 的 `doCollisions` 就被跳过；`RunDiagnosticsStage` 同理（在 `Stage::Diagnostics` 注册观察者会让**所有**诊断输出静默消失）。`RunPushStage` 的注释专门讨论过这个陷阱（用 `has_handlers(Push, Push+1)` 把接管限于“注册在核心点上的 handler”），但 collisions/diagnostics 用的是同一函数，语义上等于“观察者即接管”。建议确认这是有意的插件契约。
- **U5（`particleSystem().ContinuousFluxInjection` 被移出 `HandleParticlesAtBoundaries`）** 上游在 `HandleParticlesAtBoundaries` 内部调用它；本地移到 `Evolve` 循环（`WarpXEvolve.cpp:293`）后，凡绕过该循环而直接调用 `HandleParticlesAtBoundaries` 的路径都会漏掉注入——`ImplicitKernelServicesBuilder.cpp:107-109` 恰好把 `handle_particles_at_boundaries` 暴露给隐式（Darwin）求解器。上游 26.10 的隐式路径不调它，需确认新插件是否依赖注入。
- **U6（regrid 时机与 `t_new`) ** 新增 regrid 在一步末尾、`cur_time` 已是新时刻；`AmrCore::regrid(lbase, time)` 之后没有更新 `t_new/t_old`、没有重算 `dt`、也没有重算 EB 的 `m_eb_update_*`（`InitializeEBGridData` 只在 `MakeNewLevelFromScratch`/初始化里调）。若某次 regrid 改变了 level 数，`m_eb_update_E_fp` 等按 lev 的数组是否被补齐需验证。
- **U7（`OneStep_sub2` 的 `ApplyFilterandSumBoundaryRho(..., 0, 2*ncomps)`）** 上游此处是 `0, ncomps`，本地改成 `2*ncomps`（与 sub1 一致）。随后 `AddRhoFromFineLevelandSumBoundary(..., ncomps, ncomps)` 会再对 `[ncomps,2ncomps)` 调一次 `ApplyFilterandSumBoundaryRho`；对多 box 情形 `SumBoundary` 并非幂等（会把已更新的 ghost 再加一遍）。上游在 sub1 有同样的“重叠调用”，所以这可能是既有行为，但值得用多 box 用例确认 rho 的守恒性。
- **U8（`ComputePMLFactors` 只在 `InitData` 调）** regrid 会经 `CreatePMLForLevel` 重建 PML（`WarpXRegrid.cpp:380-382`），但新 PML 的 `sigma` 因子是否在重建后重新 `ComputePMLFactors(dt)` 需要确认（`WarpXInitData.cpp:1300-1308` 只在初始化时循环调用）。
- **U9（QDSMC cell→node 改写，已核对未列为缺陷）** 我最初怀疑 `QdsmcParticleContainer.cpp:158-163` 的 `tile_box.growHi(d,1)` 会在内部 tile/grid 交界处重复生成 marker；核对手写索引后撤回：只有当 `tile_box.bigEnd(d) == domain.bigEnd(d)`（即畴上边界）时才扩，内部交界由下一块的下节点唯一拥有，网格数恰为 `Π(n_i + 非周期?1:0)`。QDSMC 的剩余风险只在物理口径（`qdsmc_node_volume` 对角/面节点的控制体近似、RZ 轴节点用 dr/4 的环带质心），属于设计近似而非实现错误。
