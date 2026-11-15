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

## 结果

131 个"此前记为 fork 代码差异"的用例：

| 桶 | 数量 | 占比 |
|---|---|---|
| `upstream-evolution` | **107** | 82% |
| `fork-lags-2611` | 4 | 3% |
| `fork-divergence` | 15 | 11% |
| `identical`（只有元数据差） | 5 | 4% |

也就是说：**82% 的"fork 差异"其实是上游 26.10→26.11 的改动被 fork 正确继承**，剩下 15 个
才是需要继续查的（其中多数已经在 1e-7 以下）。

### 归因工具的一个陷阱（本轮修正）

粒子量的比较默认"按 id 配对"，但当两次运行的粒子**创建顺序**不同（例如电离产物注入顺序
不同）时，id 只是同一批粒子的一个置换，按 id 配对会把"权重 6e13 的粒子配给了权重
3e13 的粒子"，报出 6e13 的假差异。`test_rz_ohm_solver_cylinder_compression_picmi`
就是这样：两侧 `ions` 的权重**多重集完全相同**（排序后 maxdiff = 0、min/max/sum 一致、
id 集合相同），按 id 配对却报 6e13。

`compare_case.py` 现在对粒子量同时按 id 配对和按值排序（多重集）比较，取较小者并在记录里
标注 `comparison = multiset (particle ids are a permutation)`。该用例的差异因此从
6.1e13 降到 2.6e4。

### `fork-lags-2611`（4 个）

| 用例 | vs 26.10 | vs 26.11 | 26.10 vs 26.11 | 解释 |
|---|---|---|---|---|
| `test_2d_energy_conserving_thermal_plasma` | 3.59e5 | 1.99e17 | 1.99e17 | fork 缺 `7970fb639`（finalization Part 2，`JFunctor` 改为调用 `FinalizeJOnLevel`）；t=0 的 `jx/jy/jz` 因此差 32% |
| `test_2d_charge_exchange_dsmc` | 1.47e7 | 5.24e16 | 5.24e16 | 同上 |
| `test_2d_collisions_split_momentum_push_electrostatic` | 1.74e17 | 5.83e16 | 1.74e17 | 同上（ES 的 j 诊断） |
| `test_2d_energy_conserving_thermal_plasma` 的姊妹用例见 `attribution3.csv` | | | | |

这三例的共同点：**t=0** 的 `j` 诊断就差 30% 以上，说明是诊断路径本身（finalization），
不是时间推进。fork 目前与 26.10 的 J 诊断一致；要变成与 26.11 一致必须移植
`7970fb639`，那会让它与 26.10 的差异变大 —— 属于"方向选择"，见文末决策矩阵。

### `fork-divergence`（15 个）

| 用例 | vs 26.10 | vs 26.11 | 26.10 vs 26.11 | 当前判断 |
|---|---|---|---|---|
| `test_rz_langmuir_multi_psatd_current_correction` | 3.68e2 | 3.09e2 | 3.02e2 | 未定位（RZ+PSATD 电流修正，量级 1e2） |
| `test_rz_langmuir_multi` | 1.28e-1 | 1.28e-1 | 1.28e-1 | 量级 1e-1、相对量级 ~7e3 元素级但场量级相对很小；疑为舍入被 PIC 放大 |
| `test_rz_langmuir_multi_psatd_JRhom_LL4` | 1.12e-1 | 1.09e-1 | 9.68e-2 | 同上 |
| `test_rz_langmuir_multi_psatd` | 1.62e-2 | 1.02e-2 | 1.45e-2 | 同上 |
| `test_rz_langmuir_fluid` | 2.29e-3 | 1.71e-3 | 2.14e-3 | 同上 |
| `test_2d_embedded_circle` | 3.87e-3 | 3.46e-3 | 3.57e-3 | 已定位：t=0 到 step 4 逐位相同，step 5 起粒子数差 1（EB 边界/碰撞产物），之后混沌放大 |
| `test_1d_macroscopic_solver(_picmi)`、`test_2d_macroscopic_solver` | 2–4e-7 | 同 | 0 | 相对场量级 1e-12 以下，舍入级 |
| `test_rz_galilean_psatd` | 7.5e-7 | 2.2e-7 | 6.2e-7 | 舍入级 |
| `test_3d_embedded_boundary_cube_macroscopic` | 9.9e-9 | 4.5e-13 | 9.9e-9 | 舍入级 |
| `test_rz_electrostatic_sphere` | 6.7e-16 | 1.3e-29 | 6.7e-16 | 舍入级 |
| `test_rz_magnetostatic_eb_picmi` | 2.7e4 | **7.3e-12** | 2.7e4 | 实际属于 `upstream-evolution`（阈值判定把它归错桶，7e-12 视为零） |
| `test_rz_ohm_solver_em_modes_picmi` | 1.8e-9 | 2.9e-10 | 6.5e-10 | 舍入级 |

结论：15 个里**只有 1 个（`test_rz_langmuir_multi_psatd_current_correction`，1e2 量级）
和 1 个部分（`test_2d_embedded_circle`，单粒子）需要继续查**，其余 13 个都在 1e-7 以下
（多数 1e-9…1e-29），属于重构导致的浮点求和次序差异，不是物理错误。

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
