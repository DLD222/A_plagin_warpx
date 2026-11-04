# A1 前置：`WarpX` 静态成员清单

<!-- SPDX-License-Identifier: BSD-3-Clause -->

> 自动扫描 `WarpX.H` 的 `static` 数据成员 + `grep` 全仓库 `WarpX::<name>` 引用（`.H/.cpp`）。
> 结果：**66 个可解析 static**（另有少量 `static` 函数/常量未计入）；总引用约 **900+ 处**。
> 用途：A1“去单例”的批次划分；每个 static 的读写者必须先列全，再决定迁到 `RuntimeConfig`/`TimeState`/`Services`。

## 1. 总表（按引用数降序）

| static | 类型 | 引用 | 文件 | 分类 |
|---|---|---:|---:|---|
| `load_balance_costs_update_algo` | `auto` | 86 | 25 | config |
| `do_single_precision_comms` | `bool` | 78 | 18 | config |
| `n_rz_azimuthal_modes` | `int` | 70 | 23 | cache/compile-time |
| `electromagnetic_solver_id` | `auto` | 66 | 14 | config |
| `nox` | `int` | 53 | 22 | cache/compile-time |
| `gamma_boost` | `amrex::Real` | 43 | 16 | runtime state |
| `sync_nodal_points` | `bool` | 38 | 3 | cache/compile-time |
| `ncomps` | `int` | 31 | 15 | cache/compile-time |
| `field_boundary_hi` | `amrex::Array<FieldBoundaryType,AMREX_SPACEDIM>` | 28 | 8 | config |
| `current_deposition_algo` | `auto` | 27 | 6 | config |
| `moving_window_dir` | `int` | 27 | 13 | runtime state |
| `field_boundary_lo` | `amrex::Array<FieldBoundaryType,AMREX_SPACEDIM>` | 26 | 7 | config |
| `beta_boost` | `amrex::Real` | 21 | 9 | runtime state |
| `grid_type` | `auto` | 19 | 11 | config |
| `boost_direction` | `amrex::Vector<int>` | 17 | 5 | runtime state |
| `fft_do_time_averaging` | `bool` | 17 | 8 | runtime state |
| `electrostatic_solver_id` | `auto` | 16 | 8 | config |
| `do_dive_cleaning` | `bool` | 15 | 6 | config |
| `noz` | `int` | 15 | 4 | cache/compile-time |
| `villasenor_mass_matrices_max_grid_crossings` | `int` | 14 | 3 | cache/compile-time |
| `galerkin_interpolation` | `bool` | 14 | 12 | config |
| `shared_tilesize` | `amrex::IntVect` | 13 | 2 | cache/compile-time |
| `particle_max_grid_crossings` | `int` | 13 | 4 | cache/compile-time |
| `ect_conformal_E` | `int` | 12 | 4 | config |
| `moving_window_v` | `amrex::Real` | 12 | 8 | runtime state |
| `do_divb_cleaning` | `bool` | 11 | 6 | config |
| `use_filter` | `bool` | 10 | 6 | config |
| `use_fdtd_nci_corr` | `bool` | 9 | 4 | config |
| `filter_npass_each_dir` | `amrex::IntVect` | 8 | 4 | config |
| `do_moving_window` | `int` | 8 | 5 | runtime state |
| `particle_pusher_algo` | `auto` | 7 | 4 | config |
| `particle_boundary_lo` | `amrex::Array<ParticleBoundaryType,AMREX_SPACEDIM>` | 7 | 2 | config |
| `particle_boundary_hi` | `amrex::Array<ParticleBoundaryType,AMREX_SPACEDIM>` | 6 | 2 | config |
| `noy` | `int` | 6 | 4 | cache/compile-time |
| `field_centering_noz` | `int` | 6 | 4 | cache/compile-time |
| `field_centering_nox` | `int` | 5 | 4 | cache/compile-time |
| `poisson_solver_id` | `auto` | 5 | 4 | config |
| `ect_conformal_E_Astab` | `amrex::Real` | 4 | 2 | config |
| `macroscopic_subpixel` | `int` | 4 | 2 | config |
| `eb_particle_boundary` | `ParticleBoundaryType` | 4 | 2 | config |
| `do_shared_mem_current_deposition` | `bool` | 4 | 3 | config |
| `field_centering_noy` | `int` | 4 | 4 | cache/compile-time |
| `field_gathering_algo` | `auto` | 3 | 3 | config |
| `serialize_initial_conditions` | `bool` | 3 | 3 | config |
| `do_dynamic_scheduling` | `bool` | 3 | 2 | config |
| `start_moving_window_step` | `int` | 3 | 3 | config |
| `end_moving_window_step` | `int` | 3 | 3 | config |
| `do_shared_mem_charge_deposition` | `bool` | 2 | 2 | config |
| `shared_mem_current_tpb` | `int` | 2 | 2 | cache/compile-time |
| `m_fill_guards_fields` | `amrex::IntVect` | 2 | 1 | cache/compile-time |
| `m_fill_guards_current` | `amrex::IntVect` | 2 | 1 | cache/compile-time |
| `use_kspace_filter` | `bool` | 2 | 2 | config |
| `compute_max_step_from_btd` | `bool` | 2 | 2 | config |
| `refine_plasma` | `bool` | 2 | 2 | config |
| `n_field_gather_buffer` | `int` | 2 | 2 | cache/compile-time |
| `n_current_deposition_buffer` | `int` | 2 | 2 | cache/compile-time |
| `ect_te_borrowing` | `int` | 1 | 1 | config |
| `use_filter_compensation` | `bool` | 1 | 1 | config |
| `zmin_domain_boost_step_0` | `amrex::Real` | 1 | 1 | runtime state |
| `sort_intervals` | `ablastr::utils::text::IntervalsParser` | 1 | 1 | config |
| `sort_bin_size` | `amrex::IntVect` | 1 | 1 | cache/compile-time |
| `charge_deposition_algo` | `auto` | 0 | 0 | config |
| `time_dependency_J` | `auto` | 0 | 0 | config |
| `time_dependency_rho` | `auto` | 0 | 0 | config |
| `sort_particles_for_deposition` | `bool` | 0 | 0 | config |
| `sort_idx_type` | `amrex::IntVect` | 0 | 0 | cache/compile-time |

