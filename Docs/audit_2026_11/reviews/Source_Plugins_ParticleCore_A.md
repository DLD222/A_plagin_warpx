# Source_Plugins_ParticleCore_A 审查

分区：`Source/Plugins/ParticleCore/**`（原 `Source/Particles/*`、`Source/Initialization/*`、
`Source/AcceleratorLattice/*`、`Source/Filter/Filter.cpp`、`Source/ablastr/particles/DepositCharge.H`）
对照基线：`_upstream/warpX-26.10/`。方法：逐文件读 + `diff -u` 对照上游同名实现，只在**语义变化**处判定缺陷。

## 覆盖

| 文件 | 行数 | 类型 | 结论 |
|---|---|---|---|
| ParticleThermalizer.H | 70 | modified（改为 `IParticleThermalizer` 实现） | OK |
| ParticleThermalizer.cpp | 240 | modified（去 WarpX 单例、改 `TileIter`、简化 overlap 判定） | OK（物理与上游逐行一致） |
| ParticleThermalizerRegistration.cpp | 36 | **new** | OK |
| AcceleratorLattice/AcceleratorLattice.H | 88 | moved | OK |
| AcceleratorLattice/AcceleratorLattice.cpp | 123 | modified（`ParticleTileIter` + 2 条断言） | OK |
| AcceleratorLattice/LatticeElementFinder.H | 5 | moved（转发头 → `Modules/AcceleratorLattice/`） | OK |
| AcceleratorLattice/LatticeElementFinder.cpp | 195 | modified（模板化 iterator + 显式实例化 + 断言） | OK |
| LatticeElements/HardEdgedPlasmaLens.H | 4 | moved（转发头） | OK |
| LatticeElements/HardEdgedQuadrupole.H | 4 | moved（转发头） | OK |
| LatticeElements/HardEdged_K.H | 4 | moved（转发头） | OK |
| LatticeElements/LatticeElementBase.H | 4 | moved（转发头） | OK |
| DepositCharge.H | 213 | moved（原 `ablastr/particles/DepositCharge.H`） | OK（仅 include 改路径，实质内容未变） |
| Deposition/ChargeDeposition.H | 449 | moved | OK（diff 仅 include；RZ `2πr`/`1/r` 权重与上游一致） |
| Deposition/CurrentDeposition.H | 2718 | moved | OK（diff 仅 include；Villasenor/direct 电流沉积公式与上游一致） |
| Deposition/MassMatricesDeposition.H | 2231 | **modified（大改）**：新增 RSPHERE、`wqx/wqy/wqz` 上移、`diag_ncomp` 方形/非方形 PC 矩阵 | 可疑（见 U1/U2） |
| Deposition/SharedDepositionUtils.H | 267 | moved | OK |
| Deposition/TemperatureDeposition.H | 541 | moved | OK |
| Deposition/VarianceAccumulationBuffer.H | 49 | modified（`RuntimeContext` 注入 + `config()/data()/host()`） | OK |
| Deposition/VarianceAccumulationBuffer.cpp | 145 | modified（同上） | OK（`legacy_config{}` 默认构造安全：RuntimeConfig 是活视图） |
| ElementaryProcess/Ionization.H | 176 | modified（构造签名加 `RuntimeData/nox/...`） | OK |
| ElementaryProcess/Ionization.cpp | 88 | modified | **缺陷 F1**（`GetExternalEBField` 未传 context） |
| ElementaryProcess/QEDPairGeneration.H | 216 | modified（同上） | OK |
| ElementaryProcess/QEDPairGeneration.cpp | 76 | modified | **缺陷 F1** |
| ElementaryProcess/QEDPhotonEmission.H | 275 | modified（同上） | OK |
| ElementaryProcess/QEDPhotonEmission.cpp | 79 | modified | **缺陷 F1** |
| ElementaryProcess/VirtualPhotonCreation.H | 42 | moved（原 `Collision/BinaryCollision/`）+ 接口换 `IParticleContainerAccess` | OK |
| ElementaryProcess/VirtualPhotonCreation.cpp | 316 | moved + 新增 2 条输入断言 | OK |
| EmbeddedBoundary/ParticleBoundaryProcess.H | 176 | moved（thermal/specular 反射公式与上游一致） | OK |
| EmbeddedBoundary/ParticleScraper.H | 217 | moved（`ParticleTileIter` 替换） | OK |
| Filter/Filter.cpp | 332 | modified（load-balance cost 恒为 nullptr） | **缺陷 F2** |
| Gather/GetExternalFields.cpp | 122 | **modified（重写）** | **缺陷 F1** |
| Initialization/ExternalField.H | 320 | moved | OK |
| Initialization/ExternalField.cpp | 602 | modified（1 行语义：`query`→`get`） | **缺陷 F3** |
| Initialization/GetTemperature.H | 185 | **modified（新增 eV 温度通路）** | 可疑（U3） |
| Initialization/GetTemperature.cpp | 50 | modified | OK |
| Initialization/GetVelocity.H | 88 | moved | OK |
| Initialization/GetVelocity.cpp | 28 | moved | OK |
| Initialization/InjectorDensity.H | 237 | moved | OK |
| Initialization/InjectorMomentum.H | 632 | modified（Jüttner `theta = T_eV·q_e/mc²`） | OK（量纲核对通过） |
| Initialization/PlasmaInjector.H | 218 | moved | OK |
| Initialization/PlasmaInjector.cpp | 739 | modified（`q_tot/npart_real/focal_distance` 改 `queryWithParser`；RZ 模式数走 `legacy_config()`） | OK |
| Initialization/TemperatureProperties.H | 79 | **modified** | 可疑（U4） |
| Initialization/TemperatureProperties.cpp | 230 | **modified（重写）** | **缺陷 F4/F5** |

