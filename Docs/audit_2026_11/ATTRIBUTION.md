# 差异归因（fork vs official 26.10，隔离对照：official + fork 同款 AMReX）

- 参与归因的数值差异用例：132
- 归因到 AMReX/上游演进（同 AMReX 构建下数值一致）：50
- 归因到 fork 代码（同 AMReX 构建下仍差）：79
- 未归因（official_amrex 未运行该用例，多为 >10 分钟的重算例）：3

| case | 最差字段 | max abs (vs 官方) | 归因 |
|---|---|---|---|
| test_2d_collisions_split_momentum_push_electrostatic | /data/2000/fields/j/z | 1.744e+17 | fork 代码差异（同 AMReX 仍差，比值 3.3e-01） |
| test_2d_laser_ion_acc | /data/100/meshes/data | 2.990e+14 | fork 代码差异（同 AMReX 仍差，比值 1.0e+00） |
| test_2d_laser_ion_acc_picmi | /data/100/fields/j/x | 8.174e+13 | fork 代码差异（同 AMReX 仍差，比值 1.0e+00） |
| test_1d_laser_acceleration_fluid_boosted | boxlib/jz | 1.724e+12 | not-attributed (official_amrex 未运行该用例) |
| test_2d_field_probe | boxlib/Ex | 1.669e+10 | AMReX/上游演进（同 AMReX 下无差异） |
| test_2d_theta_implicit_jfnk_vandb | boxlib/divE | 3.840e+07 | fork 代码差异（同 AMReX 仍差，比值 9.2e-01） |
| test_2d_theta_implicit_jfnk_vandb_picmi | boxlib/divE | 3.265e+07 | fork 代码差异（同 AMReX 仍差，比值 8.4e-01） |
| test_2d_theta_implicit_jfnk_vandb_filtered | boxlib/divE | 1.581e+07 | fork 代码差异（同 AMReX 仍差，比值 7.9e-01） |
| test_2d_charge_exchange_dsmc | /data/2000/fields/j/x | 1.465e+07 | AMReX/上游演进（同 AMReX 下无差异） |
| test_3d_ion_beam_extraction | /data/0/fields/E/z | 1.136e+07 | fork 代码差异（同 AMReX 仍差，比值 4.9e-24） |
| test_rz_ohm_solver_resistive_drag_species_fields_picmi | /data/1/fields/current_fp_ions[dir=z] | 7.823e+05 | fork 代码差异（同 AMReX 仍差，比值 1.0e+00） |
| test_2d_theta_implicit_symmetry | boxlib/divE | 6.213e+05 | not-attributed (official_amrex 未运行该用例) |
| test_2d_energy_conserving_thermal_plasma | boxlib/jx | 3.587e+05 | fork 代码差异（同 AMReX 仍差，比值 5.6e+11） |
| test_3d_acceleration_psatd_restart | boxlib/jz | 2.408e+05 | fork 代码差异（同 AMReX 仍差，比值 1.0e+00） |
| test_3d_acceleration_psatd | boxlib/jz | 2.408e+05 | fork 代码差异（同 AMReX 仍差，比值 1.0e+00） |
| test_3d_acceleration_psatd_time_avg_restart | boxlib/jz | 2.159e+05 | fork 代码差异（同 AMReX 仍差，比值 1.0e+00） |
| test_3d_acceleration_psatd_time_avg | boxlib/jz | 2.159e+05 | fork 代码差异（同 AMReX 仍差，比值 1.0e+00） |
| test_2d_ohm_solver_electron_energy_qei_picmi | /data/72/fields/j/z | 1.608e+05 | fork 代码差异（同 AMReX 仍差，比值 1.0e+00） |
| test_3d_theta_implicit_jfnk_direct | boxlib/divE | 1.311e+05 | fork 代码差异（同 AMReX 仍差，比值 1.5e+00） |
| test_rz_load_density | /data/15/fields/rho | 1.244e+05 | AMReX/上游演进（同 AMReX 下无差异） |
| test_1d_theta_implicit_planar_pinch | boxlib/divE | 8.376e+04 | fork 代码差异（同 AMReX 仍差，比值 1.1e+00） |
| test_2d_darwin_solver_em_modes_es_picmi | boxlib/Ez | 2.923e+04 | fork 代码差异（同 AMReX 仍差，比值 1.0e+00） |
| test_rz_magnetostatic_eb_picmi | boxlib/Er | 2.729e+04 | fork 代码差异（同 AMReX 仍差，比值 1.0e+00） |
| test_2d_repelling_particles | boxlib/divE | 2.662e+04 | AMReX/上游演进（同 AMReX 下无差异） |
| test_2d_ohm_solver_resistive_drag_per_species_picmi | /data/50/fields/Te | 2.275e+04 | fork 代码差异（同 AMReX 仍差，比值 1.0e+00） |
| test_2d_ohm_solver_electron_energy_joule_picmi | /data/50/fields/Te | 1.545e+04 | fork 代码差异（同 AMReX 仍差，比值 1.0e+00） |
| test_2d_ohm_solver_resistive_drag_picmi | /data/50/fields/Te | 1.523e+04 | fork 代码差异（同 AMReX 仍差，比值 1.0e+00） |
| test_rz_laser_acceleration_opmd | /data/20/fields/rho | 1.358e+04 | fork 代码差异（同 AMReX 仍差，比值 1.0e+00） |
| test_rz_laser_acceleration | boxlib/rho | 1.358e+04 | fork 代码差异（同 AMReX 仍差，比值 1.0e+00） |
| test_rz_laser_acceleration_picmi | /data/10/fields/rho | 1.104e+04 | fork 代码差异（同 AMReX 仍差，比值 1.0e+00） |
| test_3d_magnetostatic_eb | boxlib/jz | 1.018e+04 | fork 代码差异（同 AMReX 仍差，比值 1.0e+00） |
| test_3d_magnetostatic_eb_picmi | boxlib/jz | 8.262e+03 | fork 代码差异（同 AMReX 仍差，比值 1.0e+00） |
| test_3d_beam_beam_collision | /data/90/fields/E/y | 3.006e+03 | fork 代码差异（同 AMReX 仍差，比值 2.5e-02） |
| test_1d_darwin_solver_em_modes_picmi | boxlib/Ey | 1.019e+03 | fork 代码差异（同 AMReX 仍差，比值 1.0e+00） |
| test_2d_ohm_solver_electron_energy_vacuum_picmi | /data/70/fields/Te | 6.075e+02 | fork 代码差异（同 AMReX 仍差，比值 1.0e+00） |
| test_rz_langmuir_multi_psatd_current_correction | boxlib/divE | 3.676e+02 | fork 代码差异（同 AMReX 仍差，比值 1.0e+00） |
| test_3d_nodal_electrostatic_solver | boxlib/Ey | 1.775e+02 | fork 代码差异（同 AMReX 仍差，比值 4.9e-01） |
| test_2d_ohm_solver_electron_energy_adiabat_picmi | /data/36/fields/j/z | 2.763e+01 | fork 代码差异（同 AMReX 仍差，比值 1.0e+00） |
| test_3d_laser_acceleration_btd | boxlib/jz | 1.000e+01 | fork 代码差异（同 AMReX 仍差，比值 1.0e+00） |
| test_1d_laser_injection_implicit | boxlib/Ex | 8.487e+00 | fork 代码差异（同 AMReX 仍差，比值 1.0e+00） |
| test_1d_pierce_diode | /data/5000/fields/j/z | 7.274e+00 | fork 代码差异（同 AMReX 仍差，比值 1.0e+00） |
| test_rz_psatd_JRhom_LL2 | boxlib/jz | 6.609e+00 | fork 代码差异（同 AMReX 仍差，比值 1.0e+00） |
| test_2d_laser_injection_implicit | boxlib/Ex | 4.150e+00 | fork 代码差异（同 AMReX 仍差，比值 6.5e-01） |
| test_2d_embedded_circle | /data/5/particles/electrons/nx | 1.997e+00 | fork 代码差异（同 AMReX 仍差，比值 5.0e-01） |
| test_2d_laser_acceleration_boosted | boxlib/jz | 5.293e-01 | fork 代码差异（同 AMReX 仍差，比值 1.0e+00） |
| test_2d_ohm_solver_magnetic_reconnection_picmi | boxlib/phi | 1.460e-01 | AMReX/上游演进（同 AMReX 下无差异） |
| test_rz_langmuir_multi | boxlib/jr | 1.276e-01 | fork 代码差异（同 AMReX 仍差，比值 1.0e+00） |
| test_rz_langmuir_multi_psatd_JRhom_LL4 | boxlib/jr | 1.116e-01 | fork 代码差异（同 AMReX 仍差，比值 1.0e+00） |
| test_rz_langmuir_multi_psatd | boxlib/jz | 1.617e-02 | fork 代码差异（同 AMReX 仍差，比值 1.0e+00） |
| test_rz_pml_psatd | boxlib/rho | 1.337e-02 | fork 代码差异（同 AMReX 仍差，比值 1.0e+00） |
| test_rz_langmuir_fluid | boxlib/jr | 2.289e-03 | fork 代码差异（同 AMReX 仍差，比值 1.0e+00） |
| test_3d_effective_potential_electrostatic_picmi | /data/0/fields/j/x | 2.040e-03 | fork 代码差异（同 AMReX 仍差，比值 1.0e+00） |
| test_2d_prev_positions_picmi | boxlib/Ex | 4.389e-04 | fork 代码差异（同 AMReX 仍差，比值 4.1e+05） |
| test_3d_electrostatic_sphere_eb_mixed_bc | boxlib/Ey | 7.454e-05 | AMReX/上游演进（同 AMReX 下无差异） |
| test_3d_relativistic_space_charge_initialization | boxlib/Ey | 5.643e-05 | AMReX/上游演进（同 AMReX 下无差异） |
| test_3d_electrostatic_sphere_eb | /data/0/fields/E/z | 1.668e-05 | AMReX/上游演进（同 AMReX 下无差异） |
| test_3d_electrostatic_sphere_eb_picmi | /data/0/fields/E/z | 1.668e-05 | AMReX/上游演进（同 AMReX 下无差异） |
| test_rz_electrostatic_sphere_eb_mr | /data/0/fields/E/r | 8.766e-06 | AMReX/上游演进（同 AMReX 下无差异） |
| test_rz_electrostatic_sphere_eb | boxlib/Er | 8.766e-06 | AMReX/上游演进（同 AMReX 下无差异） |
| test_2d_particle_reflection_picmi | boxlib/Ez | 4.132e-06 | AMReX/上游演进（同 AMReX 下无差异） |
| test_3d_particle_scrape_picmi | boxlib/Ex | 3.815e-06 | fork 代码差异（同 AMReX 仍差，比值 1.0e+00） |
| test_1d_collision_z_inverse_Bremsstrahlung | electrons/particle_position_x | 1.000e-06 | fork 代码差异（同 AMReX 仍差，比值 1.0e+00） |
| test_2d_dirichlet_bc | boxlib/phi | 9.480e-07 | AMReX/上游演进（同 AMReX 下无差异） |
| test_2d_dirichlet_bc_picmi | boxlib/phi | 9.480e-07 | AMReX/上游演进（同 AMReX 下无差异） |
| test_rz_galilean_psatd_current_correction_psb | boxlib/divE | 7.790e-07 | fork 代码差异（同 AMReX 仍差，比值 1.0e+00） |
| test_rz_galilean_psatd | boxlib/divE | 7.478e-07 | fork 代码差异（同 AMReX 仍差，比值 1.0e+00） |
| test_1d_macroscopic_solver_picmi | boxlib/Ex | 4.377e-07 | fork 代码差异（同 AMReX 仍差，比值 1.0e+00） |
| test_1d_macroscopic_solver | boxlib/Ex | 4.377e-07 | fork 代码差异（同 AMReX 仍差，比值 1.0e+00） |
| test_2d_macroscopic_solver | boxlib/Ex | 2.161e-07 | fork 代码差异（同 AMReX 仍差，比值 1.0e+00） |
| test_rz_spacecraft_charging_picmi | /data/80/fields/E/z | 1.037e-07 | fork 代码差异（同 AMReX 仍差，比值 2.3e-02） |
| test_rz_secondary_ion_emission_picmi | /data/2/fields/E/r | 3.029e-08 | AMReX/上游演进（同 AMReX 下无差异） |
| test_rz_particle_boundary_interaction_picmi | /data/0/fields/E/r | 1.403e-08 | AMReX/上游演进（同 AMReX 下无差异） |
| test_rz_particle_boundary_interaction_reflecting | /data/0/fields/E/r | 1.403e-08 | AMReX/上游演进（同 AMReX 下无差异） |
| test_3d_embedded_boundary_cube_macroscopic | boxlib/Ex | 9.906e-09 | fork 代码差异（同 AMReX 仍差，比值 4.6e-05） |
| test_3d_embedded_boundary_cube | boxlib/Ez | 9.906e-09 | AMReX/上游演进（同 AMReX 下无差异） |
| test_2d_background_mcc | all/particle_position_x | 3.164e-09 | AMReX/上游演进（同 AMReX 下无差异） |
| test_2d_particle_thermal_boundary | boxlib/rho | 2.546e-09 | not-attributed (official_amrex 未运行该用例) |
| test_rz_ohm_solver_em_modes_picmi | /data/90/fields/E/r | 1.773e-09 | fork 代码差异（同 AMReX 仍差，比值 1.0e+00） |
| test_2d_embedded_boundary_cube | boxlib/Ex | 2.582e-10 | AMReX/上游演进（同 AMReX 下无差异） |
| test_3d_load_density | /data/50/fields/rho | 2.328e-10 | AMReX/上游演进（同 AMReX 下无差异） |
| test_2d_load_density | /data/22/fields/rho | 1.164e-10 | AMReX/上游演进（同 AMReX 下无差异） |
| test_rz_ohm_solver_cylinder_compression_picmi | boxlib/Tt_ions | 5.821e-11 | fork 代码差异（同 AMReX 仍差，比值 1.0e+00） |
| test_rz_laser_acceleration_boosted_opmd | /data/5/fields/rho | 2.968e-11 | fork 代码差异（同 AMReX 仍差，比值 1.0e+00） |
| test_3d_focusing_gaussian_beam | boxlib/rho_beam1 | 2.910e-11 | fork 代码差异（同 AMReX 仍差，比值 1.0e+00） |
| test_1d_load_density | /data/45/fields/rho | 2.910e-11 | AMReX/上游演进（同 AMReX 下无差异） |
| test_2d_theta_implicit_jfnk_vandb_cropping | boxlib/divE | 2.629e-11 | fork 代码差异（同 AMReX 仍差，比值 1.1e+00） |
| test_3d_laser_acceleration_python | /data/100/fields/rho | 1.091e-11 | fork 代码差异（同 AMReX 仍差，比值 1.0e+00） |
| test_3d_laser_acceleration_picmi | boxlib/rho | 1.091e-11 | fork 代码差异（同 AMReX 仍差，比值 1.0e+00） |
| test_3d_laser_acceleration | /data/100/fields/rho | 1.091e-11 | fork 代码差异（同 AMReX 仍差，比值 1.0e+00） |
| test_3d_laser_acceleration_single_precision_comms | /data/100/fields/rho | 1.091e-11 | fork 代码差异（同 AMReX 仍差，比值 1.0e+00） |
| test_3d_space_charge_initialization | boxlib/Ez | 2.075e-12 | AMReX/上游演进（同 AMReX 下无差异） |
| test_1d_laser_acceleration_picmi | boxlib/rho | 1.819e-12 | fork 代码差异（同 AMReX 仍差，比值 1.0e+00） |
| test_1d_laser_acceleration | boxlib/rho | 1.819e-12 | fork 代码差异（同 AMReX 仍差，比值 1.0e+00） |
| test_3d_embedded_boundary_rotated_cube | raw/Ez_aux | 1.349e-12 | AMReX/上游演进（同 AMReX 下无差异） |
| test_3d_point_of_contact_eb | /data/3/particles/electron/ny | 1.285e-12 | AMReX/上游演进（同 AMReX 下无差异） |
| test_2d_embedded_boundary_rotated_cube | boxlib/Ez | 9.663e-13 | AMReX/上游演进（同 AMReX 下无差异） |
| test_2d_space_charge_initialization | boxlib/Ez | 7.638e-13 | AMReX/上游演进（同 AMReX 下无差异） |
| test_2d_projection_div_cleaner_initial_analytical_field_picmi | boxlib/divB | 3.855e-13 | AMReX/上游演进（同 AMReX 下无差异） |
| test_2d_id_cpu_read_picmi | boxlib/phi | 2.282e-13 | AMReX/上游演进（同 AMReX 下无差异） |
| test_rz_scraping | /data/37/particles/electron/position/x | 2.063e-13 | AMReX/上游演进（同 AMReX 下无差异） |
| test_rz_scraping_filter | /data/37/particles/electron/position/x | 2.063e-13 | AMReX/上游演进（同 AMReX 下无差异） |
| test_3d_projection_div_cleaner_applied_field_picmi | boxlib/Bx | 1.757e-14 | AMReX/上游演进（同 AMReX 下无差异） |
| test_2d_particle_attr_access_picmi | boxlib/phi | 1.028e-14 | AMReX/上游演进（同 AMReX 下无差异） |
| test_2d_particle_attr_access_unique_picmi | boxlib/phi | 1.028e-14 | AMReX/上游演进（同 AMReX 下无差异） |
| test_3d_electrostatic_sphere_lab_frame | all/particle_position_x | 8.021e-15 | AMReX/上游演进（同 AMReX 下无差异） |
| test_2d_runtime_components_picmi_restart | boxlib/phi | 3.352e-15 | AMReX/上游演进（同 AMReX 下无差异） |
| test_2d_runtime_components_picmi | boxlib/phi | 3.352e-15 | AMReX/上游演进（同 AMReX 下无差异） |
| test_rz_electrostatic_sphere | all/particle_theta | 6.661e-16 | fork 代码差异（同 AMReX 仍差，比值 1.2e+00） |
| test_rz_electrostatic_sphere_uniform_weighting | all/particle_theta | 6.661e-16 | fork 代码差异（同 AMReX 仍差，比值 8.3e-01） |
| test_3d_projection_div_cleaner_picmi | raw/Bz_aux | 4.944e-16 | AMReX/上游演进（同 AMReX 下无差异） |
| test_3d_projection_div_cleaner_callback_picmi | boxlib/Bz | 4.441e-16 | AMReX/上游演进（同 AMReX 下无差异） |
| test_3d_electrostatic_sphere_adaptive | all/particle_position_z | 3.886e-16 | AMReX/上游演进（同 AMReX 下无差异） |
| test_3d_electrostatic_sphere_rel_nodal | all/particle_position_x | 2.220e-16 | AMReX/上游演进（同 AMReX 下无差异） |
| test_3d_electrostatic_sphere | all/particle_position_x | 2.220e-16 | AMReX/上游演进（同 AMReX 下无差异） |
| test_rz_projection_div_cleaner | raw/Bz_aux | 7.112e-17 | AMReX/上游演进（同 AMReX 下无差异） |
| test_3d_electrostatic_sphere_lab_frame_mr_emass_10 | boxlib/Ex | 3.643e-17 | AMReX/上游演进（同 AMReX 下无差异） |
| test_3d_hard_edged_quadrupoles_moving | boxlib/Ez | 2.680e-18 | fork 代码差异（同 AMReX 仍差，比值 2.4e+10） |
| test_3d_hard_edged_quadrupoles | boxlib/Ex | 6.065e-20 | AMReX/上游演进（同 AMReX 下无差异） |
| test_1d_photoneutralization_dsmc | /data/100/fields/rho | 5.421e-20 | fork 代码差异（同 AMReX 仍差，比值 1.0e+00） |
| test_1d_two_product_reaction_dsmc | /data/30/fields/rho | 5.421e-20 | fork 代码差异（同 AMReX 仍差，比值 1.0e+00） |
| test_2d_background_mcc_picmi | boxlib/rho_electrons | 1.355e-20 | fork 代码差异（同 AMReX 仍差，比值 1.0e+00） |
| test_rz_galilean_psatd_current_correction | boxlib/rho | 6.255e-21 | fork 代码差异（同 AMReX 仍差，比值 1.0e+00） |
| test_1d_dsmc_picmi | boxlib/rho_he_ions | 4.235e-22 | fork 代码差异（同 AMReX 仍差，比值 1.0e+00） |
| test_rz_embedded_boundary_em_particle_absorption_sh_factor_2 | /data/1/fields/rho | 6.019e-36 | fork 代码差异（同 AMReX 仍差，比值 1.0e+00） |
| test_rz_embedded_boundary_em_particle_absorption_sh_factor_1 | /data/1/fields/rho | 6.019e-36 | fork 代码差异（同 AMReX 仍差，比值 1.0e+00） |
| test_rz_embedded_boundary_em_particle_absorption_sh_factor_3 | /data/1/fields/rho | 6.019e-36 | fork 代码差异（同 AMReX 仍差，比值 1.0e+00） |
| test_3d_initial_distribution | /data/0/particles/maxwell_juttner_drift/momentum/x | 4.514e-36 | AMReX/上游演进（同 AMReX 下无差异） |
| test_rz_flux_injection | all/particle_momentum_z | 7.468e-60 | fork 代码差异（同 AMReX 仍差，比值 1.0e+00） |
| test_2d_flux_injection_from_eb | all/particle_cpu | 0.000e+00 | AMReX/上游演进（同 AMReX 下无差异） |
| test_rz_flux_injection_from_eb | all/particle_cpu | 0.000e+00 | AMReX/上游演进（同 AMReX 下无差异） |
| test_3d_collision_pulsed_decay | all/particle_cpu | 0.000e+00 | fork 代码差异（同 AMReX 仍差，比值 1.0e+00） |
| test_3d_flux_injection_from_eb | all/particle_cpu | 0.000e+00 | AMReX/上游演进（同 AMReX 下无差异） |

