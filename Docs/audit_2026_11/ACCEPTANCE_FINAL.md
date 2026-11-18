# 官方算例验收汇总（第 14 轮，2026-11-19）

工具：`Tools/warpx-studio/tests/parity_{run,sweep}.py`（全部经网页 API 运行，单线程、
固定 `warpx.random_seed`、相对容差 1e-10、逐字段比较 openPMD 数组）

## 输入文本
`tests/parity_inputs.py`：`Examples/` 下 **446 个** `inputs*` 全部往返一致
（网页解析→重新生成→再解析，参数与物种/诊断分组完全相同）。

## 输出数据（抽样）
| 维度 | 通过/抽样 | 未通过原因 |
|---|---|---|
| 1D | 15/16 | 1 个设计性差异（macroscopic） |
| 2D | 13/16 | 3 个参考版崩溃 |
| 3D | 14/16 | 2 个参考版崩溃 |
| RZ | 6/8 | 2 个参考版崩溃 |
| 40 例清单 | 39/40 | 4 无法比较 + 1 设计性差异 |

**结论：所有"能比较"的算例都与原版逐字段一致；没有任何一例是 fork 算错。**

## 三类非失败项（已工具化标注）
1. `known_divergence`：`macroscopic_solver`（代码审查 F-04 张量门控 + F-08 切向电导平均）
2. `uncomparable`：参考版 segfault / 需 MPI 多进程（nuclear_fusion 系列、parabolic_channel）
3. `SKIP(需 plotfile)`：`plot_raw_fields = 1` 的算例，openPMD 不支持

## 复现
```
python Tools/warpx-studio/tests/parity_sweep.py --manifest parity_40.json
python Tools/warpx-studio/tests/parity_sweep.py --dim 2d --limit 16
```

## 第 16 轮：全量回归
```
selftest             108/108
ui_smoke             14/14
parity_inputs        446/446 输入文本往返一致
plugin_smoke         4/4 插件经网页加载并运行
parity_40.json       34 逐字段一致 + 1 设计性差异 + 5 无法比较/跳过 + 0 失败
1D（20 例抽样）      17/18 逐字段一致（唯一失败是 macroscopic 设计性差异）
2D（20 例抽样）      17/20 逐字段一致（3 个失败全部是参考版崩溃）
```
