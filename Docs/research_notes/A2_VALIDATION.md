# A2 回归验证记录

> 日期：2026-11-06  
> 目标：验证 A2.1–A2.4 的 LevelResources / ClearLevel / RemakeLevel / FieldSchema 改动。

## 1. 默认构建

```bash
ctest --test-dir build -R \
 '^(runtime_|test_2d_langmuir_multi_mr\.(run|analysis)|test_2d_embedded_boundary_cube\.(run|analysis)|test_2d_pml_x_yee\.(run|analysis)$)' \
 --output-on-failure
```

```
11/11 PASS
```

覆盖：

- runtime 5 项（snapshot/particles/BTD/ES/LB）
- 多层 regrid：`test_2d_langmuir_multi_mr`
- EB：`test_2d_embedded_boundary_cube`
- PML：`test_2d_pml_x_yee`

## 2. FFT 构建

```bash
LD_LIBRARY_PATH="$HOME/third_party/fftw/lib:$LD_LIBRARY_PATH" \
ctest --test-dir build_fft -R \
 '^(runtime_|test_2d_pml_x_psatd(_restart)?\.(run|analysis)|test_2d_langmuir_multi_psatd\.(run|analysis)$)' \
 --output-on-failure
```

```
13/13 PASS
```

覆盖：

- PSATD runtime 两项
- PSATD PML 与 PML restart
- PSATD Langmuir

## 3. ASAN

```bash
cmake -S . -B build_asan \
  -DWarpX_DIMS=2 -DWarpX_EB=ON -DWarpX_MPI=ON -DWarpX_COMPUTE=OMP \
  -DWarpX_PYTHON=OFF -DWarpX_OPENPMD=OFF -DWarpX_FFT=OFF \
  -DCMAKE_BUILD_TYPE=Debug -DAMReX_USE_ASAN=ON

ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build_asan \
  -R '^(runtime_|test_2d_embedded_boundary_cube\.(run|analysis)$)' \
  --output-on-failure
```

```
7/7 PASS
```

未报告 use-after-free / 越界。

## 4. FieldSchema 覆盖

当前 `AllocLevelField*` 约 **50** 个字段，占 128 个 `alloc_init` 的约 40%。

已覆盖：

```
base E/B/J
E_old / current_fp_nodal / current_fp_vay
averaging E/B
edge_lengths / face_areas / area_mod / Venl / ECTRhofield
rho_fp / phi_fp / distance_to_eb / dual_*
current_store / current_fp_non_suborbit
```

未覆盖：

```
Bfield_aux / Efield_aux（多种条件/BA）
F_fp / G_fp
rho_medium_fp
material 字段
ECT iMultiFab（m_flag_info_face / m_flag_ext_face）
m_eb_update_* / m_eb_reduce_particle_shape（不走 m_fields）
buffer masks
```

## 5. 结论

- LevelResources 的资源所有权与生命周期通过 EB/PML/MR/PSATD 回归；
- FieldSchema 只覆盖简单 MultiFab 字段，aux/条件字段尚未迁移；
- A2.4 不能声称“128 处全部 schema 化”；
- A2.5 的多层/EB/PML/PSATD/LB 目标已验证，但 A2.4 仍为进行中。

## 6. 剩余风险

1. `Bfield_aux/Efield_aux` 有多种 BA（`nba` 与 `ba`），不能直接套当前 helper；
2. `m_eb_update_*` 是 iMultiFab，不属于 `m_fields`；
3. material/macroscopic 字段由独立类管理，不应强塞 schema；
4. 下一步应决定：扩展 helper 支持显式 BA，还是把这些归为“非 schema 资源”。