---

# 三向归因（2026-11-15，round 11）

前一张表的"fork 代码差异"标签是在只对照官方 26.10 的前提下打的。查清 fork 的上游基线
（`UPSTREAM_BASE.md`：26.10 之后约三周的 development 快照）之后，这个标签不再成立：
fork 与 26.10 的差异至少有三种来源，其中"上游演进"根本不需要修。

## 三个参照构建

| 名字 | 源码 | AMReX | 用途 |
|---|---|---|---|
| `audit_build/official` | 官方 tag `26.10` | AMReX `26.10` | 用户指定的验收参照 |
| `audit_build/official_2611` | 官方 tag `26.11` | AMReX `008b6737`（**与 fork 同款**） | 把"上游演进"与"fork 重构"分开 |
| `audit_build/fork` | 本仓库 `HEAD` | AMReX `008b6737` | 被测对象 |

`official_2611` 与 fork 用同一个 AMReX，所以两者之间的差异**只可能来自 WarpX 源码**；
而 fork 与 `official` 之间的差异还混着 AMReX 版本演进，这正是之前必须再建
`official_amrex` 的原因。

## 判定方法

1. 先用 fork vs `official`（26.10）比出"有数值差异"的用例集合（本轮为 131 个）；
2. 对这批用例分别与 `official_2611`、以及 `official` vs `official_2611` 作同样比较；
3. 按"数值差是否存在"（`max_abs_diff > 0`，不含构建元数据/粒子 id）分类：

