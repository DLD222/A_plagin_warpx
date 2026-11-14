# Source_Plugins_Diagnostics_B 审查

审查对象：`Source/Plugins/Diagnostics/` 后半（FlushFormats / Full&MultiDiagnostics / ParticleDiag /
OpenPMD 辅助 / ReducedDiags 全部）。参照物：`_upstream/warpX-26.10/Source/Diagnostics/`。
方法：先按文件名逐一对 `diff -u`，再对每一处语义变化读上下文推导；纯改名
（`WarpX::` → `data()/host()/config()/frame()`、`WarpXParticleContainer` →
`warpx::runtime::IParticleContainer`、`WarpXParIter` → `ParticleTileIter`）不计为缺陷。

结论先行：本分区**整体是忠实的机械重构**，派生公式、RZ 权重、单位、dt 因子与上游逐字一致
（FieldEnergy RZ `2πr`…、DifferentialLuminosity 的 `dV`/`dt`、FieldPoyntingFlux 的
`∫g·dA·dt`、ParticleMomentum 光子 `m_e` 特例等均未改动）。真实缺陷集中在**新写的
生命周期/接口代码**（regrid 后的 buffer 重建、诊断对仿真的副作用、context 空指针），
而不是物理公式。

## 覆盖

| 文件 | 行数 | 类型 | 结论 |
|---|---|---|---|
| FlushFormats/FlushFormatPlotfile.cpp | 732 | modified | 可疑（F5 关联） |
| FlushFormats/FlushFormatSensei.cpp | 106 | moved（与上游逐字相同） | OK |
| FullDiagnostics.H | 118 | modified | OK |
| FullDiagnostics.cpp | 1174 | modified（含新增 2 函数） | **缺陷 F1** |
| MultiDiagnostics.H | 72 | modified | OK |
| MultiDiagnostics.cpp | 171 | modified（新增 ReadCheckpointData） | OK（与上游 WarpXIO.cpp:244-291 等价） |
| OpenPMDHelpFunction.cpp | 53 | moved | OK |
| ParticleDiag/ParticleDiag.H | 50 | modified | OK |
| ParticleDiag/ParticleDiag.cpp | 128 | modified（`std::ranges::fill`） | OK |
| ReducedDiags/BeamRelevant.cpp | 442 | modified | OK |
| ReducedDiags/ChargeOnEB.cpp | 231 | modified（整体加 EB guard） | 可疑（F6） |
| ReducedDiags/ColliderRelevant.cpp | 561 | modified | 可疑（F3、注 1） |
| ReducedDiags/DifferentialLuminosity.cpp | 353 | modified | OK |
| ReducedDiags/DifferentialLuminosity2D.cpp | 441 | modified | OK（`this->data()` 规避命名遮蔽，正确） |
| ReducedDiags/FieldEnergy.cpp | 247 | modified | OK（新增 Darwin 警告） |
| ReducedDiags/FieldMaximum.cpp | 254 | modified | OK |
| ReducedDiags/FieldMomentum.cpp | 199 | modified | OK |
| ReducedDiags/FieldPoyntingFlux.H | 451 | modified | OK |
| ReducedDiags/FieldPoyntingFlux.cpp | 348 | modified（新增 `dt` 参数 + OMP reduction） | OK（见注 2） |
| ReducedDiags/FieldProbe.cpp | 723 | modified（新增 Darwin E 合成） | OK |
| ReducedDiags/FieldProbeParticleContainer.H/.cpp | 81/118 | moved | OK |
| ReducedDiags/FieldReduction.H | 243 | modified（1D `k`→`i`） | 差异需确认（见注 3） |
| ReducedDiags/FieldReduction.cpp | 122 | moved | OK |
| ReducedDiags/LoadBalanceCosts.cpp | 384 | modified | **缺陷 F2** |
| ReducedDiags/LoadBalanceEfficiency.cpp | 86 | modified | OK |
| ReducedDiags/MultiReducedDiags.H | 82 | modified | **缺陷 F4** |
| ReducedDiags/MultiReducedDiags.cpp | 209 | modified | 可疑（F6） |
| ReducedDiags/ParticleEnergy.cpp | 172 | modified | OK |
| ReducedDiags/ParticleExtrema.cpp | 400 | modified | 可疑（F3） |
| ReducedDiags/ParticleHistogram.cpp | 294 | modified | OK |
| ReducedDiags/ParticleHistogram2D.cpp | 337 | modified（value 缺省回退到 w） | 差异需确认（见注 4） |
| ReducedDiags/ParticleMomentum.cpp | 234 | modified（改用 `sumParticleMomentum()`） | OK（语义等价，见证据 E1） |
| ReducedDiags/ParticleNumber.cpp | 140 | modified | OK |
| ReducedDiags/ReducedDiags.H | 160 | modified（新增成员/虚函数签名） | 可疑（F7） |
| ReducedDiags/ReducedDiags.cpp | 215 | modified（新增 data/config/frame 回退） | OK（优于 Diagnostics.cpp 的静态回退） |
| ReducedDiags/RhoMaximum.cpp | 179 | modified | OK |
| ReducedDiags/Timestep.cpp | 84 | modified（dt 改从 ctx 取） | OK（`refresh_time()` 每步末调用，见证据 E2） |
| Diagnostics/WarpXOpenPMD.H | 388 | modified | 可疑（F5） |
| Diagnostics/WarpXOpenPMD.cpp | 1732 | modified（仅元数据，数值写入未变） | 可疑（F5、F8） |

