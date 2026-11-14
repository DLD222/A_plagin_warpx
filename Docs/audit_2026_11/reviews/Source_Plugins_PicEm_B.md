# Source_Plugins_PicEm_B 审查

分区：`Source/Plugins/PicEm` 后半（RuntimeBridge、MomentsSolver 测试、并行化/边界/Python 绑定、WarpX.cpp/H、WarpXKernel）。
参考：`_upstream/warpX-26.10`（单一 release commit `ec1660d`，`git describe` = 26.10）。
方法：清单 53 个文件逐个读完；与上游同名文件 `diff -u` 对照（大文件逐个 hunk 读），新写文件全文读；可疑点用数值反例/调用序/caller 约定验证。

## 覆盖

| 文件 | 行数 | 类型 | 结论 |
|---|---|---|---|
| Modules/MomentsSolver/Tests/test_coupling_nd.cpp | 216 | new(本地 MomentsSolver) | 缺陷 F5 |
| Modules/MomentsSolver/Tests/test_em_dispersion.cpp | 79 | new | OK |
| Modules/MomentsSolver/Tests/test_eos.cpp | 111 | new | OK（失败会 return 1） |
| Modules/MomentsSolver/Tests/test_euler_sod.cpp | 99 | new | OK |
| Modules/MomentsSolver/Tests/test_friction.cpp | 142 | new | OK |
| Modules/MomentsSolver/Tests/test_fv_nd.cpp | 306 | new | 缺陷 F5 |
| Modules/MomentsSolver/Tests/test_gauss_conservation.cpp | 191 | new | OK（唯一做对照的测试：naive J 必须漂移） |
| Modules/MomentsSolver/Tests/test_langmuir.cpp | 81 | new | OK |
| Modules/MomentsSolver/Tests/test_maxwell.cpp | 68 | new | OK |
| Modules/MomentsSolver/Tests/test_ten_moment.cpp | 57 | new | OK |
| Modules/MomentsSolver/Tests/test_ten_moment_k0.cpp | 60 | new | OK |
| Modules/MomentsSolver/Tests/test_ten_moment_wave.cpp | 72 | new | OK |
| Modules/MomentsSolver/Tests/test_yee_deposition.cpp | 204 | new | 缺陷 F5 |
| Modules/MomentsSolver/Tests/run_tests.sh | 13 | new | 缺陷 F6 |
| Modules/Parallelization/GuardCellManager.cpp | 413 | moved | OK（与上游差异仅 include 路径） |
| Modules/Parallelization/WarpXComm.cpp | 1978 | modified | 缺陷 F1 |
| Modules/Parallelization/WarpXRegrid.cpp | 464 | modified | 缺陷 F2；另见"待复核" |
| Modules/Parallelization/WarpXSumGuardCells.cpp | 37 | moved | OK（仅 1 行 include） |
| Modules/PicEm/Deposition/DepositionHandler.H/.cpp | 26/40 | new | OK |
| Modules/PicEm/FieldSolve/FieldSolveHandler.H/.cpp | 33/40 | new | OK（m_name 仅用于 spec，apply 走统一 fieldSolve，可接受） |
| Modules/PicEm/FieldSolve/Yee/YeeFieldSolveHandler.H/.cpp | 26/40 | new | OK |
| Modules/PicEm/FieldSolve/Yee/YeePackage.H/.cpp | 24/23 | new | OK |
| Modules/PicEm/ParticlePush/ParticlePushHandler.H/.cpp | 26/40 | new | 缺陷 F9（metadata） |
| Modules/PicEm/PicEmPackage.H/.cpp | 32/71 | new | OK |
| Modules/PicEm/RuntimeConfigSnapshotBridge.cpp | 52 | new | OK（快照 vs 活视图的语义见待复核） |
| Modules/Python/ImplicitSolver.cpp | 36 | moved/new | OK（薄包装，上游 ImplicitSolver 实现在别处） |
| Modules/Python/Particles/MultiParticleContainer.cpp | 76 | modified | OK（dynamic_cast 收窄，前提"PIC kernel 一定是该类型"成立） |
| Modules/Python/Particles/WarpXParticleContainer.cpp | 180 | modified | OK（新增 finalize 形参） |
| Modules/Python/WarpX.cpp | 305 | modified | OK（绑定改走 contract；`deposit_mass_matrices` 新增 dt 形参） |
| Modules/Python/callbacks.cpp | 59 | modified | 可疑但无缺陷（见待复核 3） |
| Modules/Python/pyWarpX.cpp | 140 | modified | 缺陷 F3 |
| RuntimeBridge/IO/WarpXCheckpoint.cpp | 409 | moved(=Diagnostics/WarpXIO.cpp) | 可疑（见待复核 4） |
| RuntimeBridge/Runtime/CollisionPackage.H/.cpp | 64/28 | new | OK |
| RuntimeBridge/Runtime/FrameState.cpp | 30 | new | OK（每次调用刷新，正确） |
| RuntimeBridge/Runtime/LegacyRuntimeFactory.cpp | 23 | new | OK（工厂用 `WarpX::GetInstance()`，按调用取址） |
| RuntimeBridge/Runtime/LevelResources.H/.cpp | 94/39 | new | OK（`clear` 归零整个 LevelStorage，含 pml） |
| RuntimeBridge/Runtime/RuntimeConfig.cpp | 382 | new | OK（全部 getter 转发 `WarpX::` 静态量，本身无状态） |
| RuntimeBridge/Runtime/WarpXHostServices.H/.cpp | 131/415 | new | 缺陷 F8；另见待复核 1 |
| RuntimeBridge/Runtime/WarpXRuntimeData.H/.cpp | 130/181 | new | 缺陷 F10；`Legacy()` 刷新逻辑正确 |
| RuntimeBridge/Utils/WarpXMovingWindow.cpp | 689 | moved(=Utils/) | OK（`UpdateInjectionPosition` 迁至 ParticleCore，实现与上游逐行一致） |
| WarpX.H | 1969 | modified | OK（以声明重构为主，见正文） |
| WarpX.cpp | 4094 | modified | 缺陷 F4、F7；其余为 include/工厂化重构 |
| WarpXKernel.H | 43 | new | OK |
| WarpXKernel.cpp | 205 | new | OK（单例/生命周期见 F4 待复核 6） |

