# Source_Plugins_ParticleCore_B 审查

分区：`Source/Plugins/ParticleCore/Particles/` 后半（粒子 I/O、QED 挂接、离子化、粒子滤波器/重采样、
激光粒子容器、粒子诊断接口、物理常数）。上游对照：`_upstream/warpX-26.10/Source/`。

方法：43 个文件全部读完（大文件用二分页 read 读完全文），并逐文件与上游同名实现做 `diff -u`
（上游目录被拆分，一个文件可能有多个同名候选，取最小 diff 的候选，例如 `Particles/MultiParticleContainer.cpp`
对 `Source/Particles/MultiParticleContainer.cpp`；`ParticleIO.cpp` 对 `Source/Diagnostics/ParticleIO.cpp`）。
判定：绝大多数文件是**机械重构**（`WarpX::x` → `config().x()` / `data().x()`、`WarpXParIter` → `ParticleTileIter`、
include 路径搬移、新增 `std::optional` 参数）。实质语义变化集中在下面 5 个文件：
`MultiParticleContainer.cpp`、`WarpXParticleContainer.cpp/.H`、`PhysicalParticleContainer.cpp`、
`Pusher/ImplicitPushPX.cpp`、`Resampling/VelocityCoincidenceThinning.cpp`。

## 覆盖

| 文件 | 行数 | 类型 | 结论 |
|---|---|---|---|
| Initialization/VelocityProperties.H | 66 | moved（clone 到新目录） | OK |
| Initialization/VelocityProperties.cpp | 142 | modified（`legacy_frame()`） | OK |
| Laser/LaserProfiles.H | 408 | modified（新增 `m_parser_exe` 缓存） | OK |
| Laser/LaserProfilesImpl/LaserProfileFieldFunction.cpp | 49 | modified（缓存编译结果） | OK |
| Laser/LaserProfilesImpl/LaserProfileFromFile.cpp | 723 | moved | OK |
| Laser/LaserProfilesImpl/LaserProfileGaussian.cpp | 162 | moved | OK |
| LaserParticleContainer.H | 150 | modified（机械） | OK |
| LaserParticleContainer.cpp | 969 | modified（`frame()` 替换） | 可疑（见 F3） |
| MultiParticleContainer.H | 689 | modified（IParticleSystem 契约） | 可疑（见 F1/F7） |
| MultiParticleContainer.cpp | 2485 | modified（契约 + 删碰撞 + 拆 finalize） | **缺陷**（F1/F2/F4/F5/F6） |
| ParticleBoundaries_K.H | 170 | moved | OK |
| ParticleBoundaryBuffer.H | 95 | modified（ctx 注入） | OK |
| ParticleBoundaryBuffer.cpp | 633 | modified（机械） | OK |
| ParticleCreation/AddParticles.cpp | 1753 | modified（`frame()`、删 `return`） | 可疑（F3；`return` 删除经验证无害） |
| ParticleCreation/AddPlasmaUtilities.H | 319 | moved | OK |
| ParticleCreation/FilterCreateTransformFromFAB.H | 228 | moved | OK |
| ParticleCreation/SmartCreate.H | 123 | moved | OK |
| ParticleCreation/SmartUtils.cpp | 93 | modified（未知属性名 → Zero 策略） | OK（与 `map::operator[]` 语义等价） |
| ParticleIO.cpp | 426 | modified（参数化 `RuntimeData`/快照） | OK |
| ParticleUtils.cpp | 65 | modified（`IParticleContainer::ParticleType`） | OK |
| PhotonParticleContainer.H | 143 | modified（机械 + 新纯虚实现） | OK |
| PhotonParticleContainer.cpp | 289 | modified（机械） | OK |
| PhysicalParticleContainer.H | 522 | modified（机械） | OK |
| PhysicalParticleContainer.cpp | 2289 | modified（split gather、`implicit_options`） | 可疑（F1 触发点、F10） |
| Pusher/CopyParticleAttribs.H | 110 | modified（`+a_offset`） | OK（修了上游的偏移 bug） |
| Pusher/ImplicitPushPX.cpp | 1358 | modified（RSPHERE 角量、hard-code 常数） | 可疑（F8/F9） |
| Resampling/LevelingThinning.H | 66 | moved | OK |
| Resampling/LevelingThinning.cpp | 139 | moved | OK |
| Resampling/Resampling.H | 99 | modified（ctx） | OK |
| Resampling/Resampling.cpp | 47 | modified（ctx） | OK |
| Resampling/ResamplingTrigger.H | 74 | modified（ctx） | OK |
| Resampling/ResamplingTrigger.cpp | 59 | modified（ctx + fallback） | OK |
| Resampling/VelocityCoincidenceThinning.H | 204 | moved | OK |
| Resampling/VelocityCoincidenceThinning.cpp | 341 | modified（退化角默认值 0→1） | OK（实为修正，见"已确认的修正"） |
| RigidInjectedParticleContainer.H | 124 | modified（机械） | OK |
| RigidInjectedParticleContainer.cpp | 484 | modified（机械 + 拒绝 gather_orders） | OK |
| Sorting/Partition.cpp | 155 | modified（机械） | OK |
| Sorting/SortingUtils.H | 215 | modified（机械） | OK |
| SpeciesPhysicalProperties.cpp | 295 | moved | OK |
| SpeciesUtils.H | 39 | moved | OK |
| WarpXParticleContainer.H | 766 | modified（契约/IParticleContainer） | 可疑（F3/F6） |
| WarpXParticleContainer.cpp | 3251 | modified（DepositCharge 拆分、删 RZ 缩放） | **缺陷**（F2/F5/F6） |
| WarpXParticleContainer_fwd.H | 21 | modified（`class ParticleTileIter`） | OK |

