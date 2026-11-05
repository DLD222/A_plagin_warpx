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

## 3. 解析解与逐项对比

### 3.1 Langmuir 波（Yee / PSATD）

线性冷等离子体波，2D 周期边界：

```text
E_x = ε (m_e c^2 k_x / q_e) sin(k_x x) cos(k_z z) sin(ω_p t)
E_z = ε (m_e c^2 k_z / q_e) cos(k_x x) sin(k_z z) sin(ω_p t)
```

参数（与 `inputs_base_2d` 一致）：

```text
ε = 0.01
n = 4.0e24 m^-3
k_x = k_z = 2π·2 / 40e-6 = 3.14159265e5 m^-1
ω_p = sqrt(n e² / (ε0 m_e)) = 1.1282920450858998e14 s^-1
```

最后一个 plotfile：

```text
t = 5.896635841874201e-14 s
ω_p t = 6.653127313155059 rad
sin(ω_p t) = 0.36156136194193156
理论幅值 A = ε m_e c² k / e = 5.804327310661349e8 V/m
理论 max|E| = 5.790352599418917e8 V/m
```

| 求解器 | 量 | 模拟 | 理论 | 绝对误差 | 相对 max | 相对 L2 |
|---|---|---:|---:|---:|---:|---:|
| Yee | `max|Ex|` | 5.623383090208904e8 | 5.790352599418917e8 | 1.669695092103386e7 | **2.8836e-2** | **2.7425e-2** |
| Yee | `max|Ez|` | 5.623383090208758e8 | 5.790352599418917e8 | 1.669695092104971e7 | **2.8836e-2** | **2.7425e-2** |
| PSATD | `max|Ex|` | 5.623374560412297e8 | 5.790352599418917e8 | 1.669792331839085e7 | **2.8837e-2** | **2.7425e-2** |

### 3.2 流体 Langmuir

同一解析电场，另外：

```text
J_x = -ε0 ω_p ε (m_e c² k_x / e) cos(ω_p t - ω_p dt/2) sin(k_x x) cos(k_z z)
ρ   =  ε0 ε (m_e c² (k_x²+k_z²) / e) sin(ω_p t) cos(k_x x) cos(k_z z)
```

在 `t = 5.89663584e-14 s`：

| 量 | 模拟 max | 理论 max | 绝对误差 | 相对 max | 相对 L2 |
|---|---:|---:|---:|---:|---:|
| `Ex/Ez` | 5.682241265376422e8 | 5.790352599418917e8 | 1.081141040776277e7 | **1.8671e-2** | **1.7249e-2** |
| `Jx/Jz` | 1.513818488532997e12 | 1.534561969713937e12 | 2.074348118094214e10 | **1.3518e-2** | **1.2745e-2** |
| `rho` | 3.187318935362782e3 | 3.2213180725859997e3 | 4.584147869451999e1 | **1.4231e-2** | **1.6919e-2** |

### 3.3 碰撞算例

`test_2d_collision_xz` 的 analysis 比较的是速度差对指数拟合的残差，不是解析解：

```text
mean |v_x,e - v_x,i - a exp(b j)| = 4.662553985475136e-4
```

因此它是“数值衰减拟合误差”，不能当成解析解误差引用。

### 3.4 Runtime 能量守恒

`runtime_psatd_check_2`：

```text
step 0: E = 1.77083756376000e-11 J
step 1: E = 1.77083756376000e-11 J
```

这是精确的解析值 `0.5 ε0 E² V`（E=1 V/m，V=4 m²·1 m）。

## 4. 结论

- Runtime 自测：默认 4/4、FFT 6/6；
- 四个真实物理算例（Yee、碰撞、流体、PSATD）的 `run + analysis` 全部通过；
- checksum 未作为门禁（平台相关），由数值 analysis 承担验收。