## 2. 批次划分（建议）

### Batch C：配置（迁到 `RuntimeConfig`）

只读、由输入决定；插件也应只读。先迁引用少、位于叶子模块的（Diagnostics/ReducedDiags），再迁场/粒子。

- `charge_deposition_algo`（0 处 / 0 文件）
- `time_dependency_J`（0 处 / 0 文件）
- `time_dependency_rho`（0 处 / 0 文件）
- `sort_particles_for_deposition`（0 处 / 0 文件）
- `ect_te_borrowing`（1 处 / 1 文件）
- `use_filter_compensation`（1 处 / 1 文件）
- `sort_intervals`（1 处 / 1 文件）
- `do_shared_mem_charge_deposition`（2 处 / 2 文件）
- `use_kspace_filter`（2 处 / 2 文件）
- `compute_max_step_from_btd`（2 处 / 2 文件）
- `refine_plasma`（2 处 / 2 文件）
- `field_gathering_algo`（3 处 / 3 文件）
- `serialize_initial_conditions`（3 处 / 3 文件）
- `do_dynamic_scheduling`（3 处 / 2 文件）
- `start_moving_window_step`（3 处 / 3 文件）
- `end_moving_window_step`（3 处 / 3 文件）
- `ect_conformal_E_Astab`（4 处 / 2 文件）
- `macroscopic_subpixel`（4 处 / 2 文件）
- `eb_particle_boundary`（4 处 / 2 文件）
- `do_shared_mem_current_deposition`（4 处 / 3 文件）
- `poisson_solver_id`（5 处 / 4 文件）
- `particle_boundary_hi`（6 处 / 2 文件）
- `particle_pusher_algo`（7 处 / 4 文件）
- `particle_boundary_lo`（7 处 / 2 文件）
- `filter_npass_each_dir`（8 处 / 4 文件）
- `use_fdtd_nci_corr`（9 处 / 4 文件）
- `use_filter`（10 处 / 6 文件）
- `do_divb_cleaning`（11 处 / 6 文件）
- `ect_conformal_E`（12 处 / 4 文件）
- `galerkin_interpolation`（14 处 / 12 文件）
- `do_dive_cleaning`（15 处 / 6 文件）
- `electrostatic_solver_id`（16 处 / 8 文件）
- `grid_type`（19 处 / 11 文件）
- `field_boundary_lo`（26 处 / 7 文件）
- `current_deposition_algo`（27 处 / 6 文件）
- `field_boundary_hi`（28 处 / 8 文件）
- `electromagnetic_solver_id`（66 处 / 14 文件）
- `do_single_precision_comms`（78 处 / 18 文件）
- `load_balance_costs_update_algo`（86 处 / 25 文件）

