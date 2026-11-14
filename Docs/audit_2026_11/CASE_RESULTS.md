# fork vs official: case results

cases: 398

- data-differs: 152
- identical: 37
- metadata-only: 209

## Cases whose numeric output differs

| case | diffs | max abs | max rel | kinds | fork status | official status |
|---|---|---|---|---|---|---|
| test_1d_collision_z_Bremsstrahlung | 6 | 0.0 | 0.0 | build_metadata_diff;text_diff;used_inputs_diff | passed | passed |
| test_1d_collision_z_inverse_Bremsstrahlung | 12 | 9.99977793693828e-07 | 83414.75370934811 | binary_diff;build_metadata_diff;text_diff;used_inputs_diff | passed | passed |
| test_1d_darwin_solver_em_modes_picmi | 16 | 1019.1315170663001 | 9.999999999999999e+299 | binary_diff;build_metadata_diff;text_diff;used_inputs_diff | passed | failed |
| test_1d_energy_conserving_thermal_plasma | 8 | 1.7478244079637142e+17 | 103.0756907490828 | binary_diff;build_metadata_diff;text_diff;used_inputs_diff | passed | passed |
| test_1d_laser_acceleration | 5 | 1.8189894035458565e-12 | 1.6023389170591465e-16 | binary_diff;build_metadata_diff;used_inputs_diff | passed | passed |
| test_1d_laser_acceleration_fluid_boosted | 4 | 1724161248754.875 | 0.05710709903449392 | binary_diff;build_metadata_diff;used_inputs_diff | passed | passed |
| test_1d_laser_acceleration_picmi | 6 | 1.8189894035458565e-12 | 1.6023389170591465e-16 | binary_diff;build_metadata_diff;used_inputs_diff | passed | passed |
| test_1d_laser_injection_implicit | 62 | 1.1333717783559759e+179 | 2.119696219344424e+43 | binary_diff;build_metadata_diff;hdf5_diff;used_inputs_diff | passed | passed |
| test_1d_load_density | 8 | 2.9103830456733704e-11 | 2.076020821683102e-16 | hdf5_diff;used_inputs_diff | passed | passed |
| test_1d_macroscopic_solver | 5 | 4.377216100692749e-07 | 3.827290173107129e-14 | binary_diff;build_metadata_diff;used_inputs_diff | passed | passed |
| test_1d_macroscopic_solver_picmi | 5 | 4.377216100692749e-07 | 3.827290173107129e-14 | binary_diff;build_metadata_diff;used_inputs_diff | passed | failed |
| test_1d_ohm_solver_em_modes_picmi | 4 | 0.0 | 0.0 | binary_diff;build_metadata_diff;used_inputs_diff | passed | failed |
| test_1d_ohm_solver_em_modes_rkf45_picmi | 4 | 0.0 | 0.0 | binary_diff;build_metadata_diff;used_inputs_diff | passed | failed |
| test_1d_ohm_solver_ion_beam_picmi | 5 | 0.0 | 0.0 | binary_diff;build_metadata_diff;used_inputs_diff | passed | failed |
| test_1d_pierce_diode | 2 | 7.274445344944464 | 0.1877392800347486 | hdf5_diff;used_inputs_diff | passed | passed |
| test_1d_resample_velocity_coincidence_thinning | 9 | 5.349781776799788e+306 | inf | binary_diff;build_metadata_diff;used_inputs_diff | passed | passed |
| test_1d_resample_velocity_coincidence_thinning_cartesian | 9 | 7.715546439398192e+282 | inf | binary_diff;build_metadata_diff;used_inputs_diff | passed | passed |
| test_1d_theta_implicit_planar_pinch | 14 | 83755.73834991455 | 0.015012188805379979 | binary_diff;build_metadata_diff;text_diff;used_inputs_diff | passed | passed |
| test_2d_collision_xz | 26 | 114734116.20706801 | 64.05892620914736 | binary_diff;build_metadata_diff;used_inputs_diff | passed | passed |
| test_2d_collision_xz_global_debye | 26 | 86435718.21091071 | 24.640374641583517 | binary_diff;build_metadata_diff;used_inputs_diff | passed | passed |
| test_2d_collisions_split_momentum_push_electromagnetic | 3 | 0.0 | 0.0 | text_diff;used_inputs_diff | passed | passed |
| test_2d_collisions_split_momentum_push_electromagnetic_vay | 3 | 0.0 | 0.0 | text_diff;used_inputs_diff | passed | passed |
| test_2d_collisions_split_momentum_push_electrostatic | 5 | 1.7435172437126816e+17 | 85633.71931028098 | hdf5_diff;text_diff;used_inputs_diff | passed | passed |
| test_2d_darwin_solver_em_modes_es_picmi | 19 | 29225.478673513553 | 9.999999999999999e+299 | binary_diff;build_metadata_diff;text_diff;used_inputs_diff | passed | failed |
| test_2d_dirichlet_bc | 102 | 9.48017451207761e-07 | 3.2557013523451184e+38 | binary_diff;build_metadata_diff;used_inputs_diff | passed | passed |
| test_2d_dirichlet_bc_picmi | 102 | 9.48017451207761e-07 | 3.2557013523451184e+38 | binary_diff;build_metadata_diff;used_inputs_diff | passed | failed |
| test_2d_embedded_boundary_cube | 346 | 2.5818280846579e-10 | 2044.4689925207094 | binary_diff;build_metadata_diff;used_inputs_diff | passed | passed |
| test_2d_embedded_boundary_rotated_cube | 5 | 9.663381206337363e-13 | 3.3897318563252423e-12 | binary_diff;build_metadata_diff;used_inputs_diff | passed | passed |
| test_2d_energy_conserving_thermal_plasma | 11 | 1.9933151391511174e+17 | 390.22732659902084 | binary_diff;build_metadata_diff;text_diff;used_inputs_diff | passed | passed |
| test_2d_field_probe | 27 | 6.47110344714606e+178 | inf | binary_diff;build_metadata_diff;text_diff;used_inputs_diff | passed | passed |
| test_2d_flux_injection_from_eb | 10 | 0.0 | 0.0 | binary_diff;build_metadata_diff;text_diff;used_inputs_diff | passed | passed |
| test_2d_id_cpu_read_picmi | 23 | 2.276010491186753e-09 | 0.000757104051466435 | binary_diff;build_metadata_diff;used_inputs_diff | passed | failed |
| test_2d_ionization_picmi | 4 | 0.0 | 0.0 | binary_diff;build_metadata_diff;used_inputs_diff | passed | failed |
| test_2d_laser_acceleration_boosted | 8 | 0.529296875 | 0.00013570927177520244 | binary_diff;build_metadata_diff;used_inputs_diff | passed | passed |
| test_2d_laser_acceleration_mr_picmi | 4 | 0.0 | 0.0 | binary_diff;build_metadata_diff;used_inputs_diff | passed | passed |
| test_2d_laser_injection_implicit | 6 | 4.149785041809082 | 4.839603268605406e-08 | binary_diff;build_metadata_diff;used_inputs_diff | passed | passed |
| test_2d_laser_ion_acc | 11 | 299034118652320.0 | inf | hdf5_diff;text_diff;used_inputs_diff | passed | passed |
| test_2d_laser_ion_acc_picmi | 10 | 81735687538271.69 | 3.655327255719425 | binary_diff;hdf5_diff;text_diff;used_inputs_diff | passed | passed |
| test_2d_load_density | 66 | 1.1641532182693481e-10 | 4.3759842169844794e-16 | hdf5_diff;used_inputs_diff | passed | passed |
| test_2d_macroscopic_solver | 6 | 2.1606683731079102e-07 | 1.1306090085251453e-12 | binary_diff;build_metadata_diff;used_inputs_diff | passed | passed |
| test_2d_ohm_solver_checkpoint_picmi | 9 | 0.0 | 0.0 | build_metadata_diff;text_diff;used_inputs_diff | passed | failed |
| test_2d_ohm_solver_checkpoint_picmi_restart | 6 | 0.0 | 0.0 | build_metadata_diff;text_diff;used_inputs_diff | passed | failed |
| test_2d_ohm_solver_electron_energy_adiabat_picmi | 11 | 27.634670250517047 | 1.0 | hdf5_diff;used_inputs_diff | passed | failed |
| test_2d_ohm_solver_electron_energy_joule_picmi | 8 | 15452.975893395953 | 19.241556961261225 | hdf5_diff;text_diff;used_inputs_diff | passed | failed |
| test_2d_ohm_solver_electron_energy_qei_picmi | 11 | 160845.328335294 | 299852.97691392136 | hdf5_diff;used_inputs_diff | passed | failed |
| test_2d_ohm_solver_electron_energy_vacuum_picmi | 9 | 607.5238499050029 | 0.0032173436763194272 | hdf5_diff;used_inputs_diff | passed | failed |
| test_2d_ohm_solver_landau_damping_picmi | 4 | 0.0 | 0.0 | binary_diff;build_metadata_diff;used_inputs_diff | passed | failed |
| test_2d_ohm_solver_magnetic_reconnection_picmi | 17 | 0.14595773136443313 | 6.097137383563936e+33 | binary_diff;build_metadata_diff;used_inputs_diff | passed | failed |
| test_2d_ohm_solver_resistive_drag_per_species_picmi | 7 | 22750.112852419727 | 26.476373550167978 | hdf5_diff;text_diff;used_inputs_diff | passed | failed |
| test_2d_ohm_solver_resistive_drag_picmi | 7 | 15228.96458470542 | 7.078214710568445 | hdf5_diff;text_diff;used_inputs_diff | passed | failed |
| test_2d_particle_attr_access_picmi | 8 | 1.0282621261214044e-14 | 7.743005766228754e-09 | binary_diff;build_metadata_diff;used_inputs_diff | passed | failed |
| test_2d_particle_attr_access_unique_picmi | 8 | 1.0282621261214044e-14 | 7.743005766228754e-09 | binary_diff;build_metadata_diff;used_inputs_diff | passed | failed |
| test_2d_particle_boundary_interaction_thermal | 4 | 0.0 | 0.0 | build_metadata_diff;text_diff;used_inputs_diff | passed | passed |
| test_2d_particle_reflection_picmi | 7 | 4.131610978674871e-06 | 2804.0729044418727 | binary_diff;build_metadata_diff;used_inputs_diff | passed | failed |
| test_2d_particle_thermal_boundary | 11 | 2.546357791288756e-09 | 0.20937015087214195 | binary_diff;build_metadata_diff;text_diff;used_inputs_diff | passed | passed |
| test_2d_pec_field_insulator_implicit | 23 | 134014.61160545144 | 0.8936548079352381 | binary_diff;build_metadata_diff;text_diff;used_inputs_diff | passed | passed |
| test_2d_pec_field_insulator_implicit_restart | 12 | 30450.484309987747 | 0.7714386919822692 | binary_diff;build_metadata_diff;text_diff;used_inputs_diff | passed | passed |
| test_2d_prev_positions_picmi | 11 | 178.86088449810524 | 656.6482864472147 | binary_diff;build_metadata_diff;used_inputs_diff | passed | failed |
| test_2d_projection_div_cleaner_initial_analytical_field_picmi | 13 | 3.8546943414985435e-13 | 3.567146578120628e+287 | binary_diff;build_metadata_diff;used_inputs_diff | passed | failed |
| test_2d_repelling_particles | 65 | 26624.0 | 3.6111506029812155e+299 | binary_diff;build_metadata_diff;used_inputs_diff | passed | passed |
| test_2d_runtime_components_picmi | 23 | 7.707967597525567e-10 | 8.277450572408662e-06 | binary_diff;build_metadata_diff;used_inputs_diff | passed | failed |
| test_2d_runtime_components_picmi_restart | 12 | 7.722746886429377e-10 | 8.27745995468147e-06 | binary_diff;build_metadata_diff;used_inputs_diff | passed | failed |
| test_2d_space_charge_initialization | 11 | 7.638334409421077e-13 | 7.86726648574841e-12 | binary_diff;build_metadata_diff;used_inputs_diff | passed | passed |
| test_2d_theta_implicit_jfnk_vandb | 10 | 38404096.0 | 3.4000941110401013e-09 | binary_diff;build_metadata_diff;used_inputs_diff | passed | passed |
| test_2d_theta_implicit_jfnk_vandb_cropping | 42 | 1.1359168950638966e+179 | 1.1368683772161602e+289 | binary_diff;build_metadata_diff;used_inputs_diff | passed | passed |
| test_2d_theta_implicit_jfnk_vandb_filtered | 10 | 15814656.0 | 1.032115704482486e-09 | binary_diff;build_metadata_diff;used_inputs_diff | passed | passed |
| test_2d_theta_implicit_jfnk_vandb_picmi | 11 | 32653312.0 | 7.468103639982643e-09 | binary_diff;build_metadata_diff;used_inputs_diff | passed | failed |
| test_2d_theta_implicit_symmetry | 24 | 1.298633005633694e+179 | 1.4346805890195036e+43 | binary_diff;build_metadata_diff;text_diff;used_inputs_diff | passed | passed |
| test_3d_acceleration_psatd | 51 | 367764.33230934606 | 268010.9059593207 | binary_diff;build_metadata_diff;used_inputs_diff | passed | passed |
| test_3d_acceleration_psatd_restart | 25 | 367764.3323093817 | 8211.31651827435 | binary_diff;build_metadata_diff;used_inputs_diff | passed | passed |
| test_3d_acceleration_psatd_time_avg | 75 | 367764.33338940935 | 1244905.1712906533 | binary_diff;build_metadata_diff;used_inputs_diff | passed | passed |
| test_3d_acceleration_psatd_time_avg_restart | 37 | 367764.33338946704 | 357967.2890664591 | binary_diff;build_metadata_diff;used_inputs_diff | passed | passed |
| test_3d_beam_beam_collision | 10 | 3005.5 | 0.0003858541310759061 | hdf5_diff;text_diff | passed | passed |
| test_3d_collision_pulsed_decay | 13 | 0.0 | 0.0 | binary_diff;build_metadata_diff;text_diff;used_inputs_diff | passed | passed |
| test_3d_collision_xyz | 21 | 88778578.74718887 | 184.75045343990993 | binary_diff;build_metadata_diff;used_inputs_diff | passed | passed |
| test_3d_deuterium_deuterium_fusion_intraspecies | 13 | 0.0 | 0.0 | build_metadata_diff;text_diff;used_inputs_diff | passed | passed |
| test_3d_effective_potential_electrostatic_picmi | 5 | 0.002040202422546542 | 9.029842556581187e+266 | hdf5_diff;used_inputs_diff | passed | failed |
| test_3d_electrostatic_sphere | 12 | 2.220446049250313e-16 | 1.827152144485348e-12 | binary_diff;build_metadata_diff;hdf5_diff;used_inputs_diff | passed | passed |
| test_3d_electrostatic_sphere_adaptive | 25 | 3.885780586188048e-16 | 1.6381229273155366e-12 | binary_diff;build_metadata_diff;hdf5_diff;text_diff;used_inputs_diff | passed | passed |
| test_3d_electrostatic_sphere_eb | 5 | 1.667714719211233e-05 | 7.011176053242491e-06 | hdf5_diff;text_diff;used_inputs_diff | passed | passed |
| test_3d_electrostatic_sphere_eb_mixed_bc | 9 | 7.454431362363678e-05 | 0.020359308635374933 | binary_diff;build_metadata_diff;used_inputs_diff | passed | passed |
| test_3d_electrostatic_sphere_eb_picmi | 6 | 1.667714719211233e-05 | 7.011176053242491e-06 | hdf5_diff;text_diff;used_inputs_diff | passed | failed |
| test_3d_electrostatic_sphere_lab_frame | 13 | 8.021361352916756e-15 | 7.150855425538347e-11 | binary_diff;build_metadata_diff;hdf5_diff;used_inputs_diff | passed | passed |
| test_3d_electrostatic_sphere_lab_frame_mr_emass_10 | 18 | 3.642919299551295e-17 | 3.118564815503798e-12 | binary_diff;build_metadata_diff;hdf5_diff;used_inputs_diff | passed | passed |
| test_3d_electrostatic_sphere_rel_nodal | 12 | 2.220446049250313e-16 | 1.827152144485348e-12 | binary_diff;build_metadata_diff;hdf5_diff;used_inputs_diff | passed | passed |
| test_3d_embedded_boundary_cube | 7 | 9.905906038996983e-09 | 30793865.221289646 | binary_diff;build_metadata_diff;used_inputs_diff | passed | passed |
| test_3d_embedded_boundary_cube_macroscopic | 7 | 9.906159448291874e-09 | 2881916.4407577463 | binary_diff;build_metadata_diff;used_inputs_diff | passed | passed |
| test_3d_embedded_boundary_rotated_cube | 29 | 1.3486989303146402e-12 | 4.487604191634505e+33 | binary_diff;build_metadata_diff;used_inputs_diff | passed | passed |
| test_3d_flux_injection_from_eb | 10 | 0.0 | 0.0 | binary_diff;build_metadata_diff;text_diff;used_inputs_diff | passed | passed |
| test_3d_focusing_gaussian_beam | 4 | 2.9103830456733704e-11 | 3.1958948864632477e-15 | binary_diff;build_metadata_diff;used_inputs_diff | passed | passed |
| test_3d_hard_edged_quadrupoles | 10 | 6.064805533177544e-20 | 7.270712980259791e-07 | binary_diff;build_metadata_diff;used_inputs_diff | passed | passed |
| test_3d_hard_edged_quadrupoles_moving | 10 | 6.416809891743088e-08 | 1.0 | binary_diff;build_metadata_diff;used_inputs_diff | passed | passed |
| test_3d_initial_distribution | 3 | 4.513898307157584e-36 | 2.9619619130218704e-11 | hdf5_diff;text_diff;used_inputs_diff | passed | passed |
| test_3d_ion_beam_extraction | 4 | 11357066.211156225 | 1.7315626221003555e+293 | hdf5_diff;used_inputs_diff | passed | passed |
| test_3d_laser_acceleration | 3 | 1.0913936421275139e-11 | 4.697259232899143e-16 | hdf5_diff;used_inputs_diff | passed | passed |
| test_3d_laser_acceleration_btd | 19 | 10.0 | 1.9000700986553264e-11 | binary_diff;build_metadata_diff;hdf5_diff;used_inputs_diff | passed | passed |
| test_3d_laser_acceleration_picmi | 6 | 1.0913936421275139e-11 | 4.697259232899143e-16 | binary_diff;build_metadata_diff;used_inputs_diff | passed | passed |
| test_3d_laser_acceleration_python | 3 | 1.0913936421275139e-11 | 4.697259232899143e-16 | hdf5_diff;used_inputs_diff | passed | passed |
| test_3d_laser_acceleration_single_precision_comms | 3 | 1.0913936421275139e-11 | 4.697259232899143e-16 | hdf5_diff;used_inputs_diff | passed | passed |
| test_3d_linear_breit_wheeler_angular | 3 | 0.0 | 0.0 | text_diff;used_inputs_diff | passed | passed |
| test_3d_linear_breit_wheeler_angular_relativistic | 4 | 0.0 | 0.0 | text_diff;used_inputs_diff | passed | passed |
| test_3d_linear_breit_wheeler_many_photons | 55 | 0.0 | 0.0 | build_metadata_diff;text_diff;used_inputs_diff | passed | passed |
| test_3d_load_density | 15 | 2.3283064365386963e-10 | 5.645074044583491e-16 | hdf5_diff;used_inputs_diff | passed | passed |
| test_3d_magnetostatic_eb | 8 | 10175.060287153945 | 3.7404974950749904e+282 | binary_diff;build_metadata_diff;used_inputs_diff | passed | passed |
| test_3d_magnetostatic_eb_picmi | 10 | 8261.58237059516 | 1.6263032587282567e+281 | binary_diff;build_metadata_diff;used_inputs_diff | passed | failed |
| test_3d_nodal_electrostatic_solver | 9 | 177.5 | 7061464.401858599 | binary_diff;build_metadata_diff;text_diff;used_inputs_diff | passed | passed |
| test_3d_particle_scrape_picmi | 13 | 3.814697265625e-06 | 545.113601414873 | binary_diff;build_metadata_diff;text_diff;used_inputs_diff | passed | failed |
| test_3d_point_of_contact_eb | 2 | 1.2849443731255406e-12 | 0.45833333333523973 | hdf5_diff;used_inputs_diff | passed | passed |
| test_3d_projection_div_cleaner_applied_field_picmi | 7 | 1.7569279364693102e-14 | 1.9961976057168008e+43 | binary_diff;build_metadata_diff;used_inputs_diff | passed | failed |
| test_3d_projection_div_cleaner_callback_picmi | 8 | 4.440892098500626e-16 | 1.1176398061555543e-14 | binary_diff;build_metadata_diff;used_inputs_diff | passed | failed |
| test_3d_reduced_diags | 6 | 0.0 | 0.0 | build_metadata_diff;text_diff;used_inputs_diff | passed | passed |
| test_3d_reduced_diags_load_balance_costs_heuristic | 4 | 0.0 | 0.0 | build_metadata_diff;text_diff;used_inputs_diff | passed | passed |
| test_3d_reduced_diags_load_balance_costs_timers | 7 | 0.0 | 0.0 | binary_diff;build_metadata_diff;text_diff;used_inputs_diff | passed | passed |
| test_3d_reduced_diags_load_balance_costs_timers_picmi | 7 | 0.0 | 0.0 | binary_diff;build_metadata_diff;text_diff;used_inputs_diff | passed | failed |
| test_3d_reduced_diags_load_balance_costs_timers_psatd | 7 | 1.0 | 0.49612403100775193 | binary_diff;build_metadata_diff;text_diff;used_inputs_diff | passed | passed |
| test_3d_relativistic_space_charge_initialization | 11 | 4.088490749679702e+179 | 2.4945670877160346e+255 | binary_diff;build_metadata_diff;used_inputs_diff | passed | passed |
| test_3d_space_charge_initialization | 11 | 2.0747847884194925e-12 | 1.6412137586671915e-09 | binary_diff;build_metadata_diff;used_inputs_diff | passed | passed |
| test_3d_theta_implicit_jfnk_direct | 10 | 2.828165709969193e+307 | inf | binary_diff;build_metadata_diff;used_inputs_diff | passed | passed |
| test_rz_electrostatic_sphere | 13 | 6.661338147750939e-16 | 5.775570162617128e-14 | binary_diff;build_metadata_diff;hdf5_diff;used_inputs_diff | passed | passed |
| test_rz_electrostatic_sphere_eb | 9 | 8.76554975270949e-06 | 2.4377982602728698e-06 | binary_diff;build_metadata_diff;used_inputs_diff | passed | passed |
| test_rz_electrostatic_sphere_eb_mr | 3 | 8.76554975270949e-06 | 2.4377982602728698e-06 | hdf5_diff;used_inputs_diff | passed | passed |
| test_rz_electrostatic_sphere_uniform_weighting | 13 | 6.661338147750939e-16 | 1.8771609528340928e-13 | binary_diff;build_metadata_diff;hdf5_diff;used_inputs_diff | passed | passed |
| test_rz_embedded_boundary_em_particle_absorption_sh_factor_1 | 3 | 6.018531076210112e-36 | 2.705888319855986e-16 | hdf5_diff;used_inputs_diff | passed | passed |
| test_rz_embedded_boundary_em_particle_absorption_sh_factor_2 | 3 | 6.018531076210112e-36 | 2.705888319855986e-16 | hdf5_diff;used_inputs_diff | passed | passed |
| test_rz_embedded_boundary_em_particle_absorption_sh_factor_3 | 5 | 6.018531076210112e-36 | 3.386343767444935e-16 | hdf5_diff;used_inputs_diff | passed | passed |
| test_rz_flux_injection | 5 | 7.46761833343337e-60 | 4.985603563381006e-12 | binary_diff;build_metadata_diff;used_inputs_diff | passed | passed |
| test_rz_flux_injection_from_eb | 10 | 0.0 | 0.0 | binary_diff;build_metadata_diff;text_diff;used_inputs_diff | passed | passed |
| test_rz_galilean_psatd | 22 | 7.47783351240372e-07 | 0.01165576309442763 | binary_diff;build_metadata_diff;used_inputs_diff | passed | passed |
| test_rz_galilean_psatd_current_correction | 18 | 6.254936228147125e-21 | 5.616642801191687e-07 | binary_diff;build_metadata_diff;used_inputs_diff | passed | passed |
| test_rz_galilean_psatd_current_correction_psb | 22 | 7.789758242893186e-07 | 0.03349384295509165 | binary_diff;build_metadata_diff;used_inputs_diff | passed | passed |
| test_rz_langmuir_fluid | 6 | 0.002288818359375 | 9.11156822546348e-08 | binary_diff;build_metadata_diff;used_inputs_diff | passed | passed |
| test_rz_langmuir_multi | 35 | 0.12755171954631805 | 6329.540982169377 | binary_diff;build_metadata_diff;used_inputs_diff | passed | passed |
| test_rz_langmuir_multi_psatd | 35 | 0.01617431640625 | 2.2736109533817183e-06 | binary_diff;build_metadata_diff;used_inputs_diff | passed | passed |
| test_rz_langmuir_multi_psatd_JRhom_LL4 | 35 | 0.11163330078125 | 2617450.991501519 | binary_diff;build_metadata_diff;used_inputs_diff | passed | passed |
| test_rz_langmuir_multi_psatd_current_correction | 24 | 367.6469841003418 | 538.1857850934471 | binary_diff;build_metadata_diff;used_inputs_diff | passed | passed |
| test_rz_laser_acceleration | 9 | 13578.342664776257 | 22.914192788995635 | binary_diff;build_metadata_diff;used_inputs_diff | passed | passed |
| test_rz_laser_acceleration_boosted_opmd | 2 | 2.9676329607887835e-11 | 11275.019417729514 | hdf5_diff;used_inputs_diff | passed | passed |
| test_rz_laser_acceleration_opmd | 4 | 13578.342664776581 | 411.7562513749134 | hdf5_diff;used_inputs_diff | passed | passed |
| test_rz_laser_acceleration_picmi | 4 | 11039.302979493277 | 46.76887038001002 | binary_diff;hdf5_diff;used_inputs_diff | passed | passed |
| test_rz_load_density | 15 | 124415.08738071173 | 1.2741414606618031 | hdf5_diff;used_inputs_diff | passed | passed |
| test_rz_magnetostatic_eb_picmi | 10 | 27294.26427309122 | 2669.646900322519 | binary_diff;build_metadata_diff;used_inputs_diff | passed | failed |
| test_rz_ohm_solver_cylinder_compression_picmi | 34 | 9.120681264369507e+304 | 4.41250026860091 | binary_diff;build_metadata_diff;used_inputs_diff | passed | failed |
| test_rz_ohm_solver_em_modes_picmi | 19 | 1.7734009816194884e-09 | 1.2338102281136157e-08 | binary_diff;build_metadata_diff;hdf5_diff;used_inputs_diff | passed | failed |
| test_rz_ohm_solver_resistive_drag_species_fields_picmi | 3 | 782312.7660482959 | 0.24703557312252977 | hdf5_diff;used_inputs_diff | passed | failed |
| test_rz_particle_boundary_interaction_picmi | 25 | 1.4026811806858552e-08 | 10324.188809099127 | hdf5_diff;used_inputs_diff | passed | failed |
| test_rz_particle_boundary_interaction_reflecting | 25 | 1.4026811806858552e-08 | 38.316980968020495 | hdf5_diff;used_inputs_diff | passed | passed |
| test_rz_pml_psatd | 6 | 0.013369046781522589 | 1.9999999999999944 | binary_diff;build_metadata_diff;used_inputs_diff | passed | passed |
| test_rz_psatd_JRhom_LL2 | 358 | 6.609375 | 36516.42270780344 | binary_diff;build_metadata_diff;used_inputs_diff | passed | passed |
| test_rz_scraping | 40 | 2.0634882691439316e-13 | 1.2975813083972712e-09 | build_metadata_diff;hdf5_diff;used_inputs_diff | passed | passed |
| test_rz_scraping_filter | 40 | 2.0627943797535409e-13 | 1.2975813083972712e-09 | build_metadata_diff;hdf5_diff;used_inputs_diff | passed | passed |
| test_rz_secondary_ion_emission_picmi | 5 | 3.02947337748647e-08 | 8563.792088909573 | hdf5_diff;used_inputs_diff | passed | failed |
| test_rz_spacecraft_charging_picmi | 103 | 1.0370007430537953e-07 | 2.220446049250313e+284 | hdf5_diff;used_inputs_diff | passed | passed |