共 **43** 个文件，全部读毕（大文件用 `diff -u` 做逐行等价核对 + 分块阅读关键段）。

## 缺陷清单

### F1 `GetExternalEBField` 未拿到 `RuntimeContext` 时静默丢掉加速器晶格场，并在 parser 外场下空指针解引用
- 文件:行
  - `Source/Plugins/ParticleCore/Particles/Gather/GetExternalFields.cpp:30`（`ps` 只来自 `ctx->particles`，`ctx==nullptr` 即 `ps==nullptr`）
  - `GetExternalFields.cpp:38-42`（晶格分支对 `ps==nullptr` 做了保护 ⇒ `d_lattice_element_finder` 保持未初始化）
  - `GetExternalFields.cpp:68-73`、`76-81`（parser 分支**没有**判空：`ps->externalFieldParser("Ex")->compile<4>()`）
  - 触发点（本分区文件）：`ElementaryProcess/Ionization.cpp:62`、`ElementaryProcess/QEDPairGeneration.cpp:50`、`ElementaryProcess/QEDPhotonEmission.cpp:53` 均只传 `(a_pti, a_offset)`；构造函数第 3 个参数 `ctx` 的默认实参就是 `nullptr`（`Modules/Particles/Gather/GetExternalFields.H:29-31`）
