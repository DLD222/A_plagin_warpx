# 已修复缺陷清单（含依据与验证）

原则：只改**确认**的缺陷；每条都给出官方源码/语言规则依据、最小修法与验证证据；
不确定或有设计取舍的条目一律不在这里（见 `REPORT.md` §8 与 `reviews/`）。

| # | 缺陷 | 位置 | 依据 | 验证 | commit |
|---|---|---|---|---|---|
| 1 | 1D/2D/RZ 维度编译中断 | `Source/Plugins/NewtonGravity/StarGravityPlugin.cpp`、`Source/Plugins/FluidMoments/StarFluidPlugin.cpp` | AMReX `IntVectND<dim>` 变参构造要求参数个数==dim（`AMReX_IntVect.H:201-203`）；仓库惯用 `AMREX_D_DECL` | 修复后 fork 在 `1;2;3;RZ` 四维下编译通过（`audit_build/fork`） | `2829581` |
| 2 | 插件 MODULE 链接失败（relocation R_X86_64_PC32） | `Source/Plugins/{NewtonGravity,FluidMoments}/CMakeLists.txt` | 进入共享库的目标文件必须 `-fPIC`；同仓库 `ablastr_${SD}` 等均已显式开 PIC | `lib/star_{gravity,fluid}_plugin.so` 生成 | `2829581` |
| 3 | `-DWarpX_amrex_src` 下找不到 `AMReX_buildInfo.H` | `CMakeLists.txt:533`、`Source/Plugins/Diagnostics/CMakeLists.txt:30` | 硬编码 `_deps/fetchedamrex-build`；AMReX 已导出 `AMREX_C_SCRIPTS_DIR`（本构建缓存中指向 `amrex-src/Tools/C_scripts`，含该头） | 三套构建均在 `-DWarpX_amrex_src` 下编译通过 | `2829581` |
| 4 | pywarpx 公共 API `WarpXParIter` 断链 | `Source/Plugins/PicEm/Modules/Python/Particles/WarpXParticleContainer.cpp` | 类改名后只注册 `ParticleTileIter`，而 `Python/pywarpx/particle_containers.py:166/211/250` 仍用旧名（2021 年起的公开名） | 重建后 `hasattr(m,"WarpXParIter")==True`，两个名字并存 | `2829581` |
| 5 | 诊断 J 的沉积路径与上游不一致（多 `SumGuardCells`+`FinalizeJ`，并改写 live 电流） | `Source/Plugins/PicEm/RuntimeBridge/Runtime/WarpXHostServices.cpp` | 上游 `_upstream/.../ComputeDiagFunctors/JFunctor.cpp:46-55` 只做 `DepositCurrent`+`FillBoundary` | 1D energy-conserving 与 3 个 collision 用例由 1e7–1e17 降为**逐位一致** | `edd72c4` |
| 6 | `divE/divB` 诊断用初始化前快照的 `ncomps`（RZ 多模越界写） | `Source/Plugins/Diagnostics/ComputeDiagFunctors/DivEFunctor.cpp:56`、`DivBFunctor.cpp:37` | `RuntimeConfig::ncomps()` 是活配置；`RuntimeConfigSnapshot::ncomps{1}` 是默认值；`InitData` 前读快照会拿到 1 | 相关 theta/PSATD/divb 用例回归通过 | `edd72c4` |
| 7 | `check_plugin_boundaries.py` 结尾覆盖 `errors`，“重复头文件/依赖环”永不失败 | `Tools/Modularization/check_plugin_boundaries.py` | 代码路径：先 `errors.append(...)`，后被 `errors = sorted(...)` 覆盖 | 修复后仓仍 `OK (0 known)`；反例路径现在会 exit 1 | `89d248c` |
| 8 | `parse_tests.py` 把 `_upstream/` 当本地树，上游一致性门恒过 | `Tools/verify/parse_tests.py` | `SKIP_DIRS` 缺 `_upstream/audit_*/warpx-data/...` | 现在 `local 424` vs `reference 408`（此前 832，翻倍） | `89d248c` |

