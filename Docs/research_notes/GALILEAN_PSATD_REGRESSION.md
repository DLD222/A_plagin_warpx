# Galilean PSATD 回归定位

> 日期：2026-11-06  
> 结论：这是本分支引入的真实回归，不是环境问题，也不是 handler 路径问题。

## 1. 基线对照

| 版本 | 结果 |
|---|---|
| 基线 `7bcb44a`（上游快照） | `test_2d_galilean_psatd` **run + analysis 通过** |
| 当前 `ded1e56` | run 通过，analysis `err_energy = 15.906565729487289` |

基线用同一 AMReX commit：

```
008b67375045d2154a6b1295994a330eef4ad822
```

同一 FFTW、同一输入、同一 Release 构建。

## 2. 不是 handler 路径

在 StagePlan 已真正走 `Run*Stage` 之后，设置：

```bash
WARPX_DISABLE_PLUGIN_HANDLERS=1
OMP_NUM_THREADS=1
```

当前版本仍得到：

```
ratio = 15.906565729487289
```

因此与 PluginHost / CollisionHandlerAdapter 无关。

## 3. 不是初始化

比较 `diag1000000`：

```
Ex Ey Ez Bx By Bz  maxdiff = 0.0
```

初始场完全一致。

## 4. 第一 step 已发散

把 `max_step` 改为 2、diagnostics 每步输出后：

| step | maxdiff Ex | maxdiff Ez |
|---:|---:|---:|
| 0 | 0.0 | 0.0 |
| 1 | **6.14** | **2.61** |
| 2 | **7.76** | **5.42** |

说明问题发生在**第一个时间步的推进**，不是长期累积。

## 5. 排查方向

Galilean PSATD 在第一步涉及：

```
PushParticlesandDeposit
SyncCurrentAndRho / FinalizeRho / FinalizeJ
PushPSATD
```

由于标准 PSATD Langmuir 用例逐位通过，重点应放在：

1. `PhysicalParticleContainer` 粒子推进 / 电流沉积；
2. `WarpXParticleContainer` 的几何量（`lowerCorner` / `invCellSize` / `refRatio`）；
3. `SpectralSolver` 中 Galilean 系数的输入；
4. `m_v_galilean` 传递是否被截断或清零。

日志已确认：

```
v_galilean = (0,0,-298289729.4)
```

两版一致，因此不是输入解析问题。

## 6. 当前处理

- **不把该测试标记为通过**；
- 不修改 analysis 容差；
- 下一步用逐提交 bisect 或对 `PhysicalParticleContainer` 做逐函数 A/B。

## 7. 复现

```bash
cd build_fft/bin/test_2d_galilean_psatd
LD_LIBRARY_PATH=$HOME/third_party/fftw/lib:$LD_LIBRARY_PATH \
  ../../bin/warpx.2d.MPI.OMP.DP.PDP.FFT.EB.QED inputs_test_2d_galilean_psatd
python ../../../Examples/Tests/nci_psatd_stability/analysis_galilean.py diags/diag1000400
```