- 严重度: **critical**
- 置信度: confirmed
- 现象与依据:
  - 上游该构造函数内部自己取容器（`_upstream/warpX-26.10/Source/Particles/Gather/GetExternalFields.cpp:18-38`：
    `auto& mypc = warpx.GetPartContainer(); ... warpx.get_accelerator_lattice(lev)`），因此**与调用者用什么 iterator、传不传 context 无关**。
    fork 改成"容器只能从 context 来"（注释 `GetExternalFields.cpp:28-29`），但三个调用点没跟着改。
  - 后果 1（静默物理错误，更危险）：`lattic_ptr == nullptr` ⇒ `d_lattice_element_finder.m_initialized == false` ⇒
    `operator()` 里的 `if (d_lattice_element_finder.m_initialized)`（`Modules/Particles/Gather/GetExternalFields.H:76-79`）整段跳过。
    `Ionization.H:108-124`（外部场在 `:118` 叠加）把晶格/外场的 E/B **叠加**到网格 gather 到的 E/B 上再算 ADK/隧穿场强，
    `QEDPhotonEmission.H:148-166`（`:159`）同理用于算光深。所以 "accelerator lattice + ionization/QED" 组合下，
    电离率/光子发射率用的是**不含晶格场**的场强，程序不报错。上游同一组合是对的。
  - 后果 2（崩溃）：`E_ext_particle_s`/`B_ext_particle_s = "parse_e/b_ext_particle_function"` 时，
    `cfg` 走 `legacy_config()` 回退仍会返回真实字符串（`RuntimeConfig` 是活视图，见 `Source/Core/Runtime/RuntimeConfig.H:22-30`），
    于是进入 parser 分支并在 `ps == nullptr` 上解引用 → segfault。若 `externalFieldParser()` 返回 `nullptr`
    （`MultiParticleContainer.cpp:1297-1307` 在未配置时确实返回 nullptr），是第二处空指针。
  - 一致性证据：其它全部调用点都传了 context（`PhysicalParticleContainer.cpp:1263,1459`、`PhotonParticleContainer.cpp:149`、
    `RigidInjectedParticleContainer.cpp:351`、`ImplicitPushPX.cpp:466,803`、`WarpXParticleContainer.cpp:1204`、
    `ReducedDiags/ParticleExtrema.cpp:297`、`ColliderRelevant.cpp:469`），而 `PhysicalParticleContainer::getIonizationFunc`
    （`PhysicalParticleContainer.cpp:1744-1776`）本身就能访问 `runtime_context()`，只是没往下传。
- 建议修法: 三处调用改为 `GetExternalEBField(a_pti, a_offset, runtime_context())`（QED 两个 functor 需要把 context 作为
  构造参数从 `MultiParticleContainer.cpp:2288/2368` 一路带下去）；并在构造函数里加
  `WARPX_ALWAYS_ASSERT_WITH_MESSAGE(ps != nullptr || (cfg.E_ext_particle_s() == "none" && ... ), ...)` 与 parser 指针判空，
  避免"静默降级"。
- 验证方法:
  - 崩溃：`ctest -R load_external_field` 覆盖不到 QED；最小用例：`algo.maxwell_solver=yee` + 任一 species 开
    `do_ionization=1`，再设 `particles.E_ext_particle_s = parse_e_ext_particle_function` + `E_ext_particle_function(x,y,z,t)`。
  - 静默丢场：`Examples/Tests/accelerator_lattice` 的输入里给某 species 打开 `do_ionization`，
    对比 `IonizationFilterFunc` 里的 `E`（`Ionization.H:134-140`）在有无 `lattice.elements` 时是否相同 —— 现在会相同（错误）。

### F2 `Filter::ApplyStencil` 把 load-balance cost 恒置为 `nullptr`，负载均衡计时统计被静默删除
- 文件:行 `Source/Plugins/ParticleCore/Particles/Filter/Filter.cpp:44-46`、`206-208`（另有 4 处 `if (cost && ...)` 成为死代码：`:50`、`:63`、`:218`、`:237`）
- 严重度: **minor**
- 置信度: confirmed
- 现象与依据: 上游 `_upstream/warpX-26.10/Source/Filter/Filter.cpp:43`、`:204` 为
  `amrex::LayoutData<amrex::Real>* cost = WarpX::getCosts(lev);`；fork 改成
  `amrex::LayoutData<amrex::Real>* cost = nullptr;`，注释写"没有 level resources 就没什么可记的"，
  但同一文件仍然调用 `warpx::runtime::legacy_config().load_balance_costs_update_algo()`，说明成本数组并非不可得。
  后果：`LoadBalanceCostsUpdateAlgo::Timers` 下滤波耗时不再计入 cost，`amrex::AmrMesh` 的负载均衡会低估有滤波的
  block 的代价（物理结果不变，仅影响 load balance 质量）。注释与代码不一致也属于协议列出的"注释保证 X 但没做"。
