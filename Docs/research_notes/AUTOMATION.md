# 技术路线自动化：任务图、运行器与门禁

<!-- SPDX-License-Identifier: BSD-3-Clause -->

> 把 [`最终的技术路线.md`](最终的技术路线.md) 的 A0–A6 拆成 **131 个可执行任务**，每个任务带依赖、产出、验收和门禁。
> 任务图：`Tools/automation/route_tasks.json`；运行器：`Tools/automation/route_tasks.py`；门禁：`Tools/automation/gates/`。

---

## 1. 为什么不是“一键完成”

路线里的“去单例”“LevelResources”“schedule”是**代码重构**，无法由脚本自动生成正确实现。自动化的边界是：

| 自动化 | 不自动化 |
|---|---|
| 任务依赖、下一个可做任务 | 设计决策（接口粒度/ABI） |
| 编译、冒烟、合规、审计一致性 | 重构代码本身 |
| 状态记录、日志、失败定位 | 物理正确性判断（A/B 数字要人看） |
| CI 快速门禁 | GPU/多机全矩阵（本地跑） |

---

## 2. 任务图

`Tools/automation/route_tasks.json`：

```json
{
  "id": "A1.4",
  "phase": "A1",
  "title": "Batch C：ReducedDiags 去 GetInstance",
  "kind": "refactor",
  "depends": ["A1.1", "A1.2", "A1.3"],
  "status": "pending",
  "gate": "build+smoke",
  "effort_days": 5.0,
  "acceptance": "每文件一批；不再直接读 WarpX::static"
}
```

- **kind**：`audit`（脚本可跑）、`refactor`（人写代码，脚本只跑门禁）、`test`、`docs`。
- **depends**：DAG；运行器会做未知依赖/环检测。
- **gate**：`fast`（合规+审计）、`build+smoke`（再加 2D 构建+langmuir）、`full`（本地全门）。
- **status**：`done` 会写入 `.route_state.json`（gitignored）。

当前阶段：

| 阶段 | 任务数 | 已完成 |
|---|---:|---:|
| A0 审计 + 对齐层 | 8 | 8（含 A0.6 DataBus/FieldSpec/alias） |
| A1 去单例 | 54 | 54（A1.0 facade、A1.1–A1.3 类型、A1.3a context、A1.4a Timestep、A1.4b 场三件套、A1.4c Poynting/Reduction、A1.4d 粒子统计、A1.4e Probe/Extrema/Histogram、A1.4 C6 专用诊断、A1.5a Diagnostics context、A1.5b FullDiag/Checkpoint/Plotfile、A1.5 HostServices、A1.6a Yee/FDTD、A1.6b 静电、A1.6c PSATD+FFTW、A1.6 Hybrid、A1.7a 边界缓冲、A1.7b 粒子容器配置、A1.7c 派生容器、A1.7d 流体、A1.7e Qdsmc/碰撞、A1.7f 注入、A1.7 完成、A1.8a 所有权反转、A1.8b functor context、A1.8c 粒子 functor、A1.8d Div 场代数、A1.8e Jdisp/Darwin、A1.8f Rho/J、A1.8g Phi、A1.8h BTD、A1.8i variance、A1.8j ES + 静电算例、A1.8k 流体/Qdsmc、A1.8l 粒子基类、A1.8m MultiParticleContainer、A1.8n PhysicalParticleContainer、A1.8o BTDiagnostics、A1.8p DivCleaner、A1.8q Magnetostatic、A1.8r PML、A1.8s ParIter、A1.8t 粒子几何、A1.8u Resampling、A1.8v 外场、A1.8w 粒子创建、A1.8x 边界/parser、A1.8y ParticleIO、A1.8z variance host、A1.9a 基线门、A1.9 回归门） |
| A2 Level 资源 | 20 | 20 |
| A3 生命周期拆包 | 53 | 48（A3.1 InitData、A3.2 PIC stage、A3.3 PIC/碰撞包） |
| A4 状态 schema | 5 | 0 |
| A5 调度器/插件 | 6 | 0 |
| A6 动态 regrid | 5 | 0（新增 A6a 基础设施） |

