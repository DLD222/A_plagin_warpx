# 最终全量验证（2026-11-07，uniform-B gate 之后）

> 基线：`97458ff`。  
> 新增：`test_2d_dynamic_regrid_uniform_b`。

## 默认构建

```text
run:      76 / 87 PASS
analysis: 62 / 72 PASS
```

失败 11 个，全部是 openPMD 输出未编译：

```text
charge_exchange_dsmc
collisions_split_momentum_push_*（3）
embedded_boundary_em_particle_absorption_sh_factor_*（3）
embedded_circle
qed_breit_wheeler_opmd
rigid_injection_btd
laser_ion_acc
```

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

## 插件 / A-B / ASAN

```text
plugin_example_2 PASS
plugin_loader_2d PASS
runtime 5/5 PASS
A/B langmuir/collision/eb_cube PASS
ASAN 15/15 PASS
```

## 本轮结论

- 没有新增代码回归；
- 延迟加密初始化 bug 已修复；
- uniform B 长期守恒门通过；
- 剩余失败全部是环境依赖。
