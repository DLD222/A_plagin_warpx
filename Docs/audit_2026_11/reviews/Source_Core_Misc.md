# Source_Core_Misc 审查

审查范围：清单 36 条 = **35 个唯一文件**（`Modules/Particles/Filter/FilterFunctors.H` 与
`Modules/Particles/FilterFunctors.H` 各列一次，内容逐字节相同，见 F4），共 7813 行。
这一批是"内核 ↔ 插件"之间的通用层：粒子属性契约（`PIdx`/tile SoA 访问）、gather 内核、
`Fields.H` 的 FieldType 注册表、PETSc/DOF 求解器向量、PML `SigmaBox`、`main.cpp`。

判定基准：`_upstream/warpX-26.10/Source/...`（按文件名逐一定位并 `diff -u`，见下表"上游物"列）。
忽略的纯机械差异：`WarpXParticleContainer::` → `warpx::runtime::IParticleContainer::`、include 路径
`Particles/X` → `Modules/Particles/X`、clang-format 空行。
凡"语义变化"（不只是改名）都在缺陷清单或"已验证的偏离"里逐条说明。

## 覆盖

| # | 文件 | 行数 | 类型 | 上游物（diff 行数） | 结论 |
|---:|---|---:|---|---|---|
| 1 | Modules/AcceleratorLattice/LatticeElementFinder.H | 250 | moved+modified | AcceleratorLattice/LatticeElementFinder.H (35) | OK（模板化 `ParIterType`，见 U2） |
| 2 | Modules/FieldSolver/ImplicitOptions.H | 26 | moved+modified | FieldSolver/ImplicitSolvers/ImplicitOptions.H (25) | 缺陷（F3） |
| 3 | Modules/Particles/Collision/ParticleShufflers.H | 397 | moved | Particles/Collision/BinaryCollision/ParticleShufflers.H (11，仅注释) | OK |
| 4 | Modules/Particles/Filter/FilterFunctors.H | 179 | moved | Particles/Filter/FilterFunctors.H (21，仅 include/using) | OK |
| 5 | Modules/Particles/FilterFunctors.H | 179 | **重复副本** | 同上，与 #4 逐字节相同 | 缺陷（F4） |
| 6 | Modules/Particles/Gather/FieldGather.H | 2400 | moved+modified | Particles/Gather/FieldGather.H (116) | 缺陷（F1、F2）；RZ 符号见 F7 |
| 7 | Modules/Particles/Gather/GetExternalFields.H | 192 | moved+modified | Particles/Gather/GetExternalFields.H (30) | OK（显式实例化在插件侧，见 U1） |
| 8 | Modules/Particles/IParticleAttributeAccess.H | 19 | new | —（上游属性访问散在 `WarpXParticleContainer.H`） | OK |
| 9 | Modules/Particles/IParticleIterator.H | 8 | new | —（= `ParticleTileIter` 别名） | OK |
| 10 | Modules/Particles/IParticleReduce.H | 20 | new | —（转发 `amrex::ParticleReduce`） | OK（当前无使用者，见 U3） |
| 11 | Modules/Particles/IParticleSpeciesAccess.H | 18 | new | — | OK |
| 12 | Modules/Particles/IQEDEngineAccess.H | 15 | new | — | OK |
| 13 | Modules/Particles/ParticleAttributes.H | 30 | new（抽取） | WarpXParticleContainer.H:64-86 的 `PIdx` | OK（6 种维度逐分支比对：枚举顺序/`names`/`static_assert` 完全一致） |
| 14 | Modules/Particles/ParticleCreation/DefaultInitialization.H | 275 | moved+modified | Particles/ParticleCreation/DefaultInitialization.H (69) | OK（`t_new` 来自 `pc.particleData()`，见下） |
| 15 | Modules/Particles/ParticleCreation/FilterCopyTransform.H | 331 | moved | 同名 (11，仅 include) | OK |
| 16 | Modules/Particles/ParticleCreation/InitializationPolicy.H | 27 | new（抽取） | DefaultInitialization.H 中的 enum+两个函数 | OK（语义等价，`-std::log(Random)` 一致） |
| 17 | Modules/Particles/ParticleCreation/SmartCopy.H | 166 | moved | 同名 (13，仅 include) | OK |
| 18 | Modules/Particles/ParticleCreation/SmartUtils.H | 73 | moved+modified | 同名 (20) | OK（`getPolicies` 去掉 `noexcept`，声明/定义一致；见 U4） |
| 19 | Modules/Particles/ParticleIO.H | 116 | moved+modified | Particles/ParticleIO.H (54) | OK（多传 `RuntimeData`/`RuntimeConfigSnapshot`；文档参数错位，见 U5） |
| 20 | Modules/Particles/ParticleTileIter.H | 36 | new（改名） | Particles/WarpXParticleContainer.H 的 `WarpXParIter` | OK（成员函数逐个对照一致；定义在 ParticleCore，见 U1） |
| 21 | Modules/Particles/ParticleUtils.H | 780 | moved+modified | Utils/ParticleUtils.H (54) | OK（`amrex::Math::sincos` 返回 `(sin,cos)`，已核 AMReX_Math.H:303） |
| 22 | Modules/Particles/Pusher/GetAndSetPosition.H | 330 | moved | 同名 (20) | OK（维度分支与上游逐字一致） |
| 23 | BoundaryConditions/MultiSigmaBox.H | 124 | new（抽取） | BoundaryConditions/PML.H:47-135 逐字搬移 + `warpx::pml` 命名空间 | OK（本地无残留重复定义） |
| 24 | EmbeddedBoundary/WarpXFaceInfoBox.H | 121 | moved | 同名 (20) | OK（`vecs_size = 0` 是改进） |
| 25 | Fields.H | 176 | moved+modified | Fields.H (64) | OK（见 F5 的核查：注册键与 `isFieldArray` 一致） |
| 26 | IO/FieldIO.cpp | 240 | moved | Diagnostics/FieldIO.cpp (23，仅 include 块) | OK（多余 include，风格） |
| 27 | NonlinearSolvers/PETScReinitialize.H | 32 | new | — | OK（见 U6） |
| 28 | NonlinearSolvers/PETScReinitialize.cpp | 31 | new | — | OK |
| 29 | Parallelization/WarpXSumGuardCells.H | 58 | moved+modified | 同名 (64) | OK（实现与文档一致，已核 .cpp；默认实参被删=源兼容性变化） |
| 30 | Particles/ParticleBoundaries.cpp | 176 | moved | 同名 (11，仅 include) | OK |
| 31 | Utils/Algorithms/IsIn.H | 59 | moved+modified | 同名 (36) | OK（C++20 `requires` 替换 SFINAE，约束等价） |
| 32 | Solvers/WarpXSolverDOF.H | 43 | moved+modified | FieldSolver/ImplicitSolvers/WarpXSolverDOF.H (23) | OK |
| 33 | Solvers/WarpXSolverDOF.cpp | 221 | moved+modified | 同名 (116) | 可疑（F6）；`m_row_offsets` 与上游 `proc_begin` 等价（已推导） |
| 34 | Solvers/WarpXSolverVec.H | 357 | moved+modified | 同名 (79) | 可疑（F6）；`dofRowOffsets()` 契约与 `MatrixPCAlgMG::partition` 一致 |
| 35 | Solvers/WarpXSolverVec.cpp | 270 | moved+modified | 同名 (126) | 可疑（F6） |
| 36 | main.cpp | 38 | moved+modified | main.cpp (43) | 缺陷（F7）；生命周期见 U7 |

