# 代码审查汇总（189 条）

审查协议见 `REVIEW_PROTOCOL.md`；逐文件报告在 `reviews/`（17 份）。
覆盖：`Source/` 842 个 `.cpp/.H/.py` 全部通读（364 个与上游同路径不同内容做逐 hunk diff、
238 个新文件逐行读），另有 `Python/`、`Examples/` 的 PICMI 脚本、`Tools/`、`tests/`、`benchmarks/`。

## 总数

| 严重度 | 数量 |
|---|---|
| critical | 12 |
| major | 62 |
| minor | 113 |
| unrated | 2 |

## 按分区

| 分区报告 | critical | major | minor | 合计 |
|---|---|---|---|---|
| `reviews/Tools_Tests.md` | 1 | 15 | 15 | 31 |
| `reviews/Benchmarks.md` | 0 | 7 | 12 | 20 |
| `reviews/Source_Core_Runtime_B.md` | 1 | 5 | 9 | 15 |
| `reviews/Source_Plugins_Star.md` | 4 | 9 | 0 | 13 |
| `reviews/Source_Core_Runtime_A.md` | 0 | 3 | 10 | 13 |
| `reviews/Source_Plugins_PicEm_A.md` | 0 | 3 | 9 | 12 |
| `reviews/Source_Plugins_FieldSolver_B.md` | 0 | 3 | 7 | 11 |
| `reviews/Source_Plugins_ParticleCore_B.md` | 2 | 2 | 6 | 10 |
| `reviews/Source_Plugins_PicEm_B.md` | 0 | 4 | 6 | 10 |
| `reviews/Python.md` | 1 | 2 | 6 | 9 |
| `reviews/Source_Plugins_Collision_B.md` | 0 | 1 | 7 | 8 |
| `reviews/Source_Plugins_Diagnostics_A.md` | 1 | 1 | 5 | 7 |
| `reviews/Source_Plugins_Diagnostics_B.md` | 1 | 1 | 5 | 7 |
| `reviews/Source_Core_Misc.md` | 0 | 2 | 5 | 7 |
| `reviews/Source_Plugins_FieldSolver_A.md` | 0 | 3 | 4 | 7 |
| `reviews/Source_Plugins_ParticleCore_A.md` | 1 | 0 | 4 | 5 |
| `reviews/Source_Plugins_Collision_A.md` | 0 | 1 | 3 | 4 |

## critical 条目的当前状态（2026-11-15）

| # | 条目 | 状态 |
|---|---|---|
| 1 | pywarpx 仍调用已改名的 `WarpXParIter` | **已修**（`2829581`，两个名字并存） |
| 2 | `warpx_stage_field()` 把异常抛过 C ABI | **已修**（`deefbf6`：写入口 + `warpx_stage_geometry`/`has_field*` 全部 try/catch） |
| 3 | DivE/DivB 用 `config_snapshot().ncomps` | **已修**（`edd72c4`） |
| 4 | regrid 后 TimeAveraged 诊断累加和被清零 | 待修（`Source_Plugins_Diagnostics_B` 有复现步骤） |
| 5 | `GetExternalEBField` 拿不到 `RuntimeContext` → 空指针 | **已修**（`deefbf6`）；另见条目 6 |
| 6 | QED/电离三条路径不传 ctx | **已修**（`deefbf6`：三个 functor 显式接收并转发 context） |
| 7 | `DepositCharge` 丢掉 RZ 反体积缩放，finalizer 不覆盖 `rho_fp_<species>` | **不成立**：`HybridPICDepositRhoAndJ` 里 fork 已经调用 `FinalizeRhoOnLevel(rho_spec)` + `FinalizeJOnLevel`（26.10 是在沉积后直接 `ApplyInverseVolumeScalingToChargeDensity`），两者等价；且 26.11 也把 finalization 统一到这一层 |
| 8 | 1D/2D 下 `bx.smallEnd(1)/(2)` 越界 | **已修**（`6312f53`：196 处改为 `star_box_lo/hi`，四维编译 + 33 个插件单测通过） |
| 9 | 自引力在 runtime 中完全不存在（不算 g、也不用 g） | **已修**（本轮：`gravity_step` 用 `computeAcceleration` 发布 `star_g*`，此前是 `setVal(0.0)`） |
| 10 | 单位系统三处互斥（cgs 状态 vs SI 几何 vs SI 泊松） | **已修**（本轮：`solvePotential` 的 `G` 变成参数，star 插件按 cgs 状态传 `6.67430e-8 cm³ g⁻¹ s⁻²`） |
| 11 | `applyElectromagneticSources` 取错场分量 E=(Ex,Ex,Ex) | 待修 |
| 12 | `check_plugin_boundaries.py` 两条检查被丢弃 | **已修**（`89d248c`） |

## 全部条目

