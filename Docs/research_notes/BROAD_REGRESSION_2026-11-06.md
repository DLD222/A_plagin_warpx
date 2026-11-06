# 2D 全量回归（2026-11-06）

> 目的：在 A1–A3.3 改动后，用官方 2D 算例做广泛数值验收，区分“代码回归”和“环境缺件”。

## 1. 构建

| 构建 | 配置 |
|---|---|
| `build` | 2D, EB, MPI+OMP, QED, openPMD **OFF** |
| `build_fft` | 同上 + FFT/PSATD |
| `build_asan` | 2D, Debug, `AMReX_USE_ASAN=ON` |

## 2. 默认构建

### run 测试

```bash
ctest --test-dir build -R '\.run$' -j 4 --output-on-failure
```

```
71 / 83 PASS
```

12 个失败全部为环境缺件：

| 类别 | 测试 |
|---|---|
| 缺少 `warpx-data/MCC_cross_sections` | `charge_exchange_dsmc`, `embedded_boundary_em_particle_absorption_sh_factor_1/2/3`, `embedded_circle`, `background_mcc` |
| openPMD 未编译 | `collisions_split_momentum_push_electrostatic`, `..._electromagnetic`, `..._electromagnetic_vay`, `qed_breit_wheeler_opmd`, `rigid_injection_btd`, `laser_ion_acc` |

### analysis 测试

```
58 / 68 PASS
```

10 个失败与上表一一对应（`embedded_circle`、`background_mcc` 无 analysis 或对应 run 已失败）。

## 3. FFT 构建

```bash
LD_LIBRARY_PATH="$HOME/third_party/fftw/lib:$LD_LIBRARY_PATH" \
ctest --test-dir build_fft -R '\.run$' -j 3 --output-on-failure
```

```
95 / 107 PASS
```

12 个失败仍是同一组环境缺件（openPMD/MCC 数据）。

FFT analysis 还存在一组 **Galilean PSATD** 失败：

```
test_2d_galilean_psatd
test_2d_averaged_galilean_psatd
test_2d_averaged_galilean_psatd_hybrid
test_2d_galilean_psatd_current_correction
test_2d_galilean_psatd_current_correction_psb
test_2d_pml_x_galilean
```

典型：

```
err_energy = 15.906565729487289
tol_energy = 1e-8
```

已用 `if (false)` 临时禁用所有 PluginHost handler 复跑，**误差不变**，
说明不是 push/finalize/field-solve handler 路径导致。该组作为独立问题继续排查，
不能算环境缺件，也不能算通过。

## 4. 本轮修复的真实 bug

`SmartUtils::getPolicies` 对用户自定义粒子属性（如 `orig_z`）直接 abort：

```
No initialization policy registered for particle component 'orig_z'
```

修复：

```cpp
h_policies[kv.second] = (policy_it != end) ? policy_it->second
                                           : InitializationPolicy::Zero;
```

用户属性默认零初始化；QED 的 `opticalDepth*` 仍在静态表中使用 `RandomExp`。

修复后以下测试恢复：

```
test_2d_ionization_lab
test_2d_ionization_lab_restart
test_2d_ionization_boost
test_2d_proton_boron_fusion
test_2d_proton_boron_fusion_modulus_shuffle
test_2d_qed_breit_wheeler
test_2d_qed_quantum_sync
```

## 5. 定量结果（与解析/参考值）

| 算例 | 量 | 结果 |
|---|---|---|
| `test_2d_langmuir_multi` | Ex/Ez vs 解析 | **2.88e-2** |
| `test_2d_langmuir_fluid` | Ex/Ez | **1.87e-2** |
| | Jx/Jz | **1.35e-2** |
| | rho | **1.42e-2** |
| `test_2d_pml_x_yee` | error_rel | **7.63e-5** |
| `test_2d_space_charge_initialization` | Ex | **0.071**（tol 0.165） |
| | Ey | **0.073**（tol 0.165） |
| `test_2d_collision_xz` | 拟合残差 | **4.66e-4**（tol 1e-3） |

## 6. ASAN

```bash
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build_asan \
  -R '^(runtime_|test_2d_collision_xz\.(run|analysis)$)' --output-on-failure
```

```
7/7 PASS
```

无 use-after-free / 越界。

## 7. 结论

- 官方 2D 算例绝大多数通过；
- 剩余失败已分类为 **openPMD 未编译** 与 **warpx-data 缺失**，不是本架构改动引入；
- 修复了真实的用户属性初始化策略 bug；
- Galilean PSATD 的 analysis 异常仍需单独排查，不能隐藏；
- 该结果不能替代 A/B 逐位门。

## 8. 复现命令

```bash
# 默认
ctest --test-dir build -R '\.run$' -j 4 --output-on-failure
ctest --test-dir build -R '\.analysis$' -j 4 --output-on-failure

# FFT
LD_LIBRARY_PATH="$HOME/third_party/fftw/lib:$LD_LIBRARY_PATH" \
ctest --test-dir build_fft -R '\.run$' -j 3 --output-on-failure
```

## 9. 待办

1. `warpx-data` 子模块缺失导致 6 个 MCC/embedded 测试无法运行；
2. `WarpX_OPENPMD=OFF` 导致 6 个 openPMD 输出测试无法运行；
3. Galilean PSATD analysis 的 `energy_ref` 比值异常，需与上游基线对比；
4. 全量 A/B 逐位门仍属于 A5。