## 缺陷清单

### F1 `doDirectGatherVectorField(order,…)` 运行时版本对非法阶数**静默不做任何 gather**
- 文件:行：`Source/Core/Modules/Particles/Gather/FieldGather.H:346-400`（缺口在 399-400）
- 严重度：major
- 置信度：confirmed
- 现象与依据：新增的运行时派发重载只有 `order == 1/2/3/4` 四个分支，**没有 `else`**。若
  `order` 为 0 或 >4（`order` 是插件/用户可传的 `int`），函数直接返回：`Fxp/Fyp/Fzp` 保持调用者传入的
  值，即"场没有被 gather"。对比同文件模板版：`Compute_shape_factor<depos_order>`
  （`Modules/Particles/ShapeFactors.H:37-88`）对未知阶数走 `WARPX_ABORT_WITH_MESSAGE`，即编译期版本
  会明确报错。此处的失败模式是"粒子用零场推进"——静默错误物理，正是审计重点类型。
  当前树内调用点都做了前置校验，所以今天不触发：`PhysicalParticleContainer.cpp:1352/1357` 传
  `orders.e_shape/b_shape`，而 `SemiImplicitDarwin.cpp:68-69` 断言 `magnetostatic_particle_shape ∈
  [1, nox]`、`e_shape = config().nox()`（`algo.particle_shape` 本身限 1–4）。但 Core 对外暴露的
  接口是插件契约，越界就是静默失效。
