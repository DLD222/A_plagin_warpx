# Numeric-difference analysis 

- cases with numeric differences: 78
- cases with only log/text differences: 0
- cases with only metadata/used-inputs differences: 1

## Cases with numeric differences (worst field per case)

| case | worst field | n differ | max abs | max rel | shape-mismatch |
|---|---|---|---|---|---|
| test_2d_collisions_split_momentum_push_electrostatic | /data/2000/fields/j/z | 256 | 1.744e+17 | 2.098e+03 | 0 |
| test_2d_laser_ion_acc | /data/100/meshes/data | 544 | 2.990e+14 | inf | 30 |
| test_2d_laser_ion_acc_picmi | /data/100/fields/j/x | 122 | 8.174e+13 | 4.041e-01 | 30 |
| test_2d_theta_implicit_jfnk_vandb | boxlib/divE | 1599 | 3.840e+07 | 8.870e-11 | 0 |
| test_2d_theta_implicit_jfnk_vandb_picmi | boxlib/divE | 1600 | 3.265e+07 | 1.144e-10 | 0 |
| test_2d_theta_implicit_jfnk_vandb_filtered | boxlib/divE | 1596 | 1.581e+07 | 5.169e-11 | 0 |
| test_3d_ion_beam_extraction | /data/0/fields/E/z | 5516 | 1.136e+07 | 5.205e+02 | 30 |
| test_rz_ohm_solver_resistive_drag_species_fields_picmi | /data/1/fields/current_fp_ions[dir=z] | 8 | 7.823e+05 | 2.470e-01 | 0 |
| test_2d_energy_conserving_thermal_plasma | boxlib/jx | 64 | 3.587e+05 | 5.442e-10 | 0 |
| test_3d_acceleration_psatd_restart | boxlib/jz | 15680 | 2.408e+05 | 8.927e-05 | 0 |
| test_3d_acceleration_psatd | boxlib/jz | 15680 | 2.408e+05 | 8.929e-05 | 0 |
| test_3d_acceleration_psatd_time_avg_restart | boxlib/jz | 15680 | 2.159e+05 | 3.349e-04 | 0 |
| test_3d_acceleration_psatd_time_avg | boxlib/jz | 15680 | 2.159e+05 | 3.349e-04 | 0 |
| test_2d_ohm_solver_electron_energy_qei_picmi | /data/72/fields/j/z | 128 | 1.608e+05 | 1.396e+02 | 0 |
| test_3d_theta_implicit_jfnk_direct | boxlib/divE | 3045 | 1.311e+05 | 1.195e-11 | 0 |
| test_1d_theta_implicit_planar_pinch | boxlib/divE | 220 | 8.376e+04 | 6.641e-05 | 0 |
| test_2d_darwin_solver_em_modes_es_picmi | boxlib/Ez | 2048 | 2.923e+04 | 1.116e+03 | 0 |
| test_rz_magnetostatic_eb_picmi | boxlib/Er | 13312 | 2.729e+04 | 4.585e-02 | 0 |
| test_2d_ohm_solver_resistive_drag_per_species_picmi | /data/50/fields/Te | 256 | 2.275e+04 | 3.834e-03 | 0 |
| test_2d_ohm_solver_electron_energy_joule_picmi | /data/50/fields/Te | 256 | 1.545e+04 | 2.606e-03 | 0 |
| test_2d_ohm_solver_resistive_drag_picmi | /data/50/fields/Te | 256 | 1.523e+04 | 2.569e-03 | 0 |
| test_rz_laser_acceleration_opmd | /data/20/fields/rho | 2229 | 1.358e+04 | 4.118e+02 | 0 |
| test_rz_laser_acceleration | boxlib/rho | 565 | 1.358e+04 | 9.870e-01 | 0 |
| test_rz_laser_acceleration_picmi | /data/10/fields/rho | 394 | 1.104e+04 | 1.173e+01 | 0 |
| test_3d_magnetostatic_eb | boxlib/jz | 48896 | 1.018e+04 | 8.524e-01 | 0 |
| test_3d_magnetostatic_eb_picmi | boxlib/jz | 41472 | 8.262e+03 | 5.191e-01 | 0 |
| test_3d_beam_beam_collision | /data/90/fields/E/y | 262138 | 3.006e+03 | 5.942e-06 | 0 |
| test_1d_darwin_solver_em_modes_picmi | boxlib/Ey | 256 | 1.019e+03 | 2.754e+00 | 0 |
| test_2d_ohm_solver_electron_energy_vacuum_picmi | /data/70/fields/Te | 512 | 6.075e+02 | 1.392e-03 | 0 |
| test_rz_langmuir_multi_psatd_current_correction | boxlib/divE | 8190 | 3.676e+02 | 5.382e+02 | 0 |
| test_3d_nodal_electrostatic_solver | boxlib/Ey | 2082195 | 1.775e+02 | 1.512e-11 | 0 |
| test_2d_ohm_solver_electron_energy_adiabat_picmi | /data/36/fields/j/z | 256 | 2.763e+01 | 6.379e-01 | 0 |
| test_3d_laser_acceleration_btd | boxlib/jz | 837 | 1.000e+01 | 6.543e-13 | 0 |
| test_1d_laser_injection_implicit | boxlib/Ex | 213 | 8.487e+00 | 5.734e-09 | 0 |
| test_rz_psatd_JRhom_LL2 | boxlib/jz | 1975 | 6.609e+00 | 9.616e-09 | 0 |
| test_2d_laser_injection_implicit | boxlib/Ex | 165890 | 4.150e+00 | 1.966e-08 | 0 |
| test_2d_embedded_circle | /data/5/particles/electrons/nx | 192 | 1.999e+00 | 2.011e+01 | 272 |
| test_2d_laser_acceleration_boosted | boxlib/jz | 64 | 5.293e-01 | 1.378e-10 | 0 |
| test_rz_langmuir_multi | boxlib/jr | 1551 | 1.276e-01 | 1.731e-09 | 0 |
| test_rz_langmuir_multi_psatd_JRhom_LL4 | boxlib/jr | 7371 | 1.116e-01 | 7.750e-08 | 0 |
| test_rz_langmuir_multi_psatd | boxlib/jz | 7360 | 1.617e-02 | 7.814e-08 | 0 |
| test_rz_pml_psatd | boxlib/rho | 2 | 1.337e-02 | 2.000e+00 | 0 |
| test_rz_langmuir_fluid | boxlib/jr | 8160 | 2.289e-03 | 1.046e-08 | 0 |
| test_2d_prev_positions_picmi | boxlib/Ex | 4096 | 4.389e-04 | 3.615e-04 | 0 |
| test_3d_particle_scrape_picmi | boxlib/Ey | 8393 | 4.768e-06 | 6.042e-12 | 0 |
| test_1d_collision_z_inverse_Bremsstrahlung | electrons/particle_position_x | 102400 | 1.000e-06 | 4.708e+04 | 21 |
| test_rz_galilean_psatd_current_correction_psb | boxlib/divE | 8192 | 7.790e-07 | 2.377e-04 | 0 |
| test_rz_galilean_psatd | boxlib/divE | 8192 | 7.478e-07 | 3.009e-04 | 0 |
| test_1d_macroscopic_solver | boxlib/Ex | 121 | 4.377e-07 | 3.827e-14 | 0 |
| test_1d_macroscopic_solver_picmi | boxlib/Ex | 121 | 4.377e-07 | 3.827e-14 | 0 |
| test_2d_macroscopic_solver | boxlib/Ex | 3780 | 2.161e-07 | 6.744e-13 | 0 |
| test_rz_spacecraft_charging_picmi | /data/80/fields/E/z | 3178 | 1.037e-07 | 5.186e-07 | 0 |
| test_3d_embedded_boundary_cube_macroscopic | boxlib/Ex | 32768 | 9.906e-09 | 1.265e+03 | 0 |
| test_rz_ohm_solver_em_modes_picmi | /data/90/fields/E/r | 8190 | 1.773e-09 | 8.821e-10 | 0 |
| test_3d_effective_potential_electrostatic_picmi | /data/36/fields/E/z | 175444 | 5.585e-10 | 3.116e-07 | 0 |
| test_rz_ohm_solver_cylinder_compression_picmi | boxlib/Tt_ions | 168 | 5.821e-11 | 6.295e-16 | 0 |
| test_rz_laser_acceleration_boosted_opmd | /data/5/fields/rho | 24505 | 2.968e-11 | 1.128e+04 | 0 |
| test_3d_focusing_gaussian_beam | boxlib/rho_beam1 | 459 | 2.910e-11 | 3.196e-15 | 0 |
| test_2d_theta_implicit_jfnk_vandb_cropping | boxlib/divE | 250 | 2.629e-11 | 3.667e+00 | 0 |
| test_3d_laser_acceleration | /data/100/fields/rho | 921 | 1.091e-11 | 4.671e-16 | 0 |
| test_3d_laser_acceleration_picmi | boxlib/rho | 921 | 1.091e-11 | 4.671e-16 | 0 |
| test_3d_laser_acceleration_python | /data/100/fields/rho | 921 | 1.091e-11 | 4.671e-16 | 0 |
| test_3d_laser_acceleration_single_precision_comms | /data/100/fields/rho | 921 | 1.091e-11 | 4.671e-16 | 0 |
| test_1d_laser_acceleration | boxlib/rho | 1 | 1.819e-12 | 1.602e-16 | 0 |
| test_1d_laser_acceleration_picmi | boxlib/rho | 1 | 1.819e-12 | 1.602e-16 | 0 |
| test_rz_electrostatic_sphere | all/particle_theta | 74 | 6.661e-16 | 1.124e-15 | 0 |
| test_rz_electrostatic_sphere_uniform_weighting | all/particle_theta | 177 | 6.661e-16 | 1.343e-15 | 0 |
| test_3d_hard_edged_quadrupoles_moving | boxlib/Ez | 2048 | 2.680e-18 | 1.814e-10 | 0 |
| test_1d_photoneutralization_dsmc | /data/100/fields/rho | 2 | 5.421e-20 | 2.725e-16 | 0 |
| test_1d_two_product_reaction_dsmc | /data/30/fields/rho | 1 | 5.421e-20 | 1.651e-16 | 0 |
| test_2d_background_mcc_picmi | boxlib/rho_electrons | 7 | 1.355e-20 | 4.406e-16 | 0 |
| test_rz_galilean_psatd_current_correction | boxlib/rho | 8192 | 6.255e-21 | 6.950e-08 | 0 |
| test_1d_dsmc_picmi | boxlib/rho_he_ions | 1 | 4.235e-22 | 1.755e-16 | 0 |
| test_rz_embedded_boundary_em_particle_absorption_sh_factor_1 | /data/1/fields/rho | 4 | 6.019e-36 | 2.706e-16 | 0 |
| test_rz_embedded_boundary_em_particle_absorption_sh_factor_2 | /data/1/fields/rho | 4 | 6.019e-36 | 2.706e-16 | 0 |
| test_rz_embedded_boundary_em_particle_absorption_sh_factor_3 | /data/1/fields/rho | 8 | 6.019e-36 | 3.386e-16 | 0 |
| test_rz_flux_injection | all/particle_momentum_z | 3480 | 7.468e-60 | 4.986e-12 | 0 |
| test_3d_collision_pulsed_decay | all/particle_cpu | None | 0.000e+00 | 0.000e+00 | 45 |