- 建议修法: 用与 `getExternalEB` 同一套 host services（`host()` 里找出 cost 的访问器）恢复
  `cost = <host services>.getCosts(lev)`；若确实有意移除，则删掉死分支并改注释。
- 验证方法: `ctest -R load_balance`（若存在）或在 input 里设 `algo.load_balance_costs_update = timers`，
  对比 `warpx.costs` 输出在滤波开启前后是否变化（现在不变化）。

### F3 `warpx.read_fields_from_path` 由可选变成必需（`query` → `get`）
- 文件:行 `Source/Plugins/ParticleCore/Particles/Initialization/ExternalField.cpp:185-188`
- 严重度: **minor**
- 置信度: confirmed
- 现象与依据: 上游 `_upstream/warpX-26.10/Source/Initialization/ExternalField.cpp:187-188` 是
  `pp_warpx.query("read_fields_from_path", external_fields_path);`（缺省留空，同一作用域还声明了未使用的
  `read_fields_from_path="./"` 默认值）。fork 改为 `pp_warpx.get(...)`。`amrex::ParmParse::get` 在缺失时会 abort，
  因此所有 `E/B_ext_grid_init_style = read_from_file` 且没写该参数的输入（旧行为可跑）现在直接终止。
  文档 `Docs/source/usage/parameters.rst:2799,2835` 写的是 "must be specified"，所以是"代码向文档收敛"，
  但对既有输入是行为破坏，且按 `AGENTS.md` 的 Backward Compatibility 约定应有 guard 或至少 warning。
- 建议修法: 恢复 `query` + 显式默认（`"./"`）并在为空时 `WARPX_ALWAYS_ASSERT_WITH_MESSAGE(!external_fields_path.empty(), ...)`，
  或保留 `get` 但在 `WarpX::BackwardCompatibility()` 里给出可操作的错误信息。
- 验证方法: 任取 `Examples/Tests/load_external_field/inputs_test_3d_load_external_field_grid_picmi.py`，删掉
  `warpx.read_fields_from_path` 后运行：fork 立即 abort，上游会用 `./` 继续。

### F4 eV 温度的 `mass`/`species_type` 解析绕开了 `source_name`，且与 `injection_file` 提供的质量不兼容
- 文件:行 `Source/Plugins/ParticleCore/Particles/Initialization/TemperatureProperties.cpp:40`（`pp.query("species_type", ...)`）、
  `:49`（`utils::parser::queryWithParser(pp, "mass", mass)`）、`:50-52`（`mass > 0` 断言）
- 严重度: **minor**
- 置信度: likely
- 现象与依据: 同文件其它所有参数都走 `(pp, source_name, "name", v)` 形式（`ParserUtils.H:221-241` 会同时查
  `"<source>.name"` 和 `"name"`），只有这两个走 3 参数版本，只查 `"name"`。
  - 情形 A：多 source 物种写成 `<species>.<source>.mass` 时，`temperature_in_eV` 转换拿不到质量 → 直接 abort；
  - 情形 B：`PlasmaInjector` 允许质量来自 `injection_file` 的 `mass` record（`PlasmaInjector.cpp:514,539-559`，
    `queryMass()` 见 `:655-661`），此时 `m_q_e_over_mc2` 无法建立 → `:50` 断言失败。上游的 `theta` 是无量纲量，
    不需要质量，所以这类输入过去是可跑的。这属于新特性的能力缺口，而不是上游行为的回归。
  - 另：`body`（`:44-47`）对 `physical_species == photon` 的拒绝只在用户写了 `species_type` 时生效。