- 建议修法：在 `else` 里 `amrex::Abort("doDirectGatherVectorField: unsupported order")`（GPU 上
  用 `AMREX_ALWAYS_ASSERT`/`WARPX_ABORT_WITH_MESSAGE` 的 device 友好版本），把 `order` 校验成
  `1 <= order <= 4`。
- 验证方法：单测或 grep：新增一个 `FieldGather.H` 的 host 测试，`order=5` 时应 abort；
  `grep -n "order == " Source/Core/Modules/Particles/Gather/FieldGather.H` 可见无兜底分支。

### F2 split-gather 路径把 B 的 Galerkin 降阶丢了（与 fused 路径语义不一致）
- 文件:行：`Source/Core/Modules/Particles/Gather/FieldGather.H:500-511`（fused 语义）与
  `Source/Plugins/ParticleCore/Particles/PhysicalParticleContainer.cpp:1357-1361`（split 调用，
  写死 `false`）；API 定义在 `FieldGather.H:346-349`
- 严重度：major
- 置信度：likely
- 现象与依据：上游 `doGatherShapeN<depos_order, galerkin_interpolation>` 在 x 方向对
  **`ex`、`by`、`bz`** 同时用降一阶的 shape factor（`sx_ex/sx_by/sx_bz` 取 `*_galerkin` 数组，
  FieldGather.H:500/504/505），因为 Yee 网格上这三个分量在 x 上是 NODE。fork 的新接口把
  "降阶"退化成一个标量 `reduce_para`，语义是"每个分量沿自身方向降一阶"，这对 E 恰好等价
  （`doDirectGatherVectorField<E_ord, E_ord-1>` ↔ 上游 E 的 galerkin 分组），但对 B 不等价：
  B 的 nodal 方向不是"自身方向"，所以调用点只能传 `false`（`orders` 结构里也没有 `b_galerkin`
  字段，见 `Modules/Particles/FieldGatherOrders.H:24-29`）。结果：当
  `implicit_evolve.predictor_use_galerkin = 1` 时（`SemiImplicitDarwin.cpp:191-193` 把
  `m_predictor_use_galerkin` 传给 `e_galerkin`），同一次推进的 E 用降阶、B 用满阶，
  而上游的 `doGatherShapeN` 是 E、B 都降阶——即 **fork 的 Darwin predictor 对 B 的插值模板
  与上游不同**（B 插值误差回到 O(h^nox) 而非 O(h^(nox-1))）。
- 建议修法：最小改动是给 `FieldGatherOrders` 加 `bool b_galerkin`，并在
  `doDirectGatherVectorField` 里按分量分别决定 para 阶数（B 需要"在 x、z 同时降阶"的第三套
  模板参数），或直接用两个模板函数 `doDirectGatherE<ord,galerkin>` / `doDirectGatherB<ord>`；
  若确认 implicit Darwin 只想要"分量自向降阶"，则应在 `SemiImplicitDarwin.cpp:191` 附近加注释
  说明"与上游 galerkin 语义不同"，并在回归测试里锁住数值。
