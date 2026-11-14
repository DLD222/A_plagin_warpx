# Numeric-difference analysis (fork vs official 26.10)

- cases with numeric differences: 129
- cases with only log/text differences: 17
- cases with only metadata/used-inputs differences: 215

## Cases with numeric differences (worst field per case)

| case | worst field | n differ | max abs | max rel | shape-mismatch |
|---|---|---|---|---|---|
| test_2d_energy_conserving_thermal_plasma | boxlib/jy | 64 | 1.993e+17 | 3.902e+02 | 0 |
| test_1d_energy_conserving_thermal_plasma | boxlib/jz | 2 | 1.748e+17 | 4.734e-01 | 0 |
| test_2d_collisions_split_momentum_push_electrostatic | /data/2000/fields/j/z | 256 | 1.744e+17 | 6.899e+01 | 0 |
| test_2d_laser_ion_acc | /data/100/meshes/data | 544 | 2.990e+14 | inf | 30 |
| test_2d_laser_ion_acc_picmi | /data/100/fields/j/x | 122 | 8.174e+13 | 4.041e-01 | 30 |
| test_1d_laser_acceleration_fluid_boosted | boxlib/jz | 1 | 1.724e+12 | 4.271e-02 | 0 |
| test_2d_field_probe | boxlib/Ex | 37116 | 1.669e+10 | inf | 0 |
| test_2d_collision_xz | boxlib/jx | 40 | 1.147e+08 | 2.614e+00 | 0 |
| test_3d_collision_xyz | boxlib/jx | 416 | 8.878e+07 | 5.677e+01 | 0 |
| test_2d_collision_xz_global_debye | boxlib/jz | 38 | 8.644e+07 | 2.464e+01 | 0 |
| test_2d_theta_implicit_jfnk_vandb | boxlib/divE | 1599 | 3.840e+07 | 8.870e-11 | 0 |
| test_2d_theta_implicit_jfnk_vandb_picmi | boxlib/divE | 1600 | 3.265e+07 | 1.144e-10 | 0 |
| test_2d_theta_implicit_jfnk_vandb_filtered | boxlib/divE | 1596 | 1.581e+07 | 5.169e-11 | 0 |
| test_3d_ion_beam_extraction | /data/0/fields/E/z | 5516 | 1.136e+07 | 5.205e+02 | 30 |
| test_rz_ohm_solver_resistive_drag_species_fields_picmi | /data/1/fields/current_fp_ions[dir=z] | 8 | 7.823e+05 | 2.470e-01 | 0 |
| test_2d_theta_implicit_symmetry | boxlib/divE | 256 | 6.213e+05 | 2.193e-10 | 0 |
| test_3d_acceleration_psatd_restart | boxlib/jz | 15680 | 2.408e+05 | 8.927e-05 | 0 |
| test_3d_acceleration_psatd | boxlib/jz | 15680 | 2.408e+05 | 8.929e-05 | 0 |
| test_3d_acceleration_psatd_time_avg_restart | boxlib/jz | 15680 | 2.159e+05 | 3.349e-04 | 0 |
| test_3d_acceleration_psatd_time_avg | boxlib/jz | 15680 | 2.159e+05 | 3.349e-04 | 0 |
| test_2d_ohm_solver_electron_energy_qei_picmi | /data/72/fields/j/z | 128 | 1.608e+05 | 1.396e+02 | 0 |
| test_3d_theta_implicit_jfnk_direct | boxlib/divE | 3045 | 1.311e+05 | 1.195e-11 | 0 |
| test_rz_load_density | /data/15/fields/rho | 120 | 1.244e+05 | 1.259e+00 | 0 |
| test_1d_theta_implicit_planar_pinch | boxlib/divE | 220 | 8.376e+04 | 6.641e-05 | 0 |
| test_2d_darwin_solver_em_modes_es_picmi | boxlib/Ez | 2048 | 2.923e+04 | 1.116e+03 | 0 |
| test_rz_magnetostatic_eb_picmi | boxlib/Er | 13312 | 2.729e+04 | 4.585e-02 | 0 |
| test_2d_repelling_particles | boxlib/divE | 16383 | 2.662e+04 | 8.201e-07 | 0 |
| test_2d_ohm_solver_resistive_drag_per_species_picmi | /data/50/fields/Te | 256 | 2.275e+04 | 3.834e-03 | 0 |
| test_2d_ohm_solver_electron_energy_joule_picmi | /data/50/fields/Te | 256 | 1.545e+04 | 2.606e-03 | 0 |
| test_2d_ohm_solver_resistive_drag_picmi | /data/50/fields/Te | 256 | 1.523e+04 | 2.569e-03 | 0 |
| test_rz_laser_acceleration_opmd | /data/20/fields/rho | 2229 | 1.358e+04 | 4.118e+02 | 0 |
| test_rz_laser_acceleration | boxlib/rho | 565 | 1.358e+04 | 9.870e-01 | 0 |
| test_rz_laser_acceleration_picmi | /data/10/fields/rho | 394 | 1.104e+04 | 1.173e+01 | 0 |
| test_3d_magnetostatic_eb | boxlib/jz | 48896 | 1.018e+04 | 8.524e-01 | 0 |
| test_3d_magnetostatic_eb_picmi | boxlib/jz | 41472 | 8.262e+03 | 5.191e-01 | 0 |
| test_3d_beam_beam_collision | /data/90/fields/E/y | 262138 | 3.006e+03 | 5.942e-06 | 0 |
| test_1d_resample_velocity_coincidence_thinning | boxlib/jx | 9 | 2.667e+03 | 1.121e+01 | 0 |
| test_1d_resample_velocity_coincidence_thinning_cartesian | boxlib/jx | 16 | 2.617e+03 | 4.878e+00 | 0 |
| test_1d_darwin_solver_em_modes_picmi | boxlib/Ey | 256 | 1.019e+03 | 2.754e+00 | 0 |
| test_2d_ohm_solver_electron_energy_vacuum_picmi | /data/70/fields/Te | 512 | 6.075e+02 | 1.392e-03 | 0 |
| test_rz_langmuir_multi_psatd_current_correction | boxlib/divE | 8190 | 3.676e+02 | 5.382e+02 | 0 |
| test_2d_prev_positions_picmi | boxlib/jx | 228 | 1.789e+02 | 1.000e+00 | 0 |
| test_3d_nodal_electrostatic_solver | boxlib/Ey | 2082195 | 1.775e+02 | 1.512e-11 | 0 |
| test_2d_ohm_solver_electron_energy_adiabat_picmi | /data/36/fields/j/z | 256 | 2.763e+01 | 6.379e-01 | 0 |
| test_3d_laser_acceleration_btd | boxlib/jz | 837 | 1.000e+01 | 6.543e-13 | 0 |
| test_1d_laser_injection_implicit | boxlib/Ex | 213 | 8.487e+00 | 5.734e-09 | 0 |
| test_1d_pierce_diode | /data/5000/fields/j/z | 4 | 7.274e+00 | 1.877e-01 | 0 |
| test_rz_psatd_JRhom_LL2 | boxlib/jz | 1975 | 6.609e+00 | 9.616e-09 | 0 |
| test_2d_laser_injection_implicit | boxlib/Ex | 165890 | 4.150e+00 | 1.966e-08 | 0 |
| test_2d_laser_acceleration_boosted | boxlib/jz | 64 | 5.293e-01 | 1.378e-10 | 0 |
| test_2d_ohm_solver_magnetic_reconnection_picmi | boxlib/phi | 16384 | 1.460e-01 | 1.693e-01 | 0 |
| test_rz_langmuir_multi | boxlib/jr | 1551 | 1.276e-01 | 1.731e-09 | 0 |
| test_rz_langmuir_multi_psatd_JRhom_LL4 | boxlib/jr | 7371 | 1.116e-01 | 7.750e-08 | 0 |
| test_rz_langmuir_multi_psatd | boxlib/jz | 7360 | 1.617e-02 | 7.814e-08 | 0 |
| test_rz_pml_psatd | boxlib/rho | 2 | 1.337e-02 | 2.000e+00 | 0 |
| test_rz_langmuir_fluid | boxlib/jr | 8160 | 2.289e-03 | 1.046e-08 | 0 |
| test_3d_effective_potential_electrostatic_picmi | /data/0/fields/j/x | 41230 | 2.040e-03 | 1.223e+04 | 0 |
| test_3d_electrostatic_sphere_eb_mixed_bc | boxlib/Ey | 238216 | 7.454e-05 | 2.036e-02 | 0 |
| test_3d_relativistic_space_charge_initialization | boxlib/Ey | 2097152 | 5.643e-05 | 3.420e+03 | 0 |
| test_3d_electrostatic_sphere_eb | /data/0/fields/E/z | 261640 | 1.668e-05 | 7.011e-06 | 0 |
| test_3d_electrostatic_sphere_eb_picmi | /data/0/fields/E/z | 261640 | 1.668e-05 | 7.011e-06 | 0 |
| test_rz_electrostatic_sphere_eb | boxlib/Er | 3392 | 8.766e-06 | 2.438e-06 | 0 |
| test_rz_electrostatic_sphere_eb_mr | /data/0/fields/E/r | 3392 | 8.766e-06 | 2.438e-06 | 0 |
| test_2d_particle_reflection_picmi | boxlib/Ez | 4096 | 4.132e-06 | 3.603e-04 | 0 |
| test_3d_particle_scrape_picmi | boxlib/Ex | 7157 | 3.815e-06 | 1.280e-12 | 0 |
| test_1d_collision_z_inverse_Bremsstrahlung | electrons/particle_position_x | 102400 | 1.000e-06 | 4.708e+04 | 21 |
| test_2d_dirichlet_bc | boxlib/phi | 512 | 9.480e-07 | 4.715e-08 | 0 |
| test_2d_dirichlet_bc_picmi | boxlib/phi | 512 | 9.480e-07 | 4.715e-08 | 0 |
| test_rz_galilean_psatd_current_correction_psb | boxlib/divE | 8192 | 7.790e-07 | 2.377e-04 | 0 |
| test_rz_galilean_psatd | boxlib/divE | 8192 | 7.478e-07 | 3.009e-04 | 0 |
| test_1d_macroscopic_solver | boxlib/Ex | 121 | 4.377e-07 | 3.827e-14 | 0 |
| test_1d_macroscopic_solver_picmi | boxlib/Ex | 121 | 4.377e-07 | 3.827e-14 | 0 |
| test_2d_macroscopic_solver | boxlib/Ex | 3780 | 2.161e-07 | 6.744e-13 | 0 |
| test_rz_spacecraft_charging_picmi | /data/80/fields/E/z | 3178 | 1.037e-07 | 5.186e-07 | 0 |
| test_3d_hard_edged_quadrupoles_moving | boxlib/jz | 6 | 6.417e-08 | 1.000e+00 | 0 |
| test_rz_secondary_ion_emission_picmi | /data/2/fields/E/r | 4072 | 3.029e-08 | 8.564e+03 | 0 |
| test_rz_particle_boundary_interaction_picmi | /data/0/fields/E/r | 4072 | 1.403e-08 | 3.832e+01 | 0 |
| test_rz_particle_boundary_interaction_reflecting | /data/0/fields/E/r | 4072 | 1.403e-08 | 3.832e+01 | 0 |
| test_3d_embedded_boundary_cube_macroscopic | boxlib/Ex | 32768 | 9.906e-09 | 1.265e+03 | 0 |
| test_3d_embedded_boundary_cube | boxlib/Ez | 32768 | 9.906e-09 | 1.265e+03 | 0 |
| test_2d_particle_thermal_boundary | boxlib/rho | 156 | 2.546e-09 | 2.094e-01 | 0 |
| test_rz_ohm_solver_em_modes_picmi | /data/90/fields/E/r | 8190 | 1.773e-09 | 8.821e-10 | 0 |
| test_2d_embedded_boundary_cube | boxlib/Ex | 482 | 2.582e-10 | 7.320e+02 | 0 |
| test_3d_load_density | /data/50/fields/rho | 3236 | 2.328e-10 | 5.423e-16 | 0 |
| test_2d_load_density | /data/22/fields/rho | 35 | 1.164e-10 | 4.183e-16 | 0 |
| test_rz_ohm_solver_cylinder_compression_picmi | boxlib/Tt_ions | 168 | 5.821e-11 | 6.295e-16 | 0 |
| test_rz_laser_acceleration_boosted_opmd | /data/5/fields/rho | 24505 | 2.968e-11 | 1.128e+04 | 0 |
| test_1d_load_density | /data/45/fields/rho | 2 | 2.910e-11 | 2.076e-16 | 0 |
| test_3d_focusing_gaussian_beam | boxlib/rho_beam1 | 459 | 2.910e-11 | 3.196e-15 | 0 |
| test_2d_theta_implicit_jfnk_vandb_cropping | boxlib/divE | 250 | 2.629e-11 | 3.667e+00 | 0 |
| test_3d_laser_acceleration | /data/100/fields/rho | 921 | 1.091e-11 | 4.671e-16 | 0 |
| test_3d_laser_acceleration_picmi | boxlib/rho | 921 | 1.091e-11 | 4.671e-16 | 0 |
| test_3d_laser_acceleration_python | /data/100/fields/rho | 921 | 1.091e-11 | 4.671e-16 | 0 |
| test_3d_laser_acceleration_single_precision_comms | /data/100/fields/rho | 921 | 1.091e-11 | 4.671e-16 | 0 |
| test_3d_space_charge_initialization | boxlib/Ez | 2096974 | 2.075e-12 | 1.635e-09 | 0 |
| test_1d_laser_acceleration | boxlib/rho | 1 | 1.819e-12 | 1.602e-16 | 0 |
| test_1d_laser_acceleration_picmi | boxlib/rho | 1 | 1.819e-12 | 1.602e-16 | 0 |
| test_3d_embedded_boundary_rotated_cube | raw/Ez_aux | 258316 | 1.349e-12 | 1.346e-07 | 0 |
| test_3d_point_of_contact_eb | /data/3/particles/electron/ny | 1 | 1.285e-12 | 9.921e-12 | 0 |
| test_2d_embedded_boundary_rotated_cube | boxlib/Ez | 476 | 9.663e-13 | 3.390e-12 | 0 |
| test_2d_space_charge_initialization | boxlib/Ez | 16086 | 7.638e-13 | 7.639e-12 | 0 |
| test_2d_projection_div_cleaner_initial_analytical_field_picmi | boxlib/divB | 15079 | 3.855e-13 | 3.567e+287 | 0 |
| test_2d_id_cpu_read_picmi | boxlib/phi | 4096 | 2.282e-13 | 5.021e-07 | 0 |
| test_rz_scraping | /data/37/particles/electron/position/x | 3840 | 2.063e-13 | 2.064e-12 | 0 |
| test_rz_scraping_filter | /data/37/particles/electron/position/x | 768 | 2.063e-13 | 2.064e-12 | 0 |
| test_3d_projection_div_cleaner_applied_field_picmi | boxlib/Bx | 32707 | 1.757e-14 | 1.188e-10 | 0 |
| test_2d_particle_attr_access_picmi | boxlib/phi | 4096 | 1.028e-14 | 7.743e-09 | 0 |
| test_2d_particle_attr_access_unique_picmi | boxlib/phi | 4096 | 1.028e-14 | 7.743e-09 | 0 |
| test_3d_electrostatic_sphere_lab_frame | all/particle_position_x | 8744 | 8.021e-15 | 8.727e-14 | 0 |
| test_2d_runtime_components_picmi_restart | boxlib/phi | 4096 | 3.352e-15 | 7.831e-09 | 0 |
| test_2d_runtime_components_picmi | boxlib/phi | 4096 | 3.352e-15 | 7.831e-09 | 0 |
| test_rz_electrostatic_sphere | all/particle_theta | 74 | 6.661e-16 | 1.124e-15 | 0 |
| test_rz_electrostatic_sphere_uniform_weighting | all/particle_theta | 177 | 6.661e-16 | 1.343e-15 | 0 |
| test_3d_projection_div_cleaner_callback_picmi | boxlib/Bz | 40947 | 4.441e-16 | 4.337e-16 | 0 |
| test_3d_electrostatic_sphere_adaptive | all/particle_position_z | 8171 | 3.886e-16 | 9.833e-15 | 0 |
| test_3d_electrostatic_sphere | all/particle_position_x | 8452 | 2.220e-16 | 5.574e-15 | 0 |
| test_3d_electrostatic_sphere_rel_nodal | all/particle_position_x | 8452 | 2.220e-16 | 5.574e-15 | 0 |
| test_3d_electrostatic_sphere_lab_frame_mr_emass_10 | boxlib/Ex | 2093920 | 3.643e-17 | 2.989e-12 | 0 |
| test_3d_hard_edged_quadrupoles | boxlib/Ex | 2048 | 6.065e-20 | 1.805e-07 | 0 |
| test_rz_galilean_psatd_current_correction | boxlib/rho | 8192 | 6.255e-21 | 6.950e-08 | 0 |
| test_rz_embedded_boundary_em_particle_absorption_sh_factor_1 | /data/1/fields/rho | 4 | 6.019e-36 | 2.706e-16 | 0 |
| test_rz_embedded_boundary_em_particle_absorption_sh_factor_2 | /data/1/fields/rho | 4 | 6.019e-36 | 2.706e-16 | 0 |
| test_rz_embedded_boundary_em_particle_absorption_sh_factor_3 | /data/1/fields/rho | 8 | 6.019e-36 | 3.386e-16 | 0 |
| test_3d_initial_distribution | /data/0/particles/maxwell_juttner_drift/momentum/x | 372828 | 4.514e-36 | 2.962e-11 | 0 |
| test_rz_flux_injection | all/particle_momentum_z | 3480 | 7.468e-60 | 4.986e-12 | 0 |
| test_2d_flux_injection_from_eb | all/particle_cpu | None | 0.000e+00 | 0.000e+00 | 48 |
| test_3d_collision_pulsed_decay | all/particle_cpu | None | 0.000e+00 | 0.000e+00 | 45 |
| test_3d_flux_injection_from_eb | all/particle_cpu | None | 0.000e+00 | 0.000e+00 | 54 |
| test_rz_flux_injection_from_eb | all/particle_cpu | None | 0.000e+00 | 0.000e+00 | 54 |