| 9 | 诊断 rho 的“边界+体积标定”与 guard-cell 求和的次序与上游相反 | `Source/Plugins/PicEm/RuntimeBridge/Runtime/WarpXHostServices.cpp::computeRho` | 上游 `DepositCharge(..., apply_boundary_and_scale_volume=true)` 先做边界条件与径向体积标定，再做 `ApplyFilterandSumBoundaryRho` | 同二进制 A/B：`test_rz_laser_acceleration` 的 rho 最大差 **1.36e4 → 1.09e4（-19.5%）**；`test_rz_ohm_solver_*` 与 `test_rz_magnetostatic_eb_picmi` **不变**；`test_2d_langmuir_multi` 仍通过 | 见下一行 commit |

## 回撤：修复 #9 是错的方向（2026-11-15 更正）

修复 #9（提交 `ca4a7cb`）把 `computeRho` 改成"先 `FinalizeRhoOnLevel`、后
`ApplyFilterandSumBoundaryRho`"，依据是"对齐上游 `DepositCharge(...,
apply_boundary_and_scale_volume=true)` 的次序"。这个依据**看错了参照版本**：

- 该次序是 **26.10** 的诊断路径；
- fork 的上游基线是 26.10 之后约三周的 `development`（≈2026-10-30，见
  `UPSTREAM_BASE.md`）；
- 26.11 的 `RhoFunctor` 是"先 `ApplyFilterandSumBoundaryRho`、后 `FinalizeRhoOnLevel`"
  （上游 `ec99a479a` #7341 引入），fork 原本就是对的。

因此该"修复"把正确的实现改成了旧实现。本轮已撤销，恢复为：

```cpp
m_warpx.ApplyFilterandSumBoundaryRho(lev, lev, *rho, 0, rho->nComp());
m_warpx.FinalizeRhoOnLevel(lev, rho.get(), PatchType::fine);
```

撤销后的实测：`test_rz_laser_acceleration` t=0 的 `rho` 三个分量与**官方 26.11 逐字节相同**
（最大差 0.0），与官方 26.10 的差异回到 1.3578e4 —— 这 1.3578e4 已由上游
`ec99a479a` + `a180bc361` 完全解释（`RZ_RHO_ROOT_CAUSE.md`）。

| # | 更正内容 | 位置 | 依据 | 验证 |
|---|---|---|---|---|
| 9′ | 撤销 #9，恢复 26.11 的 finalization/过滤次序 | `Source/Plugins/PicEm/RuntimeBridge/Runtime/WarpXHostServices.cpp::computeRho` | 26.11 `Diagnostics/ComputeDiagFunctors/RhoFunctor.cpp` 的调用次序（`ec99a479a`） | fork 与官方 26.11 的 t=0 `rho` 逐字节一致 |
| 10 | 移除 fork 独有的 `ApplyVolumeWeightedFilter(..., is_volume_scaled)` 扩展 | `Source/Plugins/PicEm/Modules/Parallelization/WarpXComm.cpp`、`Source/Plugins/PicEm/WarpX.H` | 26.10 / 26.11 / development 均无此参数，`rho` 调用点传 `false` 使 fork 偏离上游 | 与上游函数体逐行一致 |
| 11 | `GetExternalEBField` 在电离/QED 路径上拿不到 `RuntimeContext` | `Source/Plugins/ParticleCore/Particles/ElementaryProcess/{Ionization,QEDPairGeneration,QEDPhotonEmission}.{H,cpp}`、`PhysicalParticleContainer.cpp::getIonizationFunc`、`MultiParticleContainer.cpp`（QED 循环） | 无上下文时 `ps == nullptr`，`E_ext_particle_function`/`repeated_plasma_lens` 段会解引用空指针（或静默丢外场）；其余 5 处调用点都传了 `runtime_context()` | 编译通过；QED/电离用例回归通过 |

