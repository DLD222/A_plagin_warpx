# Handler 路径 A/B 逐位验证

> 日期：2026-11-06  
> 目的：验证 `StagePlan + PluginHost` 路径与旧直接调用路径在确定性条件下**逐位一致**。

## 1. 方法

新增环境变量：

```bash
WARPX_DISABLE_PLUGIN_HANDLERS=1
```

设置后 Runtime 不注册 PIC/collision handler，`Run*Stage` 回退到旧直接调用。

A/B 必须同时满足：

```bash
OMP_NUM_THREADS=1
warpx.random_seed = <fixed>
```

原因：默认 OpenMP 下粒子采样/归约顺序使同一输入两次运行都不逐位一致；
先验证 off/off 可复现，才能比较 on/off。

## 2. Langmuir（Yee）

输入：`Examples/Tests/langmuir/inputs_test_2d_langmuir_multi`  
附加：

```
warpx.random_seed = 12345
OMP_NUM_THREADS=1
```

比较 `diag1000080`：

```
Ex Ey Ez Bx By Bz jx jy jz -> 全部 array_equal
```

## 3. Langmuir（PSATD）

输入：`inputs_test_2d_langmuir_multi_psatd`  
FFT 构建，`OMP_NUM_THREADS=1`，seed 12345。

比较 `diag1000080`：

```
Ex Ey Ez jx jy jz -> 全部 array_equal
```

## 4. 碰撞

输入：`Examples/Tests/collision/inputs_test_2d_collision_xz`  
seed 2034958209，OMP=1。

比较 step 150 的：

```
diag1000150
diag_parser_filter000150
diag_random_filter000150
diag_uniform_filter000150
```

所有字段 `maxdiff = 0.0`。

## 5. 结论

在确定性条件下：

- PIC push/finalize/field-solve handler 与旧路径逐位一致；
- CollisionHandlerAdapter 与旧 `mypc->doCollisions` 逐位一致；
- 之前观察到的 1e-6 差异来自 OpenMP 非确定性，不是 handler 语义差异。

## 6. 限制

- 该验证不是全量 A/B；只覆盖 Yee、PSATD、碰撞三个代表路径；
- 不能替代 A5 的全量逐位门；
- `WARPX_DISABLE_PLUGIN_HANDLERS` 是调试开关，不应在正式运行中使用。