- 验证方法：构造 `implicit_evolve.predictor_use_galerkin = 1` 的一维/二维 Darwin 算例，与
  `git stash` 掉 split_gather（强制 fused）的结果比较 B 场/粒子轨迹，应能看到 O(h) 量级差异；
  或静态检查：`grep -n "b_galerkin" Source` 无命中。

### F3 `ImplicitOptions::particle_shape` 未初始化，且新增的 `ncomp_*` 块是死状态
- 文件:行：`Source/Core/Modules/FieldSolver/ImplicitOptions.H:16-23`（`particle_shape` 在 `:23`）
- 严重度：minor
- 置信度：confirmed
- 现象与依据：fork 在 Core 的 `ImplicitOptions` 里新增了
  `ncomp_xx/yy/zz/ncomp_pc_xx/yy/zz`（都有 `= amrex::IntVect{0}`）和 `int particle_shape;`
  （**无初值**）。全树核查：这些 `ncomp_*`/`particle_shape` 成员**没有任何读取点**
  （`grep -rn "ncomp_xx\|\.particle_shape" Source` 只命中 `ImplicitSolver::m_ncomp_xx` 这套
  自己的成员和 `ImplicitSolver.cpp:822` 的写入），真正的状态是 `ImplicitSolver.H:388/402` 的
  `m_ncomp_xx`/`m_ncomp_pc_xx`。所以 `particle_shape` 是"写而不读"的未初始化标量：一旦后续
  有消费方（插件）读到它，就是 UB/随机 shape。同时"同一物理量存在两份状态"（`ImplicitOptions`
  与 `ImplicitSolver::m_*`）是明显的漂移风险。另注：该结构体跨 Core/插件边界按引用/指针传递
  （`ImplicitKernelServices.H:77/81`、`IParticleSystem.H:74`），新增成员改变了布局；若存在按旧布局
  编译的外部插件，属于 ABI 变化（`PluginABI.H` 未见版本号随之调整）。
- 建议修法：删掉 `ImplicitOptions` 里这 7 个成员（保留在 `ImplicitSolver` 内），或至少写成
  `int particle_shape = 0;`；若确有插件需要，改为显式 `set/get` 并在 ABI 版本里体现。
- 验证方法：`grep -rn "particle_shape" Source | grep -v "magnetostatic_particle_shape"`；
  `-Wuninitialized` 下无告警（因为是聚合成员），故需靠代码审查。

### F4 `FilterFunctors.H` 存在两份同 include-guard 的副本
- 文件:行：`Source/Core/Modules/Particles/FilterFunctors.H:1-179` 与
  `Source/Core/Modules/Particles/Filter/FilterFunctors.H:1-179`（两者 `diff` 为空，guard 都是
  `WARPX_FILTERFUNCTORS_H`）
- 严重度：minor
- 置信度：confirmed
- 现象与依据：`grep -rn "FilterFunctors.H" Source` 显示只有 `WarpXOpenPMD.cpp:15` 与
  `FlushFormatPlotfile.cpp:9` 引用 `Modules/Particles/Filter/FilterFunctors.H`；`Modules/Particles/`
  下那份是孤儿。因为 guard 同名，今天"包含两份"不会有重复定义错误——但这也意味着：若以后有人
  只改孤儿副本，编译**静默**沿用被包含的那份（或反之），是典型的"改了却没生效"陷阱。该文件
  在全局作用域定义 `using SuperParticleType = …`（`:19`），两份不一致时还会变成静默 ODR 违规。
- 建议修法：删除 `Source/Core/Modules/Particles/FilterFunctors.H`（保留 `Filter/` 下的那份）。
- 验证方法：`ls Source/Core/Modules/Particles/FilterFunctors.H`；全量构建后
  `grep -rn "Modules/Particles/FilterFunctors.H" Source` 应为空。

