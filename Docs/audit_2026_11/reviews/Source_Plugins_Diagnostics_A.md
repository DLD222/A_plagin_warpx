# Source_Plugins_Diagnostics_A 审查

审查对象：`Source/Plugins/Diagnostics/`（诊断插件前半：FlushFormats / ComputeDiagFunctors / Diagnostics 基类 /
BTDiagnostics / BoundaryScraping）。上游参照：`_upstream/warpX-26.10/Source/Diagnostics/`。
方法：逐文件 `diff -u`（映射 `Source/Diagnostics/X` ↔ `Source/Plugins/Diagnostics/X`），对差异逐段判断语义；
新增文件全文阅读；对每个 `data()/host()/config()/config_snapshot()` 消费点回溯 adapter 实现
（`Source/Plugins/PicEm/RuntimeBridge/Runtime/*`、`Source/Core/Runtime/*`）。

## 覆盖

| 文件 | 行数 | 类型 | 结论 |
|---|---|---|---|
| BTD_Plotfile_Header_Impl.cpp | 538 | moved | OK（新增 `m_int_comp_names.resize()` 实为修复上游漏读，见待复核 4） |
| BTDiagnostics.H | 425 | modified | OK |
| BTDiagnostics.cpp | 1543 | modified | 缺陷 F4 |
| BoundaryScrapingDiagnostics.H | 73 | moved | OK |
| BoundaryScrapingDiagnostics.cpp | 165 | modified | OK（`void*`→`Base*` static_cast 类型正确） |
| ComputeDiagFunctors/BackTransformFunctor.cpp | 327 | modified | OK（`config().ncomps()` 改用 live 值，正确） |
| ComputeDiagFunctors/BackTransformParticleFunctor.H | 252 | moved | OK |
| ComputeDiagFunctors/BackTransformParticleFunctor.cpp | 186 | modified | 缺陷 F5 |
| ComputeDiagFunctors/CellCenterFunctor.cpp | 30 | moved | OK |
| ComputeDiagFunctors/ComputeDiagFunctor.H | 136 | modified | 缺陷 F4（fallback 静默默认值） |
| ComputeDiagFunctors/ComputeDiagFunctor.cpp | 81 | new | 缺陷 F4 |
| ComputeDiagFunctors/ComputeParticleDiagFunctor.H | 90 | modified | OK |
| ComputeDiagFunctors/ComputeParticleDiagFunctor.cpp | 31 | new | OK |
| ComputeDiagFunctors/DarwinEfieldFunctor.cpp | 57 | moved | OK |
| ComputeDiagFunctors/DivBFunctor.cpp | 67 | modified | 缺陷 F2 |
| ComputeDiagFunctors/DivEFunctor.cpp | 83 | modified | 缺陷 F2 |
| ComputeDiagFunctors/EBCoveredFunctor.cpp | 47 | moved | OK |
| ComputeDiagFunctors/JFunctor.cpp | 57 | modified | 缺陷 F6 |
| ComputeDiagFunctors/JdispFunctor.cpp | 73 | modified | 可疑（待复核 3） |
| ComputeDiagFunctors/PartPerCellFunctor.cpp | 53 | moved | OK |
| ComputeDiagFunctors/PartPerGridFunctor.cpp | 56 | moved | OK |
| ComputeDiagFunctors/ParticleReductionFunctor.cpp | 134 | moved | OK |
| ComputeDiagFunctors/PhiFunctor.cpp | 39 | modified | OK（`computePhi` adapter 与上游两分支等价） |
| ComputeDiagFunctors/ProcessNumberFunctor.cpp | 50 | moved | OK |
| ComputeDiagFunctors/RhoFunctor.cpp | 52 | modified | 缺陷 F1（+ 见 PicEm_B F8） |
| ComputeDiagFunctors/TemperatureFunctor.cpp | 46 | moved | OK |
| Diagnostics.H | 376 | modified | OK |
| Diagnostics.cpp | 743 | modified | 缺陷 F7（+ F2 的根因） |
| DiagnosticsDependencies.H | 13 | new | OK |
| DiagnosticsServiceFactory.cpp | 38 | new | OK（静态注册 + linker anchor） |
| FlushFormats/FlushFormat.H | 55 | modified | OK |
| FlushFormats/FlushFormat.cpp | 56 | new | OK（缺 ctx 时抛异常，实测不会触发：见正文） |
| FlushFormats/FlushFormatAscent.cpp | 76 | moved | OK |
| FlushFormats/FlushFormatCatalyst.cpp | 212 | moved | OK |
| FlushFormats/FlushFormatCheckpoint.cpp | 275 | modified | 缺陷 F3（字段集合语义保持，见正文） |
| FlushFormats/FlushFormatInSitu.cpp | 66 | moved | OK |
| FlushFormats/FlushFormatOpenPMD.H | 64 | modified | OK |
| FlushFormats/FlushFormatOpenPMD.cpp | 192 | modified | OK |
| FlushFormats/FlushFormatPlotfile.H | 69 | modified | OK（`is_checkpoint` 见正文核对） |