- 建议修法: 改为 `utils::parser::query(pp, source_name, "species_type", ...)` 与
  `utils::parser::queryWithParser(pp, source_name, "mass", mass)`；若两者都取不到质量，回退到
  `PlasmaInjector::queryMass()` 的结果（把 `PlasmaInjector` 已算出的质量通过构造函数传入，避免两条独立的质量来源）。
- 验证方法: 用 `Examples/Tests/initial_distribution/inputs_test_3d_initial_distribution` 的
  `maxwell_juttner_low_theta` 物种，删掉 `mass = m_e` 而只留 `species_type`（应通过）→ 再改成
  `injection_file` 提供质量（当前 abort）。

### F5 `maxwell_juttner` 的 `theta` 输入被删除，没有 BackwardCompatibility guard
- 文件:行 `Source/Plugins/ParticleCore/Particles/Initialization/TemperatureProperties.cpp:131-137`（只认
  `maxwell_juttner_temperature_in_eV_distribution_type` / `temperature_in_eV`，见 `:54-70`）；
  `Initialization/InjectorMomentum.H:267-268`（`theta = temperature(x,y,z)*m_q_e_over_mc2`）
- 严重度: **minor**
- 置信度: confirmed
- 现象与依据: 上游 `_upstream/.../Initialization/TemperatureProperties.cpp:38-70` 读的是
  `<species>.theta_distribution_type` + `<species>.theta`（无量纲 `kT/mc²`）。fork 里 `Source/` 下已找不到任何
  `theta` 温度参数的解析（仅剩 `particle_thermalizer.theta` 与几何记号）。换算本身是对的：
  `theta = T_eV·q_e/(m c²)`（`WarpXConst` SI，核对：1 eV / 511 keV = 1.957e-6 = `q_e·1/(m_e·c²)`），
  Maxwellian 的 `u_std = sqrt(theta)`（`GetTemperature.H:20-30`）也与文档
  `Docs/source/usage/parameters.rst:2018-2019` 一致；fork 自己的 Examples/Docs 已迁移
  （`Examples/Tests/initial_distribution/inputs_test_3d_initial_distribution:97-98`）。所以这是**有意的 API 变更**，
  但按仓库约定 `AGENTS.md`「Backward Compatibility」应在 `PhysicalParticleContainer::BackwardCompatibility()`
  （或 `WarpX::BackwardCompatibility()`）里对 `theta`/`theta_distribution_type` 给出迁移提示，否则老输入只得到
  一句 "Temperature parameter temperature_in_eV not specified"。
- 建议修法: 在该函数里 `if (pp.contains("theta")) WARPX_ABORT_WITH_MESSAGE("theta was replaced by temperature_in_eV; theta = T_eV*q_e/(m c^2)")`。
- 验证方法: 用上游 26.10 的任一 `maxwell_juttner` 输入（含 `theta = 1.`）运行 fork：现在 abort 且无迁移提示。

## 不确定但值得复核的点

- **U1（MassMatricesDeposition.H，2D 分段核）** `:1447-1456`
  `diag_width_zz0 = diag_width_xx1;`（zz 的 x 方向半宽取的是 **yy** 的宽度，`:1455`），`:1475-1490` 起
  `comp_zz = diag_col + diag_ncomp_zz0*diag_row;`
  列步长用 `ncomp_zz[0]`、行宽用 `ncomp_yy[0]`（`:1447`）。当前所有配置下二者恰好相等
  （Direct: 全部 `1+2*shape`，`ImplicitSolver.cpp:848-857`；Villasenor XZ: `ncomp_yy[0]=1+2*shape+2mgc`、
  `ncomp_zz[0]=1+2*shape+2mgc`，`ImplicitSolver.cpp:939-947`），我用
  `ImplicitSolver.cpp:1157-1215`（`SyncMassMatricesPCAndApplyBCs`）的 `diag_comp_xx + ii + N0*(jj + N1*kk)` 反推过
  3D/Direct 的 `Σ = width*(1+n+n²) == (n³-1)/2`，索引一致。但这是"巧合相等"，一旦将来 RZ 的 Sxx/Szz 行数不同就会串列。
  建议改为 `diag_width_zz0 = (ncomp_zz[0]-1)/2`。