### F5 `Fields.H`：FieldType 枚举中段插入新值（数值全平移）——已核查无消费者，仅记录风险
- 文件:行：`Source/Core/Fields.H:41-43`（`rho_medium_fp`）、`:75-78`（`area_mod`、`dual_A_y/dual_L_x/dual_L_z`）
- 严重度：minor
- 置信度：confirmed（风险已排查，当前无缺陷）
- 现象与依据：与上游相比在枚举**中间**插入了 4 个新 FieldType，其后所有枚举值整体平移。核查结论：
  (a) 无任何代码按整数值索引/序列化 `FieldType`（`grep -rn "\[FieldType::\|static_cast<int>(.*FieldType" Source`
  无命中；`Runtime/FieldRegistry.H` 不引用 `FieldType`）；(b) 键是名字（`AMREX_ENUM`），
  `MultiFabRegister` 按字符串/枚举哈希查找，故平移安全。
  (c) `ArrayFieldTypes` 新增的 `Efield_fp_external/Bfield_fp_external/hybrid_electron_velocity_fp/
  edge_lengths/face_areas/area_mod` 与注册方式**一致**：前三个与 EB 四个都在
  `Source/Plugins/PicEm/WarpX.cpp:2986-3024/3234-3265` 用 `Direction{0..2}` 三向注册，
  `HybridPICModel.cpp:265-271` 同样三向注册；而真正是标量的 `dual_A_y/dual_L_x`
  （`WarpX.cpp:3001-3002` 用 `AllocLevelFieldScalar`）**没有**进 `ArrayFieldTypes`，一致。
  唯一残余风险是插件 ABI：`FieldType` 值跨插件边界（`FieldRegistry`/`PluginABI`）时，
  旧插件二进制会拿到错位枚举。
- 建议修法：把新枚举值**追加在枚举末尾**（保持既有值不变），或在 `PluginABI.H` 里加
  `static_assert` 锁住关键值 + 提升 ABI 版本。
- 验证方法：`git diff _upstream/warpX-26.10/Source/Fields.H Source/Core/Fields.H`。

### F6 `WarpXSolver{DOF,Vec}` 把 `"vector_potential_fp_nodal"` 映射到 `FieldType::vector_potential_fp_nodal`
- 文件:行：`Source/Core/Solvers/WarpXSolverVec.cpp:50-51`、`Source/Core/Solvers/WarpXSolverDOF.cpp:29-30`
- 严重度：minor
- 置信度：confirmed（偏离已核对为"与 fork 自身一致"，但分支不可达）
- 现象与依据：上游 `WarpXSolverVec.cpp:49-50` / `WarpXSolverDOF.cpp:28-29` 把该名字映射到
  **非 nodal** 的 `FieldType::vector_potential_fp`；fork 改成 `…_nodal`。判定：fork 内部是自洽的——
  磁静力学解实际存在 `vector_potential_fp_nodal`
  （`Plugins/FieldSolver/Magnetostatic/MagnetostaticSolverImpl.cpp:82/88`、
  `Plugins/PicEm/WarpX.cpp:2897-2899`），dot-mask 分支也只认 `…_nodal`
  （`WarpX.cpp:4039-4041`）；上游那条 `FieldType::vector_potential_fp` 分支在 `WarpX.cpp:3692`
  读的是从未被解的场。所以 fork 这处是**修好了上游的不一致**。但两边都不可达：全树没有
  `Define(..., "vector_potential_fp_nodal")` 的调用点（只有 `"Efield_fp"`/`"Bfield_fp"`），
  属死分支。
- 建议修法：无需改行为；建议删掉死分支或在注释里写明"磁静力学 solver vector 未使用"。
- 验证方法：`grep -rn "\"vector_potential_fp_nodal\"" Source`（应只剩这两处 + 报错字串）。