## 缺陷清单

### F1 诊断 rho 的 ngrow 断言把"各向同性分配"与"逐方向需求"相比，cfl>1 时直接 abort
- 文件:行 `Source/Plugins/Diagnostics/ComputeDiagFunctors/RhoFunctor.cpp:38`（把 `data().ngDeposRho()` 作为**必需 ngrow** 传入）
  → `Source/Plugins/PicEm/RuntimeBridge/Runtime/WarpXHostServices.cpp:170-172`（`rho->nGrowVect() == ngrow` 否则 `WARPX_ALWAYS_ASSERT`）。
- 严重度: major
- 置信度: likely（机制与数值反例确定，未运行）
- 现象与依据:
  - 分配侧：`Source/Plugins/ParticleCore/Particles/WarpXParticleContainer.cpp:1970-1972`
    `const int ng_rho = data().ngDeposRho().max();` → `MultiFab(nba, dm, ncomps, ng_rho)`，即**各方向统一取 max**。
  - 需求侧：`WarpX::get_ng_depos_rho()` 返回**逐方向** IntVect（`Source/Plugins/PicEm/WarpX.H:989`
    `return guard_cells.ng_depos_rho;`）。其构造为 `GuardCellManager.cpp:143` `ng_alloc_Rho = ng_alloc_J+1`，
    再在 `:170` `ng_alloc_Rho[i] += ceil(c*dt_Rho/dx[i])`（显式/PSATD 分支）。因此
    `ng_depos_rho[i]` 只在 `ceil(c*dt_Rho/dx_i)` 各方向相同时才等于其 `.max()`。
  - PSATD 的 dt 为 `deltat = cfl*minDim(dx)/c`（`Source/Plugins/PicEm/Modules/Evolve/WarpXComputeDt.cpp:80-82`），
    于是 `c*dt/dx_i = cfl*min(dx)/dx_i`。数值反例：
    `Examples/Tests/nci_psatd_stability/inputs_base_2d_averaged`（`warpx.cfl = 4.`，`n_cell=128 128`，
    `prob_hi-prob_lo = 24.7552 × 99.0208` → `dx=0.1934, dz=0.7736`）：
    `ceil(c dt/dx)=ceil(4)=4`，`ceil(c dt/dz)=ceil(1)=1` → `ng_depos_rho=(…+4, …+1)`，相差 3。
    只要该用例再写 `diag.fields_to_plot = … rho`（默认 2D/RZ 列表不含 rho，见 `Diagnostics.cpp:128-137`），
    这行断言立即 abort："computeRho: requested ngrow does not match the deposited rho"。
    同样可由 `warpx.const_dt` 大于 CFL 或 `psatd.JRhom + do_time_averaging`（`dt_Rho=2dt`）触发。
  - 上游对照：`_upstream/.../ComputeDiagFunctors/RhoFunctor.cpp:33-79` 完全没有对 rho 的 ngrow 做任何比较；
    这个断言是 fork 新加的自检，且两侧量的定义不一致（AMReX 分配只能用标量/IntVect，上游刻意用 `.max()`）。
  - 与之对照，fork 自己在别处已注意到"snapshot/live 值"的问题（`BackTransformFunctor.cpp:131,227,249` 用
    `config().ncomps()` 并注明 "live, see InitData()"），说明这类"两侧定义不一致"的检查是 AI 自造、未经验证。
- 建议修法: 删掉 `WarpXHostServices.cpp:170-172` 的 ngrow 断言；若需保留自检，改为
  `WARPX_ALWAYS_ASSERT(rho->nGrowVect() >= ngrow)`（逐分量 ≥），或与 `ngDeposRho().max()` 比较。