## 缺陷清单

### F1 regrid 后 TimeAveraged 诊断的累加和被清零（静默错误）
- 文件:行 `Source/Plugins/Diagnostics/FullDiagnostics.cpp:908-915`（`RemakeLevel`）、
  `:869-874`（`InitializeBufferData` 里 `m_sum_mf_output[...].setVal(0.)`）；调用点
  `Source/Plugins/PicEm/Modules/Evolve/WarpXEvolve.cpp:405-410`
- 严重度: **critical**（静默错误结果；触发条件 = TimeAveraged 诊断 + AMR 且平均窗口内发生 regrid）
- 置信度: confirmed（纯代码路径推导）
- 现象与依据:
  - `Diagnostics::ComputeAndPack`（`Source/Plugins/Diagnostics/Diagnostics.cpp:695-698`）对
    TimeAveraged 诊断执行 `m_sum_mf_output += m_mf_output`，只在 `Flush()` 的 dynamic 分支
    （`FullDiagnostics.cpp:275-279`）清零，即"跨步累加、窗口末才清零"。
  - 新增的 `FullDiagnostics::RemakeLevel(lev)` 直接调用 `InitializeBufferData(i_buffer, lev)`，
    而后者在 `m_diag_type == DiagTypes::TimeAveraged` 时无条件
    `m_sum_mf_output[i_buffer][lev] = MultiFab(...); setVal(0.)`。
  - `WarpXEvolve` 每个 regrid 都对所有 level 调用 `multi_diags->RemakeLevel(lev)`，因此
    **只要在平均窗口内发生一次 regrid，之前累积的所有步都被丢弃**；最终输出 =
    `sum(regrid 之后步)/m_average_period_steps`，比真值小 `steps_since_regrid/period` 倍。
    上游不做 buffer 重建（`_upstream/.../FullDiagnostics.cpp` 无此函数），因此这是本次
    重构引入的回归，且没有任何 assert/warn 提示。
  - 同一函数里 `InitializeBufferData` 还会重建 `m_mf_output`/`m_geom_output`，这部分是
    regrid 后本来就该做的（上游缺失），所以"修 regrid 一致性"的动机成立，但顺手把
    平均值累加器也清了。
- 建议修法: `RemakeLevel`/`ResizeLevels` 里对 TimeAveraged 诊断采用
  "重建 `m_mf_output`/`m_geom_output`，用 `ParallelCopy` 把旧 `m_sum_mf_output`
  拷到新布局（重叠区相加、非重叠区置 0）"；最小改动版是给
  `InitializeBufferData` 加 `bool preserve_sum = false` 参数，`RemakeLevel` 传 true 并在
  重建后 `ParallelCopy`。至少应在清零时 `WMRecordWarning` 高优先级告警。
- 验证方法:
  - 用例：`amr.max_level = 1`、`amr.regrid_int = 5`、`max_step = 60`，一个
    `diag_type = TimeAveraged`、`intervals = 60`、`average_period_steps = 40` 的 plotfile 诊断，
    对比"有 regrid"与"`amr.max_level = 0`"两次的场平均值；当前实现两者差异 ~(60-45)/40。
  - 或 `grep -n "setVal(0.)" Source/Plugins/Diagnostics/FullDiagnostics.cpp`（行 873）确认清零路径。