## 缺陷清单

### F1 新抽象把 `GetExternalEBField` 的容器来源改成"只来自 ctx"，QED / 离子化三条路径不传 ctx：解析式外场下空指针崩溃，lattice/plasma-lens 外场静默丢失
- 文件:行
  - 根因：`Source/Core/Modules/Particles/Gather/GetExternalFields.H:30`（`ctx` 默认 `nullptr`）；
    `Source/Plugins/ParticleCore/Particles/Gather/GetExternalFields.cpp:26-31`（`ps = ctx ? ctx->particles : nullptr`）、
    `:71-81`（`ps->externalFieldParser("Ex")->compile<4>()`，**无 null 检查**）。
  - 不传 ctx 的调用点（全部由本分区文件构造）：`Particles/ElementaryProcess/QEDPairGeneration.cpp:50`
    （由 `MultiParticleContainer.cpp:2288` `doQedBreitWheeler` 调用）、`QEDPhotonEmission.cpp:53`
    （由 `MultiParticleContainer.cpp:2368` `doQedQuantumSync` 调用）、`Ionization.cpp:62`
    （由 `PhysicalParticleContainer.cpp:1745-1767` `getIonizationFunc` 调用）。
- 严重度: critical
- 置信度: confirmed
- 现象与依据：
  - 三条路径构造 `GetExternalEBField(a_pti, a_offset)` 时 `ctx == nullptr` ⇒ `ps == nullptr`。
    只要输入里有 `particles.E_ext_particle_s = parse_e_ext_particle_function`（或 B 侧同类），
    构造函数就会执行 `ps->externalFieldParser("Ex")` ⇒ 对空指针调用成员函数（UB/段错误）。
    触发条件：**任意 QED 过程（Breit-Wheeler / 量子同步辐射）或场电离 + 解析式外场**同时开启，
    与滤波器设置无关。
  - 同一构造函数里 `repeatedPlasmaLens()`（`:31`）和 `acceleratorLattice(lev)`（`:38`）都有
    `ps != nullptr` 保护，所以 `repeated_plasma_lens` / 加速器 lattice 在这三条路径上会**静默**
    变成空外场（`m_n_lenses=0`、`lattice_ptr=nullptr`），粒子不再感受到透镜/lattice 场，且无任何警告。
  - 上游对照：`_upstream/warpX-26.10/Source/Particles/Gather/GetExternalFields.cpp:22-24`
    `auto& warpx = WarpX::GetInstance(); auto& mypc = warpx.GetPartContainer();`
    —— 上游从不依赖调用方传入容器，故三条 QED/离子化路径天然安全；这是本次抽象化引入的回归。
