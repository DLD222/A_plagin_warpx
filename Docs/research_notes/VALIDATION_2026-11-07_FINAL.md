# 最终全量验证（2026-11-07，A6.1 之后）

> 基线：`ce2f10e`（A6a + A6.1）。  
> 新增测试：`dynamic_regrid`、`dynamic_regrid_remove`、`dynamic_regrid_remake`。

## 1. 默认构建

```text
run:      75 / 86 PASS
analysis: 61 / 71 PASS
```

失败 11 个，仍全部是 openPMD 输出未编译：

```text
charge_exchange_dsmc
collisions_split_momentum_push_*（3）
embedded_boundary_em_particle_absorption_sh_factor_*（3）
embedded_circle
qed_breit_wheeler_opmd
rigid_injection_btd
laser_ion_acc
```

## 2. FFT 构建

```text
run:      99 / 110 PASS
analysis: 83 / 93 PASS
```

失败同一组 openPMD 依赖。

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

本机 C++ openPMD 缺 parallel HDF5。

## 4. 修改功能 + 动态 regrid 定向验证

```text
33 / 33 PASS
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
dynamic_regrid / remove / remake
```

## 5. 插件 / A-B / ASAN

```text
plugin_example_2 PASS
plugin_loader_2d PASS
runtime 5/5 PASS
A/B langmuir PASS
A/B collision PASS
A/B eb_cube PASS
ASAN 15/15 PASS
```

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

## 7. 本轮结论

- 没有发现新增代码回归；
- 动态 regrid 的创建、销毁、已有 level BA 改变均通过；
- 剩余失败全部是环境依赖：
  - 默认/FFT 未启用 openPMD；
  - openPMD 构建缺 parallel HDF5；
- 没有把环境失败算作代码通过。

## 8. 已知限制

- `dynamic_regrid_remake` 关闭粒子输出；
- 已有 level 改变 BA 时，`FullDiagnostics` 的粒子输出 buffer 尚未重建；
- 该限制已写入 `A6.1_REMAKELEVEL.md`。