## Cases with only log/text differences

| case | file | differing lines |
|---|---|---|

## Cases where only the composed input deck differs

| case | lines |
|---|---|
| test_1d_pierce_diode | 1 |

## 结论（2026-11-15，冻结二进制重跑）

- 79 个用例用当前源码的二进制干净重跑：**79/79 通过**（`/tmp/ctest_fork79b.log`）。
- 再对比官方 26.10：**78 个仍有数值差异、1 个只剩元数据差异**。
  这与“79 这个列表本身就是在修复 #5/#6 之后统计出来的”一致，所以不期待该列表变小；
  真正的进展需要继续修结构性根因。
- **更正（同日复核）**：先前把 `2.8e307`、`1.5e179` 当成“未初始化诊断场”是**误判**。
  这些数来自 `compare_case.py` 的 `binary_diff` 兜底分支：当两个二进制文件逐字节不同时，
  脚本会把整份文件按 float64 重新解释来估量级；而 AMReX plotfile 的 `Cell_D_*` 里除数据外
  还有每 FAB 的小头部/填充，按 float64 读会得到 1e179 这类垃圾值。
  **以逐变量（yt / h5py）的结果为准**，这些用例的场量级都在正常物理范围
  （例如 `test_2d_embedded_circle` 的 `phi` 差 3.9e-4、`rho` 差 3.3e-3）。
  工具侧的正确口径是 `analyze_results.py`（只看 plotfile 变量与 hdf5 dataset），
  而不是 JSON 顶层的 `max_abs_diff`（它含 binary_diff 兜底值）。

