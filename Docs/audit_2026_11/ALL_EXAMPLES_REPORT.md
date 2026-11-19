# 全部官方算例跑批报告（2026-11-19，第 21 轮）

## 范围
`Examples/` 下 **446 个** `inputs*` 文件，全部经**网页 API** 运行（`parity_all.py`，可断点续跑）。

## 结果
| 分类 | 数量 | 说明 |
|---|---|---|
| 输入文本往返一致 | **446** | 网页解析→重生成→再解析，参数与分组完全相同 |
| 输出数据逐字段一致 | **242** | 相对容差 1e-10 |
| 跳过：PICMI/脚本 | 76 | 需要 Python 驱动（`*_picmi.py`） |
| 跳过：基础片段 | 36 | `inputs_base_*`，需 `inputs_test_*` 覆盖 |
| 跳过：需外部数据 | 20 | load_density / read_*_from_path |
| 跳过：无 geometry.dims | 15 | 由运行脚本设置 |
| 跳过：需 plotfile | ~30 | `plot_raw_fields = 1`，openPMD 不支持 |
| 跳过：参考版崩溃 | ~20 | 需 MPI 多进程或环境限制，两边都无输出 |
| **差异（已定性）** | **6** | 见下 |

## 6 处差异的定性
1. `embedded_boundary_cube`（2d/3d ×3）—— fork 的阶梯式 PEC/EB 掩码是**设计差异**
2. `boundaries/inputs_3d_solar_amr` —— 恒星插件算例，插件加载与否本就不同（**设计差异**）
3. `macroscopic_solver/inputs_1d_periodic_interface` —— 代码审查 F-04/F-08 的**有意修复**
4. `langmuir_fluids/inputs_test_rz_langmuir_fluid` —— **真实 bug**：RZ 流体第 80 步整场 NaN，
   已记录在 `FLUID_RZ_BUG.md`，并加了写盘前 NaN 检查（现在会明确 abort 而不是写 NaN）

## 复现
```
python Tools/warpx-studio/tests/parity_all.py --state /tmp/parity_all.jsonl
python Tools/warpx-studio/tests/parity_run.py --examples <某个 inputs> --dim 2d
```