| 12 | star/fluid 插件 1D/2D 下 `bx.smallEnd(1)/(2)`、`bx.bigEnd(1)/(2)` 越界（196 处）：循环上界读到 IntVect 数组外，实际写穿 FAB | `Source/Plugins/NewtonGravity/*`、`Source/Plugins/FluidMoments/*` | AMReX `Box` 只有 `AMREX_SPACEDIM` 个方向；两个插件在 `1;2;3;RZ` 四维都参与构建。新增插件内 `star_box_lo/hi`（`d < AMREX_SPACEDIM ? b.smallEnd(d) : 0`），低维的缺失方向取 0，与 3D 形状核函数在低维退化成单层的语义一致 | 四维编译通过；`ctest -R '^(gravity_|fluid_)'` **33/33 通过** | `6312f53` |
| 13 | 自引力解出来却从不使用：`gravity_step` 把 `star_gx/gy/gz` 写成 0 | `Source/Plugins/NewtonGravity/StarGravityPlugin.cpp` | 物理规范 §1.2 要求 g = −∇φ 并交给流体；`NewtonGravitySolver::computeAcceleration` 已实现该步，但插件从未调用 | 现按层调用 `computeAcceleration` 发布 g（`gravity_*` 6/6 通过） | 本轮 |
| 14 | 引力常数单位混用：star 状态是 cgs（g/cm³、cm），求解器用 SI `G = 6.6743e-11` | `Source/Plugins/NewtonGravity/NewtonGravity.{H,cpp}` | 直接乘错 `G` 使 φ 与 g 相差 1e7；规范 §0 常数表给 SI 值，但调用方的状态由 profile 决定为 cgs | `solvePotential` 的 `G` 变成参数（默认 SI，保持 `gravity_*` 单测不变），star 插件显式传 `gravitational_constant_cgs = 6.67430e-8` | 本轮 |

| 15 | 高斯 cgs 下洛伦兹力缺 1/c（力大 3e10 倍） | `Source/Plugins/FluidMoments/Electromagnetic.H` | cgs 里 `J=σE'` 的单位是 statA/cm²，力密度是 `J×B/c`；焦耳热 σ|E'|² 两种约定相同，所以单测（只查 f·B=0、Q≥0）抓不到 | 补 `c_light_cgs`；`fluid_*` 33/33 通过 | `75c5466` |
| 16 | 显式扩散子步超 3D von Neumann 限（最短模增长） | `Radiation.H`、`Neutrino.H`、`Composition.H` | 放大因子 `1-4dhα/dx²`，3D 要求 `h≤dx²/(2·3·α)`，原值 0.2 超限；单测初值只沿 x 变化故未暴露 | 改为 `0.4dx²/(2·AMREX_SPACEDIM·D)` | `75c5466` |
| 17 | 辐射扩散把未初始化内存写回辐射场 | `Radiation.H` | `er_new` 未清零 + 对 `rho<=0` 的格子 `continue` + `ParallelCopy` 整块写回 | `er_new.setVal(0)`，跳过格显式保留原值 | `75c5466` |
| 18 | 显式算子 `max_sub` 到顶静默少走时间（minor m12） | 四个流体算子 | `break` 无提示，`remaining` 仍 >0 | 上限提到 100000，到顶打印含剩余时间的警告 | `75c5466` |
| 19 | 引力开放边界（Dirichlet）ghost 从未填充 → 势解依赖未初始化内存 | `NewtonGravity.cpp`、`StarGravityPlugin.cpp` | `setLevelBC` 对 Dirichlet 从 `phi` 的 ghost 取值；`rhs` 也只 `Copy(...,0)` | `rhs.setVal(0)`；插件用解析 `-GM/r` 填 `phi` 的 ghost（质量取自同一剖面采样）；`fluid_*`/`gravity_*` 33/33 通过 | 本轮 |

## 未修复但已定位（星体/流体插件，需要与通量形式重写一起做）

- **Conduction 的 3D 稳定子步**：改成 `dx²/(2·d·α)` 后 `fluid_conduction_3` 的能量守恒断言
  从 <1e-12 升到 1.24e-12（=1.2e-15/子步，已是舍入前沿）。该算子目前只守恒到舍入，
  因此这一步要与"通量形式（严格守恒）重写"一起做，代码里留了 open 注记。
- **F6 正定性保护**（HLLC/MUSCL 强膨胀产生 NaN）、**F10 多层势**（逐层独立求解、每层重盖
  同一剖面、static 状态不随 restart 复位）、**F2 的 restart 语义**：都属于插件内部结构性改造。

## 回归证据摘要（修复 #5）