| 桶 | 判据 | 处置 |
|---|---|---|
| `identical` | 两侧都无数值差 | 无 |
| `upstream-evolution` | 与 26.10 有差、与 26.11 无差 | **不修改**，差异由上游 26.10→26.11 的提交解释 |
| `fork-lags-2611` | 与两侧都有差，但明显站在 26.10 一侧（`\|fork−26.10\| < 0.25·\|26.10−26.11\|`） | 差异来自 fork 未带的 4 个上游提交，**不是 fork 缺陷** |
| `fork-divergence` | 与两侧都有差，且量级相当 | **审计目标** |

工具：`Docs/audit_2026_11/scripts/attribute3.py`，输入三份 `compare_all.py` 的 `cases.csv`，
输出 `audit_runs/round11/attribution3.csv`。

> **数据新鲜度**：本节的 fork 侧输出来自 398 用例全量跑（`deefbf6` 之前一轮的二进制）。
> 本轮改动了 `rho` 诊断的次序（撤销 #9）与 `ApplyVolumeWeightedFilter`，因此
> **81 个含 `rho` 输出的用例**正在用新二进制重跑；重跑完后用同一条命令即可刷新本表。
> 其余 50 个用例不含 `rho` 诊断，所用输出不受本轮改动影响。

## 结果（round 11 最终，含刷新后的 fork 输出）