### F2 LoadBalanceCosts 诊断写入仿真的 live cost 数组（诊断产生副作用）
- 文件:行 `Source/Plugins/Diagnostics/ReducedDiags/LoadBalanceCosts.cpp:117-126`
- 严重度: **major**
- 置信度: likely（代码路径可读，未跑 heuristic LB 用例）
- 现象与依据:
  - 上游签名是 `WarpX::ComputeCostsHeuristic(amrex::Vector<std::unique_ptr<LayoutData<Real>>>& a_costs)`，
    诊断先把 live costs 拷进私有 `costs`，再让启发式累加到**副本**上
    （`_upstream/.../Source/Parallelization/WarpXRegrid.cpp` 中 `(*a_costs[lev])[...] += ...`），
    live costs 不被诊断触碰。
  - 本地 `RuntimeData` 只提供无参 `computeCostsHeuristic()`，其实现
    （`Source/Plugins/PicEm/Modules/Parallelization/WarpXRegrid.cpp:400-418`）对
    `m_level_resources.level(lev).costs` **原地 `+=`**，即诊断在每次输出时污染了
    负载均衡真正使用的 cost 数组；随后 `CheckLoadBalance` → `LoadBalance()` 又调用一次
    `ComputeCostsHeuristic()`，启发式部分被**重复累加**，分布映射（SFC/KnapSack）权重被
    抬高粒子/网格项（`costs_heuristic_particles_wt` / `cells_wt` 各算两遍）。
  - 打印出来的数值本身与上游一致（两边都等于 live + heuristic），所以问题是"副作用 + 下次 LB 偏置"，
    属隐蔽缺陷。
- 建议修法: 给 `RuntimeData` 增加 `computeCostsHeuristic(amrex::Vector<std::unique_ptr<LayoutData<Real>>>&)`
  重载（或在诊断里改为对副本求 heuristics），保持诊断只读。
- 验证方法: `load_balance_costs_update_algo = heuristic` + `warpx.load_balance_intervals = 20` +
  `LoadBalanceCosts` 诊断，在 `WarpX::ComputeCostsHeuristic()` 打印
  `(*m_level_resources.level(0).costs)` 前后值；本实现会看到诊断那次调用后数组变大。

### F3 `GetExternalEBField(pti, offset, myspc.runtime_context())` 的 parser 分支不判空
- 文件:行 `Source/Plugins/Diagnostics/ReducedDiags/ParticleExtrema.cpp:297`、
  `Source/Plugins/Diagnostics/ReducedDiags/ColliderRelevant.cpp:469`；
  被调方 `Source/Plugins/ParticleCore/Particles/Gather/GetExternalFields.cpp:44-46, 84-96`
- 严重度: minor
- 置信度: needs-verification（当前所有构造路径都会给出非空 ctx + 非空 `ctx->particles`）
- 现象与依据: 构造函数对 `data`/`config` 都有 nullptr 回退
  （`legacy_data()` / `legacy_config()`），却对 `ps = ctx->particles` 直接解引用：
  `m_Exfield_partparser = ps->externalFieldParser("Ex")->compile<4>();`。若 `ctx == nullptr`
  或 `ctx->particles == nullptr` 且输入里设了 `E_ext_particle_init_style =
  parse_e_ext_particle_function`/`parse_b_...`，就是空指针解引用（段错误），而不是明确报错。
  `ReducedDiags.H:67` 的注释就写着"nullptr on the legacy/Python path"，说明该路径被预期存在。
  上游用 per-container 的 `m_E_ext_particle_s`/`m_E_external_particle`，没有这个风险。
- 建议修法: 在 parser 分支前 `WARPX_ALWAYS_ASSERT_WITH_MESSAGE(ps != nullptr, "external particle
  field parsers need a particle system in the runtime context")`，或在无 ps 时退回
  `legacy_runtime_data()` 对应的容器。
- 验证方法: 临时用 `FullDiagnostics(i, name, type)` 的默认 `ctx = nullptr` 构造（或直接
  `GetExternalEBField(pti, 0, nullptr)`）跑一个带 parser 外场的 ParticleExtrema 用例，应崩。

### F4 头文件里把 `#include` 写进了版权注释
- 文件:行 `Source/Plugins/Diagnostics/ReducedDiags/MultiReducedDiags.H:3`
- 严重度: minor
- 置信度: confirmed
- 现象与依据: 第 1-6 行是 `/* ... */` 版权块，第 3 行插入了
  `#include "Runtime/IReducedDiagsService.H"`（在注释内部，不起作用；真正的 include 在
  第 12 行）。典型 AI 插入位置错误，功能上无害但会误导后续维护与 include-what-you-use 检查。
