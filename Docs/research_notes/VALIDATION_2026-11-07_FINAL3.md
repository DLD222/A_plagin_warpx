# 最终全量验证（2026-11-07，ABI 加固 + uniform-B 之后）

> 基线：`45d9557`。

## 默认构建

```text
run:      76 / 87 PASS
analysis: 62 / 72 PASS
```

失败 11 个，全部是 openPMD 输出未编译。

## FFT 构建

```text
run:      100 / 111 PASS
analysis: 84 / 94 PASS
```

失败同一组 openPMD 依赖。

## openPMD 构建

```text
16 / 22 PASS
```

仅剩 3 个 HDF5 测试：

```text
qed_breit_wheeler_opmd
rigid_injection_btd
laser_ion_acc
```

## 修改功能 + 动态 regrid

```text
35 / 35 PASS
```

包含：

```text
collision / ionization / fusion / MCC
langmuir_multi_mr / nodal
pml_x_yee
embedded_boundary_cube / rotated_cube
ionization_lab_restart
macroscopic_solver
dynamic_regrid / remove / remake / uniform_b
```

## 插件 / A-B / ASAN

```text
plugin + runtime 11/11 PASS
A/B langmuir/collision/eb_cube PASS
ASAN 26/26 PASS
```

## 定量值

| 算例 | 量 | 数值 |
|---|---|---:|
| Langmuir | Ex/Ez | **2.883581031442768e-2** |
| Langmuir fluid | Ex/Ez | **1.86714197833958e-2** |
| | Jx/Jz | **1.35e-2** |
| | rho | **1.42e-2** |
| PML | error_rel | **7.625989896352262e-5** |
| Space charge | Ex | **0.071** |
| | Ey | **0.073** |
| Collision | 残差 | **4.662553985475136e-4** |
| Global debye | 残差 | **3.490197456173893e-4** |
| Ionization lab | error_rel | **2.9296874999999375e-3** |
| Ionization boost | error_rel | **4.165694962686567e-2** |
| Langmuir MR | Ex/Ez | **3.806812989427458e-2** |
| Galilean | energy ratio | **6.692782038457815e-9** |

## 动态 regrid

```text
test_2d_dynamic_regrid             PASS
test_2d_dynamic_regrid_remove      PASS
test_2d_dynamic_regrid_remake      PASS
test_2d_dynamic_regrid_uniform_b   PASS
```

uniform B：

```text
step 15..40 总场能恒为 4096
rel < 1e-12
```

## 本轮结论

- 没有新增代码回归；
- 插件 ABI v2 与失败路径测试全部通过；
- 剩余失败全部是环境依赖；
- 没有把环境失败算作代码通过。