## 缺陷清单

### F1 radial 几何下 rho 的 volume-weighted filter 用了错误的标定分支
- 文件:行 `Source/Plugins/PicEm/Modules/Parallelization/WarpXComm.cpp:1613-1620`（分支体）、`:1872-1874`（调用点传 `is_volume_scaled=false`）
- 严重度: major
- 置信度: confirmed（公式 + 数值反例 + 全仓库调用序）
- 现象与依据:
  - 上游同一函数只有一支：`v = u + (0.25/w0)*(w_hi*(u_{i+1}-u_i) - w_lo*(u_i-u_{i-1}))`（`_upstream/warpX-26.10/Source/Parallelization/WarpXComm.cpp:1609-1613`）。当 `u` 是**未做体积标定的密度**时，该式等价于对电荷 `q=w*u` 做守恒通量更新（Σw·Δu 逐面望远镜抵消），且常量密度不动。
  - fork 新增的 `is_volume_scaled=false` 分支改写成 `density = u/w0` 后差分（`u_{i+1}/pw(i+1) - u_i/pw(i)`），只有当 `u` 已被体积因子标定（`u = w·ρ`）时才是守恒通量；而 fork 的 rho 在**所有** filter 调用点都是未标定的（见下）。
  - 调用序证明 rho 未标定：`MultiParticleContainer::DepositCharge` 已不再调用 `ApplyInverseVolumeScalingToChargeDensity`（`grep` 在 `Plugins/ParticleCore` 内为 0 命中；上游 `MultiParticleContainer.cpp:648-654` 有），改由 `WarpX::FinalizeRhoOnLevel`（`Modules/Evolve/WarpXEvolve.cpp:1251-1260`）在主循环 `SyncCurrentAndRho()`（filter+sum）**之后**才做；`WarpXHostServices::computeRho`(118-119)、`EffectivePotentialES.cpp:170-172`、`WarpXComm.cpp:1394/1423` 全部是"先 filter 后 finalize"。同一时刻 J 已经过 `ApplyInverseVolumeScalingToCurrentDensity`（`WarpXEvolve.cpp:1866-1874`），所以 J 仍用默认 `true`、rho 传 `false`——两个都是未标定密度的场被用两套约定处理，必有一套错。
  - 数值反例（dr=1，cell-centered r=(i+½)，常量密度 ρ≡1，3 点通量滤波器）：
    `i=1: 上游式 → 1.000000，fork 的 false 分支 → 1.200000`；`i=2 → 1.047619`；`i=3 → 1.022222`（近轴 O(1) 伪电荷，正是 RZ 最敏感的区域）。
  - 可达性（重要）：RZ/RSPHERE 下 FDTD 默认 `use_filter=false`，且 `Warpx.cpp:1081-1083` 断言 `filter_npass_each_dir[0]==0`（`HybridPIC` 例外，`Warpx.cpp:1076` 的 else 分支不检查）。所以径向 pass 只在 **RZ + HybridPIC + use_filter=1 + filter_npass_each_dir[0]≥1** 时发生；此时 dir=0 sweep 会走到 false 分支，轴向 sweep（`WarpXComm.cpp:1622-1637`）两分支同式、不受影响。
  - 另有文档/代码自相矛盾：`WarpX.H:1105-1107` 形参名 `is_volume_scaled`，而 false 分支的注释写 "u = volume factor * density"（即 u **是**标定的），语义与命名相反。
