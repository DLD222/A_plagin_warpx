# 修复复核报告：上一轮 26 项问题是否已被修复

<!-- SPDX-License-Identifier: BSD-3-Clause -->

> **文档性质**：只读复核报告，**未修改任何源码**。
> **复核日期**：2026-11-06
> **复核基线**：[`CODE_REVIEW_REPORT.md`](CODE_REVIEW_REPORT.md)（7 高 / 12 中 / 7 低，共 26 项）
> **复核对象**：`HEAD = 4959feb` 的当前工作树
> **一句话结论**：**26 项问题，0 项被修复，26 项全部原样存在**。原因是**自审查报告提交以来没有任何代码改动**——`4959feb` 本身就是一个只加文档的提交。

---

## 0. 结论速览

### 0.1 状态统计

| 状态 | 数量 | 编号 |
|---|---:|---|
| ✅ 已修复 | **0** | — |
| ❌ 未修复（静态证据逐条复核仍在） | **26** | H1–H7、M1–M12、L1–L7 |
| 🟡 形式达成、实质未达 | **0**（已计入未修复） | A1 验收项见 §2 |
| ⏳ 附加运行时确认（不改变 ❌ 判定） | **2** | H5、H6（另有 2 项实验用于交叉验证，见 §6） |

> 上表口径：26 = 7（H）+ 12（M）+ 7（L）。M9/M10 属"测试输入/测试规范"，L1–L7 属"低危/风格"，均**未修复**。
> H5/H6 的静态判据已足以判定"未修复"；§6 的实测只是把结论从"静态判定"升级为"实测判定"，不存在"可能已修"的中间态。

### 0.2 关键事实：没有发生"代码改动"

```text
$ git log --oneline 4277ca1..HEAD -- Source tests Tools .github
（空）

$ git diff --stat 4277ca1..HEAD -- Source tests Tools .github
（空）

$ git status --short
（空）
```

- `4277ca1` = 写入 P1 原则（文档）；
- `4959feb` = 写入审查报告（文档）；
- 两者之间、以及 `4959feb` 之后，**`Source/`、`tests/`、`Tools/`、`.github/` 均无任何变更**。

因此本报告不是"检查补丁是否生效"，而是**"把 26 项逐条重新取证，确认它们仍在"**。

---

## 1. 复核方法

判定"已修复"需要同时满足三条，缺一即视为未修复：

| 标准 | 含义 |
|---|---|
| S1 代码变更 | 该问题涉及的源文件在审查报告之后有实际 diff |
| S2 证据消失 | 报告中引用的代码模式（`grep` 可命中）不再存在 |
| S3 行为正确 | 新代码本身不引入等价问题（例如把 `static` 换成 `thread_local` 并不算修复） |

本次 26 项中：
- **S1 全部不满足**（零 diff）；
- 因此只需执行 S2 取证即可判定，逐条命令与输出见附录 A。

> 说明：本沙箱仍无 AMReX 头文件、无 conda、无 `build/`，**依然无法编译或跑 ctest**。凡涉及运行期行为的结论，一律标为"需实测"，不臆断。

---

## 2. 先回答一个容易混淆的问题：A1 这批改动是不是"修复"？

有人可能把"上一轮的 A1 实现"当成对审查问题的修复。需要区分两件事：

| 对象 | 性质 | 与 26 项问题的关系 |
|---|---|---|
| A1 实现（`01d3ece`…`67b292a`…） | **被审查的对象**，是问题的**来源** | 不是修复；它就是审查报告要指出的东西 |
| 审查报告（`4959feb`） | 指出问题 | 不下补丁 |
| 复核（本文） | 确认问题是否被处理 | 结论：**未处理** |

A1 这批改动**确实完成了它的既定目标**（把 16 个 ReducedDiags 从直连 `WarpX` 迁到 `data()/config()/frame()` 三入口，且数值不变）。这一点在上一份报告 §8 已给出 11 条正向结论，本报告**不推翻**。

但它的目标**不是**修复审查报告中的问题。两者是"实现"与"评审"的关系，不是"缺陷"与"补丁"的关系。所以在解释 A 与解释 B 下，答案都是同一句话：**没有任何一项被修复**。

