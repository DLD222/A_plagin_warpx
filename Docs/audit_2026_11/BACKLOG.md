# Review 条目总表（189 条）

状态：`fixed` 已修复（附提交/验证）· `invalid` 复核后不成立（26.11 基准下本来就是对的）·
`todo` 待处理。本表由 review 文件自动抽取标题与严重度生成，状态列手工维护。

| 报告 | 条目 | 严重度 | 标题 | 状态 |
|---|---|---|---|---|
| Benchmarks | F1 | major | `mie_cylinder.rcs()` 多乘了 `j^n`，RCS 角分布整体旋转 90°（自检打印 0.93 却无断言） | todo |
| Benchmarks | F2 | major | Mie `by` 偏振的数值归一化少一个 `1/c`，近场 L2 误差恒为 ~100% | todo |
| Benchmarks | F3 | major | `sigma_exponential_test.py` 用 Crank–Nicolson 冒充代码里的 backward Euler | todo |
| Benchmarks | F4 | major | `inputs_tm` 的初值磁场符号反了 → 波朝 −z 传播（与算例说明相反） | todo |
| Benchmarks | F5 | major | `verify_medium_wave.py` 的"相速度"判据有 +105% 系统偏差，正确解也会 FAIL | todo |
| Benchmarks | F6 | major | 多个分析脚本没有对应的输入文件，无法复现 | todo |
| Benchmarks | F7 | major | 复现命令必然失败：所有 runner 的默认二进制路径是相对**运行目录**的 | todo |
| Benchmarks | F8 | minor | `inputs_ect_2d_degen` 的注释把 PEC 侧写反，且 `fp2` 落在导体内部使一半比较失效 | todo |
| Benchmarks | F9 | minor | `inputs_ect_2d_45deg_tm` 注释写 TE、实际是 TM | todo |
| Benchmarks | F10 | minor | 五个脚本周转着用了三个不同的 ε₀，而 README 的"约定"本身写错 | todo |
| Benchmarks | F11 | minor | `preflight.py` 的 σ 剖面包出死分支、非 `z>` 形式会崩、且 `eps_profile` 文档不符 | todo |
| Benchmarks | F12 | minor | `p1_tensor_reference.py` 的"切向旋转"用例是逐字节重复，未测任何东西；`--compare` 不返回失败 | todo |
| Benchmarks | F13 | minor | 关键阈值来自"观测到的散布"（事后调参），且没有预注册文件 | todo |
| Benchmarks | F14 | minor | 三份 md 的"代码事实"已被本 fork 自己的修改推翻，但仍以现状语气陈述 | todo |
| Benchmarks | F15 | minor | 目录卫生：`__pycache__/*.pyc` 入库 | todo |
| Benchmarks | F16 | minor | `conductor_slab` 的"纯前向波"说法与自身另一脚本矛盾；提交的探针恰在被排除区 | todo |
| Benchmarks | F17 | minor | 多处 `print` 无门禁 / 无 assert（"verify"脚本不 verify） | todo |
| Benchmarks | F18 | minor | `verify_fresnel_rt.py` 的 q1/q2 注释与代码相反（代码是对的，注释会误导） | todo |
| Benchmarks | F19 | minor | `make_mie_cases.py` 的注释/文档与实际网格不符；单网格是为了绕开未定性不稳定 | todo |
| Benchmarks | F20 | ? | `c03` 若干小问题（合并） | todo |
| Python | F1 | critical | `pywarpx` 仍调用已改名的旧迭代器类 `WarpXParIter` → 运行时 `AttributeError` | fixed |
| Python | F2 | major | 公开 API `ParticleContainerWrapper.deposit_charge_density` 被删（连同 C++ 绑定） | todo |
| Python | F3 | major | 测试容差被系统性放宽（6 处） | todo |
| Python | F4 | minor | 新增碰撞分析断言有复制粘贴错误 + 精确浮点相等 | fixed |
| Python | F5 | minor | `read_dims_from_file` 的 `FILE` 正则仍会把 `#` 吞进路径 | fixed |
| Python | F6 | minor | `Simulation` 的若干公开属性/扩展点被改为私有（无 property 兼容） | todo |
| Python | F7 | minor | 所有 PICMI 对象改为 pydantic 严格校验，未使用的多余关键字从"忽略"变"报错" | fixed |
| Python | F8 | minor | `_amrex_argv`：脚本自身的 `--flag` 若出现在第一个覆盖参数之后会破坏 ParmParse | fixed |
| Python | F9 | minor | 打包/卫生问题 | todo |
| Source_Core_Misc | F1 | major | `doDirectGatherVectorField(order,…)` 运行时版本对非法阶数**静默不做任何 gather** | fixed |
| Source_Core_Misc | F2 | major | split-gather 路径把 B 的 Galerkin 降阶丢了（与 fused 路径语义不一致） | todo |
| Source_Core_Misc | F3 | minor | `ImplicitOptions::particle_shape` 未初始化，且新增的 `ncomp_*` 块是死状态 | fixed |
| Source_Core_Misc | F4 | minor | `FilterFunctors.H` 存在两份同 include-guard 的副本 | todo |
| Source_Core_Misc | F5 | minor | `Fields.H`：FieldType 枚举中段插入新值（数值全平移）——已核查无消费者，仅记录风险 | todo |
| Source_Core_Misc | F6 | minor | `WarpXSolver{DOF,Vec}` 把 `"vector_potential_fp_nodal"` 映射到 `FieldType::vector_potential_fp_noda | todo |
| Source_Core_Misc | F7 | minor | main.cpp：`Runtime` 构造期抛异常时 AMReX/MPI/PETSc 永不收尾（无 try/catch） | todo |
| Source_Core_Runtime_A | F1 | major | `SpeciesHandle::numberOfParticles()` 永远返回启动时的粒子数（过期快照） | fixed |
| Source_Core_Runtime_A | F2 | major | `declare()` 的规格会参与 `add()` 的写冲突检查 → "先声明计划、后注册执行 handler"这一受支持模式会直接抛异常 | fixed |
| Source_Core_Runtime_A | F3 | major | 生命周期钩子 `on_level_created` / `on_checkpoint` / `on_restart` 从未被调用（死 API） | fixed |
| Source_Core_Runtime_A | F4 | minor | `StageContext::config` 只给 `RuntimeConfigSnapshot`，且该 snapshot 在一次 run 里不会刷新 | todo |
| Source_Core_Runtime_A | F5 | minor | `CollisionHandlerAdapter::apply` 的 null 检查是不可达的死代码，异常分支永远进不去 | todo |
| Source_Core_Runtime_A | F6 | minor | 命名回调总线：重名注册静默覆盖、缺失名字静默 no-op | todo |
| Source_Core_Runtime_A | F7 | minor | `IGeometryProvider.H` 的 include guard 是从 `ISimulationKernel` 抄来的 | todo |
| Source_Core_Runtime_A | F8 | minor | `IFiniteDifferenceSolver.H` 的工厂块写在 `#endif` 之后（不在 include guard 保护内） | todo |
| Source_Core_Runtime_A | F9 | minor | `IFieldSolver.H` / `IFieldSolverEngine.H` 是完全没有实现、没有使用者的死接口 | todo |
| Source_Core_Runtime_A | F10 | minor | `ordered_specs()` 在"只有声明没有 handler"时退化为纯 priority 排序（跳过依赖拓扑排序与环检测） | todo |
| Source_Core_Runtime_A | F11 | minor | `DataBus::alias` 对"已存在的目标名"不做任何声明校验 | todo |
| Source_Core_Runtime_A | F12 | minor | `FieldGatherOrders` 的成员没有默认初始化器 | todo |
| Source_Core_Runtime_A | F13 | minor | 碰撞适配器把物种名硬编码成 `electrons/ions/gas/photons` | todo |
| Source_Core_Runtime_B | F1 | critical | `warpx_stage_field()`（写入口）把访问检查异常抛过 C ABI，违反自己的「返回 0、绝不抛」契约 | fixed |
| Source_Core_Runtime_B | F2 | major | `IPluginLifecycle` 的 7 个钩子里有 4 个从未被调用（幻觉式集成） | fixed |
| Source_Core_Runtime_B | F3 | major | 内核契约 `ISimulationKernel::finalize()` 从未被调用 | fixed |
| Source_Core_Runtime_B | F4 | major | `warpx_stage_geometry()` 无法对"不存在的层"返回 0，且没有上界检查 | fixed |
| Source_Core_Runtime_B | F5 | major | `StateRegistry::clear()` 不清理 scalar（契约与实现不符） | fixed |
| Source_Core_Runtime_B | F6 | major | `SpeciesHandle::numberOfParticles()` 是注册时刻的冻结快照，且全树无人调用 | todo |
| Source_Core_Runtime_B | F7 | minor | `ScalarRegistry::declare()` 不比较初值：同名 scalar 冲突被静默吞掉 | todo |
| Source_Core_Runtime_B | F8 | minor | `ScalarSpec::checkpoint` 被接受但从未实现（声明成功 = 静默失效） | todo |
| Source_Core_Runtime_B | F9 | minor | 服务注册表的"冲突"是**全局单串**：任一服务种类冲突会让所有无关服务创建失败 | todo |
| Source_Core_Runtime_B | F10 | minor | `legacy_runtime_data()` / `legacy_host_services()` 的函数级静态持有者不随 `clear_service_factories()` 重置 | todo |
| Source_Core_Runtime_B | F11 | minor | C ABI 把 handler 注册失败全部折叠成返回码 4，loader 又完全忽略它 | todo |
| Source_Core_Runtime_B | F12 | minor | `register_stage()` 可以注册执行引擎永远到不了的 ordinal，且没有任何提示 | todo |
| Source_Core_Runtime_B | F13 | minor | 三处工厂仍是"最后一个赢"且缺省静默返回 nullptr（与 ServiceFactory 的 F-09 修复不一致） | todo |
| Source_Core_Runtime_B | F14 | minor | `legacy_costs()`：注册了 provider，却没有任何调用者（死契约） | todo |
| Source_Core_Runtime_B | F15 | minor | `Runtime` 构造函数抛出时 AMReX 不会被 finalize（资源泄漏 + AMReX 退出断言） | todo |
| Source_Plugins_Collision_A | F1 | major | Bremsstrahlung 饱和分支把 `fmulti` 除以了“未夹紧的 arg”，光子权重与上游不同 | todo |
| Source_Plugins_Collision_A | F2 | minor | 三个“接受 `MultiParticleContainer const*`”的构造函数只声明、无定义（悬空声明） | todo |
| Source_Plugins_Collision_A | F3 | minor | `#include` 被放在文件第 1 行、版权头与 include guard 之前 | todo |
| Source_Plugins_Collision_A | F4 | minor | `BinaryCollisionUtils` 新增的两个转发重载在树内无调用者（死代码 + 多余快照） | todo |
| Source_Plugins_Collision_B | F1 | major | `pulsed_decay` 删除了上游必填参数 `fixed_product_weight`，无 BackwardCompatibility 保护，旧输入静默改物理 | todo |
| Source_Plugins_Collision_B | F2 | minor | `pulsed_decay` 新采样器：整片母粒子被销毁 + 产物权重按 `weight_ratio` 逐代缩小；统计方差与文档表述都变了 | todo |
| Source_Plugins_Collision_B | F3 | minor | `CollisionSpeciesTable` 的 `SpeciesHandle` 全套机制是死代码，且构造时付一次"全粒子数全局归约" | todo |
| Source_Plugins_Collision_B | F4 | minor | `InverseBremsstrahlung`：`calculateNuei` 在缺少回调时**静默空转**，随后无检查地解引用 `nuei_*` / `N_*` 场 | fixed |
| Source_Plugins_Collision_B | F5 | minor | `NuclearFusionFunc::executor()` 无检查解引用只在 `AllocData` 里赋值的 `m_data`（以及 `fields().get()` 的返回值） | todo |
| Source_Plugins_Collision_B | F6 | minor | `declare_state` 与 `NuclearFusionFunc` 用**不同机制**解析同一个 `save_particle_production`，schema 路径可能被静默跳 | todo |
| Source_Plugins_Collision_B | F7 | minor | `declare_state` 只在 kernel 路径生效；legacy/Python 路径的 handler 是"首步懒创建"，永不走 schema | todo |
| Source_Plugins_Collision_B | F8 | minor | 头文件/包含卫生类残留（guard 外的 include、重复 include、空 `#ifdef`、被遗留的重载） | todo |
| Source_Plugins_Diagnostics_A | F1 | major | 诊断 rho 的 ngrow 断言把"各向同性分配"与"逐方向需求"相比，cfl>1 时直接 abort | fixed |
| Source_Plugins_Diagnostics_A | F2 | critical | DivE/DivB 诊断用 `config_snapshot().ncomps` 分配，legacy(PICMI) 路径上恒为 1 → RZ 多模写越界/只留 mode 0 | fixed |
| Source_Plugins_Diagnostics_A | F3 | minor | checkpoint 新增的 `field_schema.txt` 由所有 MPI rank 同时写，无 IOProcessor 保护 | fixed |
| Source_Plugins_Diagnostics_A | F4 | minor | BTD 的场 functor 从未拿到 RuntimeContext，`config_snapshot()` 静默返回全默认值 | fixed |
| Source_Plugins_Diagnostics_A | F5 | minor | `LorentzTransformParticles` 绕过传入的 context，直接用 `FrameState::FromLegacy()` | fixed |
| Source_Plugins_Diagnostics_A | F6 | minor | 诊断 J 的按需沉积新增 `ApplyJfieldBoundary`，边界处与上游不同 | invalid |
| Source_Plugins_Diagnostics_A | F7 | minor | 用户可见报错消息被替换成 C++ 语法；注释与代码不一致 | fixed |
| Source_Plugins_Diagnostics_B | F1 | critical | regrid 后 TimeAveraged 诊断的累加和被清零（静默错误） | fixed |
| Source_Plugins_Diagnostics_B | F2 | major | LoadBalanceCosts 诊断写入仿真的 live cost 数组（诊断产生副作用） | fixed |
| Source_Plugins_Diagnostics_B | F3 | minor | `GetExternalEBField(pti, offset, myspc.runtime_context())` 的 parser 分支不判空 | fixed |
| Source_Plugins_Diagnostics_B | F4 | minor | 头文件里把 `#include` 写进了版权注释 | fixed |
| Source_Plugins_Diagnostics_B | F5 | minor | `WarpXOpenPMDPlot` 对 `m_runtime_context` / `config_snapshot` 直接解引用 | todo |
| Source_Plugins_Diagnostics_B | F6 | minor | 非 EB 构建下 ChargeOnEB 从"运行时明确报错"变成"未注册类型" | fixed |
| Source_Plugins_Diagnostics_B | F7 | minor | 基类新增 `unique_ptr` 成员使"默认拷贝"静默失效 | todo |
| Source_Plugins_FieldSolver_A | F1 | major | 宏观介质 E 推进：`algo.macroscopic_sigma_method`（Lax-Wendroff）被静默忽略，永远走一阶 Backward-Euler | fixed |
| Source_Plugins_FieldSolver_A | F2 | major | ECT 共形 Ampere 路径丢掉 div(E) 清理项 `grad(F)`，F 场只演化不生效 | todo |
| Source_Plugins_FieldSolver_A | F3 | major | 静电场 1D 分支（`computeE` 的 `#else`）丢失 `-1` 项：共位网格 + 径向几何时 `E_r` 退化为 `β_xβ_z·∂φ` | todo |
| Source_Plugins_FieldSolver_A | F4 | minor | 有效势 ES：rho 的边界折叠/RZ 体权标被挪到 sum 之后 | todo |
| Source_Plugins_FieldSolver_A | F5 | minor | 隐式求解器：`mass_matrices_pc_width` 默认 0→1，叠加新 abort，3D Villasenor 旧算例会直接中止 | todo |
| Source_Plugins_FieldSolver_A | F6 | minor | 新张量材料填充只按二维取样（3D 下 y 方向冻结） | todo |
| Source_Plugins_FieldSolver_A | F7 | minor | 亚像素张量：正定性/退化检查用 `AMREX_ASSERT`（release 构建被编译掉） | todo |
| Source_Plugins_FieldSolver_B | F1 | major | PSATD 谱求解器工厂：PML 调用点漏填 `lev` / `n_rz_azimuthal_modes`（RZ 下静默用 0 层、1 个方位模） | todo |
| Source_Plugins_FieldSolver_B | F2 | minor | `WeightedJacobiImpl::Update()` 每次 PC 更新都用 Gershgorin 界覆盖已自适应的 omega | todo |
| Source_Plugins_FieldSolver_B | F3 | minor | Newton 发散判据从"直接 abort"改为"警告 + exit_status=-9" | todo |
| Source_Plugins_FieldSolver_B | F4 | minor | `SpectralFieldData.H` 重复的前置声明 | todo |
| Source_Plugins_FieldSolver_B | F5 | major | Darwin MLMG 预条件系数 `chi_dd` 去掉交错插值，改成"同下标直取"（半格偏移） | todo |
| Source_Plugins_FieldSolver_B | F6 | minor | `SemiImplicitDarwin::m_magnetostatic_particle_shape` 未初始化 | fixed |
| Source_Plugins_FieldSolver_B | F7 | major | PETSc 分支（含全部新逻辑）在本审计构建中**完全没有编译** | todo |
| Source_Plugins_FieldSolver_B | F8 | minor | 类型/契约不一致：`SpectralFieldData` 持 `RuntimeContext*`，`SpectralFieldDataRZ` 持 `RuntimeData*` | todo |
| Source_Plugins_FieldSolver_B | F9 | minor | `precon_mat.verbose` 改名：旧输入 `pc_petsc.verbose` 直接 abort | todo |
| Source_Plugins_FieldSolver_B | F10 | ? | 备注（已核对为**良性**，但看起来像缺陷）：`ThetaImplicitEM::ResetStep()` 不恢复 `Efield_fp`/`B_old` | todo |
| Source_Plugins_FieldSolver_B | F11 | minor | `MagnetostaticSolverImpl::defineVectorPotentialBCs(geom)` 公开接口会用到默认初始化的边界类型 | todo |
| Source_Plugins_ParticleCore_A | F1 | critical | `GetExternalEBField` 未拿到 `RuntimeContext` 时静默丢掉加速器晶格场，并在 parser 外场下空指针解引用 | todo |
| Source_Plugins_ParticleCore_A | F2 | minor | `Filter::ApplyStencil` 把 load-balance cost 恒置为 `nullptr`，负载均衡计时统计被静默删除 | todo |
| Source_Plugins_ParticleCore_A | F3 | minor | `warpx.read_fields_from_path` 由可选变成必需（`query` → `get`） | todo |
| Source_Plugins_ParticleCore_A | F4 | minor | eV 温度的 `mass`/`species_type` 解析绕开了 `source_name`，且与 `injection_file` 提供的质量不兼容 | todo |
| Source_Plugins_ParticleCore_A | F5 | minor | `maxwell_juttner` 的 `theta` 输入被删除，没有 BackwardCompatibility guard | todo |
| Source_Plugins_ParticleCore_B | F1 | critical | 新抽象把 `GetExternalEBField` 的容器来源改成"只来自 ctx"，QED / 离子化三条路径不传 ctx：解析式外场下空指针崩溃，lattice/plasma-lens  | todo |
| Source_Plugins_ParticleCore_B | F2 | critical | `DepositCharge` 丢掉 RZ/RCYLINDER/RSPHERE 的 `1/(2πr)` 反体积缩放，唯一的替代 finalizer 不覆盖 `rho_fp_<species> | todo |
| Source_Plugins_ParticleCore_B | F3 | major | `WarpXParticleContainer::frame()` 用一次性 `static` 快照，与"live view"设计及可用的 `legacy_frame()` 提供者相矛盾 | todo |
| Source_Plugins_ParticleCore_B | F4 | major | `full_mass_matrices` 的判据从 `Szz->nComp() > 1` 换成 `implicit_options.use_mass_matrices_jacobian`，两 | todo |
| Source_Plugins_ParticleCore_B | F5 | minor | `DepositCurrentOnLevel` 的文档仍声称"apply boundary conditions"，实现只有 guard-cell 求和 | todo |
| Source_Plugins_ParticleCore_B | F6 | minor | 新增的三个隐式辅助函数把层号 hard-code 为 `lev = 0`（多层的循环被注释掉） | todo |
| Source_Plugins_ParticleCore_B | F7 | minor | 新增契约实现里缺属性保护与"接口注释重复"等小问题 | todo |
| Source_Plugins_ParticleCore_B | F8 | minor | RSPHERE 的隐式质量矩阵角度分支是本 fork 新增（上游 RZ/RCYLINDER only），球坐标角量耦合到"平均半径/瞬时半径"混用 | todo |
| Source_Plugins_ParticleCore_B | F9 | minor | `villasenor_mass_matrices_max_grid_crossings` 的单一真源被复制成魔法数 `3` | todo |
| Source_Plugins_ParticleCore_B | F10 | minor | `GetChargeDensityOnLevel(local=true, finalize=true)` 现在直接 assert；Python 绑定的 `finalize` 默认 `true | todo |
| Source_Plugins_PicEm_A | F1 | major | AMR 时间步中的 regrid 路径是新写的，缺两步收尾 | todo |
| Source_Plugins_PicEm_A | F2 | major | `ComputeDualGeometry` 只在 XZ 下真正实现，却在所有维度被调用/断言 | todo |
| Source_Plugins_PicEm_A | F3 | minor | `ComputeDualGeometry` 留下无条件调试打印与自相矛盾的断言文案 | fixed |
| Source_Plugins_PicEm_A | F4 | minor | 3D 张量介质中 `rho_medium` 种子的 Dy 用了错的 eps 分量 | fixed |
| Source_Plugins_PicEm_A | F5 | minor | 3D 各向异性下另一处同类风险（`eps2` 变量未使用，`Ex2/Ez2` 的 FillBoundary 缺 `Ey2`） | todo |
| Source_Plugins_PicEm_A | F6 | minor | `bnd_shrink` 用域周期判断却作用于每块 tilebox ⇒ CPU 分块时 rho_medium 少更新一层 | todo |
| Source_Plugins_PicEm_A | F7 | minor | `fluids.moment` 原型只断言 `AMREX_SPACEDIM==2`，RZ 构建同样满足 ⇒ 静默按直角算 | todo |
| Source_Plugins_PicEm_A | F8 | minor | 电荷守恒残差返回值被丢弃 | todo |
| Source_Plugins_PicEm_A | F9 | minor | `AddExternalFields` 的层循环由 `max_level` 改为 `finestLevel()` | todo |
| Source_Plugins_PicEm_A | F10 | minor | `WARPX_DISABLE_DIAGNOSTICS` 环境变量会静默替换已构造的诊断服务 | todo |
| Source_Plugins_PicEm_A | F11 | major | 流体–Maxwell 耦合源的 “内能不变” 能量更新多乘了一个 rho | fixed |
| Source_Plugins_PicEm_A | F12 | minor | `Maxwell6` 数值通量注释声称精确 Roe/upwind，实现是分量式 Rusanov | todo |
| Source_Plugins_PicEm_B | F1 | major | radial 几何下 rho 的 volume-weighted filter 用了错误的标定分支 | invalid |
| Source_Plugins_PicEm_B | F2 | major | 每次 load-balance/regrid 都重建 PML，PML 内的场被清零 | todo |
| Source_Plugins_PicEm_B | F3 | major | Python 绑定类名 `WarpXParIter` → `ParticleTileIter`，pywarpx 调用点未同步 | fixed |
| Source_Plugins_PicEm_B | F4 | major | legacy 宿主/数据适配器用函数局部 `static` 冻结了第一个 WarpX 实例 | todo |
| Source_Plugins_PicEm_B | F5 | minor | 三个 MomentsSolver 测试二进制永远返回 0，FAIL 只打印不生效 | fixed |
| Source_Plugins_PicEm_B | F6 | minor | `run_tests.sh` 的路径在插件化移动后失效 | fixed |
| Source_Plugins_PicEm_B | F7 | minor | `GetPartContainer()` 无空指针保护，与 `particleSystem()` 的设计不一致 | fixed |
| Source_Plugins_PicEm_B | F8 | minor | 诊断 rho 现在也走 `FinalizeRhoOnLevel`（含 PEC/反射边界清零），与上游诊断语义不同 | invalid |
| Source_Plugins_PicEm_B | F9 | minor | `ParticlePushHandler` 的 HandlerSpec 声称不读任何场 | fixed |
| Source_Plugins_PicEm_B | F10 | minor | `WarpXRuntimeData.H` 的 include 落在版权头与 include guard 之前；`WarpXHostServices.cpp` 有两个 `namespace`  | fixed |
| Source_Plugins_Star | F1 | critical | 【critical】1D/2D 下 `bx.smallEnd(1)/(2)` 越界读 → 循环上界是垃圾，写穿 FAB | fixed |
| Source_Plugins_Star | F2 | critical | 【critical】自引力在 runtime 中完全不存在：不算 `g`、也不用 `g` | fixed |
| Source_Plugins_Star | F3 | critical | 【critical】单位系统三处互斥（cgs 状态 vs SI 几何 vs SI 泊松），可观测 | fixed |
| Source_Plugins_Star | F4 | critical | 【critical/major】`applyElectromagneticSources` 取错场分量：E=(Ex,Ex,Ex) | fixed |
| Source_Plugins_Star | F5 | major | 【major】高斯 cgs 下洛伦兹力缺 1/c（同时又是 SI 公式，两套都说不通） | fixed |
| Source_Plugins_Star | F6 | major | 【major】HLLC/MUSCL 全链路无正定性/下限保护，强膨胀必产生 NaN | fixed |
| Source_Plugins_Star | F7 | major | 【major】显式扩散子步 0.2·dx²/D 超过 3D 稳定极限 dx²/(6D)，与"绝不静默失稳"的自述相反 | fixed |
| Source_Plugins_Star | F8 | major | 【major】辐射扩散把未初始化内存 `ParallelCopy` 进辐射场 | fixed |
| Source_Plugins_Star | F9 | major | 【major】开放边界（Dirichlet）的边界数据从未填充 → MLMG 用未初始化 ghost | fixed |
| Source_Plugins_Star | F10 | major | 【major】"AMR 多层势"实为逐层独立求解 + 每层重新盖同一解析剖面（含 restart 语义错误） | todo |
| Source_Plugins_Star | F11 | major | 【major】1D 构建下 `applyCentrifugalSource` 的 assert 必崩（默认参数就是非法值） | todo |
| Source_Plugins_Star | F12 | major | 【major】`test_solar_column` 的能量账本断言是自我比较（恒真），且源项全部关闭 | todo |
| Source_Plugins_Star | F13 | major | 【major】`test_stellar_model` 栈数组越界：`w[4]` 被索引到 8 | fixed |
| Tools_Tests | F1 | critical | `check_plugin_boundaries.py` 的两条检查被无条件丢弃（可复现反例） | fixed |
| Tools_Tests | F2 | major | `parse_tests.py` 把仓库内的上游副本 `_upstream/` 当成"本地树"，使上游一致性门恒过 | fixed |
| Tools_Tests | F3 | major | `check_kernel_coupling.py` 的 5 个监控目录有 3 个不存在，静默跳过仍报 OK | fixed |
| Tools_Tests | F4 | major | `check_legacy_coverage.py` 候选路径不含 `Core/Modules/` 与其它插件根，554/829 假"缺失"；且与文档"missing: 0"冲突 | fixed |
| Tools_Tests | F5 | major | `check_core_purity.py` 的匹配规则可被命名空间别名绕过，注释判定也不可靠 | fixed |
| Tools_Tests | F6 | major | 四个"不变量检查器"没有进入任何自动门（CI / pre-commit / run_gates 都没有） | fixed |
| Tools_Tests | F7 | major | `route_tasks.py` 可以在"什么都没验证"的情况下把任务标成 done，且 153/153 已是 done | fixed |
| Tools_Tests | F8 | major | macroscopic 物理核验脚本根本不做比较：只打印，调用方只 grep 关键词 | fixed |
| Tools_Tests | F9 | major | A/B 上游对照工具链永远不会失败（run_ab 忽略退出码、run_all 恒 exit 0、compare_fields 只打印） | todo |
| Tools_Tests | F10 | major | `check_star_runtime_plugin.sh` 明确容忍"关机时堆损坏"的非零退出 | fixed |
| Tools_Tests | F11 | major | OPAL 不透明度自检的容差（1 dex / 中位数 2 倍、只抽样 1/4 层）宽到无法发现系统性错误 | todo |
| Tools_Tests | F12 | major | `eos.py` 的 Debye 屏蔽离子和对与自身文档不符（用 (Z+1)² 代替 Z²），HeⅡ 的连续谱降低也用错倍数 | todo |
| Tools_Tests | F13 | major | `stellar_structure.selftest` 的"四方程积分"只检查打印文本里出现 `R/R_sun`，NaN 也能通过（且已被注册为 ctest） | todo |
| Tools_Tests | F14 | major | 新增 pytest 用例在**仓库根**写文件，`mkdir()` 无 `exist_ok`，并行维度互相覆盖 | todo |
| Tools_Tests | F15 | major | `test_plugin_lifecycle.cpp` 的 "satisfied requirement" 用例断言的是"被拒绝"，role 名作为 provider 从未被测 | todo |
| Tools_Tests | F16 | major | `check_build_boundaries.py` 的分层覆盖不完整，并留有 KeyError 地雷 | todo |
| Tools_Tests | F17 | minor | 5 个 runtime "contract test" 实际是 `int main(){return 0;}` 且没有注册进 CTest | todo |
| Tools_Tests | F18 | minor | `test_nonpic_kernel.cpp` 打印的断言数写死为 8（实际 7） | todo |
| Tools_Tests | F19 | minor | `sod_reference.py` 稀疏波分支的解析导数写错（靠括号保护的 Newton 兜住） | todo |
| Tools_Tests | F20 | minor | `ssm_compare.py` 自检里有一条恒真断言，README 的"5%"与断言的 15% 不符，打印的 eps/kappa 比值在 SSM 为 0 处失真 | todo |
| Tools_Tests | F21 | minor | `solar_compare_plot.py --check` 向源码树写 PNG，且只钉住 L、R 两个数 | todo |
| Tools_Tests | F22 | minor | 插件测试的构建假定 ELF/macOS 以外的编译器与 `.so` 后缀 | todo |
| Tools_Tests | F23 | minor | 若干 shell 核验脚本的硬编码与静默 no-op | todo |
| Tools_Tests | F24 | minor | A0 两段流水线的输入路径对不上，文档化的"重新生成"无法执行 | todo |
| Tools_Tests | F25 | minor | 两个"门"的退出码恒为 0 | todo |
| Tools_Tests | F26 | minor | `check_field_solver_extraction.py` 永远 exit 0，且统计的是已经搬空的旧树 | todo |
| Tools_Tests | F27 | minor | `route_tasks.py` 的"JSON 权威"是死代码 | todo |
| Tools_Tests | F28 | minor | `a5_ab.py` 未接入任何门，且硬编码二进制与构建目录 | todo |
| Tools_Tests | F29 | minor | `build_and_run_4d.sh` 的 conda 前缀变量名疑似笔误 | todo |
| Tools_Tests | F30 | minor | `inventory_field_solver.py` 写入源码树文档，且 `classify()` 有多余/不可达分支 | todo |
| Tools_Tests | F31 | minor | 其它可靠性/自证性较弱的点（合并记录，共 9 条） | todo |
