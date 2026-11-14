# 数值差异的逐案原因索引

- 数值差异用例：132；其中 fork 代码差异 79、AMReX/上游演进 50。
- 本文件按物理类别聚合 "fork 代码差异" 的用例，给出最差字段/量级与当前状态。
- 逐案原始记录：`audit_runs/compare_fork_vs_official/cases/<name>.json`；归因表：`ATTRIBUTION.md`。

## 按类别

- 激光/加速: 16 例
- PSATD/谱: 12 例
- 其他: 11 例
- RZ: 10 例
- Ohm/混合: 9 例
- 隐式/Darwin: 8 例
- 碰撞/DSMC: 7 例
- EB/嵌入边界: 4 例
- 静电: 2 例

## 逐案（fork 代码差异，按最大绝对差排序）

| 用例 | 类别 | 最差字段 | max abs | 状态/根因 |
|---|---|---|---|---|
| test_2d_collisions_split_momentum_push_electrostatic | 碰撞/DSMC | /data/2000/fields/j/z | 1.74e+17 | 待定根因（见 REPORT §5） |
| test_2d_laser_ion_acc | 激光/加速 | /data/100/meshes/data | 2.99e+14 | 待定根因（见 REPORT §5） |
| test_2d_laser_ion_acc_picmi | 激光/加速 | /data/100/fields/j/x | 8.17e+13 | 待定根因（见 REPORT §5） |
| test_2d_theta_implicit_jfnk_vandb | 隐式/Darwin | boxlib/divE | 3.84e+07 | 待定根因（见 REPORT §5） |
| test_2d_theta_implicit_jfnk_vandb_picmi | 隐式/Darwin | boxlib/divE | 3.26e+07 | 待定根因（见 REPORT §5） |
| test_2d_theta_implicit_jfnk_vandb_filtered | 隐式/Darwin | boxlib/divE | 1.58e+07 | 待定根因（见 REPORT §5） |
| test_3d_ion_beam_extraction | 其他 | /data/0/fields/E/z | 1.14e+07 | 待定根因（见 REPORT §5） |
| test_rz_ohm_solver_resistive_drag_species_fields_picmi | Ohm/混合 | /data/1/fields/current_fp_ions[dir=z] | 7.82e+05 | 待定根因（见 REPORT §5） |
| test_2d_energy_conserving_thermal_plasma | 其他 | boxlib/jx | 3.59e+05 | 待定根因（见 REPORT §5） |
| test_3d_acceleration_psatd_restart | PSATD/谱 | boxlib/jz | 2.41e+05 | 待定根因（见 REPORT §5） |
| test_3d_acceleration_psatd | PSATD/谱 | boxlib/jz | 2.41e+05 | 待定根因（见 REPORT §5） |
| test_3d_acceleration_psatd_time_avg_restart | PSATD/谱 | boxlib/jz | 2.16e+05 | 待定根因（见 REPORT §5） |
| test_3d_acceleration_psatd_time_avg | PSATD/谱 | boxlib/jz | 2.16e+05 | 待定根因（见 REPORT §5） |
| test_2d_ohm_solver_electron_energy_qei_picmi | Ohm/混合 | /data/72/fields/j/z | 1.61e+05 | 待定根因（见 REPORT §5） |
| test_3d_theta_implicit_jfnk_direct | 隐式/Darwin | boxlib/divE | 1.31e+05 | 待定根因（见 REPORT §5） |
| test_1d_theta_implicit_planar_pinch | 隐式/Darwin | boxlib/divE | 8.38e+04 | 待定根因（见 REPORT §5） |
| test_2d_darwin_solver_em_modes_es_picmi | 隐式/Darwin | boxlib/Ez | 2.92e+04 | 待定根因（见 REPORT §5） |
| test_rz_magnetostatic_eb_picmi | RZ | boxlib/Er | 2.73e+04 | 待定根因（见 REPORT §5） |
| test_2d_ohm_solver_resistive_drag_per_species_picmi | Ohm/混合 | /data/50/fields/Te | 2.28e+04 | 待定根因（见 REPORT §5） |
| test_2d_ohm_solver_electron_energy_joule_picmi | Ohm/混合 | /data/50/fields/Te | 1.54e+04 | 待定根因（见 REPORT §5） |
| test_2d_ohm_solver_resistive_drag_picmi | Ohm/混合 | /data/50/fields/Te | 1.52e+04 | 待定根因（见 REPORT §5） |
| test_rz_laser_acceleration_opmd | 激光/加速 | /data/20/fields/rho | 1.36e+04 | 待定根因（见 REPORT §5） |
| test_rz_laser_acceleration | 激光/加速 | boxlib/rho | 1.36e+04 | 待定根因（见 REPORT §5） |
| test_rz_laser_acceleration_picmi | 激光/加速 | /data/10/fields/rho | 1.10e+04 | 待定根因（见 REPORT §5） |
| test_3d_magnetostatic_eb | EB/嵌入边界 | boxlib/jz | 1.02e+04 | 待定根因（见 REPORT §5） |
| test_3d_magnetostatic_eb_picmi | EB/嵌入边界 | boxlib/jz | 8.26e+03 | 待定根因（见 REPORT §5） |
| test_3d_beam_beam_collision | 碰撞/DSMC | /data/90/fields/E/y | 3.01e+03 | 待定根因（见 REPORT §5） |
| test_1d_darwin_solver_em_modes_picmi | 隐式/Darwin | boxlib/Ey | 1.02e+03 | 待定根因（见 REPORT §5） |
| test_2d_ohm_solver_electron_energy_vacuum_picmi | Ohm/混合 | /data/70/fields/Te | 6.08e+02 | 待定根因（见 REPORT §5） |
| test_rz_langmuir_multi_psatd_current_correction | PSATD/谱 | boxlib/divE | 3.68e+02 | 待定根因（见 REPORT §5） |
| test_3d_nodal_electrostatic_solver | 静电 | boxlib/Ey | 1.78e+02 | 待定根因（见 REPORT §5） |
| test_2d_ohm_solver_electron_energy_adiabat_picmi | Ohm/混合 | /data/36/fields/j/z | 2.76e+01 | 待定根因（见 REPORT §5） |
| test_3d_laser_acceleration_btd | 激光/加速 | boxlib/jz | 1.00e+01 | 待定根因（见 REPORT §5） |
| test_1d_laser_injection_implicit | 激光/加速 | boxlib/Ex | 8.49e+00 | 待定根因（见 REPORT §5） |
| test_1d_pierce_diode | 其他 | /data/5000/fields/j/z | 7.27e+00 | 待定根因（见 REPORT §5） |
| test_rz_psatd_JRhom_LL2 | PSATD/谱 | boxlib/jz | 6.61e+00 | 待定根因（见 REPORT §5） |
| test_2d_laser_injection_implicit | 激光/加速 | boxlib/Ex | 4.15e+00 | 待定根因（见 REPORT §5） |
| test_2d_embedded_circle | EB/嵌入边界 | /data/5/particles/electrons/nx | 2.00e+00 | 待定根因（见 REPORT §5） |
| test_2d_laser_acceleration_boosted | 激光/加速 | boxlib/jz | 5.29e-01 | 待定根因（见 REPORT §5） |
| test_rz_langmuir_multi | RZ | boxlib/jr | 1.28e-01 | 待定根因（见 REPORT §5） |
| test_rz_langmuir_multi_psatd_JRhom_LL4 | PSATD/谱 | boxlib/jr | 1.12e-01 | 待定根因（见 REPORT §5） |
| test_rz_langmuir_multi_psatd | PSATD/谱 | boxlib/jz | 1.62e-02 | 待定根因（见 REPORT §5） |
| test_rz_pml_psatd | PSATD/谱 | boxlib/rho | 1.34e-02 | 待定根因（见 REPORT §5） |
| test_rz_langmuir_fluid | RZ | boxlib/jr | 2.29e-03 | 待定根因（见 REPORT §5） |
| test_3d_effective_potential_electrostatic_picmi | 静电 | /data/0/fields/j/x | 2.04e-03 | 待定根因（见 REPORT §5） |
| test_2d_prev_positions_picmi | 其他 | boxlib/Ex | 4.39e-04 | 待定根因（见 REPORT §5） |
| test_3d_particle_scrape_picmi | 其他 | boxlib/Ex | 3.81e-06 | 待定根因（见 REPORT §5） |
| test_1d_collision_z_inverse_Bremsstrahlung | 碰撞/DSMC | electrons/particle_position_x | 1.00e-06 | 待定根因（见 REPORT §5） |
| test_rz_galilean_psatd_current_correction_psb | PSATD/谱 | boxlib/divE | 7.79e-07 | 待定根因（见 REPORT §5） |
| test_rz_galilean_psatd | PSATD/谱 | boxlib/divE | 7.48e-07 | 待定根因（见 REPORT §5） |
| test_1d_macroscopic_solver_picmi | 其他 | boxlib/Ex | 4.38e-07 | 待定根因（见 REPORT §5） |
| test_1d_macroscopic_solver | 其他 | boxlib/Ex | 4.38e-07 | 待定根因（见 REPORT §5） |
| test_2d_macroscopic_solver | 其他 | boxlib/Ex | 2.16e-07 | 待定根因（见 REPORT §5） |
| test_rz_spacecraft_charging_picmi | RZ | /data/80/fields/E/z | 1.04e-07 | 待定根因（见 REPORT §5） |
| test_3d_embedded_boundary_cube_macroscopic | EB/嵌入边界 | boxlib/Ex | 9.91e-09 | 待定根因（见 REPORT §5） |
| test_rz_ohm_solver_em_modes_picmi | Ohm/混合 | /data/90/fields/E/r | 1.77e-09 | 待定根因（见 REPORT §5） |
| test_rz_ohm_solver_cylinder_compression_picmi | Ohm/混合 | boxlib/Tt_ions | 5.82e-11 | 待定根因（见 REPORT §5） |
| test_rz_laser_acceleration_boosted_opmd | 激光/加速 | /data/5/fields/rho | 2.97e-11 | 待定根因（见 REPORT §5） |
| test_3d_focusing_gaussian_beam | 其他 | boxlib/rho_beam1 | 2.91e-11 | 待定根因（见 REPORT §5） |
| test_2d_theta_implicit_jfnk_vandb_cropping | 隐式/Darwin | boxlib/divE | 2.63e-11 | 待定根因（见 REPORT §5） |
| test_3d_laser_acceleration_python | 激光/加速 | /data/100/fields/rho | 1.09e-11 | 待定根因（见 REPORT §5） |
| test_3d_laser_acceleration_picmi | 激光/加速 | boxlib/rho | 1.09e-11 | 待定根因（见 REPORT §5） |
| test_3d_laser_acceleration | 激光/加速 | /data/100/fields/rho | 1.09e-11 | 待定根因（见 REPORT §5） |
| test_3d_laser_acceleration_single_precision_comms | 激光/加速 | /data/100/fields/rho | 1.09e-11 | 待定根因（见 REPORT §5） |
| test_1d_laser_acceleration_picmi | 激光/加速 | boxlib/rho | 1.82e-12 | 待定根因（见 REPORT §5） |
| test_1d_laser_acceleration | 激光/加速 | boxlib/rho | 1.82e-12 | 待定根因（见 REPORT §5） |
| test_rz_electrostatic_sphere | RZ | all/particle_theta | 6.66e-16 | 待定根因（见 REPORT §5） |
| test_rz_electrostatic_sphere_uniform_weighting | RZ | all/particle_theta | 6.66e-16 | 待定根因（见 REPORT §5） |
| test_3d_hard_edged_quadrupoles_moving | 其他 | boxlib/Ez | 2.68e-18 | 待定根因（见 REPORT §5） |
| test_1d_photoneutralization_dsmc | 碰撞/DSMC | /data/100/fields/rho | 5.42e-20 | 待定根因（见 REPORT §5） |
| test_1d_two_product_reaction_dsmc | 碰撞/DSMC | /data/30/fields/rho | 5.42e-20 | 待定根因（见 REPORT §5） |
| test_2d_background_mcc_picmi | 其他 | boxlib/rho_electrons | 1.35e-20 | 待定根因（见 REPORT §5） |
| test_rz_galilean_psatd_current_correction | PSATD/谱 | boxlib/rho | 6.26e-21 | 待定根因（见 REPORT §5） |
| test_1d_dsmc_picmi | 碰撞/DSMC | boxlib/rho_he_ions | 4.24e-22 | 待定根因（见 REPORT §5） |
| test_rz_embedded_boundary_em_particle_absorption_sh_factor_2 | RZ | /data/1/fields/rho | 6.02e-36 | 待定根因（见 REPORT §5） |
| test_rz_embedded_boundary_em_particle_absorption_sh_factor_1 | RZ | /data/1/fields/rho | 6.02e-36 | 待定根因（见 REPORT §5） |
| test_rz_embedded_boundary_em_particle_absorption_sh_factor_3 | RZ | /data/1/fields/rho | 6.02e-36 | 待定根因（见 REPORT §5） |
| test_rz_flux_injection | RZ | all/particle_momentum_z | 7.47e-60 | 待定根因（见 REPORT §5） |
| test_3d_collision_pulsed_decay | 碰撞/DSMC | all/particle_cpu | 0.00e+00 | 待定根因（见 REPORT §5） |
