# 弓形激波算例与论文的逐项核对（2026-11-17）

对照项来自目标要求：**边界条件、初始条件、漂移方向、磁场取向、障碍物几何**。

| 项 | 论文（Burdiak+ 2017, Phys. Plasmas 24, 072713） | 本算例实现 | 依据 |
|---|---|---|---|
| 几何 | 2D，等离子体流绕圆柱导体 | `geometry.dims = 2`，`warpx.eb_implicit_function = (1e-3)^2-(x^2+z^2)`，圆柱半径 **1 mm** | 见 deck 注释 |
| 来流方向 | 沿 +x 流向障碍物 | `ions.ux > 0`，圆柱在原点 → 上游 x<0 | 同 |
| 磁场取向（B∥ 情形） | B 沿圆柱轴（垂直纸面） | `warpx.B_external_grid = 0 <B> 0`（y 方向 = 2D 的"轴向"） | 同 |
| 漂移与磁场的平衡 | 磁化等离子体垂直 B 漂移需 E = −v×B | `warpx.E_external_grid = 0 0 -v*B`（第 2 轮修正；实测 E_z = −2.143e8 ≈ −vB） | `BOW_SHOCK_REVIEW.md` §2 |
| 边界条件 | 开放/远场 | `boundary.field_*=periodic`（周期性；因上游只有 2 mm、演化 0.75 mm，未见回流污染） | deck |
| 初始条件（密度） | 均匀磁化等离子体 | `ions.density = 1e24`，`NUniformPerCell 4×4` | deck |
| 初始条件（速度） | 冷/近冷漂移 | `ions.momentum_distribution_type = constant`，`ux = v/c` | deck |
| 离子种类 | Al（Z=5, A=27） | `ions.charge = 5*q_e`，`ions.mass = 27*m_p` | deck |
| 电子 | 流体（hybrid） | `algo.maxwell_solver = hybrid`，**无 electrons 物种**（第 2 轮修正的 bug） | `BOW_SHOCK_REVIEW.md` §2 |
| 障碍物内部 | 理想导体 | PEC：覆盖格 `E=0`（阶梯掩码）+ `B_n` 连续 | `WarpXFieldBoundaries.cpp` |

## 无量纲判据（是否处于"能形成离子弓形激波"的参数区）

```
d_i/R  = 0.24     (需 ≲ 1)
r_ci/R = 0.47     (需 ≲ 几)
M_A    = 2        (需 > 1)
M_s    = 2.97     (需 > 1)
```

四个条件全部满足 ✓。作为对照，论文原参数在 R=250 µm 下给出 `d_i/R = 473`，**物理上不可能**形成
离子尺度的弓形激波——这就是最初"形状不对"的根因。

## 定量验收（iteration 300 / 500，两次测量一致）

| 判据 | 300 | 500 | 期望 |
|---|---|---|---|
| 导体内离子数 | 1 | 3 | ≈ 0 |
| 密度压缩比 | 10.75 | 10.99 | > 1.5 |
| 驻点位置 | −1.4 mm | −1.4 mm | < 0（障碍物上游） |
| 磁场压缩比 | 1.31 | 1.32 | > 1.2 |

## 与上游 WarpX 的差异（为什么不是"原版就能跑"）

- 原版不支持 `BoundaryConditions` 插件层与本 fork 的 PEC 阶梯实现；
- hybrid + EB 在本 fork 需要 `use_rkf45 = 0`（RKF45 的误差估计在 cut cell 处退化）——
  这条已写进 deck 注释与 `BOW_SHOCK_REVIEW.md` §7。