- 建议修法: 最小改动是把 `WarpXComm.cpp:1872-1873` 改回默认 `true`（rho 未标定、与上游公式一致），并让 false 分支要么删除、要么改名为 `input_is_volume_scaled` 并在 `WarpX.H` 注明调用点必须传标定后的场。
- 验证方法: `ctest -R rz_hybrid`（或任意 RZ + HybridPIC + `warpx.use_filter=1` + `warpx.filter_npass_each_dir=1 0` 用例）输出 rho 诊断，与 `is_volume_scaled=true` 对比首列数值；或加一次性打印：对 ρ≡const 的初始化跑 1 步后比较 cell i=1 的 rho。

### F2 每次 load-balance/regrid 都重建 PML，PML 内的场被清零
- 文件:行 `Source/Plugins/PicEm/Modules/Parallelization/WarpXRegrid.cpp:381`（`if (do_pml) { CreatePMLForLevel(lev, ba, dm); }`，在 `RecreateLevelResources` 内）；实现见 `Modules/Initialization/WarpXInitData.cpp:1196-1213`
- 严重度: major
- 置信度: likely（代码路径确定；未运行）
- 现象与依据:
  - `CreatePMLForLevel` 先 `m_level_resources.level(lev).pml.reset()`，再遍历 `m_fields.list()` 把本层所有 `pml_*` 字段 `erase_internal`，然后重新构造 `PML`/`PML_RZ`（构造函数重新 `alloc_init` 这些字段 → 初值 0）。
  - 上游 `RemakeLevel` 完全不做这件事（`_upstream/.../WarpXRegrid.cpp:189-330` 无 PML 重建），只 `m_fields.remake_level(lev, dm)`；而 `MultiFabRegister::remake_level`（`Source/Core/ablastr/fields/MultiFabRegister.cpp:273-329`）是**保数据**的（`m_redistribute_on_remake` 时 `new_mf.Redistribute(mf,...)`）。上游 `PML.H` 不缓存 `MultiFab*`（每次经 register 取），所以重映射无需重建 PML。
  - 因此 fork 这个新增调用在任何 `dm` 变化的 load balance（`WarpXRegrid.cpp` `LoadBalance()` → `RemakeLevel(lev, …, newdm)`，`ba` 不变也进 `RecreateLevelResources`，见 `:188-196`）都会把 PML 区域的 E/B/F/G 清零，形成一次"PML 内场瞬间归零"的静默源/反射。regrid（`ba` 变）时该重建是合理的，LB 时不是。
