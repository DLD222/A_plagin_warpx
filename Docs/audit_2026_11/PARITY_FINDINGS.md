# 官方算例一致性排查记录（2026-11-19）

## 已确认的判据
- 官方二进制**默认不可复现**（同命令两次 max|Δ| ~1e-3）；固定 `warpx.random_seed` 后逐位可复现。
- 多线程会改变随机数调用顺序 → 比对必须单线程（`omp=1`）。
- 判据用**相对**误差（`j` 峰值可达 1e12，绝对差 1e-3 实为 1e-15 相对）。

## 已通过（相对容差 1e-10）
1D: synchronize_velocity、resample_velocity_coincidence_thinning(+cartesian)、
    collision_z_correct_conservation、langmuir_multi、langmuir_fluid、
    semi_implicit_picard、theta_implicit_picard、theta_implicit_planar_pinch
2D: langmuir_multi、bilinear_filter、qed_quantum_sync、leveling_thinning、theta_implicit_jfnk_vandb_cropping
3D: focusing_gaussian_beam(+photons)、rotated_gaussian_beam、collider_diagnostics、beamsize_effect
RZ: electrostatic_sphere_eb、electrostatic_sphere_eb_mr

## 未通过且已定性
- 参考版崩溃：nuclear_fusion(2d/rz)、parabolic_channel、macroscopic_solver(1d, 参考版 segfault)
- 片段算例：inputs_base_*（需 inputs_test_* 覆盖）
- 需要外部数据：load_density、read_density_from_file 等

## 未解决：dynamic_regrid（真实差异）
证据（iter 1）：
```
参考版  B/y max=1.770e-03   B_lvl1/y max=3.945e-02
fork    B/y max=3.826e-07   无 B_lvl1/y
```
即 fork 在该时刻**没有 level-1 输出**，且 level-0 的场小了约 4600 倍；到 iter 8 两者都有 lvl1，
但 `j` 相对差 7%（level0）/ 97%（level1）。
已用临时打印确认 `RemakeLevel` 保存并回填了 33 个字段（old_max == new_max），
所以问题不在"remake 丢数据"，而在**重网格发生的时机**（fork 在诊断之后、参考在之前）
或 level-0 场在 regrid 前后被覆盖。待查。