- 建议修法：把 context 一路传进 `PairGenerationTransformFunc` / `PhotonEmissionTransformFunc` /
  `IonizationFilterFunc` 的构造函数（`MultiParticleContainer` 已有 `m_runtime_context`），
  并在 `GetExternalFields.cpp` 里对 `ps == nullptr` 用 `ABORT`/`WARPX_ALWAYS_ASSERT_WITH_MESSAGE`
  显式失败而不是解引用，或按 `legacy_host()` 那样回退到已注册的 legacy 提供者。
- 验证方法：跑一个 `WARPX_QED=ON` 的 BW 用例，输入加 `particles.E_ext_particle_s = parse_e_ext_particle_function`
  与 `particles.E_external_particle_function(x,y,z,t)=...`；或无 QED，加 `do_field_ionization=1` +
  同样外场，并用 ASAN 构建（`cmake -DWarpX_COMPUTE=OMP -DCMAKE_CXX_FLAGS=-fsanitize=address`）。

### F2 `DepositCharge` 丢掉 RZ/RCYLINDER/RSPHERE 的 `1/(2πr)` 反体积缩放，唯一的替代 finalizer 不覆盖 `rho_fp_<species>`：RZ hybrid + per-species 场时电荷密度静默错标度
- 文件:行
  - 本分区：`Source/Plugins/ParticleCore/Particles/MultiParticleContainer.cpp:627-650`
    （`DepositCharge(rho, relative_time)` 尾部已无 RZ 缩放块；上游 `_upstream/.../MultiParticleContainer.cpp:646-651` 有
    `for lev: ApplyInverseVolumeScalingToChargeDensity(rho[lev], lev)`）；
    `Source/Plugins/ParticleCore/Particles/WarpXParticleContainer.cpp:1860-1950`
    （`DepositCharge`/`DepositChargeOnLevel` 的 `apply_boundary_and_scale_volume` 参数被删除，函数体只剩裸沉积 + `SumBoundary`）。
  - 未覆盖的消费方：`Source/Plugins/PicEm/Modules/FieldSolver/WarpXPushFieldsHybridPIC.cpp:311-345`
    （per-species 沉积，RZ 分支只剩 `ApplyInverseVolumeScalingToCurrentDensity(J_spec…)`，
    **丢掉了上游 `:324` 的 `ApplyInverseVolumeScalingToChargeDensity(rho_spec[lev], lev)`**，
    而 `:328-332` 的注释仍写着"apply the inverse-volume scaling to the per-species deposits
    so they carry physical charge/current densities"）；`rho_species_sum`（`:376-379`）由未缩放的
    `rho_spec` 累加而来，同样未缩放。
  - 唯一的 finalize 入口 `WarpX::FinalizeRho()`（`Source/Plugins/PicEm/Modules/Evolve/WarpXEvolve.cpp:1244-1260`）
    只处理 `FieldType::rho_fp` / `rho_cp`，**不会**碰 `"rho_fp_"+spec` 或 `hybrid_rho_species_sum_fp`。
- 严重度: critical
- 置信度: confirmed
- 现象与依据：
  - 新设计是"沉积不做缩放，交给 `FinalizeRho()` 统一做 `ApplyRhofieldBoundary` + RZ 反体积缩放"。
    主路径自洽：ES（`LabFrameExplicitES.cpp:57-60` 调 `finalizeRhoOnLevel`）、FDTD
    （`OneStep_FinalizeDeposition` → `FinalizeRho()`）、hybrid 总量（`rho_fp`）都补上了。
  - 但 hybrid 的 per-species 电荷密度既不被 `DepositCharge` 缩放（上游在函数内做），也不被
    `FinalizeRho()` 缩放（字段名不同），于是 RZ 下 `rho_fp_<spec>` 与 `hybrid_rho_species_sum_fp`
    缺 `1/(2π r)`（径向几何归一）因子，而对应 `J_spec` 已缩放 —— `Vs = Js/rhos`、species fraction、
    per-species resistivity、电子能量方程全部被同一个 r 因子污染。上游 `:368` 与本分支对照可确认
    "总量在 `SyncCurrentAndRho` 之后缩放、per-species 在沉积之后单独缩放"的两段式设计被破坏。
  - 该文件不在我的分区，但它是我这个分区（`WarpXParticleContainer::DepositCharge` 删除参数与 RZ 块）
    的直接后果，请 lead 转发给 `Source_PicEm_FieldSolver`/HybridPIC 审查者。