| # | 严重度 | 分区 | 标题 | 位置 |
|---|---|---|---|---|
| 1 | critical | Python | pywarpx` 仍调用已改名的旧迭代器类 `WarpXParIter` → 运行时 `AttributeError` | 见分区报告 |
| 2 | critical | Source_Core_Runtime_B | warpx_stage_field()`（写入口）把访问检查异常抛过 C ABI，违反自己的「返回 0、绝不抛」契约 | 见分区报告 |
| 3 | critical | Source_Plugins_Diagnostics_A | DivE/DivB 诊断用 `config_snapshot().ncomps` 分配，legacy(PICMI) 路径上恒为 1 → RZ 多模写越界/只留 mode 0 | 见分区报告 |
| 4 | critical | Source_Plugins_Diagnostics_B | regrid 后 TimeAveraged 诊断的累加和被清零（静默错误） | 见分区报告 |
| 5 | critical | Source_Plugins_ParticleCore_A | GetExternalEBField` 未拿到 `RuntimeContext` 时静默丢掉加速器晶格场，并在 parser 外场下空指针解引用 | 见分区报告 |
| 6 | critical | Source_Plugins_ParticleCore_B | 新抽象把 `GetExternalEBField` 的容器来源改成"只来自 ctx"，QED / 离子化三条路径不传 ctx：解析式外场下空指针崩溃，lattice/plasma-lens 外场静默丢失 | 见分区报告 |
| 7 | critical | Source_Plugins_ParticleCore_B | DepositCharge` 丢掉 RZ/RCYLINDER/RSPHERE 的 `1/(2πr)` 反体积缩放，唯一的替代 finalizer 不覆盖 `rho_fp_<species>`：RZ hybrid + pe | 见分区报告 |
| 8 | critical | Source_Plugins_Star | 【critical】1D/2D 下 `bx.smallEnd(1)/(2)` 越界读 → 循环上界是垃圾，写穿 FAB | 见分区报告 |
| 9 | critical | Source_Plugins_Star | 【critical】自引力在 runtime 中完全不存在：不算 `g`、也不用 `g` | 见分区报告 |
| 10 | critical | Source_Plugins_Star | 【critical】单位系统三处互斥（cgs 状态 vs SI 几何 vs SI 泊松），可观测 | 见分区报告 |
| 11 | critical | Source_Plugins_Star | 【critical/major】`applyElectromagneticSources` 取错场分量：E=(Ex,Ex,Ex) | 见分区报告 |
| 12 | critical | Tools_Tests | check_plugin_boundaries.py` 的两条检查被无条件丢弃（可复现反例） | 见分区报告 |
| 13 | major | Benchmarks | mie_cylinder.rcs()` 多乘了 `j^n`，RCS 角分布整体旋转 90°（自检打印 0.93 却无断言） | 见分区报告 |
| 14 | major | Benchmarks | Mie `by` 偏振的数值归一化少一个 `1/c`，近场 L2 误差恒为 ~100% | 见分区报告 |
| 15 | major | Benchmarks | sigma_exponential_test.py` 用 Crank–Nicolson 冒充代码里的 backward Euler | 见分区报告 |
| 16 | major | Benchmarks | inputs_tm` 的初值磁场符号反了 → 波朝 −z 传播（与算例说明相反） | 见分区报告 |
| 17 | major | Benchmarks | verify_medium_wave.py` 的"相速度"判据有 +105% 系统偏差，正确解也会 FAIL | 见分区报告 |
| 18 | major | Benchmarks | 多个分析脚本没有对应的输入文件，无法复现 | 见分区报告 |
| 19 | major | Benchmarks | 复现命令必然失败：所有 runner 的默认二进制路径是相对**运行目录**的 | 见分区报告 |
| 20 | major | Python | 公开 API `ParticleContainerWrapper.deposit_charge_density` 被删（连同 C++ 绑定） | 见分区报告 |
| 21 | major | Python | 测试容差被系统性放宽（6 处） | 见分区报告 |
| 22 | major | Source_Core_Misc | doDirectGatherVectorField(order,…)` 运行时版本对非法阶数**静默不做任何 gather** | 见分区报告 |
| 23 | major | Source_Core_Misc | split-gather 路径把 B 的 Galerkin 降阶丢了（与 fused 路径语义不一致） | 见分区报告 |
| 24 | major | Source_Core_Runtime_A | SpeciesHandle::numberOfParticles()` 永远返回启动时的粒子数（过期快照） | 见分区报告 |
| 25 | major | Source_Core_Runtime_A | declare()` 的规格会参与 `add()` 的写冲突检查 → "先声明计划、后注册执行 handler"这一受支持模式会直接抛异常 | 见分区报告 |
| 26 | major | Source_Core_Runtime_A | 生命周期钩子 `on_level_created` / `on_checkpoint` / `on_restart` 从未被调用（死 API） | 见分区报告 |
| 27 | major | Source_Core_Runtime_B | IPluginLifecycle` 的 7 个钩子里有 4 个从未被调用（幻觉式集成） | 见分区报告 |
| 28 | major | Source_Core_Runtime_B | 内核契约 `ISimulationKernel::finalize()` 从未被调用 | 见分区报告 |
| 29 | major | Source_Core_Runtime_B | warpx_stage_geometry()` 无法对"不存在的层"返回 0，且没有上界检查 | 见分区报告 |
| 30 | major | Source_Core_Runtime_B | StateRegistry::clear()` 不清理 scalar（契约与实现不符） | 见分区报告 |
| 31 | major | Source_Core_Runtime_B | SpeciesHandle::numberOfParticles()` 是注册时刻的冻结快照，且全树无人调用 | 见分区报告 |
| 32 | major | Source_Plugins_Collision_A | Bremsstrahlung 饱和分支把 `fmulti` 除以了“未夹紧的 arg”，光子权重与上游不同 | 见分区报告 |
| 33 | major | Source_Plugins_Collision_B | pulsed_decay` 删除了上游必填参数 `fixed_product_weight`，无 BackwardCompatibility 保护，旧输入静默改物理 | 见分区报告 |
| 34 | major | Source_Plugins_Diagnostics_A | 诊断 rho 的 ngrow 断言把"各向同性分配"与"逐方向需求"相比，cfl>1 时直接 abort | 见分区报告 |
| 35 | major | Source_Plugins_Diagnostics_B | LoadBalanceCosts 诊断写入仿真的 live cost 数组（诊断产生副作用） | 见分区报告 |
| 36 | major | Source_Plugins_FieldSolver_A | 宏观介质 E 推进：`macroscopic_solver`（Lax-Wendroff）被静默忽略，永远走一阶 Backward-Euler | 见分区报告 |
| 37 | major | Source_Plugins_FieldSolver_A | ECT 共形 Ampere 路径丢掉 div(E) 清理项 `grad(F)`，F 场只演化不生效 | 见分区报告 |
| 38 | major | Source_Plugins_FieldSolver_A | 静电场 1D 分支（`computeE` 的 `#else`）丢失 `-1` 项：共位网格 + 径向几何时 `E_r` 退化为 `β_xβ_z·∂φ` | 见分区报告 |
| 39 | major | Source_Plugins_FieldSolver_B | PSATD 谱求解器工厂：PML 调用点漏填 `lev` / `n_rz_azimuthal_modes`（RZ 下静默用 0 层、1 个方位模） | 见分区报告 |
| 40 | major | Source_Plugins_FieldSolver_B | Darwin MLMG 预条件系数 `chi_dd` 去掉交错插值，改成"同下标直取"（半格偏移） | 见分区报告 |
| 41 | major | Source_Plugins_FieldSolver_B | PETSc 分支（含全部新逻辑）在本审计构建中**完全没有编译** | 见分区报告 |
| 42 | major | Source_Plugins_ParticleCore_B | WarpXParticleContainer::frame()` 用一次性 `static` 快照，与"live view"设计及可用的 `legacy_frame()` 提供者相矛盾 | 见分区报告 |
| 43 | major | Source_Plugins_ParticleCore_B | full_mass_matrices` 的判据从 `Szz->nComp() > 1` 换成 `implicit_options.use_mass_matrices_jacobian`，两者在"只开 PC 质量矩阵"时不 | 见分区报告 |
| 44 | major | Source_Plugins_PicEm_A | AMR 时间步中的 regrid 路径是新写的，缺两步收尾 | 见分区报告 |
| 45 | major | Source_Plugins_PicEm_A | ComputeDualGeometry` 只在 XZ 下真正实现，却在所有维度被调用/断言 | 见分区报告 |
| 46 | major | Source_Plugins_PicEm_A | 流体–Maxwell 耦合源的 “内能不变” 能量更新多乘了一个 rho | 见分区报告 |
| 47 | major | Source_Plugins_PicEm_B | radial 几何下 rho 的 volume-weighted filter 用了错误的标定分支 | 见分区报告 |
| 48 | major | Source_Plugins_PicEm_B | 每次 load-balance/regrid 都重建 PML，PML 内的场被清零 | 见分区报告 |
| 49 | major | Source_Plugins_PicEm_B | Python 绑定类名 `WarpXParIter` → `ParticleTileIter`，pywarpx 调用点未同步 | 见分区报告 |
| 50 | major | Source_Plugins_PicEm_B | legacy 宿主/数据适配器用函数局部 `static` 冻结了第一个 WarpX 实例 | 见分区报告 |
| 51 | major | Source_Plugins_Star | 【major】高斯 cgs 下洛伦兹力缺 1/c（同时又是 SI 公式，两套都说不通） | 见分区报告 |
| 52 | major | Source_Plugins_Star | 【major】HLLC/MUSCL 全链路无正定性/下限保护，强膨胀必产生 NaN | 见分区报告 |
| 53 | major | Source_Plugins_Star | 【major】显式扩散子步 0.2·dx²/D 超过 3D 稳定极限 dx²/(6D)，与"绝不静默失稳"的自述相反 | 见分区报告 |
| 54 | major | Source_Plugins_Star | 【major】辐射扩散把未初始化内存 `ParallelCopy` 进辐射场 | 见分区报告 |
| 55 | major | Source_Plugins_Star | 【major】开放边界（Dirichlet）的边界数据从未填充 → MLMG 用未初始化 ghost | 见分区报告 |
| 56 | major | Source_Plugins_Star | 【major】"AMR 多层势"实为逐层独立求解 + 每层重新盖同一解析剖面（含 restart 语义错误） | 见分区报告 |
| 57 | major | Source_Plugins_Star | 【major】1D 构建下 `applyCentrifugalSource` 的 assert 必崩（默认参数就是非法值） | 见分区报告 |
| 58 | major | Source_Plugins_Star | 【major】`test_solar_column` 的能量账本断言是自我比较（恒真），且源项全部关闭 | 见分区报告 |
| 59 | major | Source_Plugins_Star | 【major】`test_stellar_model` 栈数组越界：`w[4]` 被索引到 8 | 见分区报告 |
| 60 | major | Tools_Tests | parse_tests.py` 把仓库内的上游副本 `_upstream/` 当成"本地树"，使上游一致性门恒过 | 见分区报告 |
| 61 | major | Tools_Tests | check_kernel_coupling.py` 的 5 个监控目录有 3 个不存在，静默跳过仍报 OK | 见分区报告 |
| 62 | major | Tools_Tests | check_legacy_coverage.py` 候选路径不含 `Core/Modules/` 与其它插件根，554/829 假"缺失"；且与文档"missing: 0"冲突 | 见分区报告 |
| 63 | major | Tools_Tests | check_core_purity.py` 的匹配规则可被命名空间别名绕过，注释判定也不可靠 | 见分区报告 |
| 64 | major | Tools_Tests | 四个"不变量检查器"没有进入任何自动门（CI / pre-commit / run_gates 都没有） | 见分区报告 |
| 65 | major | Tools_Tests | route_tasks.py` 可以在"什么都没验证"的情况下把任务标成 done，且 153/153 已是 done | 见分区报告 |
| 66 | major | Tools_Tests | macroscopic 物理核验脚本根本不做比较：只打印，调用方只 grep 关键词 | 见分区报告 |
| 67 | major | Tools_Tests | A/B 上游对照工具链永远不会失败（run_ab 忽略退出码、run_all 恒 exit 0、compare_fields 只打印） | 见分区报告 |
| 68 | major | Tools_Tests | check_star_runtime_plugin.sh` 明确容忍"关机时堆损坏"的非零退出 | 见分区报告 |
| 69 | major | Tools_Tests | OPAL 不透明度自检的容差（1 dex / 中位数 2 倍、只抽样 1/4 层）宽到无法发现系统性错误 | 见分区报告 |
| 70 | major | Tools_Tests | eos.py` 的 Debye 屏蔽离子和对与自身文档不符（用 (Z+1)² 代替 Z²），HeⅡ 的连续谱降低也用错倍数 | 见分区报告 |
| 71 | major | Tools_Tests | stellar_structure.selftest` 的"四方程积分"只检查打印文本里出现 `R/R_sun`，NaN 也能通过（且已被注册为 ctest） | 见分区报告 |
| 72 | major | Tools_Tests | 新增 pytest 用例在**仓库根**写文件，`mkdir()` 无 `exist_ok`，并行维度互相覆盖 | 见分区报告 |
| 73 | major | Tools_Tests | test_plugin_lifecycle.cpp` 的 "satisfied requirement" 用例断言的是"被拒绝"，role 名作为 provider 从未被测 | 见分区报告 |
| 74 | major | Tools_Tests | check_build_boundaries.py` 的分层覆盖不完整，并留有 KeyError 地雷 | 见分区报告 |
| 75 | minor | Benchmarks | inputs_ect_2d_degen` 的注释把 PEC 侧写反，且 `fp2` 落在导体内部使一半比较失效 | 见分区报告 |
| 76 | minor | Benchmarks | inputs_ect_2d_45deg_tm` 注释写 TE、实际是 TM | 见分区报告 |
| 77 | minor | Benchmarks | 五个脚本周转着用了三个不同的 ε₀，而 README 的"约定"本身写错 | 见分区报告 |
| 78 | minor | Benchmarks | preflight.py` 的 σ 剖面包出死分支、非 `z>` 形式会崩、且 `eps_profile` 文档不符 | 见分区报告 |
| 79 | minor | Benchmarks | p1_tensor_reference.py` 的"切向旋转"用例是逐字节重复，未测任何东西；`--compare` 不返回失败 | 见分区报告 |
| 80 | minor | Benchmarks | 关键阈值来自"观测到的散布"（事后调参），且没有预注册文件 | 见分区报告 |
| 81 | minor | Benchmarks | 三份 md 的"代码事实"已被本 fork 自己的修改推翻，但仍以现状语气陈述 | 见分区报告 |
| 82 | minor | Benchmarks | 目录卫生：`__pycache__/*.pyc` 入库 | `benchmarks/cases/c02_0_pec/__pycache__/p1_tensor_reference.cpython-314.py` |
| 83 | minor | Benchmarks | conductor_slab` 的"纯前向波"说法与自身另一脚本矛盾；提交的探针恰在被排除区 | 见分区报告 |
| 84 | minor | Benchmarks | 多处 `print` 无门禁 / 无 assert（"verify"脚本不 verify） | 见分区报告 |
| 85 | minor | Benchmarks | verify_fresnel_rt.py` 的 q1/q2 注释与代码相反（代码是对的，注释会误导） | 见分区报告 |
| 86 | minor | Benchmarks | make_mie_cases.py` 的注释/文档与实际网格不符；单网格是为了绕开未定性不稳定 | 见分区报告 |
| 87 | minor | Python | 新增碰撞分析断言有复制粘贴错误 + 精确浮点相等 | 见分区报告 |
| 88 | minor | Python | read_dims_from_file` 的 `FILE` 正则仍会把 `#` 吞进路径 | 见分区报告 |
| 89 | minor | Python | Simulation` 的若干公开属性/扩展点被改为私有（无 property 兼容） | 见分区报告 |
| 90 | minor | Python | 所有 PICMI 对象改为 pydantic 严格校验，未使用的多余关键字从"忽略"变"报错" | 见分区报告 |
| 91 | minor | Python | _amrex_argv`：脚本自身的 `--flag` 若出现在第一个覆盖参数之后会破坏 ParmParse | 见分区报告 |
| 92 | minor | Python | 打包/卫生问题 | 见分区报告 |
| 93 | minor | Source_Core_Misc | ImplicitOptions::particle_shape` 未初始化，且新增的 `ncomp_*` 块是死状态 | 见分区报告 |
| 94 | minor | Source_Core_Misc | FilterFunctors.H` 存在两份同 include-guard 的副本 | 见分区报告 |
| 95 | minor | Source_Core_Misc | Fields.H`：FieldType 枚举中段插入新值（数值全平移）——已核查无消费者，仅记录风险 | 见分区报告 |
| 96 | minor | Source_Core_Misc | WarpXSolver{DOF,Vec}` 把 `"vector_potential_fp_nodal"` 映射到 `FieldType::vector_potential_fp_nodal` | 见分区报告 |
| 97 | minor | Source_Core_Misc | main.cpp：`Runtime` 构造期抛异常时 AMReX/MPI/PETSc 永不收尾（无 try/catch） | 见分区报告 |
| 98 | minor | Source_Core_Runtime_A | StageContext::config` 只给 `RuntimeConfigSnapshot`，且该 snapshot 在一次 run 里不会刷新 | 见分区报告 |
| 99 | minor | Source_Core_Runtime_A | CollisionHandlerAdapter::apply` 的 null 检查是不可达的死代码，异常分支永远进不去 | 见分区报告 |
| 100 | minor | Source_Core_Runtime_A | 命名回调总线：重名注册静默覆盖、缺失名字静默 no-op | 见分区报告 |
| 101 | minor | Source_Core_Runtime_A | IGeometryProvider.H` 的 include guard 是从 `ISimulationKernel` 抄来的 | 见分区报告 |
| 102 | minor | Source_Core_Runtime_A | IFiniteDifferenceSolver.H` 的工厂块写在 `#endif` 之后（不在 include guard 保护内） | 见分区报告 |
| 103 | minor | Source_Core_Runtime_A | IFieldSolver.H` / `IFieldSolverEngine.H` 是完全没有实现、没有使用者的死接口 | 见分区报告 |
| 104 | minor | Source_Core_Runtime_A | ordered_specs()` 在"只有声明没有 handler"时退化为纯 priority 排序（跳过依赖拓扑排序与环检测） | 见分区报告 |
| 105 | minor | Source_Core_Runtime_A | DataBus::alias` 对"已存在的目标名"不做任何声明校验 | 见分区报告 |
| 106 | minor | Source_Core_Runtime_A | FieldGatherOrders` 的成员没有默认初始化器 | 见分区报告 |
| 107 | minor | Source_Core_Runtime_A | 碰撞适配器把物种名硬编码成 `electrons/ions/gas/photons` | 见分区报告 |
| 108 | minor | Source_Core_Runtime_B | ScalarRegistry::declare()` 不比较初值：同名 scalar 冲突被静默吞掉 | 见分区报告 |
| 109 | minor | Source_Core_Runtime_B | ScalarSpec::checkpoint` 被接受但从未实现（声明成功 = 静默失效） | 见分区报告 |
| 110 | minor | Source_Core_Runtime_B | 服务注册表的"冲突"是**全局单串**：任一服务种类冲突会让所有无关服务创建失败 | 见分区报告 |
| 111 | minor | Source_Core_Runtime_B | legacy_runtime_data()` / `legacy_host_services()` 的函数级静态持有者不随 `clear_service_factories()` 重置 | 见分区报告 |
| 112 | minor | Source_Core_Runtime_B | C ABI 把 handler 注册失败全部折叠成返回码 4，loader 又完全忽略它 | 见分区报告 |
| 113 | minor | Source_Core_Runtime_B | register_stage()` 可以注册执行引擎永远到不了的 ordinal，且没有任何提示 | 见分区报告 |
| 114 | minor | Source_Core_Runtime_B | 三处工厂仍是"最后一个赢"且缺省静默返回 nullptr（与 ServiceFactory 的 F-09 修复不一致） | 见分区报告 |
| 115 | minor | Source_Core_Runtime_B | legacy_costs()`：注册了 provider，却没有任何调用者（死契约） | 见分区报告 |
| 116 | minor | Source_Core_Runtime_B | Runtime` 构造函数抛出时 AMReX 不会被 finalize（资源泄漏 + AMReX 退出断言） | 见分区报告 |
| 117 | minor | Source_Plugins_Collision_A | 三个“接受 `MultiParticleContainer const*`”的构造函数只声明、无定义（悬空声明） | 见分区报告 |
| 118 | minor | Source_Plugins_Collision_A | #include` 被放在文件第 1 行、版权头与 include guard 之前 | 见分区报告 |
| 119 | minor | Source_Plugins_Collision_A | BinaryCollisionUtils` 新增的两个转发重载在树内无调用者（死代码 + 多余快照） | 见分区报告 |
| 120 | minor | Source_Plugins_Collision_B | pulsed_decay` 新采样器：整片母粒子被销毁 + 产物权重按 `weight_ratio` 逐代缩小；统计方差与文档表述都变了 | 见分区报告 |
| 121 | minor | Source_Plugins_Collision_B | CollisionSpeciesTable` 的 `SpeciesHandle` 全套机制是死代码，且构造时付一次"全粒子数全局归约" | 见分区报告 |
| 122 | minor | Source_Plugins_Collision_B | InverseBremsstrahlung`：`calculateNuei` 在缺少回调时**静默空转**，随后无检查地解引用 `nuei_*` / `N_*` 场 | 见分区报告 |
| 123 | minor | Source_Plugins_Collision_B | NuclearFusionFunc::executor()` 无检查解引用只在 `AllocData` 里赋值的 `m_data`（以及 `fields().get()` 的返回值） | 见分区报告 |
| 124 | minor | Source_Plugins_Collision_B | declare_state` 与 `NuclearFusionFunc` 用**不同机制**解析同一个 `save_particle_production`，schema 路径可能被静默跳过 | 见分区报告 |
| 125 | minor | Source_Plugins_Collision_B | declare_state` 只在 kernel 路径生效；legacy/Python 路径的 handler 是"首步懒创建"，永不走 schema | 见分区报告 |
| 126 | minor | Source_Plugins_Collision_B | 头文件/包含卫生类残留（guard 外的 include、重复 include、空 `#ifdef`、被遗留的重载） | 见分区报告 |
| 127 | minor | Source_Plugins_Diagnostics_A | checkpoint 新增的 `field_schema.txt` 由所有 MPI rank 同时写，无 IOProcessor 保护 | 见分区报告 |
| 128 | minor | Source_Plugins_Diagnostics_A | BTD 的场 functor 从未拿到 RuntimeContext，`config_snapshot()` 静默返回全默认值 | 见分区报告 |
| 129 | minor | Source_Plugins_Diagnostics_A | LorentzTransformParticles` 绕过传入的 context，直接用 `FrameState::FromLegacy()` | 见分区报告 |
| 130 | minor | Source_Plugins_Diagnostics_A | 诊断 J 的按需沉积新增 `ApplyJfieldBoundary`，边界处与上游不同 | 见分区报告 |
| 131 | minor | Source_Plugins_Diagnostics_A | 用户可见报错消息被替换成 C++ 语法；注释与代码不一致 | 见分区报告 |
| 132 | minor | Source_Plugins_Diagnostics_B | GetExternalEBField(pti, offset, myspc.runtime_context())` 的 parser 分支不判空 | 见分区报告 |
| 133 | minor | Source_Plugins_Diagnostics_B | 头文件里把 `#include` 写进了版权注释 | 见分区报告 |
| 134 | minor | Source_Plugins_Diagnostics_B | WarpXOpenPMDPlot` 对 `m_runtime_context` / `config_snapshot` 直接解引用 | 见分区报告 |
| 135 | minor | Source_Plugins_Diagnostics_B | 非 EB 构建下 ChargeOnEB 从"运行时明确报错"变成"未注册类型" | 见分区报告 |
| 136 | minor | Source_Plugins_Diagnostics_B | 基类新增 `unique_ptr` 成员使"默认拷贝"静默失效 | 见分区报告 |
| 137 | minor | Source_Plugins_FieldSolver_A | 有效势 ES：rho 的边界折叠/RZ 体权标被挪到 sum 之后 | 见分区报告 |
| 138 | minor | Source_Plugins_FieldSolver_A | 隐式求解器：`mass_matrices_pc_width` 默认 0→1，叠加新 abort，3D Villasenor 旧算例会直接中止 | 见分区报告 |
| 139 | minor | Source_Plugins_FieldSolver_A | 新张量材料填充只按二维取样（3D 下 y 方向冻结） | 见分区报告 |
| 140 | minor | Source_Plugins_FieldSolver_A | 亚像素张量：正定性/退化检查用 `AMREX_ASSERT`（release 构建被编译掉） | 见分区报告 |
| 141 | minor | Source_Plugins_FieldSolver_B | WeightedJacobiImpl::Update()` 每次 PC 更新都用 Gershgorin 界覆盖已自适应的 omega | 见分区报告 |
| 142 | minor | Source_Plugins_FieldSolver_B | Newton 发散判据从"直接 abort"改为"警告 + exit_status=-9" | 见分区报告 |
| 143 | minor | Source_Plugins_FieldSolver_B | SpectralFieldData.H` 重复的前置声明 | 见分区报告 |
| 144 | minor | Source_Plugins_FieldSolver_B | SemiImplicitDarwin::m_magnetostatic_particle_shape` 未初始化 | 见分区报告 |
| 145 | minor | Source_Plugins_FieldSolver_B | 类型/契约不一致：`SpectralFieldData` 持 `RuntimeContext*`，`SpectralFieldDataRZ` 持 `RuntimeData*` | 见分区报告 |
| 146 | minor | Source_Plugins_FieldSolver_B | precon_mat.verbose` 改名：旧输入 `pc_petsc.verbose` 直接 abort | 见分区报告 |
| 147 | minor | Source_Plugins_FieldSolver_B | MagnetostaticSolverImpl::defineVectorPotentialBCs(geom)` 公开接口会用到默认初始化的边界类型 | 见分区报告 |
| 148 | minor | Source_Plugins_ParticleCore_A | Filter::ApplyStencil` 把 load-balance cost 恒置为 `nullptr`，负载均衡计时统计被静默删除 | 见分区报告 |
| 149 | minor | Source_Plugins_ParticleCore_A | warpx.read_fields_from_path` 由可选变成必需（`query` → `get`） | 见分区报告 |
| 150 | minor | Source_Plugins_ParticleCore_A | eV 温度的 `mass`/`species_type` 解析绕开了 `source_name`，且与 `injection_file` 提供的质量不兼容 | 见分区报告 |
| 151 | minor | Source_Plugins_ParticleCore_A | maxwell_juttner` 的 `theta` 输入被删除，没有 BackwardCompatibility guard | 见分区报告 |
| 152 | minor | Source_Plugins_ParticleCore_B | DepositCurrentOnLevel` 的文档仍声称"apply boundary conditions"，实现只有 guard-cell 求和 | 见分区报告 |
| 153 | minor | Source_Plugins_ParticleCore_B | 新增的三个隐式辅助函数把层号 hard-code 为 `lev = 0`（多层的循环被注释掉） | 见分区报告 |
| 154 | minor | Source_Plugins_ParticleCore_B | 新增契约实现里缺属性保护与"接口注释重复"等小问题 | 见分区报告 |
| 155 | minor | Source_Plugins_ParticleCore_B | RSPHERE 的隐式质量矩阵角度分支是本 fork 新增（上游 RZ/RCYLINDER only），球坐标角量耦合到"平均半径/瞬时半径"混用 | 见分区报告 |
| 156 | minor | Source_Plugins_ParticleCore_B | villasenor_mass_matrices_max_grid_crossings` 的单一真源被复制成魔法数 `3` | 见分区报告 |
| 157 | minor | Source_Plugins_ParticleCore_B | GetChargeDensityOnLevel(local=true, finalize=true)` 现在直接 assert；Python 绑定的 `finalize` 默认 `true` | 见分区报告 |
| 158 | minor | Source_Plugins_PicEm_A | ComputeDualGeometry` 留下无条件调试打印与自相矛盾的断言文案 | 见分区报告 |
| 159 | minor | Source_Plugins_PicEm_A | 3D 张量介质中 `rho_medium` 种子的 Dy 用了错的 eps 分量 | 见分区报告 |
| 160 | minor | Source_Plugins_PicEm_A | 3D 各向异性下另一处同类风险（`eps2` 变量未使用，`Ex2/Ez2` 的 FillBoundary 缺 `Ey2`） | 见分区报告 |
| 161 | minor | Source_Plugins_PicEm_A | bnd_shrink` 用域周期判断却作用于每块 tilebox ⇒ CPU 分块时 rho_medium 少更新一层 | 见分区报告 |
| 162 | minor | Source_Plugins_PicEm_A | fluids.moment` 原型只断言 `AMREX_SPACEDIM==2`，RZ 构建同样满足 ⇒ 静默按直角算 | 见分区报告 |
| 163 | minor | Source_Plugins_PicEm_A | 电荷守恒残差返回值被丢弃 | 见分区报告 |
| 164 | minor | Source_Plugins_PicEm_A | AddExternalFields` 的层循环由 `max_level` 改为 `finestLevel()` | 见分区报告 |
| 165 | minor | Source_Plugins_PicEm_A | WARPX_DISABLE_DIAGNOSTICS` 环境变量会静默替换已构造的诊断服务 | 见分区报告 |
| 166 | minor | Source_Plugins_PicEm_A | Maxwell6` 数值通量注释声称精确 Roe/upwind，实现是分量式 Rusanov | 见分区报告 |
| 167 | minor | Source_Plugins_PicEm_B | 三个 MomentsSolver 测试二进制永远返回 0，FAIL 只打印不生效 | 见分区报告 |
| 168 | minor | Source_Plugins_PicEm_B | run_tests.sh` 的路径在插件化移动后失效 | 见分区报告 |
| 169 | minor | Source_Plugins_PicEm_B | GetPartContainer()` 无空指针保护，与 `particleSystem()` 的设计不一致 | 见分区报告 |
| 170 | minor | Source_Plugins_PicEm_B | 诊断 rho 现在也走 `FinalizeRhoOnLevel`（含 PEC/反射边界清零），与上游诊断语义不同 | 见分区报告 |
| 171 | minor | Source_Plugins_PicEm_B | ParticlePushHandler` 的 HandlerSpec 声称不读任何场 | 见分区报告 |
| 172 | minor | Source_Plugins_PicEm_B | WarpXRuntimeData.H` 的 include 落在版权头与 include guard 之前；`WarpXHostServices.cpp` 有两个 `namespace` 块 | 见分区报告 |
| 173 | minor | Tools_Tests | 5 个 runtime "contract test" 实际是 `int main(){return 0;}` 且没有注册进 CTest | 见分区报告 |
| 174 | minor | Tools_Tests | test_nonpic_kernel.cpp` 打印的断言数写死为 8（实际 7） | 见分区报告 |
| 175 | minor | Tools_Tests | sod_reference.py` 稀疏波分支的解析导数写错（靠括号保护的 Newton 兜住） | 见分区报告 |
| 176 | minor | Tools_Tests | ssm_compare.py` 自检里有一条恒真断言，README 的"5%"与断言的 15% 不符，打印的 eps/kappa 比值在 SSM 为 0 处失真 | 见分区报告 |
| 177 | minor | Tools_Tests | solar_compare_plot.py --check` 向源码树写 PNG，且只钉住 L、R 两个数 | 见分区报告 |
| 178 | minor | Tools_Tests | 插件测试的构建假定 ELF/macOS 以外的编译器与 `.so` 后缀 | 见分区报告 |
| 179 | minor | Tools_Tests | 若干 shell 核验脚本的硬编码与静默 no-op | 见分区报告 |
| 180 | minor | Tools_Tests | A0 两段流水线的输入路径对不上，文档化的"重新生成"无法执行 | 见分区报告 |
| 181 | minor | Tools_Tests | 两个"门"的退出码恒为 0 | 见分区报告 |
| 182 | minor | Tools_Tests | check_field_solver_extraction.py` 永远 exit 0，且统计的是已经搬空的旧树 | 见分区报告 |
| 183 | minor | Tools_Tests | route_tasks.py` 的"JSON 权威"是死代码 | 见分区报告 |
| 184 | minor | Tools_Tests | a5_ab.py` 未接入任何门，且硬编码二进制与构建目录 | 见分区报告 |
| 185 | minor | Tools_Tests | build_and_run_4d.sh` 的 conda 前缀变量名疑似笔误 | 见分区报告 |
| 186 | minor | Tools_Tests | inventory_field_solver.py` 写入源码树文档，且 `classify()` 有多余/不可达分支 | 见分区报告 |
| 187 | minor | Tools_Tests | 其它可靠性/自证性较弱的点（合并记录，共 9 条） | 见分区报告 |
| 188 | unrated | Benchmarks | c03` 若干小问题（合并） | 见分区报告 |
| 189 | unrated | Source_Plugins_FieldSolver_B | 备注（已核对为**良性**，但看起来像缺陷）：`ThetaImplicitEM::ResetStep()` 不恢复 `Efield_fp`/`B_old` | 见分区报告 |