| 用例 | 修复前 max abs (vs 官方 26.10) | 修复后 |
|---|---|---|
| `test_1d_energy_conserving_thermal_plasma` | 1.75e17 | 0（逐位一致） |
| `test_2d_energy_conserving_thermal_plasma` | 1.99e17 | 3.6e5（rel 3.5e-9） |
| `test_2d_collision_xz` | 1.15e8 | 0 |
| `test_2d_collision_xz_global_debye` | 8.64e7 | 0 |
| `test_3d_collision_xyz` | 8.88e7 | 0 |
| `test_1d_resample_velocity_coincidence_thinning(_cartesian)` | 5.3e306 / 7.7e282 | 0 / 0 |

35 个带 j 诊断的用例在修复后全部通过 ctest；9 个差异显著缩小或归零。

## 未修复但已确认（需要决策，不是“改不动”）

见 `REPORT.md` §8：`GetExternalEBField` 缺 `RuntimeContext`（QED/电离 + 外场时崩溃或零场）、
RZ 逐物种诊断量的边界处理（已逐格定位到 z 上边界）、star/fluid 插件（自引力未接线/1D-2D
越界/cgs-SI 混用）、6 处被放宽的测试容差、Bremsstrahlung 与官方相反但物理更自洽的顺序。

## 本轮新增（round 12 后半）

| # | 缺陷 | 位置 | 依据 | 验证 |
|---|---|---|---|---|
| 20 | `plugin_c_abi_header_*` 测试硬编码 ABI v6，而头文件已是 v7（`a158cd1` 引入 level geometry 访问器） | `tests/plugins/test_c_abi_header.c` | 头文件 `WARPX_PLUGIN_ABI_VERSION 7u`；测试里 `!= 6u` 直接失败 | 改为 7 并在注释里写明"改 ABI 时两处一起改"；测试输出 `plugin ABI v7 C header OK` |
| 21 | 内核 `finalize()` 从不被调用 / `StateRegistry::clear()` 不清 scalar / `warpx_stage_geometry()` 对不存在的层不返回 0 | `Source/Core/Runtime/{Runtime.cpp,StateRegistry.cpp,PluginCAdapter.cpp}` | review `Source_Core_Runtime_B` 的 F3/F5/F4（契约与实现不符） | `ctest -R 'plugin_|runtime_'` 除下列 1 个**既有**失败外全通过 |

## 仍然失败、但**不是本轮引入**的单测（已用 stash 重建核对）

- `runtime_snapshot_particles_2`：`tests/runtime/inputs_2d_particles` 里**没有**
  `warpx.reduced_diags_names`，而测试要读 `diags/reducedfiles/{step_info,lb_eff,part_num,
  part_energy,part_mom,rho_max,extrema}.txt` 共 15 条断言。属于测试夹具缺失
  （照 `inputs_2d` 的写法补上对应的 reduced-diag 声明即可），不是运行时缺陷。
- 该用例的期望值（256 个宏粒子、权重 4e18、`rho_max = -q_e*1e18`、extrema ±0.9375）
  已经写在测试里，补夹具时按它对齐即可。

## round 13