---

## 3. 高危 7 项复核（H1–H7）

| # | 问题 | 状态 | 复核证据（当前 HEAD） | 最小修复量 |
|---|---|---|---|---|
| **H1** | `WarpXRuntimeData::Legacy()` 静态引用在 `Finalize` 后悬空 | ❌ 未修复 | `WarpXRuntimeData.cpp:20` 仍是 `static WarpXRuntimeData legacy(WarpX::GetInstance());`；`WarpX.cpp:324` 仍是 `m_instance = nullptr` | 1 处：改为按实例持有 |
| **H2** | `Runtime` 仍是单例消费者，A1 未真正去单例 | ❌ 未修复 | `grep -c "WarpX::GetInstance" Runtime.cpp` = **5**（`:24,45,66,77,92`） | 需 A2/A3 重构，非一行 |
| **H3** | 诊断仍残留 `GetInstance`，与 `ROUTE_STATUS` 的 done 矛盾 | ❌ 未修复 | `FieldEnergy.cpp:184`、`Timestep.cpp:28,72`、`ReducedDiags.cpp:150`、`MultiReducedDiags.cpp:103` | 4 处，各 1–2 行 |
| **H4** | `Services` 零实现、上下文注入 `nullptr` | ❌ 未修复 | `Runtime.cpp:49` 第 5 个字段仍是 `nullptr`；全仓库 `grep "public Services"` **0 命中** | 1 个 `UnavailableServices` 实现类 |
| **H5** | 测试假通过：`field_probe` 无条件校验 + 共享工作目录 | ❌ 未修复 | `test:137` 的 `if (!particles_mode)` 在 `:198` 闭合，`:246` 的 `field_probe` 块在其**之外**；`CMakeLists:10,15` 两处 `WORKING_DIRECTORY` 相同；`inputs_2d_particles` 无 `field_probe` | 3 处小改（移动块 + 分目录 + 清理） |
| **H6** | 测试输入写死 2D，却注册到所有 `WarpX_DIMS` | ❌ 未修复 | `CMakeLists:1` 的 `foreach(D IN LISTS WarpX_DIMS)`；`:8` 与 `:13` 仍传 `inputs_2d*` | 加 `if(D STREQUAL "2")` 或补 1D/3D/RZ 输入 |
| **H7** | CI 只在 `main` 触发，实际主分支是 `development` | ❌ 未修复 | `route-gates.yml:6,8` 仍是 `branches: [main]` | 改 2 行 |

### H5 的补充说明（本次复核新发现的佐证）

`grep "field_probe" tests/runtime/inputs_2d_particles` → **0 命中**，与上一报告一致。
这意味着 particles 测试运行时，`diags/reducedfiles/field_probe.txt` **只能是另一个测试留下的**。结合 `last_line()` 对不存在的文件返回空串、随后解析出全 0，可确定：

```
单独运行 runtime_snapshot_particles（干净目录） → field_probe Ex 期望 1.0，实际 0.0 → FAIL
两测试顺序执行（同目录）                       → 读到上一个测试的文件 → PASS（假通过）
```

这不是推测：`test:246-256` 的校验块与 `inputs_2d_particles:17,53` 的注册表**同时成立**，逻辑上已封闭。

---

## 4. 中危 12 项复核（M1–M12）

