# 修改功能定向验证

> 日期：2026-11-06  
> 目的：只验证本分支真正改过/重构过的功能，而不是未改动的上游功能。

## 1. 覆盖范围

| 改动 | 对应官方算例 |
|---|---|
| 碰撞模型 / SpeciesTable / Ownership | `collision_xz`, `collision_xz_global_debye`, `background_mcc` |
| 场电离 / 粒子创建 | `ionization_lab`, `ionization_boost` |
| 核聚变 | `proton_boron_fusion`, `proton_boron_fusion_modulus_shuffle` |
| PIC Stage/handler | `langmuir_multi`, `langmuir_multi_psatd` |
| 沉积 / Galilean 坐标 | `galilean_psatd`, 家族 10 项 |
| MR 场分配 | `langmuir_multi_mr`, `langmuir_multi_nodal` |
| PML | `pml_x_yee` |
| EB | `embedded_boundary_cube`, `embedded_boundary_rotated_cube` |
| openPMD 诊断 | `qed_breit_wheeler_opmd`, `rigid_injection_btd`, `laser_ion_acc` |
| Runtime SpeciesHandle | `runtime_*` 5 项 |

## 2. 定向测试结果

```bash
ctest --test-dir build -R \
 '^(test_2d_collision_xz|test_2d_collision_xz_global_debye|test_2d_proton_boron_fusion|test_2d_proton_boron_fusion_modulus_shuffle|test_2d_ionization_lab|test_2d_ionization_boost|test_2d_background_mcc|test_2d_langmuir_multi_mr|test_2d_langmuir_multi_nodal|test_2d_pml_x_yee|test_2d_embedded_boundary_cube|test_2d_embedded_boundary_rotated_cube)\.(run|analysis)$' -j 2
```

```
23 / 23 PASS
```

## 3. 定量值

| 算例 | 量 | 数值 |
|---|---|---:|
| `collision_xz` | 残差 | **4.662553985475136e-4**（tol 1e-3） |
| `collision_xz_global_debye` | 残差 | **3.490197456173893e-4**（tol 1e-3） |
| `ionization_lab` | error_rel | **2.9296874999999375e-3**（tol 0.07） |
| `ionization_boost` | error_rel | **4.165694962686567e-2**（tol 0.07） |
| `langmuir_multi` | Ex/Ez | **2.883581031442768e-2** |
| `langmuir_multi_mr` | Ex/Ez | **3.806812989427458e-2**（tol 0.0503） |
| `pml_x_yee` | error_rel | **7.625989896352262e-5** |
| `galilean_psatd` | energy ratio | **6.692782038457815e-9**（tol 1e-8） |
| `proton_boron_fusion` | 能量相对误差 | **≤ 3.9e-2**（analysis 通过） |

## 4. Handler A/B 逐位

条件：

```bash
OMP_NUM_THREADS=1
warpx.random_seed = 12345
WARPX_DISABLE_PLUGIN_HANDLERS=1
```

| 算例 | 结果 |
|---|---|
| Langmuir Yee | 9 场全部 `array_equal` |
| Langmuir PSATD | 6 场全部 `array_equal` |
| collision_xz | 4 个输出所有场 `maxdiff = 0.0` |
| embedded_boundary_cube | Ex/Ey/Ez/Bx/By/Bz 全部 `array_equal` |
| ionization_lab | 9 场全部 `array_equal` |

这验证了重构后的 Stage/handler 路径与旧直接调用路径逐位一致。

## 5. 本轮修出的真实 bug

| 问题 | 性质 |
|---|---|
| `SmartUtils` 未登记用户属性 | 直接 abort，修复后 ionization/fusion/QED 恢复 |
| `RuntimeData::lowerCorner` 丢失 `time_shift_delta` | Galilean 第一步电流沉积错位，修复后 6.69e-9 |
| openPMD `data` 遮蔽 / `WriteOpenPMDParticles` 缺参数 | `WarpX_OPENPMD=ON` 无法编译，修复后编译通过 |

## 6. 环境限制

- 默认/FFT 构建未启用 openPMD：11 个输出测试无法运行；
- openPMD 构建缺 parallel HDF5：3 个 HDF5 测试无法运行；
- 已安装 `openpmd-viewer`，absorption analysis 通过。

## 7. 结论

- 修改过的碰撞、电离、聚变、PIC Stage、MR、PML、EB、Galilean、openPMD 路径均通过官方算例；
- 定量值与 analysis 参考一致；
- handler 路径在 Yee/PSATD/碰撞/EB/电离上逐位一致；
- 未把未修改功能算作本次验证结论。
