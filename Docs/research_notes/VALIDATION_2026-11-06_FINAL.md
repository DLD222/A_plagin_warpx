# 最终全量验证（2026-11-06）

> 基线：A5 完成后的 `main`。  
> 目的：检查当前架构下的官方 2D 算例、修改功能、插件路径与 A/B 一致性。

## 1. 默认构建

```text
run:      72 / 83 PASS
analysis: 58 / 68 PASS
```

失败 11 个，全部是 openPMD 输出未编译：

```text
charge_exchange_dsmc
collisions_split_momentum_push_electrostatic
collisions_split_momentum_push_electromagnetic
collisions_split_momentum_push_electromagnetic_vay
embedded_boundary_em_particle_absorption_sh_factor_1
embedded_boundary_em_particle_absorption_sh_factor_2
embedded_boundary_em_particle_absorption_sh_factor_3
embedded_circle
qed_breit_wheeler_opmd
rigid_injection_btd
laser_ion_acc
```

## 2. FFT 构建

```text
run:      96 / 107 PASS
analysis: 80 / 90 PASS
```

失败同一组 openPMD 依赖。

Galilean 家族全部通过。

## 3. openPMD 构建

```text
16 / 22 PASS
```

仅剩 3 个 HDF5 测试：

```text
qed_breit_wheeler_opmd
rigid_injection_btd
laser_ion_acc
```

原因：本机 C++ openPMD 缺 parallel HDF5。

## 4. 修改功能定向验证

```text
27 / 27 PASS
```

覆盖：

```text
collision_xz / global_debye
ionization_lab / boost
proton_boron_fusion / modulus_shuffle
background_mcc
langmuir_multi_mr / nodal
pml_x_yee
embedded_boundary_cube / rotated_cube
ionization_lab_restart
macroscopic_solver
```

## 5. 插件与 A/B

```text
plugin_example_2 PASS
plugin_loader_2d PASS
runtime 5/5 PASS
```

自动 A/B：

```text
A/B PASS langmuir
A/B PASS collision
A/B PASS eb_cube
```

ASAN：

```text
9 / 9 PASS
```

包含 runtime、plugin、collision。

## 6. 定量值

| 算例 | 量 | 数值 |
|---|---|---:|
| `langmuir_multi` | Ex/Ez | **2.883581031442768e-2** |
| `langmuir_fluid` | Ex/Ez | **1.86714197833958e-2** |
| | Jx/Jz | **1.35e-2** |
| | rho | **1.42e-2** |
| `pml_x_yee` | error_rel | **7.625989896352262e-5** |
| `space_charge_initialization` | Ex | **0.071** |
| | Ey | **0.073** |
| `collision_xz` | 残差 | **4.662553985475136e-4** |
| `collision_xz_global_debye` | 残差 | **3.490197456173893e-4** |
| `ionization_lab` | error_rel | **2.9296874999999375e-3** |
| `ionization_boost` | error_rel | **4.165694962686567e-2** |
| `langmuir_multi_mr` | Ex/Ez | **3.806812989427458e-2** |
| `galilean_psatd` | energy ratio | **6.692782038457815e-9** |

## 7. 本轮未发现的新代码 bug

全量运行后没有新增代码回归。

已确认的剩余失败全部是环境依赖：

- 默认/FFT 未启用 openPMD；
- openPMD 构建缺 parallel HDF5。

## 8. 已修复的历史 bug（同一分支）

- `SmartUtils` 用户属性策略缺失；
- Galilean `lowerCorner` 丢失 `time_shift_delta`；
- openPMD `data` 遮蔽与 `WriteOpenPMDParticles` 缺参数；
- `Services` 空接口；
- `FieldSchema` 冲突比较遗漏元数据；
- `PluginHost` spec 未登记、双写未检、聚合字段依赖未匹配；
- runtime 测试共享目录。

## 9. 未关闭项

- A6a 动态 regrid：试验确认三处阻塞，代码已回滚；
- H3：6 处 Python legacy fallback；
- M10：runtime 测试未改用 `add_warpx_test`。