| # | 问题 | 状态 | 复核证据 | 最小修复量 |
|---|---|---|---|---|
| **M1** | `Timestep` 构造期 `maxLevel()`、ComputeDiags 用 `finest_level` | ❌ 未修复 | `Timestep.cpp:29` vs `:69`（`:73` 的 fallback 分支仍是 `maxLevel()`） | 1 行：`data().maxLevel()` |
| **M2** | `static const` 快照永不刷新 | ❌ 未修复 | `ReducedDiags.cpp:180-182`（`config`）、`:191-193`（`frame`） | 2 处：改按需/实例持有 |
| **M3** | 每次 `sync_state()` 复制 6 个 Vector + 44 标量 | ❌ 未修复 | `Runtime.cpp:68,78` 调 `sync_state`；`:128` `m_time = make_time()`（getter 按值返回） | 改为脏标记 + 惰性 |
| **M4** | 构造期快照 `dt` 是 `max()` 哨兵 | ❌ 未修复 | `Runtime.cpp:46-48`：`sync_config(); sync_time();` 在 `InitData` 之前 | 加 `valid` 标志 |
| **M5** | `initialize()` 无幂等保护 | ❌ 未修复 | `Runtime.cpp:62-69`：只有 `m_finalized` 检查，无 `m_initialized` 检查 | 1 行 |
| **M6** | `RuntimeConfig` 默认值与 `WarpX` 默认值是两套真值 | ❌ 未修复 | `RuntimeConfig.H` 字段默认值仍在；唯一填充点仍是 `FromLegacy()` | 删除默认值或加测试锁死 |
| **M7** | 4 个文件重复 include | ❌ 未修复 | 实测 `Particle{Energy,Extrema,Momentum,Number}.cpp` 仍 `24/23, 43/42, 23/22, 17/16` | 删 4 行 |
| **M8** | include 分组顺序违反 `AGENTS.md` | ❌ 未修复 | 16 个诊断文件的 `Runtime/...` 仍插在 `Utils/...` 与 `WarpX.H` 之间 | 批量重排 |
| **M9** | 测试输入重复定义 `warpx.reduced_diags_names` | ❌ 未修复 | `inputs_2d_particles:17` 与 `:53` 两处同名 key | 删 1 行 |
| **M10** | 未按 `add_warpx_test()` 接入；`step_info`/`lb_eff` 从不断言 | ❌ 未修复 | `grep -c add_warpx_test tests/runtime/CMakeLists.txt` = **0**；`grep -c "step_info\|lb_eff" test_runtime_snapshot.cpp` = **0** | 需要改造测试 |
| **M11** | `refactor` 可绕过 `--accept` | ❌ 未修复 | `route_tasks.py:180` 仍是 `... or t.get("automation_command")` | 删半个条件 |
| **M12** | `ROUTE_STATUS` 与 `route_tasks.json` 矛盾；状态不入库 | ❌ 未修复 | `ROUTE_STATUS.md:15,23` 仍写 done；`route_tasks.json` 中 `A1.1/A1.4e` 仍为 `pending`；`route_tasks.py next` 仍输出 **A1.1** | 需定唯一真值 |

### M12 的现场输出（本次实测）

```text
$ python3 Tools/automation/route_tasks.py next
A1.1  RuntimeConfig（只读配置）类型
  kind: refactor  gate: build+smoke  effort: 2.0 d
  acceptance: 不改变行为；所有配置项只读

$ python3 -c "... route_tasks.json ..."
{'A1.1': 'pending', 'A1.4e': 'pending'}
```

而 `ROUTE_STATUS.md` 同一时刻写着：

```
15:| A1.1 | A1 | done | A1.0 | RuntimeConfig（只读配置）类型 |
23:| A1.4e | A1 | done | A1.4d | Batch C4：FieldProbe + ParticleExtrema + 直方图 |
```

即"代码早就写完、任务表还说没开始"。这不是代码缺陷，但会让**任何基于任务图的门禁/交接**得出错误结论。

---

## 5. 低危 7 项复核（L1–L7）

| # | 问题 | 状态 | 复核证据 |
|---|---|---|---|
| **L1** | `RuntimeContext` 裸指针聚合、可复制 | ❌ 未修复 | `RuntimeContext.H` 结构未变，无 `= delete` |
| **L2** | `~Runtime()` 无条件 `finalize_external_libraries()` | ❌ 未修复 | `Runtime.cpp:54-58` 未变 |
| **L3** | `verbose()` 依赖单例 | ❌ 未修复 | `Runtime.cpp:92`（已并入 H2） |
| **L4** | 注释里的 `GetInstance` 干扰检索 | ❌ 未修复 | `RuntimeData.H`、`Services.H` 注释未变 |
| **L5** | `field_max` 用 `1e-14` 绝对容差比对 1.0，余量仅 2 倍 | ❌ 未修复 | `test:160-163` 仍是 `1.0e-14`；`ReducedDiags.H:48` 仍是 `m_precision = 14` |
| **L6** | 非 RZ 构建也复制 `n_rz_azimuthal_modes` | ❌ 未修复 | `RuntimeConfig.cpp` 无 `#ifdef` |
| **L7** | `parse_headers.py` 硬编码 `/tmp` 路径 | ❌ 未修复 | `Tools/A0/parse_headers.py:90`：`open('/tmp/header_catalog.json','w')` |

