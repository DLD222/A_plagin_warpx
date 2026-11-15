# 并行会话工作约定（单入口文档）

新开会话请**先读这一份**，再按需读它指向的文件。目标是让 N 个会话同时干活而不互相踩。

## 1. 文档地图（都在 `Docs/audit_2026_11/`）

| 文件 | 作用 | 谁维护 |
|---|---|---|
| `WORKFLOW.md` | 本文件：怎么接手、怎么跑、怎么写 | 所有人只读 |
| `BACKLOG.md` | **任务账本**：189 条 review 条目，`fixed` / `invalid` / `todo` / `wip:<会话>` | 每个会话只改自己认领的行 |
| `claims/<分区>.md` | 单个会话的工作日志（认领、过程、结论、提交号、验证命令与输出） | **只有该会话写** |
| `reviews/` | 17 个分区的原始审查报告（缺陷清单 + 证据 + 建议修法） | 只读；会话可在自己分区文件里追加"已修/不成立"注记 |
| `TOLERANCE_AUDIT.md` | 容差专项审计（"为了过而凑出来的容差"） | 只读 |
| `ATTRIBUTION.md` | fork vs 官方 26.11 的 407 例逐用例归因与最终对照 | 只读（集成者更新） |
| `FIXES.md` / `CODE_AUDIT.md` / `REPORT.md` | 修复记录 / 代码审计状态 / 总报告 | **集成者**更新，会话不要直接改（避免冲突） |
| `UPSTREAM_BASE.md` | 基线说明：fork 的上游是 26.11（不是 26.10） | 只读 |
| `scripts/` | 对比与归因脚本（`compare_all.py`、`tri_compare.py`、`compare_case.py` …） | 只读 |

`/tmp` 下不要留结论性内容；一切结论进 `claims/<分区>.md`。

## 2. 环境与命令（都在仓库根执行）

```bash
cd "$HOME/WarpX_plugin"

# 构建（增量）
./audit_build/cleanenv.sh cmake --build audit_build/fork -j 10

# 跑单个/一组用例（必须 pin 线程，否则超订 10 倍）
OMP_NUM_THREADS=1 ./audit_build/cleanenv.sh ctest --test-dir audit_build/fork -R "<regex>" -j 4 --timeout 1800

# 九个不变量门（每次提交前）
bash Tools/verify/run_gates.sh
```

- 三个构建：`audit_build/fork`（本项目）、`audit_build/official_2611`（官方 26.11，基准）、
  `audit_build/official`（官方 26.10）。**对比一律用 `official_2611`**。
- 与官方对比：

```bash
"$HOME/miniconda3/bin/python3" Docs/audit_2026_11/scripts/tri_compare.py --cases <case> [<case> ...]
# 或全量：compare_all.py --a audit_build/fork --b audit_build/official_2611 --out <dir>
```

- `-R` 正则要加边界：`-R "ect\b"`。`-R "ect|conformal"` 会顺带匹配 `electrostatic`、`injection` 等
  200+ 个用例（我不小心踩过）。

## 3. 认领流程（避免重复劳动）

1. 选一个**分区**（`reviews/` 里的一份报告），或分区内的一段连续条目；
2. 在 `claims/<分区>.md` 顶部写一行认领：`claim: <分区> <F1,F3-F5> @<会话名> <时间>`；
3. 把 `BACKLOG.md` 里**你要做的那几行**状态改成 `wip:<会话名>`；
4. 做完一条：改 `fixed`（或 `invalid`），并在 `claims/<分区>.md` 追加：
   `F3 fixed by <commit>; 验证: <命令> → <结果>`；
5. `git add -A && git commit`（一条一个提交，消息里带条目号），然后 `git push origin main`。
   推送前先 `git pull --rebase origin main`（并行会话会互相推进）。

**只改自己分区的行**；`BACKLOG.md` 是大表，多个会话同时改同一行会冲突。

## 4. 判定规则（很重要，别把对的改错）

- **基准是 26.11**。与 26.10 不同但**与 26.11 相同** ⇒ 是上游演化，**不是缺陷**，标 `invalid`
  并写清"上游 26.10→26.11 自己改的"（已有 5 条这样被推翻：`FieldSolver_A/F3`、
  `Python/F3`、`PicEm_B/F1`、`PicEm_B/F8`、`Diagnostics_A/F6`）。
- 只在**运行时真的走得到**的路径上改；可达性不确定就在 `claims/` 里写清"可达性判定"。
- **不要为了让测试变绿而放宽容差**。发现"凑出来的容差"（实测值与容差差好几个数量级），
  收紧它、并把真实残差写进 `claims/`；`TOLERANCE_AUDIT.md` 有先例。
- 断言/检查器必须**能失败**：改完先做一次反证（故意把被检量改坏，确认测试变红）。
- 插件接口必须简单：新增跨插件耦合一律拒绝，改走 `Source/Core/Runtime` 的服务接口。

## 5. 建议的分工（互不重叠）

2026-11-15 18:25 的实际待办分布（`awk -F'|' '/\| todo \|/ {gsub(/ /,"",$2); print $2}' BACKLOG.md | sort | uniq -c`）：

| 会话 | 分区 | todo 条数 |
|---|---|---|
| A | `Tools_Tests` | 18 |
| B | `Benchmarks` | 14 |
| C | `Source_Core_Runtime_A` + `Source_Core_Runtime_B` | 10 + 10 |
| D | `Source_Plugins_ParticleCore_A` + `_B` | 5 + 10 |
| E | `Source_Plugins_PicEm_A` + `_B` + `Source_Core_Misc` | 9 + 1 + 5 |
| F | `Source_Plugins_FieldSolver_A` + `_B` | 4 + 9 |
| G | `Source_Plugins_Collision_A` + `_B` + `Source_Plugins_Diagnostics_B` | 3 + 6 + 2 |
| H | `Source_Plugins_Star` + `Python` | 1 + 3 |

（`Source_Plugins_Fluid` 没有独立 review 文件，流体/星体的条目在 `Source_Plugins_Star.md` 里。）
条目由粗到细都可以，只要**先认领再动手**。

## 6. 已知的"别碰"清单

- `Docs/audit_2026_11/{REPORT,FIXES,CODE_AUDIT,ATTRIBUTION}.md`：集成者维护，会话不要改。
- `Regression/Checksum/benchmarks_json/*.json`：只在整个用例的输出**确实**应当变化时，
  用 `CHECKSUM_RESET=ON ctest -R <test>.checksum` 重设，并在 `claims/` 写明原因。
- `_upstream/`：只读的官方克隆，不要改。
