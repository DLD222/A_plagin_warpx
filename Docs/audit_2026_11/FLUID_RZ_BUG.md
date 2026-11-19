# 已知问题：RZ 几何下流体物种会数值爆炸（NaN）

状态：**未修复**（已加兜底，不再静默写 NaN）

## 复现
```
bin/warpx.rz Examples/Tests/langmuir_fluids/inputs_test_rz_langmuir_fluid
```
上游 26.11 跑完输出正常；fork 跑到第 80 步时 `rho/E/j/B` **整场为 NaN**。

## 证据
| 步 | fork max|rho| | 参考版 max|rho| |
|---|---|---|---|
| 1 | 6.5e2 | ~1e2 量级 |
| 10 | 1.1e6 | 正常 |
| 80 | NaN（8192/8192） | 8.5e8（正常） |

第 2D(XZ) 与 1D 的同名算例两边一致，只有 RZ 发散，说明问题在柱坐标曲率项
（`centrifugal_source_rz` / RZ 源项 `N*Vx/r` / `RadialVolumeFactor`）。

## 已做的兜底（本轮）
`MultiDiagnostics::FilterComputePackFlush` 与 `...LastTimestep` 在写盘前用
`MultiFab::contains_nan` 检查；一旦发现 NaN 直接 `amrex::Abort` 并指明诊断名与步数。
（注意：`MultiFab::min/max` 会忽略 NaN，所以不能用它们做检查。）

这样用户看到的是明确报错，而不是"跑完了、图全黑"。

## 后续
需要逐项核对 RZ 曲率项的半径取值（单元中心应是 (i+1/2)dr）与轴边界条件；
一次试验表明改用单元中心半径仍会发散，因此不是单点问题，需要与上游实现逐行对照。