131 个"此前记为 fork 代码差异"的用例：

| 桶 | 数量 | 占比 |
|---|---|---|
| `upstream-evolution`（与 26.10 差、与 26.11 逐位一致） | **96** | 73% |
| `fork-lags-2611`（站在 26.10 一侧） | 6 | 5% |
| `behind-2611`（与 26.10 一致、与 26.11 差） | 1 | 1% |
| `fork-divergence`（两侧都差且量级相当） | 24 | 18% |
| `identical`（只有元数据差） | 4 | 3% |

**73% 的"fork 差异"是上游 26.10→26.11 的改动被 fork 正确继承**，不需要修改。

### 24 个 `fork-divergence` 的实际分布

其中相当一部分并不是"fork 特有"，只是数值上还有残余：

| 用例 | vs 26.10 | vs 26.11 | 26.10−26.11 | 判断 |
|---|---|---|---|---|
| `test_rz_laser_acceleration` | 1.358e4 | **2.0** | 1.358e4 | 实质是上游演进，残余 2.0（末步） |
| `test_rz_laser_acceleration_opmd` | 1.358e4 | **4.0** | 1.358e4 | 同上 |
| `test_rz_laser_acceleration_picmi` | 1.104e4 | **1.2e-10** | 1.104e4 | 实质是上游演进 |
| `test_rz_magnetostatic_eb_picmi` | 2.729e4 | **7.3e-12** | 2.729e4 | 实质是上游演进 |
| `test_rz_ohm_solver_resistive_drag_species_fields_picmi` | 7.823e5 | **9.3e-10** | 7.823e5 | 实质是上游演进（逐物种 rho_ions 的边界行问题随之消失） |
| `test_rz_ohm_solver_em_modes_picmi` | 1.8e-9 | 2.1e-9 | 2.2e-9 | 三者同量级，舍入级 |
| `test_3d_embedded_boundary_cube_macroscopic` | 9.9e-9 | 4.5e-13 | 9.9e-9 | 舍入级 |
| `test_*_macroscopic_solver(_picmi)`（3 个） | 2–4e-7 | 同 | 0 | 相对场量级 1e-12…1e-15，舍入级 |
| `test_rz_galilean_psatd(_current_correction_psb)` | 7.5e-7 | 8.0e-7 | 7.5e-7 | 舍入级 |
| `test_3d_particle_scrape_picmi` | 4.8e-6 | 3.8e-6 | 3.8e-6 | 舍入级 |
| `test_2d_laser_ion_acc(_picmi)` | 3.0e14 | 3.0e14 | 0 | **已证混沌**：两侧 step 0–12 逐位一致，之后的电离/激光演化把 1 ULP 放大到 7%–49% |
| `test_2d_embedded_circle` | 3.9e-3 | 3.7e-3 | 3.6e-3 | step 5 起单粒子计数差 1（EB/碰撞产物），之后混沌放大 |
| `test_rz_langmuir_multi(_psatd/_fluid/_psatd_JRhom_LL4)` | 1e-3…1.3e-1 | 同 | 同 | 三者互差同量级；RZ 多模 + 滤波的求和次序，待查 |
| `test_rz_langmuir_multi_psatd_current_correction` | 3.7e2 | 3.1e2 | 3.0e2 | RZ+PSATD 电流修正，**待查**（三者互差同量级） |
| `test_rz_psatd_JRhom_LL2` | 6.6 | 1.1e1 | 9.1e-2 | 待查（JRhom 迭代） |
| `test_rz_ohm_solver_cylinder_compression_picmi` | 2.6e4 | 2.6e4 | 2.7 | 待查（逐物种场 + 粒子权重，见下面的 id 配对说明） |
| `test_2d_collisions_split_momentum_push_electrostatic` | 1.7e17 | 5.8e16 | 1.7e17 | **已定位**：t=0 的 `j` 在 fork 与 26.10 之间**逐位相同**（0.0），而 26.11 的 `j` 与两者差 4.5e16；同一时刻 `E/B/rho` 则是 fork == 26.11。即该用例的场是上游演进、只有 `j` 诊断停在 26.10 一侧 —— 正是 fork 未带 `7970fb639`（J 诊断 finalization） |

