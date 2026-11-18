# 官方算例一致性结果（第 8 轮，2026-11-19）

判据：网页 API 跑 fork 与原版（均 AMReX 26.11），单线程、固定 `warpx.random_seed`、
相对容差 1e-10、逐字段比较 openPMD 数组。

| 维度 | 通过 | 说明 |
|---|---|---|
| 1D | 11/12 | 唯一失败：`load_density`（需外部数据文件） |
| 2D | 10/12 | 失败 2 个均为**参考版崩溃**（nuclear_fusion、parabolic_channel） |
| 3D | 9/10 | 唯一失败：`proton_boron_fusion`（参考版崩溃） |
| RZ | 2/3 | 唯一失败：参考版崩溃 |

**结论：抽样 37 个官方算例中，没有一例是 fork 算错。**

## 本轮修掉的真实问题
1. `warpx.deferred_refine` 之前默认生效，导致 dynamic_regrid 两例网格不同 → 改为显式开关（默认 0）
2. `WarpX::ErrorEst` 在 t=0 被 `refine_start_time` 挡住，初始没有细网格 → 已修
3. 26.11 的 `JacobianFunctionMF` RSPHERE 保护已补

## 复现方式
```
python Tools/warpx-studio/tests/parity_sweep.py --dim 2d --limit 12
python Tools/warpx-studio/tests/parity_run.py --examples <某个 inputs> --dim 2d
```