- 建议修法：最小改动是在 `WarpXPushFieldsHybridPIC.cpp` 的 RZ 分支恢复
  `ApplyInverseVolumeScalingToChargeDensity(rho_spec[lev], lev)`（并在 `rho_species_sum` 累加后同样处理，
  或直接对 `hybrid_rho_species_sum_fp` 缩放）；更好的做法是给 `GetChargeDensityOnLevel` 风格的
  per-species finalize 提供一个显式 API，避免每个调用者自己记得缩放。
- 验证方法：`Examples/` 里任一 RZ hybrid 用例（`picsar`/hybrid-PIC 目录下 `warpx.do_hybrid_pic_model=1`
  + `hybrid_pic_model.use_per_species_fields=1`，`WarpX_DIMS=RZ`），对比 `rho_fp_<spec>` 与
  `rho_fp` 的 species 分解：错误的版本里前者积分不等于后者（差一个 r 权重的系统性偏差）。

### F3 `WarpXParticleContainer::frame()` 用一次性 `static` 快照，与"live view"设计及可用的 `legacy_frame()` 提供者相矛盾
- 文件:行 `Source/Plugins/ParticleCore/Particles/WarpXParticleContainer.cpp:3221-3222`
  （`static const warpx::runtime::FrameState legacy = warpx::runtime::FrameState::FromLegacy();`）
- 严重度: major
- 置信度: likely
- 现象与依据：
  - `FrameState` 是**值快照**（`Source/Core/Runtime/FrameState.H:18-31`，字段 `gamma_boost/beta_boost/
    boost_direction/moving_window_*/start|end_moving_window_step`），`FromLegacy()` 读 `WarpX::` 静态量
    （`Source/Plugins/PicEm/RuntimeBridge/Runtime/FrameState.cpp:15-28`）。函数内 `static` 初始化只发生一次，
    之后永不刷新 ⇒ 所有在 `SetRuntimeContext()` 之前发生过的调用会把那时的 `moving_window_v`、
    `moving_window_dir`、`beta_boost` 永久固定下来。
  - 同仓库里其它粒子文件用的是**每次调用都刷新**的提供者 `warpx::runtime::legacy_frame()`
    （`Initialization/VelocityProperties.cpp:64`、`Particles/AcceleratorLattice/LatticeElementFinder.cpp:122-123`），
    而 `LegacyServices.H:35-37` 明确注册的是 `std::function<FrameState&()>`（活对象）。也就是说
    `frame()` 与本文件自己的设计说明（`RuntimeConfig.H:19-25` "every getter reads the current WarpX value,
    so … moving-window … cannot go stale"，即 audit D4 的结论）直接冲突，把 D4 类缺陷重新引入。
  - 可达性：容器在 WarpX 构造函数里就建好，`SetRuntimeContext` 在 `WarpXInitData.cpp:843-863` 才调用；
    只要有任何 `frame()` 使用发生在该点之前（例如构造期/`InitData` 早期的 `gamma_boost` 判断），
    快照就会永久冻结 `moving_window_v` 的符号，从而影响
    `ParticleCreation/AddParticles.cpp:812-813` 的 `moving_sign`（注入方向反了 → 粒子从错误一端注入）。
    目前主流路径在 `SetRuntimeContext` 之后才用 `frame()`，因此判为 likely 而非 confirmed。
- 建议修法：`frame()` 的回退分支改成 `return warpx::runtime::legacy_frame();`（活引用），删掉 `static`。
- 验证方法：`grep -n "frame()" 后加断言`，或在 legacy（Python/PICMI）路径下打印 `frame().moving_window_v`，
  观察移动窗口启用前后该值是否变化；或写一个单测：先调 `frame()`，再改 `WarpX::moving_window_v`，再调 `frame()`，
  两次结果应不同。

### F4 `full_mass_matrices` 的判据从 `Szz->nComp() > 1` 换成 `implicit_options.use_mass_matrices_jacobian`，两者在"只开 PC 质量矩阵"时不等价
- 文件:行 `Source/Plugins/ParticleCore/Particles/WarpXParticleContainer.cpp:1217`；
  上游 `_upstream/.../WarpXParticleContainer.cpp:1195`、`:1225-1330`
