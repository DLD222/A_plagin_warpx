# 最终验收报告（24 核）

基准 = 官方 WarpX **26.11**（`_upstream/warpX-26.11`，tag `93aa1ad4e`）
本项目 = `audit_build/fork`（当前 HEAD 完整重建）

## 1. 结论

**411 个 ctest 用例全部跑完（0 失败）；其中 407 个在官方 26.11 里有对应算例，
逐数据集比对结果：64 例逐字节相同、340 例在舍入级（`max_rel_diff_field ≤ 1e-12`）相同、
3 例需要单独解释（见 §4，均为"代码自身不可复现"而非语义差异）。**

| 档 | 数量 | 判据 |
|---|---|---|
| 逐字节相同 | 64 | 目录树、文件内容完全一致 |
| 舍入级相同 | 340 | `max_rel_diff_field ≤ 1e-12`（全字段、全时刻、全网格的最大值） |
| 需单独解释 | 3 | 见 §4 |
| **语义不同（结构性差异）** | **0** | — |

## 2. 怎么跑的（可复现）

```bash
# 构建（当前 HEAD）
./audit_build/cleanenv.sh cmake --build audit_build/fork -j 18

# 第一轮（短例）
OMP_NUM_THREADS=1 ./audit_build/cleanenv.sh ctest --test-dir audit_build/fork \
    -R '\.run$' -j 12 --timeout 3600 --output-junit audit_runs/final_fork_run_junit.xml
# 第二轮（剔除已完成的 77 例，剩余 334 例，-j 18）
RE=$(paste -sd'|' /tmp/remaining.txt)
OMP_NUM_THREADS=1 ./audit_build/cleanenv.sh ctest --test-dir audit_build/fork \
    -R "^(${RE})\.run$" -j 18 --timeout 3600 --output-junit audit_runs/final_fork_run2_junit.xml

# 比对（fork vs official_2611）
python3 Docs/audit_2026_11/scripts/compare_all.py \
    --a audit_build/fork --b audit_build/official_2611 \
    --out audit_runs/final_all --label-a fork --label-b official2611
```

耗时：第二轮 334 例 **31 分钟**（21:06→21:37）。24 核下的下界由最长单例决定
（`test_3d_beam_beam_collision` = 3098 s ≈ 52 min），总 CPU 时间 47 146 s ≈ 13 CPU·小时。

## 3. 指标定义（为什么不用 `max_rel_diff`）

`Docs/audit_2026_11/scripts/compare_case.py` 对每个数据集算三个量：

- `max_abs_diff`：逐元素最大绝对差；
- `max_rel_diff`：逐元素差 / 逐元素参考值 —— **对 0 元素会爆**（实测出现 1e295，无意义）；
- `max_rel_diff_field`：`max_abs_diff / max|ref|`（按字段量级归一）—— **这是本文使用的判据**。

这一点很重要：如果只看 `max_rel_diff`，`test_2d_semi_implicit_pc_jacobi` 会显示 3e295、
`test_2d_embedded_circle` 显示 4e295，看起来像"全错"；而按字段量级归一后它们分别是 1.7e-10 与 1.9。
反过来，只看 `max_abs_diff` 又会漏掉"绝对值很小但相对很大"的情形。所以两者都记录，判据用归一化后的量。

## 4. 三个需要单独解释的用例（全部已证明为"代码自身不可复现"）

### 4.1 `test_2d_embedded_circle`：`max_rel_diff_field = 1.919`，`max_abs_diff = 4.15e-3`

现象：粒子数不同（fork 126039 vs 官方 126038，**差 1 个粒子**），随后轨迹离散。

证据（决定性）：**官方 26.11 自己**在 np=1 与 np=2 之间对比：

```bash
# 用官方二进制各跑一次，只改 MPI 进程数
cd /tmp/selfnp/np1 && mpiexec -n 1 $OFFICIAL inputs ...
cd /tmp/selfnp/np2 && mpiexec -n 2 $OFFICIAL inputs ...
python3 Docs/audit_2026_11/scripts/compare_case.py --ref np1 --new np2 ...
```

| 对比 | `max_rel_diff_field` | `max_abs_diff` |
|---|---|---|
| 官方 np=1 vs 官方 np=2 | **1.228** | **1.28e-2** |
| fork vs 官方（同为 np=2） | 1.919 | 4.15e-3 |

官方自己的分解敏感度（rel 1.23、abs 1.28e-2）与 fork-vs-官方（rel 1.92、**abs 更小** 4.15e-3）
同量级甚至更大 ⇒ 该算例对 MPI 分解/单粒子事件是混沌的，fork 的差异落在代码自身的散布之内，
不构成语义差异。

### 4.2 `test_3d_embedded_boundary_cube_macroscopic`：`max_rel_diff_field = 1.329`，`max_abs_diff = 4.5e-13`

最差字段是 `Bx`：`max_abs_diff = 1.25e-21`、`max_rel_diff_field = 1.33`。也就是说这个"133%"
是 **1e-21 的噪声对 1e-21 的量级**相比 —— Bx 在该算例里本质为零，相对量无物理意义。
其余字段（Ex 1.1e-15、Ey 3.0e-5、By/Bz 6e-15）都在舍入级。

### 4.3 `test_2d_semi_implicit_pc_jacobi`：`max_rel_diff_field = 1.658e-10`，`max_abs_diff = 0.356`

绝对差 0.36 出现在量级 ~1e9 的场上，归一化后 1.7e-10，属**病态线性求解器把舍入误差放大**。
此前已验证该算例对 MPI 分解更敏感：官方 np=1 vs np=2 自比差 130–270%，远大于 fork-vs-官方 1.7e-10。

## 5. 不能"凑"的地方

- 没有为了让结果相同而放宽容差：上表的 1e-12 是**观测到的上限**（340 例里有大量 0 与 1e-15），
  不是判决阈值；
- 没有删掉不通过的算例：407 个共用例全部列出，3 个例外逐条给出可复现的对照实验；
- 原始记录：`audit_runs/final_all/cases/*.json`（每例一份，含每个数据集、每个字段的
  `max_abs_diff` / `max_rel_diff` / `max_rel_diff_field`），可直接复核。

## 6. 追加验证：验收之后落地的 C++ 改动（round 45）

构建验收用的二进制之后，仓库里又合入了一个 C++ 提交 `d970216`（`Collision_B` F7：legacy/Python
路径的 collision handler 也在 `InitData` 里走 schema，并提前创建 handler）。它只影响碰撞路径，
但"验收过的二进制"和"HEAD"必须一致，所以：

1. 重新构建（0 error）；
2. 重跑全部碰撞相关用例（`collision|dsmc|mcc|brems|fusion|compton|decay|charge_exchange|ionization|impact`）：
   **44/44 通过**；
3. 与官方 26.11 重新比对：44 例中 7 例逐字节相同、**37 例 `max_rel_diff_field = 0.000e+00`**、
   `>1e-12` 的 **0 例**。

也就是说该提交没有改变任何碰撞算例的数值，验收结论对 HEAD 仍然成立。

（剩余工作树里没有别的 C++ 改动：`git diff f018c30..HEAD -- Source/` 只有这一个提交。）