- 建议修法: 只在 `ba != boxArray(lev)`（即 `RemakeLevel` 的 else 分支 / 新增层）里调用 `CreatePMLForLevel`；`ba` 不变的 LB 路径交给 `m_fields.remake_level` 保数据重映射。
- 验证方法: 3D PML 用例（如 `Examples/Physics_applications/laser_acceleration` + `warpx.load_balance_intervals=1`）对比 `do_pml=1` 与 `do_pml=0` 的场能量曲线；或在 `CreatePMLForLevel` 加一行打印，确认 LB 步被调用；最直接：LB 步前后比较 `pml_E_fp` 的 max，若为 0 则证实。

### F3 Python 绑定类名 `WarpXParIter` → `ParticleTileIter`，pywarpx 调用点未同步
- 文件:行 `Source/Plugins/PicEm/Modules/Python/pyWarpX.cpp:70`（`init_ParticleTileIter(m)`）、`:43`（声明）；`Source/Plugins/PicEm/Modules/Python/Particles/WarpXParticleContainer.cpp:14-19`（`py::class_(..., "ParticleTileIter")`）
- 严重度: major
- 置信度: confirmed
- 现象与依据:
  - 上游注册名是 `"WarpXParIter"`（`_upstream/.../Python/Particles/WarpXParticleContainer.cpp:13-16`）；`Python/pywarpx/particle_containers.py` 仍以 `libwarpx.libwarpx_so.WarpXParIter(...)` 遍历粒子 tile，共 3 处：`:166`（`get_particle_real_arrays`）、`:211`（`get_particle_int_arrays`）、`:250`（`get_particle_idcpu_arrays`）。`grep -rn ParticleTileIter Python/` 为 0 命中 → 这三个公开 API 在运行时必然 `AttributeError`。
  - `Tools/Modularization/check_core_purity.py:24` 也仍写着 `WarpXParIter`（只影响脚本，非运行时）。
- 建议修法: 二者取一——(a) 绑定名保持 `"WarpXParIter"`（保留 C++ 类型名 `ParticleTileIter`，只改 `py::class_` 的字符串，兼容性最好）；或 (b) 同时改 3 处 Python 调用点并加 `.pyi`/文档更新。
- 验证方法: `python -c "import pywarpx, libwarpx; print(hasattr(libwarpx.libwarpx_so,'WarpXParIter'))"`；或跑任一用 `get_particle_real_arrays` 的分析脚本（如 `Examples/Tests/PICMI/...` 的 particle diag）。

### F4 legacy 宿主/数据适配器用函数局部 `static` 冻结了第一个 WarpX 实例
- 文件:行 `Source/Plugins/PicEm/WarpX.cpp:392-403`
- 严重度: major
- 置信度: likely（未跑二次初始化用例）
- 现象与依据:
  - 注册的 provider 是 `[]() -> RuntimeData& { static WarpXRuntimeData legacy(WarpX::GetInstance()); return legacy; }` 与 `... static WarpXHostServices legacy(WarpX::GetInstance()); ...`。函数局部 static 只在**首次调用**时绑定 `WarpX&`，之后永远返回同一对象；`WarpXHostServices` 持有成员 `WarpX& m_warpx`（`WarpXHostServices.H:126`），`WarpXRuntimeData` 持有 `WarpX* m_warpx`（`WarpXRuntimeData.H:123`）。`WarpX::ResetInstance()`/`WarpXKernel::clear()` 之后再次构造 WarpX（同进程二次 run、pywarpx finalize→re-init、单元测试）→ 这两个 provider 返回的适配器指向已析构对象。
  - 同一文件里 `set_legacy_frame`（`:438-442`）**每次调用都刷新**，注释还专门写了这个陷阱；`WarpXRuntimeData::Legacy()`（`WarpXRuntimeData.cpp:28-44`）也刻意每次刷新 `m_warpx`。也就是说刷新机制已存在，只是注册点没用它。
  - 消费面不小：`legacy_data()` 被 `LatticeElementFinder.cpp:37,72`、`MultiParticleContainer.cpp:2477`、`WarpXParticleContainer.cpp:3212`、`ParticleBoundaryBuffer.cpp:632`、`ResamplingTrigger.cpp:58`、`VarianceAccumulationBuffer.cpp:137`、`GetExternalFields.cpp:25` 等在无 RuntimeContext 时使用。