- 严重度: major
- 置信度: needs-verification
- 现象与依据：
  - 上游用 `const bool full_mass_matrices = (Szz->nComp() > 1);` 选择
    `doVillasenorSigmaDeposition<N,true/false,max_grid_crossings>`（全模板 vs 缩减模板）。
  - 本地改为 `implicit_options.use_mass_matrices_jacobian`。而
    `Source/Plugins/FieldSolver/Implicit/ImplicitSolver.cpp:883-918` 显示 Villasenor 分支里
    `ncomp_xx` 在 jacobian 与非 jacobian（PC）两种情况下**都** `>1`（宽度分别是 `1+2*shape+2*mgc`
    与 `1+2*(shape-1)+2*mgc`），并且 `use_mass_matrices_pc` 单独开启时 `m_use_mass_matrices_jacobian`
    仍为 false。因此"nComp>1 但 jacobian=false"是可达状态：上游走 `full=true`（宽 stencil），
    本地走 `full=false`（窄 stencil），PC 消费者（`ImplicitPushPX.cpp:1145-1160` 的
    `ncomp_pc_xx/yy/zz`）拿到的矩阵宽度就可能与沉积时用的不一致。
  - 本地注释自称"reduce subset that is only used for the PC"，暗示是有意为之；但需要确认
    `m_ncomp_pc_*` 与 `doVillasenorSigmaDeposition<...,false,...>` 的实际写入宽度一致。
- 建议修法：若确有意图，把注释改成可验证的断言（沉积前 `WARPX_ALWAYS_ASSERT(ncomp_xx == implicit_options.ncomp_pc_xx)`）；
  否则恢复 `Szz->nComp() > 1`。
- 验证方法：`WarpX_DIMS=2`、隐式 + `algo.current_deposition = villasenor` +
  `implicit_evolve.use_mass_matrices_pc = 1`、`use_mass_matrices_jacobian = 0`，比对
  `Szz->nComp()` 与 `implicit_options.ncomp_pc_zz` 的乘积，并跑隐式收敛测试。

### F5 `DepositCurrentOnLevel` 的文档仍声称"apply boundary conditions"，实现只有 guard-cell 求和
- 文件:行 `Source/Plugins/ParticleCore/Particles/WarpXParticleContainer.H:305-318`（文档）对
  `Source/Plugins/ParticleCore/Particles/WarpXParticleContainer.cpp:1530-1547`（实现）
- 严重度: minor
- 置信度: confirmed
- 现象与依据：函数体只做 `DepositCurrent(...)` + RZ 反体积缩放 + `host().particleServices().syncCurrent(mf_name)`；
  `ApplyJfieldBoundary` 被移到了唯一调用方 `Source/Plugins/PicEm/Modules/Python/Particles/WarpXParticleContainer.cpp:140-148`
  的 `warpx.FinalizeJOnLevel(...)`。函数名与文档因此误导任何新的 C++ 调用者（漏调 finalize 就会在 PEC 边界出错）。
- 建议修法：改名为 `DepositCurrentAndSyncOnLevel` 并删掉文档里的"apply boundary conditions"，或加
  `\warning must be followed by FinalizeJOnLevel()`。
- 验证方法：读文档与 `grep -rn "DepositCurrentOnLevel"` 的唯一调用点即可确认。

### F6 新增的三个隐式辅助函数把层号 hard-code 为 `lev = 0`（多层的循环被注释掉）
- 文件:行 `Source/Plugins/ParticleCore/Particles/MultiParticleContainer.cpp:1110-1145`
  （`ClearImplicitParticleVelocities`，`lev=0` 在 `:1121`）、`:1150-1190`（`FinishImplicitVelocityUpdate`，`:1161`）、
  `:1194-1245`（`PrepareImplicitVelocitiesForDeposition`，`:1216`），三处都写着
  `// for (int lev = 0; lev <= finest_level; ++lev)` 然后 `const int lev = 0;`
