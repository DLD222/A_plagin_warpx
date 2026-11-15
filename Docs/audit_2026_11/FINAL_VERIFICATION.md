# 最终验收（round 43 起，滚动更新）

基准 = 官方 WarpX **26.11**（`_upstream/warpX-26.11`，tag `93aa1ad4e`）。
本项目 = `audit_build/fork`（当前 HEAD 的完整重建）。

## 运行方式

```bash
OMP_NUM_THREADS=1 ./audit_build/cleanenv.sh ctest --test-dir audit_build/fork \
    -R '\.run$' -j 12 --timeout 3600 --output-junit audit_runs/final_fork_run_junit.xml
```

每个用例使用各自的 MPI 进程数 ×24 物理核；**必须** pin `OMP_NUM_THREADS=1`
（测试自己会设，手跑时忘了会让单例慢 10 倍）。

## 判定口径（三档，都可查）

| 档 | 含义 |
|---|---|
| **逐字节相同** | 目录树、文件内容完全一致 |
| **数值相同**（`max_rel_field = 0`，但非逐字节） | 所有数据集逐元素相等；差异只在元数据（时间戳、git hash、属性顺序） |
| **有差异** | 存在元素级差异，此时记录 `max_rel_field` / `max_abs_diff` |

"数值相同"这一档是必要的：plotfile/openPMD 的头部含运行时间与版本串，任何两次运行都不会逐字节相等。

## 进度（滚动）

| 时间 | 已完成用例 | 逐字节相同 | 数值相同 | 有差异 | 最大相对差 |
|---|---|---|---|---|---|
| 2026-11-15 21:00 | 77 / 411 | 17 | 60 | **0** | 0.000e+00 |

结论（截至已完成部分）：**已跑完的 77 例与官方 26.11 数值完全一致**（`max_rel_field = 0`，
`max_abs_diff = 0`），其中 17 例连字节都相同。

（"有差异 = 0" 的口径是 `max_rel_field == 0 && max_abs_diff == 0`；脚本
`Docs/audit_2026_11/scripts/compare_all.py` 生成每例的 JSON 记录在
`audit_runs/final_incremental/cases/`，可用 `grep max_rel_field` 复核。）