- 建议修法: 把 provider 改成直接 `return warpx::runtime::WarpXRuntimeData::Legacy();` 与一个 `Legacy()` 风格的、每次重建/刷新的 host 适配器（例如 `static thread_local WarpXHostServices h(WarpX::GetInstance())` 仍需刷新 → 改为 `thread_local std::unique_ptr` + 每次 `m_warpx = &WarpX::GetInstance()`，或直接返回每次 `WarpX::GetInstance()` 构造的轻量适配器）。
- 验证方法: 同一进程内 `WarpX::MakeWarpX(); … ; WarpX::ResetInstance(); WarpX::MakeWarpX();` 后调用 `warpx::runtime::legacy_host()`/`legacy_data()` 任一方法，ASAN 下会报 use-after-free / heap-use-after-free。

### F5 三个 MomentsSolver 测试二进制永远返回 0，FAIL 只打印不生效
- 文件:行 `Modules/MomentsSolver/Tests/test_yee_deposition.cpp:203`（4 项检查全 FAIL 也只 `return 0`）、`test_coupling_nd.cpp:216`（3 项）、`test_fv_nd.cpp:306`（6 项）
- 严重度: minor（测试门禁；但覆盖的是新模块的全部几何/守恒判据）
- 置信度: confirmed
- 现象与依据: 三者都在 `std::printf("FAIL: ...")` 之后走 `return 0`；`run_tests.sh` 用 `set -e` 判据就是退出码（`:3,:12`）。对比同目录 10 个测试都正确 `return ok ? 0 : 1`，例如 `test_gauss_conservation.cpp:190`。`test_fv_nd`/`test_coupling_nd` 的核心断言是"2D/3D 必须与 1D **逐位**相同（`worst == Real(0)`）"和"x↔y 转置对称逐位相同"，这类断言极易因循环顺序/归约顺序而失败，静默通过等于该模块没有几何回归门禁。
- 建议修法: 累积 `int fails`（或 `bool ok`）并在末尾 `return fails ? 1 : 0`；`== 0` 的逐位断言建议保留但把失败信息写进 stderr。
- 验证方法: `./test_fv_nd; echo $?` 加上故意扰动（例如把测试里 `dx` 与 `dy` 改成不等）后仍返回 0 即可复现。

### F6 `run_tests.sh` 的路径在插件化移动后失效
- 文件:行 `Modules/MomentsSolver/Tests/run_tests.sh:4`（`cd "$(dirname "$0")/../../.."  # -> ~/WarpX_fluid`）、`:6`（`-ISource -ISource/MomentsSolver`）、`:11`（`Source/MomentsSolver/Tests/test_${t}.cpp`）
- 严重度: minor
- 置信度: confirmed
- 现象与依据: 目录已从 `Source/MomentsSolver` 移到 `Source/Plugins/PicEm/Modules/MomentsSolver`（commit `ff2e9dc` "split project-owned sources into Core and Plugins/PicEm"）。`ls Source/MomentsSolver` 不存在 → 脚本在 `set -e` 下第一条 `g++` 就失败；同时 `cd ../../..` 现在落在 `Source/Plugins`。
- 建议修法: 改为仓库根 + `-ISource/Plugins/PicEm/Modules -ISource/Plugins/PicEm/Modules/MomentsSolver`，源文件用 `Source/Plugins/PicEm/Modules/MomentsSolver/Tests/...`；或改成 CMake/CTest 目标（当前 `grep run_tests` 在 CMakeLists 里 0 命中，这 13 个测试没有被 ctest 注册）。
- 验证方法: `cd Source/Plugins/PicEm/Modules/MomentsSolver/Tests && sh run_tests.sh` → 立即报找不到源文件。