- 严重度: minor
- 置信度: confirmed
- 现象与依据：这三个函数在上游不存在（`grep -rn ClearImplicitParticleVelocities _upstream/` 无结果），
  是新 `SemiImplicitDarwin` 链路（`Source/Plugins/FieldSolver/Implicit/SemiImplicitDarwin.cpp:543-557`）的服务实现。
  当前该求解器自身多处 `const int lev = 0;`（同文件 `:300/:338/:404/:501`），所以暂时自洽；
  一旦隐式求解器支持 AMR，`lev>0` 粒子的 `u`/`u_n` 不会被清零/合并，而 `DepositCurrent` 会按层遍历 →
  多层隐式结果会静默错误。
- 建议修法：把函数签名加上 `finest_level`（与同文件 `SaveParticlesAtImplicitStepStart` 一致）并恢复层循环；
  或加 `AMREX_ALWAYS_ASSERT(maxLevel()==0)` 让限制显式化。
- 验证方法：`WARPX_MAXLEVEL=1`（或 `amr.max_level=1`）+ 隐式用例，检查 `lev=1` 的 `u_n` 是否与被清零的 `u` 一致。

### F7 新增契约实现里缺属性保护与"接口注释重复"等小问题
- 文件:行 `Source/Plugins/ParticleCore/Particles/MultiParticleContainer.cpp:1318-1332`
  （`countParticlesWithSuborbits` 无条件 `pti.GetiAttribs("nsuborbits")`）；
  `Source/Plugins/ParticleCore/Particles/MultiParticleContainer.H:383-384`（重复两行 `//! IParticleSystem:` 注释）；
  `Source/Plugins/ParticleCore/Particles/MultiParticleContainer.cpp:48-49` 与
  `WarpXParticleContainer.cpp:33-34`（`#include "Runtime/LegacyServices.H"` 重复两次）
- 严重度: minor
- 置信度: confirmed
- 现象与依据：上游 `_upstream/.../ImplicitSolvers/ImplicitSolver.cpp:25` 有 `if (!pc->HasiAttrib("nsuborbits")) continue;`；
  本地把保护留在了调用方（`Source/Plugins/PicEm/Modules/FieldSolver/ImplicitKernelServicesBuilder.cpp:169` 之前
  `ImplicitSolver.cpp:33` 检查了 `species_has_attrib`），所以当前可运行；但接口没有表达这一前置条件，
  任何新调用方都会得到 AMReX 的 "attribute not found" abort 而不是跳过。
  顺带：`countParticlesWithSuborbits` 里 `amrex::ParallelFor` + `amrex::Gpu::Atomic::Add(counter,…)` 在 CPU
  + OpenMP 下非原子（见 `AGENTS.md` 的 portability 段）。该写法**与上游逐字一致**，不是本次引入的回归，
  故只作记录。
- 建议修法：函数内加 `if (!pc.HasiAttrib("nsuborbits")) { return; }`；删除重复 include 与重复注释行。
- 验证方法：对一个没有 `nsuborbits` 的 species 调 `countParticlesWithSuborbits`（或用 `suborbit_statistics`
  打开但只给部分 species 加属性）应跳过而非 abort。

### F8 RSPHERE 的隐式质量矩阵角度分支是本 fork 新增（上游 RZ/RCYLINDER only），球坐标角量耦合到"平均半径/瞬时半径"混用
- 文件:行 `Source/Plugins/ParticleCore/Particles/Pusher/ImplicitPushPX.cpp:1103-1118`
- 严重度: minor
- 置信度: needs-verification
- 现象与依据：上游 `_upstream/.../Pusher/ImplicitPushPX.cpp:1100-1105` 只有
  `#if defined(WARPX_DIM_RZ) || defined(WARPX_DIM_RCYLINDER)` 的 `costh/sinth = xp|yp / rp_mid`。
  本地新增 RSPHERE 分支：`rp_mid` 用 `(r_new + r_old)/2`（含 z），而 `costh/sinth` 用**瞬时**的
  `rpxy_mid = sqrt(xp²+yp²)` 归一，`cosph/sinph` 用 `rp_mid` 归一。角量约定（`theta`=方位角、
  `phi`=相对 xy 平面的极角）与 `WarpXParticleContainer.H:67-68` 的文档一致，但"cos/sin 的模长是否为 1"
  依赖 `rpxy_mid` 与 `rp_mid` 的一致性（`cosph²+sinph² = (r_xy²+z²)/rp_mid² = 1` 只在同一点取样时成立，
  这里 `r_xy` 用瞬时位置、`rp_mid` 用平均半径，二者不严格互补）。RZ 分支上游也是这么混用的，
  所以不能断定是 bug，但 RSPHERE 没有上游可用作对照。