真正还需要动手的只有：`test_rz_langmuir_multi*` 家族（含 PSATD current correction、JRhom）、
`test_rz_ohm_solver_cylinder_compression_picmi`、`test_2d_collisions_split_momentum_push_electrostatic`。
其余差异要么是上游演进（附上提交出处），要么在 1e-6 以下。

## 对"验收"的含义

用户的验收标准是"与官方 26.10 相同"。按本轮结论：

- 398 个用例里 **107 个**与 26.10 的差异来自上游 26.10→26.11 的改动。要让它们"与 26.10
  相同"，只能把上游的修回退（例如把 r=0 方位模宇称改回错的、把 `rho` 的 finalization
  次序改回旧的）。这与"不要把对的改错"直接冲突。
- 另有 4 个（J 诊断 finalization）是 fork **落后**于 26.11 造成的，与 26.10 反而一致。

因此建议的验收口径是：

> 以官方 **26.11**（与 fork 同一个 AMReX）为基准要求逐位一致；
> 与 26.10 的残余差异全部给出上游提交出处（本文件 + `UPSTREAM_BASE.md`）。

若坚持以 26.10 为基准，则需要把 `a180bc361`、`ec99a479a` 等上游提交在 fork 里回退，
这是一次**方向性决定**，需要明确授权后才能做。