## Cases with only log/text differences

| case | file | differing lines |
|---|---|---|
| test_1d_collision_z_Bremsstrahlung | diags/reducedfiles/particle_energy.txt | 22 |
| test_1d_collision_z_Bremsstrahlung | diags/reducedfiles/particle_momentum.txt | 22 |
| test_1d_collision_z_Bremsstrahlung | diags/reducedfiles/particle_number.txt | 22 |
| test_2d_collisions_split_momentum_push_electromagnetic | diags/reducedfiles/field_energy.txt | 1106 |
| test_2d_collisions_split_momentum_push_electromagnetic | diags/reducedfiles/particle_energy.txt | 4002 |
| test_2d_collisions_split_momentum_push_electromagnetic_vay | diags/reducedfiles/field_energy.txt | 1194 |
| test_2d_collisions_split_momentum_push_electromagnetic_vay | diags/reducedfiles/particle_energy.txt | 4002 |
| test_2d_ohm_solver_checkpoint_picmi | diags/chk000000/WarpXHeader | 1 |
| test_2d_ohm_solver_checkpoint_picmi | diags/chk000005/WarpXHeader | 1 |
| test_2d_ohm_solver_checkpoint_picmi | diags/chk000010/WarpXHeader | 1 |
| test_2d_ohm_solver_checkpoint_picmi_restart | diags/chk000010/WarpXHeader | 1 |
| test_2d_ohm_solver_checkpoint_picmi_restart | diags/chk000015/WarpXHeader | 1 |
| test_2d_particle_boundary_interaction_thermal | diags/reducedfiles/pen.txt | 26 |
| test_2d_pec_field_insulator_implicit | diags/reducedfiles/fieldenergy.txt | 2 |
| test_2d_pec_field_insulator_implicit_restart | diags/reducedfiles/fieldenergy.txt | 2 |
| test_3d_deuterium_deuterium_fusion_intraspecies | reduced_diags/particle_number.txt | 20 |
| test_3d_linear_breit_wheeler_angular | diags/reducedfiles/ParticleEnergy.txt | 22 |
| test_3d_linear_breit_wheeler_angular | diags/reducedfiles/ParticleMomentum.txt | 22 |
| test_3d_linear_breit_wheeler_angular_relativistic | diags/reducedfiles/ParticleEnergy.txt | 22 |
| test_3d_linear_breit_wheeler_angular_relativistic | diags/reducedfiles/ParticleMomentum.txt | 22 |
| test_3d_linear_breit_wheeler_angular_relativistic | diags/reducedfiles/ParticleNumber.txt | 22 |
| test_3d_linear_breit_wheeler_many_photons | diags/reducedfiles/ParticleEnergy.txt | 102 |
| test_3d_linear_breit_wheeler_many_photons | diags/reducedfiles/ParticleMomentum.txt | 102 |
| test_3d_linear_breit_wheeler_many_photons | diags/reducedfiles/ParticleNumber.txt | 60 |
| test_3d_reduced_diags | diags/reducedfiles/PF.txt | 2 |
| test_3d_reduced_diags | diags/reducedfiles/PP.txt | 4 |
| test_3d_reduced_diags | diags/reducedfiles/PX.txt | 2 |
| test_3d_reduced_diags_load_balance_costs_heuristic | diags/reducedfiles/LBC.txt | 4 |
| test_3d_reduced_diags_load_balance_costs_timers | diags/diag1000003/electrons/Header | 28 |
| test_3d_reduced_diags_load_balance_costs_timers | diags/reducedfiles/LBC.txt | 8 |
| test_3d_reduced_diags_load_balance_costs_timers_picmi | diags/diag1000003/electrons/Header | 28 |
| test_3d_reduced_diags_load_balance_costs_timers_picmi | diags/reducedfiles/LBC.txt | 8 |
| test_3d_reduced_diags_load_balance_costs_timers_psatd | diags/diag1000003/electrons/Header | 14 |
| test_3d_reduced_diags_load_balance_costs_timers_psatd | diags/reducedfiles/LBC.txt | 8 |

