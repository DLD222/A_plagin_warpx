# 验收运行记录（A1.8）

> 日期：2026-11-05  
> 构建：2D EB/MPI/OMP/QED，Release；FFT 构建使用本地 FFTW 3.3.10（见 `FFTW_SETUP.md`）  
> 说明：按仓库约定忽略 `*.checksum`（平台相关），只跑 `run + analysis`。

## 1. Runtime 自测

### 默认构建

```bash
ctest --test-dir build -R '^runtime_' --output-on-failure
```

```
runtime_snapshot_2
runtime_snapshot_particles_2
runtime_es_2
runtime_lb_2
4/4 PASS
```

### FFT 构建

```bash
LD_LIBRARY_PATH="$HOME/third_party/fftw/lib:$LD_LIBRARY_PATH" \
ctest --test-dir build_fft -R '^runtime_' --output-on-failure
```

```
runtime_snapshot_2
runtime_snapshot_particles_2
runtime_es_2
runtime_psatd_2
runtime_psatd_check_2
runtime_lb_2
6/6 PASS
```

## 2. 真实物理算例

### 2.1 2D Langmuir（Yee/PIC）

```bash
ctest --test-dir build -R '^test_2d_langmuir_multi\.(run|analysis)$' --output-on-failure
```

结果：

```
test_2d_langmuir_multi.run       PASS
test_2d_langmuir_multi.analysis  PASS
```

用途：验证 Yee/FDTD + 粒子推进 + 诊断在 Runtime context 下的完整时间步。

### 2.2 2D 碰撞（collision 模块）

```bash
ctest --test-dir build -R '^test_2d_collision_xz\.(run|analysis)$' --output-on-failure
```

结果：

```
test_2d_collision_xz.run       PASS
test_2d_collision_xz.analysis  PASS
```

用途：验证 `Source/Particles/Collision` 迁移后仍保持碰撞物理。

### 2.3 2D 流体 Langmuir（WarpXFluidContainer）

```bash
ctest --test-dir build -R '^test_2d_langmuir_fluid\.(run|analysis)$' --output-on-failure
```

结果：

```
test_2d_langmuir_fluid.run       PASS
test_2d_langmuir_fluid.analysis  PASS
```

用途：验证 `WarpXFluidContainer` 的 context/geometry 迁移没有破坏流体推进。

### 2.4 2D PSATD Langmuir（FFT）

```bash
LD_LIBRARY_PATH="$HOME/third_party/fftw/lib:$LD_LIBRARY_PATH" \
ctest --test-dir build_fft -R '^test_2d_langmuir_multi_psatd\.(run|analysis)$' --output-on-failure
```

结果：

```
test_2d_langmuir_multi_psatd.run       PASS
test_2d_langmuir_multi_psatd.analysis  PASS
```

用途：验证 PSATD context 链与本地 FFTW 构建。

## 3. 定量结果

| 算例 | 量 | 结果 |
|---|---|---|
| `test_2d_langmuir_multi` | `Ex` vs 解析解最大相对误差 | **2.88e-2** |
| | `Ez` vs 解析解最大相对误差 | **2.88e-2** |
| `test_2d_langmuir_multi_psatd` | `Ex/Ez` vs 解析解 | **2.88e-2** |
| `test_2d_langmuir_fluid` | `Ex/Ez` | **1.87e-2** |
| | `Jx/Jz` | **1.35e-2** |
| | `rho` | **1.42e-2** |
| `test_2d_collision_xz` | analysis 误差 | **4.66e-4** |
| `runtime_psatd_check_2` | 场能量守恒 | **1.77083756376e-11 J**（step 0/1 一致） |

这些数值来自当前 `build` / `build_fft` 实际运行，不是“测试退出码为 0”本身。

## 4. 结论

- Runtime 自测：默认 4/4、FFT 6/6；
- 四个真实物理算例（Yee、碰撞、流体、PSATD）的 `run + analysis` 全部通过；
- checksum 未作为门禁（平台相关），由数值 analysis 承担验收。