- 验证方法: 取 `inputs_base_2d_averaged`，追加
  `diagnostics.diags_names=diag1 / diag1.diags_type=Full / diag1.fields_to_plot=rho` 后跑 1 步：
  无断言则通过，有断言则证实（也可 `ctest -R nci_psatd` 加 rho 变体）。

### F2 DivE/DivB 诊断用 `config_snapshot().ncomps` 分配，legacy(PICMI) 路径上恒为 1 → RZ 多模写越界/只留 mode 0
- 文件:行 `Source/Plugins/Diagnostics/ComputeDiagFunctors/DivEFunctor.cpp:56`
  （`amrex::MultiFab divE(ba, dmap, config_snapshot().ncomps, ng);`）、
  `Source/Plugins/Diagnostics/ComputeDiagFunctors/DivBFunctor.cpp:37`（同）；`DivEFunctor.cpp:51` 用同一快照取 solver id。
- 严重度: critical（对 1 分量 MultiFab 越界写）
- 置信度: likely（代码路径确定；触发条件明确，仓库内无用例覆盖）
- 现象与依据:
  - `RuntimeConfigSnapshot` 的 `ncomps` 默认值就是 1（`Source/Core/Runtime/RuntimeConfigSnapshot.H:48`
    `int ncomps{1};`），其 `FromLegacy()` 只是**构造时刻**读取 `WarpX` 静态量。
  - 两条上下文路径的时序不同：
    * 可执行文件路径（`Source/Core/main.cpp:18` 建 `Runtime`）：`Runtime::initialize()` 先
      `m_kernel->initialize()`（整个 `WarpX::InitData`）再 `sync_config()`（`Source/Core/Runtime/Runtime.cpp:76-78`），
      snapshot 在 `ncomps` 算好之后取 → 正确。
    * Python/PICMI 路径：`WarpX::GetInstance()`（`Source/Plugins/PicEm/Modules/Python/WarpX.cpp:92`）不建 `Runtime`，
      legacy ctx 在 `InitData_BeforeInit` 里建（`Source/Plugins/PicEm/Modules/Initialization/WarpXInitData.cpp:841-842`），
      且 refresh 回调只刷新 frame/time（`:850-861`），**不刷新 snapshot**；
      而 `ncomps = n_rz_azimuthal_modes*2 - 1` 是在更晚的 `AllocLevelMFs`（`Source/Plugins/PicEm/WarpX.cpp:2846`）里赋值的。
      → legacy 路径上 `config_snapshot().ncomps == 1` 而真实 `WarpX::ncomps = 2*nmodes-1`。
  - 后果：RZ 多模（`n_rz_azimuthal_modes ≥ 2`）时 `divE/divB` 只有 1 个分量，而
    `FiniteDifferenceSolver::ComputeDivECylindrical`（`Source/Plugins/FieldSolver/FiniteDifference/ComputeDivE.cpp:39,48-56`）
    用 `nmodes = m_nmodes` 循环写 `divE(i,j,0,2*m-1)`/`2*m`。AMReX 的 `Array4` 在 i/j/k 之后按分量连续存放，
    写 index≥ncomp 会落到下一个格点分量甚至越过 FAB 末尾 → 静默污染或堆溢出；
    即便侥幸不越界，`DivEFunctor.cpp:69-72` 的模式求和对 `nComp()==1` 退化为"只取 mode 0"，物理量完全错。
  - 上游对照：`_upstream/.../DivEFunctor.cpp:51-54`（`WarpX::ncomps`、`WarpX::electromagnetic_solver_id`），
    即**运行时实时静态量**，无快照语义。
  - 同仓库的反证：fork 自己在 `BackTransformFunctor.cpp:131,227,249` 与 `BTDiagnostics.cpp:526-530`
    明确改用 `config().ncomps()` 并写了注释 "the snapshot is taken before ncomps is known"，
    唯独 DivE/DivB 没改 → 同一事实两套写法，必有一处错。
- 建议修法: `DivEFunctor.cpp:56`、`DivBFunctor.cpp:37` 改 `config().ncomps()`；
  `DivEFunctor.cpp:51` 也建议统一为 `config().electromagnetic_solver_id()`（该量在 legacy snapshot 里恰好正确，但语义不该依赖快照）。