## Cases where only the composed input deck differs

| case | lines |
|---|---|
| test_1d_collision_z | 1 |
| test_1d_collision_z_correct_conservation | 1 |
| test_1d_collision_z_modulus_shuffle | 1 |
| test_1d_fel | 1 |
| test_1d_langmuir_fluid | 1 |
| test_1d_langmuir_multi | 1 |
| test_1d_laser_injection | 1 |
| test_1d_laser_injection_from_lasy_file | 1 |
| test_1d_laser_injection_from_lasy_file_boost | 1 |
| test_1d_ohm_solver_em_modes_picmi | 9 |
| test_1d_ohm_solver_em_modes_rkf45_picmi | 9 |
| test_1d_ohm_solver_ion_beam_picmi | 11 |
| test_1d_particle_absorbing_boundary | 1 |
| test_1d_particle_absorbing_boundary_direction_dependent_thresholds | 1 |
| test_1d_plasma_acceleration_picmi | 1 |
| test_1d_semi_implicit_picard | 3 |
| test_1d_silver_mueller | 1 |
| test_1d_synchronize_velocity | 1 |
| test_1d_theta_implicit_picard | 3 |
| test_2d_averaged_galilean_psatd | 1 |
| test_2d_averaged_galilean_psatd_hybrid | 1 |
| test_2d_bilinear_filter | 1 |
| test_2d_collision_xz_picmi | 10 |
| test_2d_comoving_psatd_hybrid | 1 |
| test_2d_dive_cleaning | 1 |
| test_2d_embedded_boundary_em_particle_absorption_sh_factor_1 | 3 |
| test_2d_embedded_boundary_em_particle_absorption_sh_factor_2 | 3 |
| test_2d_embedded_boundary_em_particle_absorption_sh_factor_3 | 3 |
| test_2d_galilean_psatd | 1 |
| test_2d_galilean_psatd_current_correction | 1 |
| test_2d_galilean_psatd_current_correction_psb | 1 |
| test_2d_galilean_psatd_hybrid | 1 |
| test_2d_ionization_boost | 1 |
| test_2d_ionization_lab | 1 |
| test_2d_ionization_lab_restart | 1 |
| test_2d_ionization_picmi | 9 |
| test_2d_langmuir_fluid | 1 |
| test_2d_langmuir_multi | 1 |
| test_2d_langmuir_multi_mr | 1 |
| test_2d_langmuir_multi_mr_anisotropic | 1 |
| test_2d_langmuir_multi_mr_maxlevel2 | 1 |
| test_2d_langmuir_multi_mr_momentum_conserving | 1 |
| test_2d_langmuir_multi_mr_psatd | 1 |
| test_2d_langmuir_multi_nodal | 1 |
| test_2d_langmuir_multi_picmi | 1 |
| test_2d_langmuir_multi_psatd | 1 |
| test_2d_langmuir_multi_psatd_JRhom_LL2 | 1 |
| test_2d_langmuir_multi_psatd_JRhom_LL2_nodal | 1 |
| test_2d_langmuir_multi_psatd_current_correction | 1 |
| test_2d_langmuir_multi_psatd_current_correction_nodal | 1 |
| test_2d_langmuir_multi_psatd_momentum_conserving | 1 |
| test_2d_langmuir_multi_psatd_nodal | 1 |
| test_2d_langmuir_multi_psatd_vay_deposition | 1 |
| test_2d_langmuir_multi_psatd_vay_deposition_nodal | 1 |
| test_2d_langmuir_multi_psatd_vay_deposition_particle_shape_4 | 1 |
| test_2d_larmor | 1 |
| test_2d_laser_acceleration_mr | 1 |
| test_2d_laser_acceleration_mr_picmi | 5 |
| test_2d_laser_injection | 1 |
| test_2d_laser_injection_from_binary_file | 1 |
| test_2d_laser_injection_from_lasy_file | 1 |
| test_2d_laser_on_fine | 1 |
| test_2d_leveling_thinning | 1 |
| test_2d_maxwell_hybrid_qed_solver | 1 |
| test_2d_nci_corrector | 1 |
| test_2d_nci_corrector_mr | 1 |
| test_2d_ohm_solver_landau_damping_picmi | 1 |
| test_2d_ohm_solver_vacuum_seam_cell | 1 |
| test_2d_ohm_solver_vacuum_seam_edge | 1 |
| test_2d_ohm_solver_vacuum_seam_guarded_cell | 1 |
| test_2d_ohm_solver_vacuum_seam_guarded_edge | 1 |
| test_2d_ohm_solver_vacuum_seam_node | 1 |
| test_2d_parabolic_channel_initialization | 1 |
| test_2d_particles_in_pml | 1 |
| test_2d_particles_in_pml_mr | 1 |
| test_2d_pec_field_insulator | 1 |
| test_2d_plasma_acceleration_boosted | 1 |
| test_2d_plasma_acceleration_mr | 1 |
| test_2d_plasma_acceleration_mr_momentum_conserving | 1 |
| test_2d_plasma_mirror | 1 |
| test_2d_pml_x_ckc | 1 |
| test_2d_pml_x_galilean | 1 |
| test_2d_pml_x_psatd | 1 |
| test_2d_pml_x_psatd_restart | 1 |
| test_2d_pml_x_yee | 1 |
| test_2d_pml_x_yee_restart | 1 |
| test_2d_proton_boron_fusion | 1 |
| test_2d_proton_boron_fusion_modulus_shuffle | 1 |
| test_2d_python_wrappers_picmi | 1 |
| test_2d_qed_breit_wheeler | 1 |
| test_2d_qed_breit_wheeler_opmd | 1 |
| test_2d_qed_quantum_sync | 1 |
| test_2d_refined_injection | 1 |
| test_2d_rigid_injection_btd | 1 |
| test_2d_rigid_injection_lab | 1 |
| test_2d_silver_mueller_x | 1 |
| test_2d_silver_mueller_z | 1 |
| test_2d_subcycling_mr | 1 |
| test_2d_theta_implicit_strang_psatd | 3 |
| test_2d_uniform_plasma | 1 |
| test_2d_vay_deposition | 1 |
| test_3d_acceleration | 1 |
| test_3d_acceleration_mw_cell_boundary | 1 |
| test_3d_acceleration_mw_cell_boundary_restart | 1 |
| test_3d_acceleration_mw_not_started | 1 |
| test_3d_acceleration_mw_not_started_restart | 1 |
| test_3d_acceleration_mw_start_boundary | 1 |
| test_3d_acceleration_mw_start_boundary_restart | 1 |
| test_3d_acceleration_mw_start_stop | 1 |
| test_3d_acceleration_mw_start_stop_restart | 1 |
| test_3d_acceleration_restart | 1 |
| test_3d_averaged_galilean_psatd | 1 |
| test_3d_averaged_galilean_psatd_hybrid | 1 |
| test_3d_beamsize_effect | 1 |
| test_3d_collider_diagnostics | 1 |
| test_3d_collision_iso | 1 |
| test_3d_collision_iso_modulus_shuffle | 1 |
| test_3d_collision_iso_subcycle | 1 |
| test_3d_deuterium_deuterium_fusion | 1 |
| test_3d_deuterium_tritium_fusion | 1 |
| test_3d_diff_lumi_diag_leptons | 1 |
| test_3d_diff_lumi_diag_leptons_mr | 1 |
| test_3d_diff_lumi_diag_photons | 1 |
| test_3d_divb_cleaning | 1 |
| test_3d_dive_cleaning | 1 |
| test_3d_eb_picmi | 3 |
| test_3d_eb_picmi_restart | 3 |
| test_3d_embedded_boundary_em_particle_absorption_sh_factor_1 | 3 |
| test_3d_embedded_boundary_em_particle_absorption_sh_factor_2 | 3 |
| test_3d_embedded_boundary_em_particle_absorption_sh_factor_3 | 3 |
| test_3d_embedded_boundary_picmi | 3 |
| test_3d_flux_injection | 1 |
| test_3d_focusing_gaussian_beam_photons | 1 |
| test_3d_galilean_psatd | 1 |
| test_3d_galilean_psatd_current_correction | 1 |
| test_3d_galilean_psatd_current_correction_psb | 1 |
| test_3d_gaussian_beam_picmi | 1 |
| test_3d_hard_edged_quadrupoles_boosted | 1 |
| test_3d_ion_stopping | 1 |
| test_3d_langmuir_fluid | 1 |
| test_3d_langmuir_multi | 1 |
| test_3d_langmuir_multi_nodal | 1 |
| test_3d_langmuir_multi_picmi | 1 |
| test_3d_langmuir_multi_psatd | 1 |
| test_3d_langmuir_multi_psatd_JRhom_LL2 | 1 |
| test_3d_langmuir_multi_psatd_JRhom_LL2_nodal | 1 |
| test_3d_langmuir_multi_psatd_JRhom_LL2_picmi | 1 |
| test_3d_langmuir_multi_psatd_current_correction | 1 |
| test_3d_langmuir_multi_psatd_current_correction_nodal | 1 |
| test_3d_langmuir_multi_psatd_div_cleaning | 1 |
| test_3d_langmuir_multi_psatd_momentum_conserving | 1 |
| test_3d_langmuir_multi_psatd_nodal | 1 |
| test_3d_langmuir_multi_psatd_vay_deposition | 1 |
| test_3d_langmuir_multi_psatd_vay_deposition_nodal | 1 |
| test_3d_laser_injection | 1 |
| test_3d_laser_injection_from_lasy_file | 1 |
| test_3d_linear_breit_wheeler_two_photons | 1 |
| test_3d_linear_compton_bunch_laser | 1 |
| test_3d_linear_compton_two_particles | 1 |
| test_3d_ohm_solver_cylinder_compression_picmi | 5 |
| test_3d_open_bc_poisson_solver | 1 |
| test_3d_open_bc_poisson_solver_sliced | 1 |
| test_3d_particle_absorption | 3 |
| test_3d_particle_boundaries | 1 |
| test_3d_particle_fields_diags | 1 |
| test_3d_particle_pusher | 1 |
| test_3d_particle_scrape | 3 |
| test_3d_particles_in_pml | 1 |
| test_3d_particles_in_pml_mr | 1 |
| test_3d_pec_field | 1 |
| test_3d_pec_field_mr | 1 |
| test_3d_pec_particle | 1 |
| test_3d_photon_pusher | 1 |
| test_3d_plasma_acceleration_boosted | 1 |
| test_3d_plasma_acceleration_boosted_hybrid | 1 |
| test_3d_plasma_acceleration_mr_picmi | 3 |
| test_3d_plasma_acceleration_picmi | 3 |
| test_3d_plasma_lens | 1 |
| test_3d_plasma_lens_boosted | 1 |
| test_3d_plasma_lens_hard_edged | 1 |
| test_3d_plasma_lens_picmi | 1 |
| test_3d_plasma_lens_python | 1 |
| test_3d_plasma_lens_short | 1 |
| test_3d_pmc_field | 1 |
| test_3d_pml_psatd_dive_divb_cleaning | 1 |
| test_3d_projection_div_cleaner_applied_field_zero_picmi | 1 |
| test_3d_proton_boron_fusion | 1 |
| test_3d_qed_breit_wheeler | 1 |
| test_3d_qed_breit_wheeler_opmd | 1 |
| test_3d_qed_quantum_sync | 1 |
| test_3d_qed_schwinger_1 | 1 |
| test_3d_qed_schwinger_2 | 1 |
| test_3d_qed_schwinger_3 | 1 |
| test_3d_qed_schwinger_4 | 1 |
| test_3d_radiation_reaction | 1 |
| test_3d_rotated_gaussian_beam | 1 |
| test_3d_thomson_parabola_spectrometer | 1 |
| test_3d_uniform_plasma | 1 |
| test_3d_uniform_plasma_psatd_JRhom_CC1 | 1 |
| test_3d_uniform_plasma_restart | 1 |
| test_3d_vay_deposition | 1 |
| test_3d_virtual_photons | 1 |
| test_rz_btd | 1 |
| test_rz_collision | 1 |
| test_rz_deuterium_tritium_fusion | 1 |
| test_rz_embedded_boundary_diffraction | 3 |
| test_rz_langmuir_multi_picmi | 1 |
| test_rz_laser_injection_from_RZ_lasy_file | 1 |
| test_rz_laser_injection_from_lasy_file | 1 |
| test_rz_point_of_contact_eb | 3 |
| test_rz_silver_mueller_z | 1 |
