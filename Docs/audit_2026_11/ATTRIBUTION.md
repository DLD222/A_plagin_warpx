# 差异归因（fork vs official 26.10，隔离对照：official + fork 同款 AMReX）

- 参与归因的数值差异用例：132
- 归因到 AMReX/上游演进（同 AMReX 构建下数值一致）：45
- 归因到 fork 代码（同 AMReX 构建下仍差）：79
- 未归因（official_amrex 未运行该用例，多为 >10 分钟的重算例）：8

| case | 最差字段 | max abs (vs 官方) | 归因 |
|---|---|---|---|
| test_2d_collisions_split_momentum_push_electrostatic | /data/2000/fields/j/z | 1.744e+17 | fork 代码差异（同 AMReX 仍差，比值 3.3e-01） |
| test_2d_laser_ion_acc | /data/100/meshes/data | 2.990e+14 | fork 代码差异（同 AMReX 仍差，比值 1.0e+00） |
| test_2d_laser_ion_acc_picmi | /data/100/fields/j/x | 8.174e+13 | fork 代码差异（同 AMReX 仍差，比值 1.0e+00） |
| test_1d_laser_acceleration_fluid_boosted | boxlib/jz | 1.724e+12 | not-attributed (official_amrex 未运行) |
| test_2d_field_probe | boxlib/Ex | 1.669e+10 | AMReX/上游演进（同 AMReX 下无差异） |
| test_2d_theta_implicit_jfnk_vandb | boxlib/divE | 3.840e+07 | fork 代码差异（同 AMReX 仍差，比值 9.2e-01） |
| test_2d_theta_implicit_jfnk_vandb_picmi | boxlib/divE | 3.265e+07 | fork 代码差异（同 AMReX 仍差，比值 8.4e-01） |
| test_2d_theta_implicit_jfnk_vandb_filtered | boxlib/divE | 1.581e+07 | fork 代码差异（同 AMReX 仍差，比值 7.9e-01） |
| test_2d_charge_exchange_dsmc | /data/2000/fields/j/x | 1.465e+07 | not-attributed (official_amrex 未运行) |
| test_3d_ion_beam_extraction | /data/0/fields/E/z | 1.136e+07 | fork 代码差异（同 AMReX 仍差，比值 4.9e-24） |
| test_rz_ohm_solver_resistive_drag_species_fields_picmi | /data/1/fields/current_fp_ions[dir=z] | 7.823e+05 | fork 代码差异（同 AMReX 仍差，比值 1.0e+00） |
| test_2d_theta_implicit_symmetry | boxlib/divE | 6.213e+05 | not-attributed (official_amrex 未运行) |
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
| test_3d_electrostatic_sphere_eb | /data/0/fields/E/z | 1.668e-05 | not-attributed (official_amrex 未运行) |
| test_3d_electrostatic_sphere_eb_picmi | /data/0/fields/E/z | 1.668e-05 | AMReX/上游演进（同 AMReX 下无差异） |
| test_rz_electrostatic_sphere_eb_mr | /data/0/fields/E/r | 8.766e-06 | not-attributed (official_amrex 未运行) |
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
| test_rz_particle_boundary_interaction_reflecting | /data/0/fields/E/r | 1.403e-08 | not-attributed (official_amrex 未运行) |
| test_3d_embedded_boundary_cube_macroscopic | boxlib/Ex | 9.906e-09 | fork 代码差异（同 AMReX 仍差，比值 4.6e-05） |
| test_3d_embedded_boundary_cube | boxlib/Ez | 9.906e-09 | AMReX/上游演进（同 AMReX 下无差异） |
| test_2d_background_mcc | all/particle_position_x | 3.164e-09 | AMReX/上游演进（同 AMReX 下无差异） |
| test_2d_particle_thermal_boundary | boxlib/rho | 2.546e-09 | not-attributed (official_amrex 未运行) |
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
| test_3d_point_of_contact_eb | /data/3/particles/electron/ny | 1.285e-12 | not-attributed (official_amrex 未运行) |
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