---

## 3. 运行器

```bash
# 下一个可做任务（依赖已满足）
python3 Tools/automation/route_tasks.py next

# 列出某阶段
python3 Tools/automation/route_tasks.py list --phase A1
python3 Tools/automation/route_tasks.py list --status pending

# 查看任务详情
python3 Tools/automation/route_tasks.py show A1.4

# 跑快速/构建/全量门禁
python3 Tools/automation/route_tasks.py check fast
python3 Tools/automation/route_tasks.py check build+smoke
python3 Tools/automation/route_tasks.py check full

# 执行任务：脚本任务会跑 automation_command；refactor 任务只跑门禁
python3 Tools/automation/route_tasks.py run A0.1 --gate
python3 Tools/automation/route_tasks.py run A1.4 --gate          # 门禁过但不标 done
python3 Tools/automation/route_tasks.py run A1.4 --gate --accept # 人工确认 diff 后标 done

# 生成状态报告（提交到仓库，便于查看）
python3 Tools/automation/route_tasks.py report --write

# 手工标状态（例如回滚）
python3 Tools/automation/route_tasks.py mark A1.4 pending
```

日志：`.route_logs/<task>.log`、`.route_logs/check-<gate>.log`。

**约定**：
1. `refactor` 任务必须 `--accept` 才会标 done；构建/冒烟通过不等于重构完成。
2. 每个 `refactor` 应带 `verify_command`；运行器在标记 done 前执行它，`--accept` **不能绕过验证**。
3. A1.1–A1.3 的验证是 `tests/runtime/test_runtime_snapshot.cpp`：初始化后逐项对比 `RuntimeConfig/TimeState/FrameState` 与旧 `WarpX` 状态，再推进 1 步对比时钟。

---

## 4. 门禁

| 门 | 内容 | 文件 |
|---|---|---|
| `compliance` | 许可证/notice 文件、无第三方 PDF/日志、无个人绝对路径、无凭据、SPDX | `gates/compliance.py` |
| `audit` | 372 头文件、类目录/CSV、A0 文档齐全 | `gates/audit_check.py` |
| `build` | 2D EB/MPI/OMP Release 构建 | `gates/build_2d.sh` |
| `smoke` | `test_2d_langmuir_multi.run + .analysis` | `gates/smoke_2d.sh` |
| `full` | 以上 + （A5 后）A/B | `gates/full_gate.sh` |

本地构建需要 AMReX 源码：

```bash
export WARPX_AMREX_SRC=/path/to/amrex-src
export WARPX_JOBS=$(nproc)
bash Tools/automation/gates/full_gate.sh
```

---

## 5. CI

`.github/workflows/route-gates.yml` 在 push/PR 到 `main` 时跑 **fast** 门禁（合规 + 审计 + 任务图校验），不跑 2D 构建（GitHub 2 核构建 WarpX 太慢）。

完整门禁在本地/自托管 runner 跑：

```bash
python3 Tools/automation/route_tasks.py check full
```

---

## 6. 如何加任务

1. 在 `route_tasks.json` 加一项：`id`、`phase`、`depends`、`kind`、`gate`、`acceptance`。
2. 跑 `route_tasks.py next` 验证 DAG。
3. 若是脚本任务，把命令写进 `automation_command`；若是重构，留空，靠 `--gate --accept`。
4. 更新本文的阶段表。

---

## 7. 当前下一步

`A1.4 Batch C6：剩余专用诊断`：

- 目标：把 `Diagnostics/ReducedDiags` 的 76 处 `WarpX::GetInstance()/WarpX::static` 改成 `ctx.config()/ctx.time()`。
- 门：`build+smoke` + reduced-diag 相关 ctest。
- 完成后再进 A1.5 `FullDiagnostics`。

> 自动化的价值是**让每个增量都必须过同一组门**，不是替代设计。路线仍是 16–30 人月。