---

## 6. 建议实测的 4 个实验（覆盖 H5 / H6 / H3，不改变结论）

这 4 个实验**不是"可能已修"的候选**，而是把已经成立的静态判定升级为运行时证据。本次环境仍缺 AMReX/conda/`build/`，故保持 `⏳`。

| 优先级 | 实验 | 预期结果 | 验证的问题 |
|---|---|---|---|
| 1 | 干净目录单独跑 `ctest -R '^runtime_snapshot_particles'` | **FAIL**（`field_probe` 缺失 → 全 0） | H5 的决定性证据 |
| 2 | `-DWarpX_DIMS="1;2;3;RZ"` 后跑 `ctest -R runtime_snapshot` | 2D 之外 **FAIL**（维度不匹配） | H6 |
| 3 | `rm -rf diags && ctest -j 4 -R runtime_snapshot` | **间歇失败/竞态** | H5 的并发面 |
| 4 | RZ 构建编译 `FieldEnergy.cpp` | 可编译（但迁移未覆盖） | H3 的 RZ 分支 |

若第 1、2 项实测结果为 FAIL，H5/H6 即从"静态判定"升级为"实测判定"；若意外 PASS，本报告相应条目需修正——这也是为什么把它们单列为 `⏳`。

---

## 7. 为什么"看起来好像修好了"

三个误导源，逐条点破：

| 误导源 | 表象 | 真相 |
|---|---|---|
| 提交信息 | `feat(runtime): migrate ...`、`feat: 提问` 等连续提交 | 这些是**被审查的实现**，早于审查报告；审查之后零代码提交 |
| `ROUTE_STATUS.md` | A1.1–A1.4e 全 `done` | 任务图 JSON 里是 `pending`（M12）；且 `FieldEnergy` RZ 分支未迁移（H3） |
| 三项快速门禁 | compliance / audit 都 exit 0 | 它们只查文件名/数量，**不查本报告的任何一条**（如 `compliance.py` 只看 `git ls-files`） |
| 测试通过 | `runtime_snapshot` 曾报 PASS | 其中 particles 用例的 PASS 依赖陈旧文件（H5），属**假通过** |

---

## 8. 最小修复清单（按改动量排序，仍未实施）

| 档位 | 涉及项 | 该做什么 |
|---|---|---|
| **5 行以内** | H4（1 类）、M1（1 行）、M5（1 行）、M7（删 4 行）、M9（删 1 行）、M11（删半个条件）、H7（2 行）、H3（4 处小改） | 低风险、可一次性完成 |
| **5–30 行** | H5（3 处）、H6（维度守卫或补输入）、M2（2 处）、M10（接入 `add_warpx_test`） | 影响测试可信度，必须做 |
| **需重构** | H1（生命周期归属）、H2（去单例，属 A2/A3）、M3/M4（快照语义）、M12（状态真值） | 不应在 A1 尾巴上硬塞，随 A2 一起做 |

**建议顺序**：先 H5/H6/H7（让门禁说真话）→ 再 H1/M5/H4（消除崩溃面）→ 再 H3/M1/M2（补齐 A1 范围）→ 最后 H2/M3/M4/M12（并入 A2）。

---

## 附录 A：本次复核命令与实测输出