### F7 `GetPartContainer()` 无空指针保护，与 `particleSystem()` 的设计不一致
- 文件:行 `Source/Plugins/PicEm/WarpX.cpp:369-377`
- 严重度: minor
- 置信度: likely
- 现象与依据: `particleSystem()`（`:358-367`）在 `mypc == nullptr`（无 particle 插件的 field-only 构建）时返回 `null_particle_system()`，注释明确说明"a run without the particle plugin gets an empty system rather than a null dereference"；但紧接着的 `GetPartContainer()` 是 `return static_cast<MultiParticleContainer&>(*mypc);`（`const` 版同），`mypc` 为 null 时是 UB。`WarpX.H` 的注释把使用者限定为"tile iteration + Python 绑定"，而 Python 绑定（`Modules/Python/Particles/*`、`pywarpx` 的 container 访问）在 field-only 构建里也可能被调用。
- 建议修法: 加 `AMREX_ALWAYS_ASSERT(mypc != nullptr)` 或 `WARPX_ABORT_WITH_MESSAGE("...particle plugin not linked...")`，把静默 UB 变成明确报错。
- 验证方法: `-DWarpX_PLUGIN_PARTICLECORE=OFF` 构建后调用任一走 `GetPartContainer()` 的 Python 入口，观察是 abort 还是崩溃/静默。

### F8 诊断 rho 现在也走 `FinalizeRhoOnLevel`（含 PEC/反射边界清零），与上游诊断语义不同
- 文件:行 `Source/Plugins/PicEm/RuntimeBridge/Runtime/WarpXHostServices.cpp:118-119`
- 严重度: minor
- 置信度: likely
- 现象与依据: 上游 `RhoFunctor` 是"deposit → `ApplyFilterandSumBoundaryRho` → (RZ PSATD k-space filter) → `InterpolateMFForDiag`"（`_upstream/.../Diagnostics/ComputeDiagFunctors/RhoFunctor.cpp:33-79`）；rho 的 RZ 体积标定发生在 `DepositCharge(..., apply_boundary_and_scale_volume=true)` 内，而对 Cartesian 才额外做 `ApplyRhofieldBoundary`（`_upstream/.../Particles/WarpXParticleContainer.cpp:1916-1930`：`#if !RZ` 才反射边界）。fork 的 `computeRho` 在 filter+sum 之后调用 `FinalizeRhoOnLevel`，而后者 = `ApplyRhofieldBoundary` + `ApplyInverseVolumeScalingToChargeDensity`（`Modules/Evolve/WarpXEvolve.cpp:1251-1260`），于是**RZ 下诊断 rho 也会被 PEC/反射边界清零**（上游只在 Cartesian 这样做）。体积标定本身与上游等价，不是问题。
- 建议修法: 诊断路径只做 filter+sum+（RZ）体积标定，不要在诊断副本上应用 PEC/反射边界；或给 `finalizeRhoOnLevel` 加 `apply_boundary` 开关。
- 验证方法: RZ + `boundary.field_lo = pec` 用例，导出 rho 切片，检查 PEC 边界面上的 rho 是否为 0（上游不为 0）。