## 验收状态（对照“官方与 fork 跑同一算例结果相同”）

| 口径 | 结果 |
|---|---|
| 用例可运行性 | **398/398 两侧都能运行、都通过**（无环境阻塞、无 fork 崩溃） |
| 逐字节相同 | 13 / 398 |
| 每个输出变量逐位相同（仅构建元数据差异） | 233 / 398 |
| 仅日志文本差异 | 20 / 398 |
| 至少一个输出变量不同 | 132 / 398（其中 50 归因 AMReX/上游演进，79 归因 fork 代码，3 未归因） |
| 79 个 fork-code 用例的首个 diag | 47 例逐位相同、11 例 ≤1e-9、21 例有可测差异 |

**结论：验收标准（全部算例结果相同）尚未达成。** 当前差距分三类：
1. **结构性（应修）**：21 例在第一个输出就有差异，其中已定位到 `RZ rho 诊断`（t=0 粒子逐属性相同、
   仅 rho 的 343 格点不同）与 `RZ 逐物种 rho/current`（z 上边界一排）；其余多为求解器迭代差异
   （场量级 1e-8–1e-5）。
2. **运算次序（可对齐但成本高）**：47 例第一个输出逐位相同，后续因重构改变了求和/迭代顺序而发散；
   要对齐必须逐路径恢复与上游相同的运算次序。
3. **度量噪声（已排除）**：binary_diff 的 float64 兜底读数、first-step 与全量最大差的不同口径，
   这两类都在本报告里更正过，不应再计入“真实差异”。
