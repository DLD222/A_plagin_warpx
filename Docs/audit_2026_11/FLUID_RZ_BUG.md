# RZ 几何下流体数值爆炸（NaN）—— 已修复

## 根因
`WarpXFluidContainer::DepositCurrent` 在 RZ 下**漏乘了径向体积因子**。

WarpX 的 RZ 电流/电荷在网格上存的是"未做体积缩放的原始沉积量"，后续
`FinalizeJ` / `FinalizeRho` 再做 `1/(2πr)`（轴上 `1/(π dr)`）的反向缩放。
- `DepositCharge` 乘了体积因子 ✓
- `DepositCurrent` **没乘** ✗ → 电流被额外放大了 `2πr` 倍，轴上最严重

结果：几十步内电流/场指数增长，第 80 步整场 NaN；而写完盘后用户看到的是
"跑完了、图全黑"。

## 修复
按上游 26.11 的实现补回体积因子，且**按分量所在位置**取半径：
```cpp
rmin_x = rmin + (jx_type[0] == NODE ? 0.0 : 0.5*dr);   // 节点 i·dr，单元中心 (i+½)dr
...
jx_arr(i,j,k) += RadialVolumeFactor(rmin_x + (i-irmin)*dr, dr, use_axis_correction) * jx_tmp;
```
y/z 分量同理。

## 验证
| 项 | 修复前 | 修复后 |
|---|---|---|
| `langmuir_fluids/rz` 第 80 步 rho | 8192/8192 全 NaN | 0 个 NaN，max 1.16e4 |
| 与原版逐字段比对 | 无法比较（NaN） | **PASS（1e-10 容差内）** |
| RZ 抽样 12 例 | — | **12/12 通过** |

## 附带改进
`MultiDiagnostics::FilterComputePackFlush`（含 LastTimestep）在写盘前用
`MultiFab::contains_nan` 检查，发现 NaN 直接 abort 并指明诊断名/步数。
注意 `MultiFab::min/max` **会忽略 NaN**，不能用它们做检查。