### F7 main.cpp：`Runtime` 构造期抛异常时 AMReX/MPI/PETSc 永不收尾（无 try/catch）
- 文件:行：`Source/Core/main.cpp:18`（`warpx::runtime::Runtime runtime(argc, argv);`）
- 严重度：minor
- 置信度：confirmed
- 现象与依据：`Runtime::Runtime`（`Runtime/Runtime.cpp:26-50`）先 `initialize_runtime_libraries`
  （MPI+AMReX+FFT+PETSc），随后 `create_solver_kernel` **会抛**
  `std::runtime_error("Runtime: no PIC kernel plugin is registered…")`（`KernelFactory.cpp:44-48`）、
  `PluginLoader::load` 也可能抛。构造函数抛异常 → 析构不执行 → `finalize_runtime_libraries()`
  永不调用；`main` 没有 `try/catch`，异常逃出 `main` 直接 `std::terminate`，用户看到的是
  "terminate called after throwing…" 而非真正的缺插件信息，且 AMReX 的 finalize 检查
  （未 `amrex::Finalize` 时的告警/abort）会掩盖根因。正常路径是对的（`~Runtime` 里
  `finalize()` + `finalize_runtime_libraries()`，`main.cpp:28` 显式 `finalize()` 幂等，
  `verbose()` 在 finalize 前读取，与上游顺序一致）。
- 建议修法：`main` 包 `try { … } catch (std::exception const& e) { amrex::Print()<<e.what();
  return 1; }`，或让 `Runtime` 构造函数只做无异常的事（把 `create_solver_kernel` 推到
  `initialize()`），保证"已初始化的库一定被收尾"。
- 验证方法：不链接任何 kernel 插件构建 `warpx` 并运行 → 观察退出码与输出；
  期望清晰报错而非 terminate。

## 已验证的偏离（不是缺陷，但需在审计记录中留痕）

- **A. RZ 多模 Picnic gather 的方位角相位符号改了（`FieldGather.H:1521`）**：
  上游 `doGatherPicnicShapeN` 用 `Complex{costheta_mid, +sintheta_mid}`，fork 改为
  `-sintheta_mid`。判定为**修正上游 bug**：沉积端用 `e^{+imθ}` 累加模（
  `Plugins/ParticleCore/Particles/Deposition/CurrentDeposition.H:88,244-255`：
  `xy0={cos,+sin}`，写 `2m-1←Re`、`2m←Im`），gather 必须是共轭 `e^{-imθ}`；同文件其它 gather
  （`:230`、`:706`、`:1263`）与注释 `:1894` 都写 `e^{-i m theta}`，唯独上游这一处是 `+`。
  带入 `:1896-1897` 的重构式 `+F_cos*xy.real() - F_sin*xy.imag()` 可得：
  `-sin` ⇒ `F_cos cos(mθ)+F_sin sin(mθ)`（正确）；上游 `+sin` ⇒ `…-F_sin sin(mθ)`（第 m≥1 模
  正弦分量符号反）。影响面：仅 `n_rz_azimuthal_modes > 1` 的隐式/RZ；对 m=0 惰性。
  **建议**：补一个 RZ 多模隐式算例作为回归，否则这处"静默修正"无人守护。
- **B. `ParticleUtils.H:205-211` 用 `amrex::Math::sincos` 替换 `std::cos/std::sin`**：
  已核 `AMReX_Math.H:303-307` 返回 `(first=sin, second=cos)`，`auto [sinph, cosph]` 顺序正确；
  `:277-281` 处 `[sin_phi, cos_phi]` 同样正确。无符号/顺序错误。
- **C. `WarpXSolverDOF::fill_global_dof`（`WarpXSolverDOF.cpp:174-196`）新增 `m_row_offsets`**：
  与上游 `proc_begin` 循环等价（本进程起始 = `m_row_offsets[MyProc()]`，总量 =
  `m_row_offsets[nprocs]`），非 MPI/单进程分支填 `{0, n}`；消费方
  `MatrixPCAlgMG`（`NonlinearSolvers/MatrixPCAlgMG.cpp:47,71` → `partition.define`）的
  "每进程首行 + 总量"契约一致。`dofRowOffsets()` 在 `Define()` 末尾
  （`fill_global_dof()`）之后才有内容，构造 PC 在此之后，安全。

## 不确定但值得复核的点