## Cases with different composed inputs (warpx_used_inputs) but identical data

| case | used-inputs diff lines |
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
| test_2d_laser_injection | 1 |
| test_2d_laser_injection_from_binary_file | 1 |
| test_2d_laser_injection_from_lasy_file | 1 |
| test_2d_laser_on_fine | 1 |
| test_2d_leveling_thinning | 1 |
| test_2d_maxwell_hybrid_qed_solver | 1 |
| test_2d_nci_corrector | 1 |
| test_2d_nci_corrector_mr | 1 |
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

## Test failures

| case | fork | official | note |
|---|---|---|---|
| test_1d_background_mcc_picmi | failed | failed | warpx_used_inputs |
| test_1d_charge_exchange_dsmc | failed | failed |  |
| test_1d_darwin_solver_em_modes_picmi | passed | failed |  |
| test_1d_dsmc_picmi | failed | failed | warpx_used_inputs |
| test_1d_ionization_neutral_dsmc | failed | failed |  |
| test_1d_macroscopic_solver_picmi | passed | failed |  |
| test_1d_ohm_solver_em_modes_picmi | passed | failed |  |
| test_1d_ohm_solver_em_modes_rkf45_picmi | passed | failed |  |
| test_1d_ohm_solver_ion_beam_picmi | passed | failed |  |
| test_1d_photoneutralization_dsmc | failed | failed |  |
| test_1d_two_product_reaction_dsmc | failed | failed |  |
| test_2d_background_mcc | failed | failed |  |
| test_2d_background_mcc_picmi | failed | failed | diags/diag1000000/Header,diags/diag1000000/Level_0/Cell_D_00000 |
| test_2d_charge_exchange_dsmc | failed | failed |  |
| test_2d_collision_xz_picmi | passed | failed |  |
| test_2d_darwin_solver_em_modes_es_picmi | passed | failed |  |
| test_2d_dirichlet_bc_picmi | passed | failed |  |
| test_2d_embedded_circle | failed | failed |  |
| test_2d_id_cpu_read_picmi | passed | failed | Python_restart_runtime_components_chk00000/field_schema.txt,Python_restart_runtime_components_chk00005/field_schema.txt |
| test_2d_ionization_picmi | passed | failed |  |
| test_2d_langmuir_multi_picmi | passed | failed |  |
| test_2d_ohm_solver_checkpoint_picmi | passed | failed | diags/chk000000/field_schema.txt,diags/chk000005/field_schema.txt |
| test_2d_ohm_solver_checkpoint_picmi_restart | passed | failed | diags/chk000010/field_schema.txt,diags/chk000015/field_schema.txt |
| test_2d_ohm_solver_electron_energy_adiabat_picmi | passed | failed |  |
| test_2d_ohm_solver_electron_energy_joule_picmi | passed | failed |  |
| test_2d_ohm_solver_electron_energy_qei_picmi | passed | failed |  |
| test_2d_ohm_solver_electron_energy_vacuum_picmi | passed | failed |  |
| test_2d_ohm_solver_landau_damping_picmi | passed | failed |  |
| test_2d_ohm_solver_magnetic_reconnection_picmi | passed | failed |  |
| test_2d_ohm_solver_resistive_drag_per_species_picmi | passed | failed |  |
| test_2d_ohm_solver_resistive_drag_picmi | passed | failed |  |
| test_2d_particle_attr_access_picmi | passed | failed |  |
| test_2d_particle_attr_access_unique_picmi | passed | failed |  |
| test_2d_particle_reflection_picmi | passed | failed |  |
| test_2d_pass_mpi_comm_picmi | passed | failed |  |
| test_2d_prev_positions_picmi | passed | failed |  |
| test_2d_projection_div_cleaner_initial_analytical_field_picmi | passed | failed |  |
| test_2d_python_wrappers_picmi | passed | failed |  |
| test_2d_runtime_components_picmi | passed | failed | diags/chk000000/field_schema.txt,diags/chk000005/field_schema.txt |
| test_2d_runtime_components_picmi_restart | passed | failed | diags/chk000010/field_schema.txt |
| test_2d_theta_implicit_jfnk_vandb_picmi | passed | failed | diags/reduced_files/field_energy.txt,diags/reduced_files/particle_energy.txt,diags/reducedfiles/field_energy.txt,diags/reducedfiles/particle_energy.txt |
| test_3d_deuterium_deuterium_fusion_anisotropic_beam_target_uz_10pct | failed | failed |  |
| test_3d_deuterium_tritium_fusion_anisotropic_beam_target_uz_10pct | failed | failed |  |
| test_3d_eb_picmi | passed | failed | diags/chk000000/field_schema.txt,diags/chk000030/field_schema.txt |
| test_3d_eb_picmi_restart | passed | failed | diags/chk000060/field_schema.txt |
| test_3d_effective_potential_electrostatic_picmi | passed | failed |  |
| test_3d_electrostatic_sphere_eb_picmi | passed | failed |  |
| test_3d_embedded_boundary_picmi | passed | failed |  |
| test_3d_gaussian_beam_picmi | passed | failed |  |
| test_3d_ionization_electron_dsmc | failed | failed |  |
| test_3d_ionization_ion_dsmc | failed | failed |  |
| test_3d_langmuir_multi_picmi | passed | failed |  |
| test_3d_langmuir_multi_psatd_JRhom_LL2_picmi | passed | failed |  |
| test_3d_load_external_field_grid_picmi | failed | failed |  |
| test_3d_load_external_field_particle_multi_time | failed | failed |  |
| test_3d_load_external_field_particle_multi_time_picmi | failed | failed |  |
| test_3d_load_external_field_particle_picmi | failed | failed |  |
| test_3d_load_external_field_particle_time | failed | failed |  |
| test_3d_load_external_field_particle_time_picmi | failed | failed |  |
| test_3d_load_external_field_particle_time_restart | failed | failed |  |
| test_3d_magnetostatic_eb_picmi | passed | failed |  |
| test_3d_ohm_solver_cylinder_compression_picmi | passed | failed |  |
| test_3d_particle_scrape_picmi | passed | failed |  |
| test_3d_plasma_lens_picmi | passed | failed |  |
| test_3d_plasma_lens_python | passed | failed |  |
| test_3d_projection_div_cleaner_applied_field_picmi | passed | failed |  |
| test_3d_projection_div_cleaner_applied_field_zero_picmi | passed | failed |  |
| test_3d_projection_div_cleaner_callback_picmi | passed | failed |  |
| test_3d_projection_div_cleaner_picmi | failed | failed |  |
| test_3d_reduced_diags_load_balance_costs_timers_picmi | passed | failed |  |
| test_rz_langmuir_multi_picmi | passed | failed |  |
| test_rz_load_external_field_grid | failed | failed |  |
| test_rz_load_external_field_grid_restart | failed | failed |  |
| test_rz_load_external_field_particles | failed | failed |  |
| test_rz_load_external_field_particles_restart | failed | failed |  |
| test_rz_magnetostatic_eb_picmi | passed | failed |  |
| test_rz_ohm_solver_cylinder_compression_picmi | passed | failed |  |
| test_rz_ohm_solver_em_modes_picmi | passed | failed |  |
| test_rz_ohm_solver_resistive_drag_species_fields_picmi | passed | failed |  |
| test_rz_particle_boundary_interaction_picmi | passed | failed |  |
| test_rz_projection_div_cleaner | failed | failed |  |
| test_rz_secondary_ion_emission_picmi | passed | failed |  |