- 建议修法：统一用同一取样点（要么都用 `n+1/2` 中点位置，要么都用平均半径），并加
  `WARPX_ALWAYS_ASSERT(std::abs(costh*costh+sinth*sinth-1) < 1e-12)` 之类的自检（仅调试构建）。
- 验证方法：`WarpX_DIMS=RSPHERE` + 隐式（`implicit_evolve.use_mass_matrices_jacobian=1`）跑球对称
  均匀等离子体，检查守恒量/收敛阶；与 RZ 用例的角向行为对比。

### F9 `villasenor_mass_matrices_max_grid_crossings` 的单一真源被复制成魔法数 `3`
- 文件:行 `Source/Plugins/ParticleCore/Particles/WarpXParticleContainer.cpp:1243-1358`（8 处模板实参
  写成字面量 `3`）、`Source/Plugins/ParticleCore/Particles/Pusher/ImplicitPushPX.cpp:28`
  （新增 `constexpr int villasenor_mass_matrices_max_grid_crossings = 3;`）
- 严重度: minor
- 置信度: confirmed
- 现象与依据：上游只有一处定义 `_upstream/Source/WarpX.H:287
  static constexpr int villasenor_mass_matrices_max_grid_crossings = 3;`，并且
  `ImplicitSolver.cpp:796-798` 用**同一个常量**做断言。本地断言走 `config().villasenor_mass_matrices_max_grid_crossings()`
  （`Source/Plugins/PicEm/.../ImplicitSolver.cpp:889`，由 `RuntimeConfig` 提供），而核内模板参数是字面量 3：
  断言与实现不再由编译器绑定，改一处漏一处会静默越界。当前数值相同，故只是可维护性缺陷。
- 建议修法：让 kernel 也读 `config().villasenor_mass_matrices_max_grid_crossings()` 不行（模板参数需编译期常量），
  至少把本地 `constexpr` 提到共享头并用它替换 8 处字面量，同时加 `static_assert`。
- 验证方法：`grep -rn "villasenor_mass_matrices_max_grid_crossings" Source/` 即可看到真源分裂。

### F10 `GetChargeDensityOnLevel(local=true, finalize=true)` 现在直接 assert；Python 绑定的 `finalize` 默认 `true`
- 文件:行 `Source/Plugins/ParticleCore/Particles/WarpXParticleContainer.cpp:1975-1977`、
  `Source/Plugins/ParticleCore/Particles/MultiParticleContainer.cpp:700-710`、
  绑定 `Source/Plugins/PicEm/Modules/Python/Particles/WarpXParticleContainer.cpp:130-136`
  （`finalize` 默认 `true`）、`MultiParticleContainer.cpp:55-60`
- 严重度: minor
- 置信度: confirmed
- 现象与依据：上游 `GetChargeDensity(lev, local)` 对 `local=true` 也会做 boundary/RZ 缩放
  （`DepositCharge(..., apply_boundary_and_scale_volume=true)`），本地改成
  `AMREX_ALWAYS_ASSERT_WITH_MESSAGE(!local, "GetChargeDensityOnLevel requires local=false when finalize=true.")`。
  从 Python 调 `s.get_charge_density(lev, local=True)`（不传 finalize）会从"返回未跨 rank 求和但已缩放的密度"
  变成硬 abort。虽然是显式失败而非静默，仍是 API 行为变化。
- 建议修法：`local=true && finalize=true` 时改为"只做 RZ 缩放、跳过边界反射"，或在绑定里默认 `finalize=false`
  并要求显式请求。
- 验证方法：`Python/pywarpx` 里任一调用 `get_charge_density(local=True)` 的脚本。

## 已确认的修正（与上游不同但本地更正确，勿回退）

1. `Pusher/CopyParticleAttribs.H:58-66`：`x_n_btd/…/uz_n_btd` 指针补上了 `+ a_offset`。
   上游 `_upstream/.../CopyParticleAttribs.H` 没有偏移，而 `operator()` 用 `xpold[i]`，在
   `PhysicalParticleContainer::PushPX` 以 `offset != 0`（缓冲区粒子）构造时会写到错误的粒子槽位；
   本 fork 与同一函数里 `ux/uy/uz`（含 offset）保持一致，是对的。