- 建议修法: 删除第 3 行。
- 验证方法: `sed -n 1,6p Source/Plugins/Diagnostics/ReducedDiags/MultiReducedDiags.H`。

### F5 `WarpXOpenPMDPlot` 对 `m_runtime_context` / `config_snapshot` 直接解引用
- 文件:行 `Source/Plugins/Diagnostics/WarpXOpenPMD.cpp:604, 608, 1206-1337`
  （另见 `:1370-1382` 的 `m_runtime_context->config->ncomps()` 是有判空条件的，前后不一致）
- 严重度: minor
- 置信度: likely
- 现象与依据: `FullDiagnostics`/`BTDiagnostics`/`BoundaryScrapingDiagnostics` 的构造函数都接受
  `ctx = nullptr` 默认值，`Diagnostics::config()`/`config_snapshot()` 也都实现了 nullptr 回退，
  但 `WarpXOpenPMDPlot::WriteOpenPMDParticles` 与 `UnpackSetMeshesAttributes` 里一律
  `m_runtime_context->config_snapshot->...`，一旦 context 缺失就从"沉默降级"变成段错误。
  正常流程（`WarpXInitData_BeforeInit` 必定装 legacy context；`create_diagnostics_service(ctx)`）
  不会触发，属健壮性回归。
- 建议修法: 把 `WarpXOpenPMDPlot` 改为持有 `const RuntimeData&`/`const RuntimeConfigSnapshot&`
  （调用方 `FlushFormat` 已经有 `config()`/`config_snapshot()` 访问器），或至少加一次
  `WARPX_ALWAYS_ASSERT(m_runtime_context != nullptr && m_runtime_context->config_snapshot != nullptr)`。
- 验证方法: 单元级：`WarpXOpenPMDPlot(..., nullptr)` 后调用 `WriteOpenPMDParticles`；或用
  `FullDiagnostics` 默认构造后 `InitData()` 打开 openpmd 输出。

### F6 非 EB 构建下 ChargeOnEB 从"运行时明确报错"变成"未注册类型"
- 文件:行 `Source/Plugins/Diagnostics/ReducedDiags/MultiReducedDiags.cpp:64-66`、
  `Source/Plugins/Diagnostics/ReducedDiags/ChargeOnEB.cpp:42, 231`
- 严重度: minor
- 置信度: confirmed
- 现象与依据: 整个 `ChargeOnEB.cpp` 被 `#if defined(AMREX_USE_EB)` 包住，注册表项也同步加 guard。
  上游在无 EB 时仍注册该类，构造函数会
  `WARPX_ALWAYS_ASSERT_WITH_MESSAGE(false, "ChargeOnEB ... only works when compiling with EB support")`；
  本地则落到 `WARPX_ALWAYS_ASSERT_WITH_MESSAGE(dictionary.contains(rd_type), ...)` 的
  "ChargeOnEB is not a valid type" —— 两个 abort，但后者不说明"缺少 EB 支持"。
  另外 `ChargeOnEB.H` 未加 guard（只声明不定义），当前靠未注册才不产生链接错误，属脆弱一致。
- 建议修法: 保留注册、把 `ChargeOnEB.cpp` 的 EB guard 收在 `ComputeDiags` 内部（恢复上游行为）；
  或至少在 abort 文案里区分"编译期未启用 EB"。
- 验证方法: 非 EB 构建 + `warpx.reduced_diags_names = q` / `q.type = ChargeOnEB`，比较报错文案。

### F7 基类新增 `unique_ptr` 成员使"默认拷贝"静默失效
- 文件:行 `Source/Plugins/Diagnostics/ReducedDiags/ReducedDiags.H:79-81, 96-100`
- 严重度: minor
- 置信度: confirmed
- 现象与依据: 新增 `mutable std::unique_ptr<RuntimeData> m_legacy_data;`，但
  `ReducedDiags(const ReducedDiags&) = default;` / `operator=` 仍声明为 default ——
  实际被隐式定义为 deleted。目前所有 reduced diag 都以 `unique_ptr` 持有，不会触发，
  但这是"注释说可拷贝、实际不可拷贝"的类型谎言，日后按基类值语义使用会突然编译失败。
- 建议修法: 删除这两行 default 声明（显式 `= delete` 更诚实），或在注释里说明。
- 验证方法: `static_assert(std::is_copy_constructible_v<ReducedDiags>)` 会失败。