### Batch S：运行状态（迁到 `TimeState`/`FrameState`）

每个 step 可能变化；必须由内核维护，插件只读。

- `gamma_boost`（43 处 / 16 文件）
- `moving_window_dir`（27 处 / 13 文件）
- `beta_boost`（21 处 / 9 文件）
- `boost_direction`（17 处 / 5 文件）
- `fft_do_time_averaging`（17 处 / 8 文件）
- `moving_window_v`（12 处 / 8 文件）
- `do_moving_window`（8 处 / 5 文件）
- `zmin_domain_boost_step_0`（1 处 / 1 文件）

### Batch K：缓存/编译期（迁到 `BuildConfig`/`Services`）

有些是编译期常量（`sync_nodal_points`），有些是工作区缓存（`shared_tilesize`、buffer 大小）；不应暴露给插件。

- `n_rz_azimuthal_modes`（70 处 / 23 文件）
- `nox`（53 处 / 22 文件）
- `sync_nodal_points`（38 处 / 3 文件）
- `ncomps`（31 处 / 15 文件）
- `noz`（15 处 / 4 文件）
- `villasenor_mass_matrices_max_grid_crossings`（14 处 / 3 文件）
- `shared_tilesize`（13 处 / 2 文件）
- `particle_max_grid_crossings`（13 处 / 4 文件）
- `noy`（6 处 / 4 文件）
- `field_centering_noz`（6 处 / 4 文件）
- `field_centering_nox`（5 处 / 4 文件）
- `field_centering_noy`（4 处 / 4 文件）
- `shared_mem_current_tpb`（2 处 / 2 文件）
- `m_fill_guards_fields`（2 处 / 1 文件）
- `m_fill_guards_current`（2 处 / 1 文件）
- `n_field_gather_buffer`（2 处 / 2 文件）
- `n_current_deposition_buffer`（2 处 / 2 文件）
- `sort_bin_size`（1 处 / 1 文件）
- `sort_idx_type`（0 处 / 0 文件）

## 3. 迁移策略

1. **先建类型，不换实现**：`RuntimeConfig`（只读）、`TimeState`（step/time/dt/substep）、`FrameState`（boost/moving window）、`Services`（RNG/parser/reduce）。
2. **按文件批次替换**：每批只动一个子系统（例如 `Diagnostics/ReducedDiags`），把 `WarpX::name` 换成 `ctx.config().name` 或 `ctx.time().name`。
3. **每批编译 + 冒烟**：`test_2d_langmuir_multi.run/analysis` + 相关子系统测试；逐位/数值门。
4. **禁止“全局 getter 换名”**：如果只是把 `WarpX::nox` 换成 `Runtime::nox()`，单例还在，等于没做。

## 4. 第一刀建议

- 首选 **`Diagnostics/ReducedDiags`**：引用少、无 GPU kernel、易回归。
- 次选 **`Diagnostics/FullDiagnostics`** 的 field functor 初始化。
- 场求解器/粒子推进最后动，因为它们同时依赖 config + runtime state + cache。