- 验证方法: PICMI 脚本：`grid = picmi.CylindricalGrid(nr=..., nz=..., n_rz_azimuthal_modes=2)`，
  diag 导 `Er … divE`，跑 1 步（`AMREX_DEBUG=ON` 或 `-DAMREX_BOUND_CHECK` 会直接 abort）；
  或临时打印 `divE.nComp()` 与 `WarpX::ncomps` 对比（应为 1 vs 3）。

### F3 checkpoint 新增的 `field_schema.txt` 由所有 MPI rank 同时写，无 IOProcessor 保护
- 文件:行 `Source/Plugins/Diagnostics/FlushFormats/FlushFormatCheckpoint.cpp:141-146`
  （`std::ofstream schema_file(checkpointname + "/field_schema.txt");` 在 rank 循环外、无 `ParallelDescriptor::IOProcessor()` 判断）。
- 严重度: minor
- 置信度: confirmed
- 现象与依据: 上游 `FlushFormatCheckpoint::WriteToFile` 里所有写文件动作都在 IO rank 上（`WriteJobInfo`、`WriteDMaps`、
  `WriteReducedDiagsData` 均有 `if (ParallelDescriptor::IOProcessor())`，见本文件 `:241`、
  `_upstream/.../FlushFormatCheckpoint.cpp:264-300`；VisMF/`PreBuildDirectorHierarchy` 自带 rank 逻辑）。
  这段是纯 `std::ofstream` 直接截断同一路径：N 个 rank 同时 O_TRUNC + 写，内容虽相同但可能产生截断/交错文件，
  在并行 FS（Lustre/GPFS/NFS）上是无谓的元数据风暴。另外该文件**没有任何读者**
  （`grep -rn hybrid_pic_substeps/field_schema` 只命中写端与本文件注释），属死文件。
- 建议修法: 整段包进 `if (ParallelDescriptor::IOProcessor())`；或干脆删除（无人读）。
- 验证方法: 4 rank checkpoint，`wc -l diags/chk000010/field_schema.txt` 与 `WarpX::fieldSchema().checkpointNames().size()` 比较，
  反复重跑观察文件是否偶发截断；`strace -f -e trace=openat` 可看到每个 rank 都 open 同一路径。

### F4 BTD 的场 functor 从未拿到 RuntimeContext，`config_snapshot()` 静默返回全默认值
- 文件:行 `Source/Plugins/Diagnostics/BTDiagnostics.cpp:576` 与 `:695`（构造 `BackTransformFunctor` 后未 `SetRuntimeContext`），
  对照 `Source/Plugins/Diagnostics/FullDiagnostics.cpp:1084`（每次输出前统一传播 ctx）；
  消费点 `Source/Plugins/Diagnostics/ComputeDiagFunctors/BackTransformFunctor.cpp:108`
  （`config_snapshot().do_single_precision_comms`）与 `ComputeDiagFunctor.cpp:53-61`（fallback 为
  `static const RuntimeConfigSnapshot legacy{}`，即**结构体默认值**，不是 `FromLegacy()`）。
- 严重度: minor
- 置信度: confirmed
- 现象与依据: BTD 只有粒子 functor 拿到 ctx（`BTDiagnostics.cpp:1454`）。场 functor 于是走
  `ComputeDiagFunctor::config_snapshot()` 的 `legacy{}` 分支 → `do_single_precision_comms` 恒为 false，
  用户设 `warpx.do_single_precision_comms=1` 时 BTD 回传通信静默退回双精度（`ncomps` 已改 live，故不致命）。
  更值得担心的是"静默全默认"这个模式本身：`legacy{}` 里 `ncomps=1, nox=0, grid_type=Default…`，
  将来任何在 functor 里读快照的物理量都会静默取 0/1。相邻的 `data()/frame()` 都用了 `create_legacy_*()`/`FromLegacy()`，
  只有 `config()/config_snapshot()` 是裸默认构造（`config()` 因 `RuntimeConfig` 无状态、getter 直读 WarpX，恰好无害）。
- 建议修法: 在 `BTDiagnostics::PrepareFieldDataForOutput`/`PrepareBufferData` 中像 FullDiagnostics 一样传播 ctx；
  或把 BTD functor 的 ctx 在构造处直接传入；并把 fallback 改成 `RuntimeConfigSnapshot::FromLegacy()`。