| # | 缺陷 | 位置 | 依据 | 验证 |
|---|---|---|---|---|
| 22 | `test_runtime_snapshot_particles_2` 的夹具退化：`inputs_2d_particles` 被改成 32x32/1e-4/1e20 且**没有** `reduced_diags_names`，而测试要读 7 个 reduced diag、期望值是按 16x16/±1/1e18/外场 E=(1,0,0) 算的 | `tests/runtime/inputs_2d_particles` | 15 条断言全失败；期望值（256 粒子、权重 1.5625e16、rho=-0.16、格心 ±0.9375）与 deck 参数一一对应 | deck 改回与 `inputs_2d` 一致的几何/网格/外场 + 粒子 + 7 个 reduced diag；`plugin_|runtime_|particle_free|stage_dispatch|snapshot` **23/23** |
| 23 | `test_{yee_deposition,coupling_nd,fv_nd}` 的 13 条检查只打印 FAIL、始终 `return 0`（几何/守恒门禁永不生效） | `Source/Plugins/PicEm/Modules/MomentsSolver/Tests/` | `run_tests.sh` 用 `set -e` 判退出码，对比同目录 10 个测试都 `return ok ? 0 : 1` | 统一 `report(cond,msg)` 累积失败，末尾返回失败数；13/13 编译运行、23 PASS 0 FAIL |
| 24 | `run_tests.sh` 路径在插件化重构后失效（`cd ../../..` 落到 `Source/Plugins`、`Source/MomentsSolver` 已不存在） | 同上 | `ls Source/MomentsSolver` 不存在 ⇒ `set -e` 下第一条 g++ 就失败 | 从脚本自身位置推导仓库根，`AMREX_INC` 可指定（否则自动找 deps），失败时 `exit 1` |
| 25 | `ComputeDualGeometry` 的 `[DIAG]` 打印无条件执行（每次调用都灌 stdout），断言文案与实现不符 | `EmbeddedBoundaryInit.cpp:892,896,1018` | 同文件其它诊断都按 verbose 门控；条件接受 1/2/4/6/7/8/9 而文案只说 1/2/4 | 改 `WarpX::GetInstance().Verbose() > 2` 门控，文案对齐；5 个 EB 用例通过 |
| 26 | 3D 张量介质 `rho_medium` 种子的 `Dy` 用了 eps 的 xz 分量（应为 yy） | `Source/Plugins/PicEm/Modules/FieldSolver/WarpXPushFieldsEM.cpp:1315` | eps 分量顺序 `(xx,yy,zz,xy,xz,yz)`（`MacroscopicProperties.cpp:684`），`Dy=xy*Ex+yy*Ey+yz*Ez` | 改 `en1(...,1)`；12 个 macroscopic 用例通过（其中 `test_3d_embedded_boundary_cube_macroscopic` 的 benchmark 因这次修正重设，变化只在 1e-18 的 Bx 与 Ey 的第 7 位有效数字） |
| 27 | HLLC/MUSCL 全链路无正定性下限（rho=0 除零、负压开平方得 NaN、全局最大波速被 NaN 污染） | `FluidMoments/FluidMoments.H` | review F6；构造反例：密度比 1e4 的 Sod 型初值 | `primitive()`/`soundSpeed()` 对 rho/e_int/p 夹 `real_floor`；`fluid_*` 33/33 |
| 28 | `test_stellar_model` 的 `w[4]` 被按 9 个物种索引（栈越界，垃圾密度喂给 CF88） | 同上 Tests | review F13 | 改 `w[n_species]`；该用例 211 s 通过 |

### round 14

| # | 缺陷 | 位置 | 依据 | 验证 |
|---|---|---|---|---|
| 29 | `MomentCoupling` 焦耳加热多乘一个 rho（动量密度再乘密度） | `Modules/MomentsSolver/Coupling/MomentCoupling.H:88` | `u[1..3]` 已是动量密度 ⇒ `u·E = J·E/qm`；再乘 rho 使 rho≪1 时不加热、rho>1 时过加热，总能量不守恒 | 去掉多余 rho；30 个 moments/ten_moment/fluid 用例通过 |
| 30 | `SemiImplicitDarwin::m_magnetostatic_particle_shape` 未初始化 | `FieldSolver/Implicit/SemiImplicitDarwin.H:159` | 唯一赋值在 `Define()`，`PrintParameters()/OneStep` 可能先读 | 默认 1；12 个 darwin/semi_implicit 用例通过 |
| 31 | pywarpx `FILE = inc/base.in#comment` 把 `#comment` 吞进路径 | `Python/pywarpx/WarpX.py:114` | 非引号分支 `([^\s"#]\S*)` 的 `\S*` 又把 `#` 收回来 | 改 `([^\s"#]+)`；四种写法都解析成 `inc/base.in`；单测加 `inputs_base#attached` 参数；`ctest -R '^pytest.WarpX'` 4/4 |

### 两个"实质差异"用例的自证（详见 `ATTRIBUTION.md`）

| 用例 | 官方 26.11 np=1 vs np=2 | fork vs 官方（同分解） |
|---|---|---|
| `test_2d_laser_ion_acc` | 电子数差 79、E/x 相对 1.5e-1、rho 1.1e0 | 电子数差 13、E/x 1.2e-4 |
| `test_2d_semi_implicit_pc_jacobi` | 相对 1.3–2.7 | 1.7e-10 |