```bash
# 0. 自审查以来有无代码改动
git log --oneline 4277ca1..HEAD -- Source tests Tools .github     # 空
git diff --stat 4277ca1..HEAD -- Source tests Tools .github       # 空
git status --short                                                # 空

# H1
grep -n "static WarpXRuntimeData legacy" Source/Runtime/WarpXRuntimeData.cpp   # 20:
grep -n "m_instance = nullptr" Source/WarpX.cpp                               # 324:

# H2
grep -c "WarpX::GetInstance" Source/Runtime/Runtime.cpp                        # 5

# H3
grep -n "WarpX::GetInstance" Source/Diagnostics/ReducedDiags/FieldEnergy.cpp \
  Source/Diagnostics/ReducedDiags/Timestep.cpp \
  Source/Diagnostics/ReducedDiags/ReducedDiags.cpp \
  Source/Diagnostics/ReducedDiags/MultiReducedDiags.cpp
# 184 / 28,72 / 150 / 103

# H4
grep -rn "public Services" Source                       # 0 命中
sed -n '49p' Source/Runtime/Runtime.cpp                 # ... m_data.get(), nullptr,

# H5
grep -n "if (!particles_mode)" tests/runtime/test_runtime_snapshot.cpp   # 137:
grep -n "field_probe" tests/runtime/inputs_2d_particles                  # 0 命中
grep -n "WORKING_DIRECTORY" tests/runtime/CMakeLists.txt                 # 10:, 15:

# H6 / H7
grep -n "foreach\|inputs_2d" tests/runtime/CMakeLists.txt               # 1:,8:,13:
grep -n "branches" .github/workflows/route-gates.yml                     # 6:, 8:

# M1 / M5 / M11
grep -n "maxLevel\|finest_level" Source/Diagnostics/ReducedDiags/Timestep.cpp  # 29,69,73
sed -n '60,70p' Source/Runtime/Runtime.cpp                                    # 无 m_initialized 检查
sed -n '180p' Tools/automation/route_tasks.py                                 # or t.get("automation_command")

# M2
grep -n -A3 "ReducedDiags::config\|ReducedDiags::frame" Source/Diagnostics/ReducedDiags/ReducedDiags.cpp
# 180-182, 191-193：static const ... FromLegacy()

# M7
for f in Source/Diagnostics/ReducedDiags/*.cpp; do
  d=$(grep -c '#include' "$f"); u=$(grep '#include' "$f" | sort -u | wc -l)
  [ "$d" -ne "$u" ] && echo "$f $d/$u"
done
# ParticleEnergy 24/23, ParticleExtrema 43/42, ParticleMomentum 23/22, ParticleNumber 17/16

# M9 / M10 / M12 / L5 / L7
grep -n "reduced_diags_names" tests/runtime/inputs_2d_particles  # 17:, 53:
grep -c "add_warpx_test" tests/runtime/CMakeLists.txt           # 0
grep -c "step_info\|lb_eff" tests/runtime/test_runtime_snapshot.cpp  # 0
python3 Tools/automation/route_tasks.py next                    # 输出 A1.1
grep -n "A1.1\|A1.4e" Docs/research_notes/ROUTE_STATUS.md       # 15:, 23: done
grep -n "1.0e-14" tests/runtime/test_runtime_snapshot.cpp        # 160-163
grep -n "/tmp/header_catalog.json" Tools/A0/parse_headers.py     # 90:
```

---

## 附录 B：与既有文档的关系

| 文档 | 关系 |
|---|---|
| [`CODE_REVIEW_REPORT.md`](CODE_REVIEW_REPORT.md) | **被复核对象**。本文逐条对应其 H1–H7 / M1–M12 / L1–L7，不新增问题、不改判定口径 |
| [`最终的技术路线.md`](最终的技术路线.md) | 技术要求基线；本报告 §2 以 P1 与 A1 验收为准 |
| [`ROUTE_AUDIT_REPORT.md`](ROUTE_AUDIT_REPORT.md) | 架构层缺口（F1–F5/S1–S8/C1）。A1 实现不触碰这些，因此本报告不重复其结论 |
| [`ROUTE_STATUS.md`](ROUTE_STATUS.md) | 进度声明；本报告 M12 复核其与 `route_tasks.json` 的冲突仍存在 |

> 本文只做复核，**不提出代码补丁、不修改路线**。若后续提交了修复，请以本报告的附录 A 命令重跑，逐条把 ❌ 改为 ✅。