- 验证方法: `warpx.do_single_precision_comms=1` + BTD 场输出，在 `BackTransformFunctor.cpp:108` 打印
  `config_snapshot().do_single_precision_comms`，应为 1 实为 0。

### F5 `LorentzTransformParticles` 绕过传入的 context，直接用 `FrameState::FromLegacy()`
- 文件:行 `Source/Plugins/Diagnostics/ComputeDiagFunctors/BackTransformParticleFunctor.cpp:47-48`
  （`m_gammaboost(warpx::runtime::FrameState::FromLegacy().gamma_boost)`、`m_betaboost(...)`）。
- 严重度: minor
- 置信度: confirmed
- 现象与依据: 该类的基类 `ComputeParticleDiagFunctor` 已持有 `m_runtime_context`，同文件其它物理量
  （`data().t_new(0)`、`data().dt(0)`，`:92-93`）都走 ctx；唯独 boost 参数走进程级 legacy 静态快照。
  当前 `FrameState::FromLegacy()` 每次调用都读 `WarpX::gamma_boost`（`Source/Plugins/PicEm/RuntimeBridge/Runtime/FrameState.cpp`），
  单实例下数值正确，但语义上把"插件只能通过 ctx 访问仿真状态"的约定打破；
  多实例/嵌入式 kernel（`Runtime` 不为单例）时会取错 frame。
  另外构造两次 `FromLegacy()`（boost 参数成对使用）属可读性/一致性缺陷。
- 建议修法: 改存 `const RuntimeContext*`，在 operator()/构造函数里用 `ctx->frame->gamma_boost/beta_boost`；
  与 `BackTransformFunctor.cpp:58-60` 的写法保持一致。
- 验证方法: 构造两个 `Runtime`（不同 gamma_boost）在同进程，对比 BTD 粒子的 `uz` 反变换结果；
  或直接 `grep -n "FromLegacy" Source/Plugins/Diagnostics/**`，仅此一处。

### F6 诊断 J 的按需沉积新增 `ApplyJfieldBoundary`，边界处与上游不同
- 文件:行 `Source/Plugins/Diagnostics/ComputeDiagFunctors/JFunctor.cpp:49`
  （`host().particleServices().depositCurrent(m_lev, data().dt(m_lev));`）→
  `Source/Plugins/PicEm/RuntimeBridge/Runtime/WarpXHostServices.cpp:61-81`
  （`WarpXSumGuardCells` + `FinalizeJOnLevel`，后者 = `ApplyJfieldBoundary`，见 `Modules/Evolve/WarpXEvolve.cpp`）。
- 严重度: minor
- 置信度: likely
- 现象与依据: 上游 `_upstream/.../JFunctor.cpp:41-58` 是"`DepositCurrent` → 对 3 个分量
  `FillBoundary(periodicity)`"，注释明写 "this does not filter the current density"。
  fork 换成 `WarpXSumGuardCells`（周期边界下**求和**，比 `FillBoundary` 覆盖更正确）**加上**
  `ApplyJfieldBoundary`（PEC/其它边界对 J 的处理），因此 `plot jx/jy/jz` 在边界层会与上游不同；
  这是语义变化而非纯重构，且是"诊断路径顺带做了演化路径的收尾操作"。
  注意 fork 并未在诊断里做 RZ 的 `ApplyInverseVolumeScalingToCurrentDensity`（与上游一致），
  所以 J 的 2πr 约定没有改变。
- 建议修法: 若只想修"周期边界丢贡献"，保留 `WarpXSumGuardCells` 但去掉 `FinalizeJOnLevel`；
  若要保留，请在 `JFunctor.H` 注释说明诊断 J 现在带边界处理，并在 changelog 标注输出变化。
- 验证方法: RZ `btd_rz`（或任一带 `boundary.field_lo=pec` 的用例）导 `jz`，比较 fork 与
  `_upstream` 构建在边界 1–2 层的数值。

### F7 用户可见报错消息被替换成 C++ 语法；注释与代码不一致
- 文件:行 `Source/Plugins/Diagnostics/Diagnostics.cpp:169`
  （`"plot F only works if config().do_dive_cleaning() = 1"`）、`:177`
  （`"G can be written to file only if config().do_divb_cleaning() = 1"`）、`:172`
  （注释写 `config_snapshot().do_divb_cleaning`，代码读 `config().do_divb_cleaning()`）。