2. `Resampling/VelocityCoincidenceThinning.cpp:273-276`：退化情形下 `cos_theta/cos_phi` 的默认值由 `0` 改为 `1`。
   上游 `0` 会给出 `cos²+sin² = 0` 的非单位"旋转"，本地 `1` 对应恒等旋转，`(cos,sin)` 恢复为单位向量。
3. `WarpXParticleContainer.cpp:2407-2412`（`CalculateNuei`）：`mu = m_e*rimass/(m_e+rimass)` 改成无量纲
   `mu_norm = rimass/(m_e+rimass)`，使 `b0`、`bmin_qm` 的分母量纲自洽（上游那两处把 kg 量纲的 `mu`
   与无量纲的 `2*EF/m_e_J` 相加）。**注意**这会改变数值结果（NRL Coulomb logarithm），
   如果 CI 里有与上游对标的 checksum，需要重算基准。
4. `ParticleCreation/AddParticles.cpp:220`（`return;` 删除）：已验证无害 ——
   `PlasmaInjector` 的 `injection_style` 分支（`Initialization/PlasmaInjector.cpp:136-153`）
   `singleparticle`/`multipleparticles` 走 `setupSingleParticle/setupMultipleParticles` 后**提前 return**，
   不会建 `h_inj_pos`，故 `doInjection()` 为 false；`gaussian_beam`/`external_file` 两个分支
   （上游本来也没有 `return`）行为不变。
5. `ImplicitPushPX.cpp` / `WarpXParticleContainer.cpp` 里 `WarpX::nox/noy/noz` 合并成单个
   `particle_shape`：上游 `WarpX.cpp:142-145` 注释即为
   "This will be overwritten by setting nox = noy = noz = algo.particle_shape"，且
   `DepositCharge` 路径仍保留 `AMREX_ALWAYS_ASSERT(config().nox()==config().noy())` 检查，语义等价。

## 不确定但值得复核的点

- `Particles/Deposition/MassMatricesDeposition.H` 的 `doVillasenorSigmaDeposition` / `doDirectSigmaDeposition`
  新增了 `ncomp_xx/ncomp_yy/ncomp_zz`（`IntVect`）参数；本分区的调用点按 `(ncomp_xx[, ncomp_yy, ncomp_zz])`
  分别传 1/2/3 个 `IntVect`，看起来对应 `doDirectSigmaDeposition` 只吃 `ncomp_xx`、`doVillasenor*` 吃三个。
  该文件不在我的分区，建议由 Deposition 审查者确认"三个方向的 `ncomp` 是否分别对应 X/Y/Z 行而不是列"
  （转置会让 stencil 宽度错位但不改变总维度，编译期看不出来）。
- `QEDPairGeneration.cpp:50` / `QEDPhotonEmission.cpp:53` / `Ionization.cpp:62` 的
  `GetExternalEBField(a_pti, a_offset)` 也**没有传 `RuntimeData`**，因此外场 parser 的 `m_time`
  来自 `legacy_data()`；当 kernel 通过非 legacy 的 `RuntimeData` 运行时（多实例/Python 路径），
  `data().t_new(lev)` 可能与实际时间步不一致。与 F1 一起修更彻底。
- `MultiParticleContainer::countParticlesWithSuborbits` 用 `data().dtAll().size()` 当作层数
  （`RuntimeData` 文档未保证它等于 `finestLevel()+1`）；`WarpXRuntimeData::dtAll()` 返回
  `w().getdt()`，一般是 `finest_level+1`，但比 `finestLevel()+1` 脆弱。
- `MultiParticleContainer::acceleratorLattice(lev)` 依赖 `ctx->accelerator_lattice_at`，
  而它只在 `WarpX::SetRuntimeContext` 里、且 `context != nullptr` 时被赋值
  （`Source/Plugins/PicEm/WarpX.cpp:4062-4076`）；`LatticeElementFinder` 在 legacy 路径上的可用性建议实测。
- `MultiParticleContainer::DepositCurrent` 把 `particle_shape` 传给所有 species，包括激光 species；
  上游也是全 species 无差别，但 `LaserParticleContainer::DepositCurrent` 是空实现，故无影响。