- **U1** `GetExternalEBField` 与 `ParticleTileIter` 的构造函数**声明在 Core、定义在插件**
  （`Plugins/ParticleCore/Particles/Gather/GetExternalFields.cpp:21` 与
  `WarpXParticleContainer.cpp:138/144`），并只对 `ParticleTileIter` 显式实例化
  （`GetExternalFields.cpp:121`、`LatticeElementFinder.cpp:191-194`）。今日所有调用点都用该类型，
  但 Core 的"迭代器契约"实际依赖插件侧定义与实例化：任何插件用别的迭代器类型实例化
  `GetExternalEBField` 都会链接失败。若目标是"Core 可独立编译"，建议把模板定义下沉到 Core
  头文件（或在 Core 里 `extern template` 声明）。
- **U2** `LatticeElementFinder.H:82-85/120-125` 模板化后，`InitLatticeElementFinderDevice`
  与 `GetFinderDeviceInstance` 的实现留在插件 `.cpp` 并显式实例化（U1 同类问题）。
- **U3** `IParticleReduce.H` 的两个重载当前**无任何使用者**（`grep -rn particleReduce Source`
  无命中）；且 `amrex::ParticleReduce<ReduceData>(pc, …)` 要求 PC 提供
  `ParConstIter`/`finestLevel`，能否直接吃 `IParticleContainer`（PureSoA 基类）未实例化验证 →
  首次使用时可能有编译错误。标注 needs-verification。
- **U4** `SmartUtils.H:38` 去掉 `noexcept`（与 `.cpp:19` 一致），属于放宽而不是收紧；
  `SmartUtils.cpp:31-36` 把上游 `::initialization_policies[kv.first]`（`operator[]` 会**向全局
  map 插入**未知键）改为显式 `? : InitializationPolicy::Zero`，行为等价且不再污染表——
  是改进，但注意"未注册属性默认 Zero"这一静默默认策略仍在（与本审计关注的
  "缺属性静默默认"同源，属设计选择而非新缺陷）。
- **U5** `ParticleIO.H:99-107` 的 doxygen 参数顺序与声明
  （`:110-114`：`tmp, data, config, is_full_diagnostic, …`）错位，且 `data` 缺 `@param`；
  `storePhiOnParticles` 同理。纯文档问题。
- **U6** `PETScReinitialize.cpp:26-29` 的 `reinitialize()` 在未注册时**静默什么也不做**
  （由 `Plugins/PicEm/Modules/Initialization/WarpXInit.cpp:57-66` 的静态对象注册）。
  注册依赖该 TU 被链接（当前有
  `WarpXKernel.cpp:196-200` 的 linker anchor 保障）；若将来把 registration 移到独立翻译单元
  而静态库没被引用，`PETScKSP_Wrapper.H:71`/`PETScSNES_Wrapper.H:131` 的 PETSc 重置会静默失效。
  建议在 `reinitialize()` 里对"PETSc 已 Finalize 但未注册"的情况加断言（可选）。
- **U7** `main.cpp` 与上游的收尾顺序差异（`&warpx` 单例 → `Runtime` 局部对象）：本地靠
  `~Runtime` 在 `main` 结尾（`ABLASTR_PROFILE_VAR_STOP` 之后）调用 `finalize_runtime_libraries`，
  成员（`m_state` 等）在 `amrex::Finalize()` 之后才析构，但这些成员都不持有 AMReX 资源
  （`m_kernel/m_bus/m_plugin_host` 已在 `Runtime::finalize()` 里 `reset()`），故无
  use-after-finalize。若将来给 `Runtime` 加持有 MultiFab/DistributionMapping 的成员，这条不变量会破
  ——建议在 `Runtime.H` 里写明"析构函数体之后不得再有 AMReX 资源成员"。
- **U8** `WarpXSolverVec.H:342-353` 的 `operator=` 会复制 `m_context` 并把
  `m_is_defined = true`（上游复制 `m_WarpX`，同样是弱检查）：若源对象未 `Define`
  （`m_context == nullptr`），随后任何 `m_context->…` 都是空指针解引用。与上游同风险，未新增缺陷。
