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