- **U2（MassMatricesDeposition.H，PC-only 路径语义变化）** 上游在 `full_mass_matrices==false` 时只写
  一个"把整个 stencil 求和后"的单分量（`Sxx_arr(...,0) += weight*fpxx`），fork 改写成 `(diag_width+ii) + ncomp*(...)`
  的完整方形 stencil、并且**两半都写**（`if (!full_mass_matrices || ii+jj <= -above_diag)`）。
  与消费端一致（`ImplicitSolver.cpp:1197-1220` 按 `MM_PC_ncomp_xx` 解码、`:985-1000` 分配 `ncomp_tot_pc_xx`），
  也能编译/自洽，但这是**预条件器数值的改变**（1 分量 → 方形 stencil），需要回归测试确认收敛行为：
  建议 `ctest -R "implicit.*(1d|2d|rz)"` 对比上游 checksum，并单独看 `implicit_evolve.mass_matrices_pc_width`
  >0 与 =0 两个分支。另注意 1D 径向/1D-Z 分支里 `full_width_*` 与 `diag_*` 混用
  （`MassMatricesDeposition.H:1738-1741`、`:1918-1921`：`width_xx = full ? full_width_xx : (ncomp_xx[0]-1)/2`，
  `comp_diag = width_xx + comp_full - full_width_xx`），以及 `diag_ncomp = ncomp_yy[0]` 被当作 **Sxx 的行数**
  （`:1447` 定义、`:1476-1477` 使用），这层"Sxx 行数 == Syy 宽度"的耦合应加注释或断言。
- **U3（GetTemperature.H）** `GetTemperatureVector::operator()` 把 `TempConstantValue/TempParserFunction/TempFromFileValue`
  统一走 `ConvertTemperatureInEvToUStd`（`:163-169`，返回**各向同性**），而 `TempConstantVector` 保留各向异性。
  即 `maxwellian` 一旦用 eV 温度就只能各向同性（文档 `parameters.rst:2042-2058` 未明说），
  若有人期望 `temperature_in_eV` + 各向异性，会静默得到各向同性结果。
- **U4（TemperatureProperties.H/.cpp）** `use_temperature_in_eV`/`use_u_std` 用 `pp.contains("...")` +
  `pp.contains(source_name + ". ...")` 双重探测（`:139-151`）；若同物种的两个 source 一个用物种级（无前缀）
  `maxwellian_u_std_distribution_type`、另一个用 `<source>.maxwellian_temperature_in_eV_...`，会误触发
  "Cannot specify both" 断言。当前 Examples 未这样写，属边界问题。
- **U5（MassMatricesDeposition.H，RSPHERE）** `setMassMatricesKernels` 新增的 RSPHERE 旋转（`:143-190`）
  我用 `(R F Rᵀ)` 与上游 `doDirectJandSigmaDepositionKernel` 里被删掉的 RSPHERE 速度旋转
  （`_upstream/.../MassMatricesDeposition.H:208-226`：`wqx=+vx·cosθcosφ+vy·sinθcosφ+vz·sinφ` 等）逐项对过，
  第一行/第一列与 `[cpct, cpst, sinph]` 一致，符号无翻转；同时 fork 在 `doDirectJandSigmaDepositionKernel`
  （`:259-262`）保留了 `rpmid`（供 `xmid=(rpmid-xyzmin.x)·dinv.x` 使用，`:273`），未变成死代码。此项倾向 OK，
  但 RSPHERE 的隐式求解+质量矩阵目前没有测试覆盖，建议至少在 `ctest -N | grep -i sphere` 里确认存在用例。