### F9 `ParticlePushHandler` 的 HandlerSpec 声称不读任何场
- 文件:行 `Modules/PicEm/ParticlePush/ParticlePushHandler.cpp:22-28`（`reads_fields = {}`），对照 `apply()` 调 `particleServices().pushParticlesAndDeposit`
- 严重度: minor
- 置信度: confirmed
- 现象与依据: 粒子推进必然读 `Efield_fp`/`Bfield_fp`（以及 aux）。`DepositionHandler`（`:24-25`）与 `FieldSolveHandler`（`:24-25`）都正确声明了读写场；只有 push 的 `reads_fields` 为空。若运行时将来按 `reads_fields` 做依赖排序/驻留统计，推进会与场求解无序。
- 建议修法: `reads_fields = {"Efield_fp", "Bfield_fp"}`；`PicEmPackage::register_specs`（`PicEmPackage.cpp:43-50`）同步。
- 验证方法: 读 `Runtime/HandlerSpec.H` 消费者（如 `PluginHost` 的冲突/依赖检查），或用 `grep -rn "reads_fields" Source/Core` 确认是否有语义消费者。

### F10 `WarpXRuntimeData.H` 的 include 落在版权头与 include guard 之前；`WarpXHostServices.cpp` 有两个 `namespace` 块
- 文件:行 `RuntimeBridge/Runtime/WarpXRuntimeData.H:1-4`（在 `/* Copyright */` 与 `#ifndef WARPX_RUNTIME_WARPXRUNTIMEDATA_H_` 之前 include 了 4 个头，且 `:15-18` 又重复一次）；`WarpXRuntimeData.cpp:1-3` 同样在版权头之前；`RuntimeBridge/Runtime/WarpXHostServices.cpp:26-27`（空的 `#ifdef WARPX_QED / #endif`）、`:378`（第二个 `namespace warpx::runtime {`）
- 严重度: minor（可读性/一致性；无功能影响，头文件自身有 guard）
- 置信度: confirmed
- 现象与依据: 与仓库 C++ 规范（版权头在最前、include 顺序见 AGENTS.md）和同目录其它文件不一致；重复 include 说明是拼接遗留。
- 建议修法: 删除 `:1-4`（保留 `:15-18`）并移到版权头之后；合并 `WarpXHostServices.cpp` 的两个 namespace 块，删掉空 `#ifdef`。

## 不确定但值得复核的点