## 值得复核但不认定为缺陷的点

- **注 1（ColliderRelevant，`ColliderRelevant.cpp:230`）**：`GetChargeDensity(0)` 换成
  `GetChargeDensityOnLevel(0, /*local=*/false, /*finalize=*/true)`。后者等于
  `DepositChargeOnLevel(...)+SumBoundary+FinalizeRhoOnLevel`，而
  `WarpX::FinalizeRhoOnLevel`（`Source/Plugins/PicEm/Modules/Evolve/WarpXEvolve.cpp:1251-1259`）
  先 `ApplyRhofieldBoundary` 再做 RZ 的 `ApplyInverseVolumeScalingToChargeDensity`；上游
  `DepositCharge(..., apply_boundary_and_scale_volume=true)` 在 RZ 下**只**做体积缩放（边界反射
  被 `#if !defined(WARPX_DIM_RZ)` 排除），非 RZ 才做边界反射。因此 RZ + PEC/反射边界时 rho 会
  多一次边界反射 → chi_min/chi_max 与本征值略有偏差。默认 RZ 输入通常无该边界，影响小。
- **注 2（FieldPoyntingFlux）**：新增 `#pragma omp parallel ... reduction(+:flux)`
  （`FieldPoyntingFlux.cpp:207`）。上游在 `#pragma omp parallel` 区域内对共享 `flux` 做 `+=`
  是数据竞争，本改动是**修复**；数值在单线程下不变。
- **注 3（FieldReduction）**：1D（`WARPX_DIM_1D_Z`）分支把 `z=(k+0.5)*dx[0]+lo` 改成
  `(i+0.5)*dx[0]+lo`（`FieldReduction.H:177`）。1D 下 AMReX 只迭代 `i`、`j=k=0`，上游写法使
  z 恒为 `0.5*dx`（明显 bug），本地是**修复**，但会改变所有 1D `FieldReduction` 的参考值
  （若上游基准 JSON 里有 1D 用例需重新生成）。
- **注 4（ParticleHistogram2D，`ParticleHistogram2D.cpp:238-240`）**：未指定
  `value_function(...)` 时权重由"默认构造的 `ParserExecutor`（求值为 0）"改为粒子权重 `w`。
  更有物理意义，但属与上游不同的行为；若上游 CI 有用例依赖"0 权重直方图"会差异。
- **注 5**：`Diagnostics::config()` 的 legacy 回退是 `static const RuntimeConfig legacy{};`
  （`Source/Plugins/Diagnostics/Diagnostics.cpp:57-65`），而 `ReducedDiags::config()` 是
  每次调用 `RuntimeConfig::FromLegacy()` 刷新（`ReducedDiags.cpp:187-196`，注释已说明原因）。
  前者一旦在 `ctx == nullptr` 下被读到就会给出全零配置（`ncomps()==0`、
  `fft_do_time_averaging()==false`）。当前所有创建路径都提供 context，故未触发，但两套回退语义
  不一致，建议统一成 `ReducedDiags` 的做法。
- **注 6**：`FullDiagnostics::ResizeLevels`/`RemakeLevel` 只增不减 level；
  regrid 使 `finest_level` 下降时 `nlev_output` 与 `m_all_field_functors[>finest]` 仍指向旧
  MultiFab（可能已随 `ClearLevel` 释放）。这一点上游同样存在（`nlev_output` 只在 `InitData`
  设定，见 `_upstream/.../Diagnostics.cpp:545-547`），不算本次回归，但新加的
  `ResizeLevels` 已具备修正条件，建议一并处理。
- **证据 E1（ParticleMomentum 语义等价）**：`IParticleContainer::sumParticleMomentum()`
  （`Source/Plugins/ParticleCore/Particles/WarpXParticleContainer.cpp:3234-3251`）与上游内联
  reduce 逐项一致，且保留了光子 `isPhoton() ? m_e : getMass()` 的替换，
  `sumParticleWeightAndEnergy` 同样保留 `KineticEnergyPhotons` 分支（同文件 2550-2598）。
- **证据 E2（Timestep）**：`ctx->time->dt` 由 `RuntimeContext::refresh_time` 刷新，该回调在
  `WarpXEvolve.cpp:275-277` 每步末、诊断之前调用，且 `WarpXInitData.cpp:1039-1041` 在首次诊断前
  也调一次，故 `Timestep.cpp:68-78` 取到的 dt 不陈旧（含 `dt_update_interval` 场景）。
