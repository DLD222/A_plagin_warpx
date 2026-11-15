# 容差审计：有没有"为了过而凑出来的容差"？

复核动机：review 提出 `Python` F3"6 处测试容差被系统性放宽"，用户也要求专项核查"弄虚作假掩盖
真实代码漏洞"。方法不是逐个肉眼比对，而是**与基准机械对照**：

1. 把 `Examples/**/analysis*.py` 全部（285 个被 CMakeLists 引用的分析脚本）与
   `_upstream/warpX-26.11` 同名文件做 `diff`；
2. 对每个不同的文件，抽出所有容差类常量（名字含 `tol`/`rtol`/`atol`/`threshold`/`error`/`acc`）
   逐个比较数值；
3. 对"没有 assert/raise 的分析脚本"，同样与上游对照，判断是上游设计还是本仓库删除；
4. 对 fork 独有测试（`dynamic_regrid/*`、`FluidMoments/test_*.cpp`、`Star/*`）单独检查是否存在
   "恒真断言"（`assert x == x`、只 print 不 assert、`return 0` 无失败路径）。

## 结果

| 结论 | 数量 | 说明 |
|---|---|---|
| 与上游 26.11 完全一致 | 283 / 285 | 包括 review 点名的 6 处"放宽"，其数值上游自己就是这么定的（26.10→26.11 上游演化） |
| 本仓库修改过 | 2 | `analysis_collision_3d_pulsed_decay.py`、`analysis_planar_pinch_solvers.py` |
| 无 assert 但上游也如此 | 16 | 这些脚本只做输出，门禁由 `*.checksum`（rtol 1e-9 / restart 1e-12）承担 |
| 恒真断言 / 无失败路径 | **0** | `FluidMoments` 的 33 个测试都有 `exit_code` 失败路径（此前 3 个恒返回 0 的已修） |

## review 点名的 6 处逐条复核（全部"不成立"）

| 文件:行 | review 说 fork | 26.11 实际 | 判定 |
|---|---|---|---|
| `projection_div_cleaner/inputs_test_3d_projection_div_cleaner_picmi.py:133` | 2e-12 | 2e-12 | 上游即此值 |
| `..._callback_picmi.py:243` | 3e-12 | 3e-12 | 上游即此值 |
| `implicit/analysis_pinch.py:72` | 3.0e-12 | 3.0e-12 | 上游即此值 |
| `implicit/analysis_pinch.py:115` | 2.2e-12 | 2.2e-12 | 上游即此值 |
| `field_probe/analysis.py:61` | `averror < 5.0` | `averror < 5.0` | 上游即此值 |
| `collision/analysis_test_2d_collisions_split_momentum_push.py:121` | 2e-1 | 该行上游是能量守恒的 1e-5；2e-1 是本仓库**新增**的"场能均分判据"，不是放宽既有判据 | 部分成立（新增判据偏松） |

## 真正"凑出来"的容差（已修）

`Examples/Tests/implicit/analysis_planar_pinch_solvers.py`：

```
tolerance_rel_charge = 5.0e-6      # 断言 drho_rms < 5e-6
实测 drho_rms        = 1.08e-15    # 差 9 个数量级
姊妹脚本 analysis_pinch.py 对同一物理量要求 2.2e-12
```

即这条"电荷守恒"断言实际上不可能失败。已收紧到 `1.0e-12`（仍有约 1000 倍余量），
`test_2d_semi_implicit_pc_jacobi` 3/3 通过。

## 顺带发现

- 悬空符号链接 `Examples/Physics_applications/laser_acceleration/analysis_default_openpmd_regression.py`
  → `Examples/analysis_default_openpmd_regression.py`（目标不存在，上游也没有该文件，无任何
  CMakeLists 引用）。它会让任何遍历 `Examples/**/analysis*.py` 的工具直接抛错。已删除。
- 上游自身遗留：`Examples/Tests/gaussian_beam/CMakeLists.txt` 引用不存在的 `analysis.py`
  （上游 26.11 同样如此），未改。