1. `WarpXHostServices::depositCurrent`（`WarpXHostServices.cpp:61-81`）对 J 直接 `WarpXSumGuardCells(J, period, J.nGrowVect(), …)`（整层 guard 宽度），而主循环路径 `SumBoundaryJ`（`WarpXComm.cpp:1688-1728`）先把宽度 clamp 到 `ng_depos_J`（含 current centering / filter 增量）。若 guard 中存在上一步残留或 coarse/fine 重叠数据，整宽度求和在 box 边界会重复计入。需要多 rank（≥2）+ 多 box 用例对比 `depositCurrent` 与主循环路径的 J。
2. `WarpXRegrid.cpp:188-234` 的 `RemakeLevel` else 分支（`ba` 改变）是新实现（上游此处 `WARPX_ABORT_WITH_MESSAGE("RemakeLevel: to be implemented")`）：它用 `m_fields.list()` 里名字后缀 `"[level=N]"` 收集旧数据（命名格式与 `MultiFabRegister::mf_name` 一致，`MultiFabRegister.cpp:689-718`，可行），但 `ClearLevel → AllocLevelData → InitLevelData → InterpolateCoarseToFine` 之后再 `ParallelCopy` 回写；`InterpolateCoarseToFine`（`WarpX.cpp`）用 AMReX 的 `InterpFromCoarseLevel(mf, nghost, IntVect{0}, …)` 重载（`AMReX_FillPatchUtil.H:779`，即 `nghost_outside_domain = 0`），而文档要求调用者先保证 coarse guard cell 已填充——`AllocLevelData`/`InitLevelData` 之后 coarse guard 未必是干净的。建议配 AMR + `regrid_int>0` 的 3D 用例与上游同配置对比守恒量。
3. `Modules/Python/callbacks.cpp:22-28`：`InstallPythonCallback` 现在既写 `warpx_callback_py_map`（拷贝）又 `register_callback(name, std::move(callback))`，而执行/查询/删除都走 registry。当前全仓库没有第二处直接操作该 map 的代码，所以现在无害；但只要有人（Python 侧、测试）直接往 `warpx_callback_py_map` 里塞函数，就会静默不生效。建议删掉这个"view"或让 `ExecutePythonCallback` 查 map。
4. `RuntimeBridge/IO/WarpXCheckpoint.cpp:242-259`：读 checkpoint 末尾的 `hybrid_pic_substeps`。写侧只在 hybrid 模型存在且 `is_checkpoint` 时写（`Plugins/Diagnostics/FlushFormats/FlushFormatPlotfile.cpp:355-360`），读侧用 `is >> std::ws; if (!is.eof())` 兜"老 checkpoint"。但 `is.exceptions(failbit|badbit)` 在 `:112` 已打开：对以换行结尾、之后无数据的流，`std::ws` 在 EOF 处一般只置 `eofbit`（不抛），可得预期分支；若老 checkpoint 最后一个值恰好被 `goto_next_line` 停在 EOF 且 sentry 置 `failbit`，则会抛 `ios_base::failure` 而不是走"跳过"路径。建议改为 `try/catch` 或先 `is.peek()==EOF` 判断。
5. `WarpXHostServices.cpp:118` 与 `:119` 的"filter+sum 然后 finalize"是否会 double-finalize：已核 `ApplyRhofieldBoundary`（`Modules/BoundaryConditions/WarpXFieldBoundaries.cpp:257-285`）只做 PEC/反射清零、不做 filter/sum，`FinalizeRhoOnLevel` 也不重复 filter，所以**不是**重复滤波；但 `Errors`/`rho` 诊断路径会连续两次进入 `ApplyInverseVolumeScalingToChargeDensity` 吗？答案是不会（`finalizeRhoOnLevel` 只在服务层被调用一次）。已排除。
6. `WarpX::Create()`（`WarpX.cpp:277-314`）里没有设置 `m_instance`，但 `WarpX::WarpX()` 构造函数首部有 `m_instance = this;`（`WarpX.cpp:404`），所以 `WarpXKernel`（`WarpXKernel.cpp:62`）构造后 `GetInstance()` 返回 kernel 的实例；`pywarpx` 若先用 `GetInstance()`（走 `legacy_owner`）再经 runtime 建 kernel，会存在两个实例争用同一 `m_instance`，`clear()` 里 `m_warpx.reset()` 之后 `ResetInstance()` 又会把 `legacy_owner` 一起销毁。是否可达取决于 Python/CLI 的入口组合，建议核实"同进程先 pywarpx 再 kernel"的场景。
7. `WarpX.cpp:2959` 起，`AllocLevelData` 去掉了上游的 `if (lev == maxLevel())` 包裹（上游注释"EB info are needed only at the finest level"），EB update flags / ECT 数组在**每一层**都分配；`:3304-3306` 的注释说明这是为 coarse patch 新增的 `m_eb_update_*_cp` 有意为之。看起来是设计扩展而非笔误，但 ECT + AMR 组合应与上游同配置对比（重点看 `WarpXFaceExtensions.cpp` 里仍写死 `maxLevel()` 的用法，`:717`、`:885`）。
8. `Modules/PicEm/FieldSolve/FieldSolveHandler.cpp:37` 与 `YeeFieldSolveHandler.cpp:37` 都只调 `fieldAlgebra().fieldSolve(ctx.time)`：`FieldSolveHandler` 的 `m_name` 只用于 spec，若 `field_solver_module_name()`（`WarpXKernel.cpp:29-43`）返回 `field_solve.psatd`/`field_solve.ect`，handler 名字与实际执行不构成约束——目前无害（`fieldSolve` 内部按 `WarpX` 配置分派），但 spec 与实际能力的绑定是"名义"的。