- 严重度: minor
- 置信度: confirmed
- 现象与依据: 上游 `_upstream/.../Diagnostics.cpp:112-120` 的消息是用户可直接照抄的输入参数
  （`warpx.do_dive_cleaning = 1` / `warpx.do_divb_cleaning = 1`）。机械替换 `warpx.X` → `config().X()`
  时把消息文本一起替换了，用户拿到的提示无法使用。同类问题在 `:172` 形成"注释说 snapshot、代码读 live"的自相矛盾
  （上游注释与被读对象一致）。
- 建议修法: 消息文本恢复成 `warpx.do_dive_cleaning = 1` / `warpx.do_divb_cleaning = 1`；`:172` 注释与代码统一。
- 验证方法: 输入 `diag.fields_to_plot = F` 且 `warpx.do_dive_cleaning=0`，看 abort 消息。

## 已核对为"语义保持"的关键点（不计入缺陷）

- **`FlushFormatPlotfile::WriteToFile` / `WriteWarpXHeader` 的 `is_checkpoint`**：
  `WriteToFile` 签名未变（`FlushFormatPlotfile.H:25-39` 与上游逐字一致），只有 `WriteWarpXHeader` 增加
  `bool is_checkpoint = false`（`:44-45`，默认值保持旧调用语义）。两个调用点：
  `FlushFormatPlotfile.cpp:119`（普通 plotfile，走默认 false）与 `FlushFormatCheckpoint.cpp:135`
  （显式 true）。默认参数保证既有 plotfile 输出不受影响；新增的尾部行
  `hybrid_pic_substeps <N>`（`FlushFormatPlotfile.cpp:353-359`）只在 checkpoint 且 `hybrid_substeps >= 0`
  时写，而 `WarpXHostServices::hybridPicSubsteps()` 在无 HybridPIC 时返回 -1
  （`RuntimeBridge/Runtime/WarpXHostServices.cpp:370-374`），读端以 `is >> std::ws; if(!is.eof())` 容错
  （`RuntimeBridge/IO/WarpXCheckpoint.cpp:244-262`），因此"从 plotfile restart"仍可用。
- **checkpoint 字段集合**：上游 30 个显式 `VisMF::Write`（fp/cp × E/B/J + avg + E_old）被改写为
  "遍历 `data().fieldRegistry().checkpointNames()` ∩ 静态名字表"。核对结果：表中 30 个键名与上游文件名一一对应
  （`Efield_fp_x → Ex_fp` …），且这些键在 `AllocLevelMFs` 里确实以 `checkpoint=true` 声明
  （`WarpX.cpp:2857-2867, 2876-2878, 2940-2946, 3295-3337`），`FieldSchema` 即 `FieldRegistry` 别名
  （`Runtime/FieldSchema.H:16`）。过滤条件与上游等价：`has()` ↔ 上游的 `has_vector/has`；
  `current_*` 仅在 `isSynchronized()` 时写（上游同）；cp 仅在 lev>0 已分配时写。PML/RZ-PML 前缀也一致
  （adapter 里 `prefix + "_rz"` ≡ 上游的 `"pml_rz"`，读端 `WarpXCheckpoint.cpp:390-391` 读同一名字）。
- **checkpoint header 内容**：`istep/nsubsteps/t_new/t_old/dt/boxArray/injection_position/do_moving_window/
  time_of_last_gal_shift/BTD buffer 状态` 与上游逐项同序，仅在 `:300` 处多一个可选行（见上）。
- **RhoFunctor 的 RZ 体积标定**（`FinalizeRhoOnLevel` 被纳入诊断路径）已由 `Source_Plugins_PicEm_B.md` F8 覆盖，
  本文不重复计缺陷；F1 是同一调用点上**独立**的 ngrow 断言问题。
- **PhiFunctor**：`host().fieldAlgebra().computePhi(m_lev)`（`WarpXHostServices.cpp:176-209`）
  与上游 `#if phi_fp exists / else divE→Poisson` 两分支逐行等价（唯一差别是返回副本而非借指针）。
- **`FlushFormat::data()/host()` 缺 ctx 时抛 `std::runtime_error`**：实际不会触发 ——
  `Diagnostics::InitBaseData` 在建完 `m_flush_format` 后立即 `SetRuntimeContext`（`Diagnostics.cpp:639`），
  且 `multi_diags` 无论如何都有 ctx（`WarpXInitData.cpp:824-882` 先建 legacy ctx）。
- **`FlushFormatPlotfile` 的 `refRatioVector()`** ≡ 上游 `warpx.refRatio()`（`WarpXRuntimeData.cpp:146` 直转）；
  Ascent/Catalyst/InSitu 三处替换无行为变化。
- **`BoundaryScrapingDiagnostics`**：`getParticleBufferPointer` 返回 `void*`，落到
  `IParticleContainer::Base = amrex::ParticleContainerPureSoA<...>`，与 `WarpXParticleContainer::Base`
  （`WarpXParticleContainer.H:131`）是**同一类型**，`static_cast` 正确；flush 路径/文件名未变。

## 不确定但值得复核的点

1. **`MultiFabRegister::write_checkpoints` 与新名字表是否重叠/互补**：checkpoint 现在既按名字表逐字段 VisMF 写，
   又调 `fields().write_checkpoints(lev, prefix)`（`FlushFormatCheckpoint.cpp:167`）。上游两者都有，但上游
   `write_checkpoints` 只写带 `checkpoint_restart` 标记的字段。建议核对 `ablastr/fields/MultiFabRegister.cpp`
   的 `checkpoint_names` 与实际文件名，确认没有"同一字段写两份/漏写"。
2. **插件声明字段的 checkpoint**：`AllocPluginFields`（`WarpX.cpp:2720-2736`）允许插件以 `spec.checkpoint=true`
   注册任意名字。writer 只认静态名字表（`checkpointFieldInfo`），未知键被 `if (!info.has_value()) continue;` 静默跳过，
   但读端 `m_fields.read_restarts` 会尝试读它 → 插件字段可能"有的写有的不写"。这是"静默跳过"设计风险，
   建议在跳过时 `WMRecordWarning` 一次。
3. **`JdispFunctor.cpp:38`** `auto* hybrid_pic_model = host().diagnosticsMetadata().hasHybridPICModel() ? this : nullptr;`
   —— 把 `JdispFunctor* this` 当"模型存在"的布尔用。当前只参与 `WARPX_ALWAYS_ASSERT`/`AMREX_ASSUME`
   （`:43-45`），且 `hasHybridPICModel()` ≡ `get_pointer_HybridPICModel() != nullptr`
   （`WarpXHostServices.cpp:365-368`），故**行为等价**；但变量名、类型、指针来源三者都在骗人，
   后续若误用会立刻是 UB。建议改 `const bool has_hybrid = ...;`。
4. **`BTD_Plotfile_Header_Impl.cpp:396`** 新增 `m_int_comp_names.resize(m_num_output_int);`：
   上游 `:393-398` 在空 vector 上 `for (auto& n : m_int_comp_names) is >> n;` 实际一个名字都没读，
   于是紧随其后的字段全部错位。fork 这里是**修复**，但意味着"含 int 分量的 BTD 粒子 header"格式解析行为变了：
   建议确认 `BTD_Plotfile_Header_Impl.H` 里 `m_int_comp_names` 在复用对象时是否会被正确清空（否则 resize 会保留旧值）。
5. **Rho 的 `nComp()` 强检查**（`RhoFunctor.cpp:44-48`，`throw std::runtime_error`）：
   在 RZ 且用户要求全部模式（`convertRZmodes2cartesian=false`）时才真正比较；若将来 `nComp()` 与
   `WarpX::ncomps` 的口径变化，会从"越界读"变成"诊断抛异常中断整个 run"。异常从 functor 抛到 AMReX 主循环
   是否能优雅终止也未见处理。
6. **`FlushFormatOpenPMD` 构造顺序**：`SetRuntimeContext(ctx)` 在 ctor 第一句（`:29`），但紧接着就用
   `host().pml()/host().diagnosticsMetadata()`（`:122-127`）；若将来有人用默认实参 `ctx=nullptr` 构造，
   会在 ctor 内抛异常。当前只有一个调用点（`Diagnostics.cpp:630`）且带 ctx，属潜伏陷阱。
