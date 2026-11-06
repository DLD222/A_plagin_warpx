# 修复复核报告（v2）：26 项问题在新代码中的实际状态

<!-- SPDX-License-Identifier: BSD-3-Clause -->

> **文档性质**：只读复核报告，**未修改任何源码**。
> **复核日期**：2026-11-06
> **复核基线**：`main @ 9d06beb`（`feat(checkpoint): write additive field_schema manifest from FieldRegistry; A4.4b`）
> **复核对象**：用户最新代码（A4.4b）。**不是** `trae/agent-iGh8PB` 上的旧 A1 代码。
> **一句话结论**：**8 项完全修复，2 项部分修复，16 项仍未修复**。

---

## 0. 先更正上一版报告（v1）的错误

v1 结论是"0 项修复"，那是**基线取错**，不是代码没改。原因如下：

| 事实 | 命令 / 证据 |
|---|---|
| 当前工作区检出的是旧分支 | `git rev-parse --abbrev-ref HEAD` → `trae/agent-iGh8PB` |
| 该分支最新只有文档 + 两个只含 `*.png` 的自动备份提交 | `git show --stat 60397e2 68930db` → 仅新增/删除 `.screenshots/*.png` |
| 用户的新代码在 `main` | `git log main --pretty='%h %ad %s'` → `9d06beb 2026-11-06 20:58 A4.4b` |
| 两者**没有共同祖先** | `git merge-base main HEAD` → 空输出 |

因此 v1 里的 `git diff 4277ca1..HEAD -- Source tests Tools .github` 只能证明"旧分支没动"，
完全看不到 `main` 上的 A4.4b 改动。**v1 的 0 项修复结论作废，以本文件为准。**

---

## 1. 结论总览

| 状态 | 数量 | 编号 |
|---|---:|---|
| ✅ 完全修复 | **8** | H1、H2、H4、H5、H6、M2、M6、L3 |
| 🟡 部分修复 | **2** | H3、M3 |
| ❌ 仍未修复 | **16** | H7、M1、M4、M5、M7、M8、M9、M10、M11、M12、L1、L2、L4、L5、L6、L7 |

> 26 = 7（H）+ 12（M）+ 7（L）。判定标准：S1 代码变更、S2 证据消失、S3 行为正确。
> 本沙箱仍无 AMReX / conda / `build/`，**未编译、未跑 ctest**；所有结论均为静态取证。

---

## 2. 已完全修复的 8 项

| # | 修复证据（`main` 当前代码） | 说明 |
|---|---|---|
| **H1** | `Runtime/WarpXRuntimeData.cpp:25-38`：改为 `thread_local std::unique_ptr`，适配器**不再持有 WarpX 引用**，每次调用刷新 `m_warpx`；`w()` 在空指针时才回落 `GetInstance()` | 消除了静态析构顺序 / `Finalize` 后悬空 |
| **H2** | `Runtime.cpp` 中 `WarpX::GetInstance` **0 命中**；全程用 `m_warpx`（`Runtime.cpp:49,103,124,151`），生命周期由 `WarpX::Create()` / `ResetInstance()` 管理 | Runtime 不再是单例消费者 |
| **H4** | 新增 `Runtime/HostServices.H:38`（接口）+ `WarpXHostServices.H/.cpp`（实现）；`Runtime.cpp:67` 构造、`:71` 注入 `RuntimeContext{..., m_host.get(), ...}` | `Services` 零实现、上下文注入 `nullptr` 已解决 |
| **H5** | `test_runtime_snapshot.cpp:451-453`：`field_probe` 校验已放进 `if (!particles_mode && !psatd_mode)`；`tests/runtime/CMakeLists.txt` 新增 `warpx_runtime_test_workdir()`，每个测试独立工作目录 | 假通过 + 共享目录竞态已消除 |
| **H6** | `tests/runtime/CMakeLists.txt:14-18`：`if(NOT D STREQUAL 2) continue()`，只注册 2D | 输入写死 2D 却注册到所有维度已解决 |
| **M2** | `ReducedDiags.cpp:181-193`：`config()` / `frame()` 改为 `thread_local` **每次调用重算**，不再 `static const` 冻结 | 快照不会在首次 init 后过期 |
| **M6** | `RuntimeConfig.H` 改为**实时 getter 视图**（每个字段一个 `xxx() const` 转发到 `WarpX` 静态），无第二套默认值 | 两套真值已合并为一套 |
| **L3** | `Runtime.cpp:149-152`：`verbose()` 用 `m_warpx->Verbose()`，不再取单例 | 已修复 |

---

## 3. 部分修复的 2 项

| # | 现状 | 残留 |
|---|---|---|
| **H3** | `FieldEnergy` / `Timestep` / `MultiReducedDiags` 已 0 命中；`ReducedDiags::data()` 走上下文 | 仍有 **6 处 legacy 回落**：`Diagnostics.cpp:84,729`、`ComputeDiagFunctor.cpp:33,45`、`ComputeParticleDiagFunctor.cpp:26`、`ReducedDiags.cpp:175`。它们只在 `ctx == nullptr`（Python/无 Runtime 路径）时触发，属**有意的兼容回落**，但意味着诊断子系统尚未真正脱离单例 |
| **M3** | `RuntimeConfig` 已改为实时 getter，原来"复制 6 个 Vector + 44 标量"的大头消失 | `Runtime.cpp:221-237`：`sync_time()` 仍每次 `m_time = make_time(*m_warpx); m_frame = FrameState::FromLegacy();`，`initialize()` 与**每一步 `evolve()`** 都调 `sync_state()`。复制量已降到约 10 个标量，但"每步全量刷新"的语义仍在 |

---

## 4. 仍未修复的 16 项

| # | 问题 | 证据（`main` 当前代码） | 最小修复量 |
|---|---|---|---|
| **H7** | CI 仍只挂 `main`（上游主分支是 `development`） | `.github/workflows/route-gates.yml:6,8` → `branches: [main]` | 改 2 行 |
| **M1** | `Timestep` 构造期 `data().maxLevel()`，而上下文路径用 `finest_level` | `Timestep.cpp:29` vs `:69`（`:72` 回落仍 `maxLevel()`）。AMR 时表头列数与数据列数可能不一致 | 1 行：构造期也取 `finest_level` |
| **M4** | 构造期快照早于 `InitData`，`dt` 是哨兵值 | `Runtime.cpp:68-71`（`sync_config(); sync_time();`）发生在 `m_warpx->InitData()`（`:104`）之前 | 加 `valid` 标志，`InitData` 前不发布 `TimeState` |
| **M5** | `initialize()` 无幂等保护 | `Runtime.cpp:93-114` 只查 `m_finalized`；重复调用会二次执行 `InitData` 与 `AllocData` | 1 行：`if (m_initialized) { return; }` |
| **M7** | 4 个文件重复 `#include` | 实测 `ParticleEnergy 24/23`、`ParticleExtrema 43/42`、`ParticleMomentum 23/22`、`ParticleNumber 17/16` | 删 4 行 |
| **M8** | include 分组顺序违反 `AGENTS.md` | `FieldEnergy.cpp:10-14`：`Runtime/RuntimeData.H` 排在 `Fields.H`、`Diagnostics/...` 之前（按字母序应在 `Fields` 之后、`Utils` 之前） | 批量重排 |
| **M9** | 同一输入文件重复定义 `warpx.reduced_diags_names` | `tests/runtime/inputs_2d_particles:17` 与 `:53` 两处同名 key | 删 1 行 |
| **M10** | 未按 `add_warpx_test()` 接入；`step_info` / `lb_eff` 从不断言 | `tests/runtime/CMakeLists.txt` 中 `add_warpx_test` **0 命中**；测试只读 `field_energy/field_max/field_mom/field_poynting/field_reduce/part_*/rho_max/extrema/field_probe`，**无 `step_info.txt`、无 `lb_eff.txt`**（但 `inputs_2d_particles:53` 已配置 `lb_eff`） | 改造测试 |
| **M11** | `refactor` 可绕过 `--accept` | `Tools/automation/route_tasks.py:180` 仍是 `... or args.accept or t.get("automation_command")` | 删半个条件 |
| **M12** | 进度真值冲突 | `ROUTE_STATUS.md:18,26` 写 A1.1/A1.4e `done`；`route_tasks.json` 中两者仍 `pending`；`route_tasks.py next` 仍输出 **A1.1** | 需定唯一真值来源 |
| **L1** | `RuntimeContext` 裸指针聚合、可复制 | `RuntimeContext.H:29-40` 未变（6 个裸指针 + `std::function`，无 `= delete`） | 禁用拷贝或改 `gsl::not_null` |
| **L2** | `~Runtime()` 无条件 `finalize_external_libraries()` | `Runtime.cpp:144-147` 未变；`initialize()` 从未调用也可能执行 finalize | 按构造成功标志决定 |
| **L4** | 注释中的 `GetInstance` 干扰检索 | `RuntimeData.H:44`、`WarpXRuntimeData.H:22` 仍写 `WarpX::GetInstance()` | 改注释 |
| **L5** | `field_max` 用 `1e-14` 绝对容差比对 1.0 | `test_runtime_snapshot.cpp:354-357,460` 仍是 `1.0e-14`；`ReducedDiags.H` 精度仍 14 位 | 改相对容差或放宽到 1e-12 |
| **L6** | 非 RZ 构建也无条件读 `n_rz_azimuthal_modes` | `RuntimeConfig.cpp:226-228` 无 `#ifdef WARPX_DIM_RZ` | 加守卫 |
| **L7** | `parse_headers.py` 硬编码 `/tmp` | `Tools/A0/parse_headers.py:90`：`open('/tmp/header_catalog.json','w')` | 改为仓库内路径 |

---

## 5. 四个门禁为什么仍然说"通过"

| 误导源 | 现状 |
|---|---|
| `ROUTE_STATUS.md` | 全 `done`，但任务图 JSON 仍 `pending`（M12） |
| `compliance.py` / `audit_check.py` | 只查文件名/数量，不查本报告任何一条 |
| `runtime_snapshot` 测试 | H5 已修（不再假通过），但 `lb_eff` 配了不验（M10） |
| `route-gates.yml` | 只挂 `main`，合并到 `development` 时不触发（H7） |

---

## 6. 建议的最小修复顺序

1. **一行级**（M5、M1、M11、H7、M9、M7、L7、L6、L5、L4）—— 低风险，可一次提交。
2. **测试可信度**（M10，`add_warpx_test` + `lb_eff`/`step_info` 断言；M8 重排）—— 让门禁说真话。
3. **语义补全**（M4 构造期哨兵、L1/L2 生命周期）—— 消除隐藏崩溃面。
4. **状态真值**（M12）—— 让 `ROUTE_STATUS` 与 JSON 由同一脚本生成，禁止手改。
5. **H3 收尾**（6 处 legacy 回落）—— 需等 Python 路径也走 RuntimeContext，属 A5 范围。

---

## 7. 静态无法定论、建议实测的 3 个实验

| 优先级 | 实验 | 预期 |
|---|---|---|
| 1 | 按 `AGENTS.md` 配置 `-DWarpX_DIMS=2 -DWarpX_PYTHON=ON` 编译 | 暴露 A4.4b 新文件的编译问题 |
| 2 | `ctest -R runtime_snapshot --output-on-failure`（独立目录） | 验证 H5 修复后是否真通过 |
| 3 | `-DWarpX_DIMS="1;3;RZ"` 构建 | 验证 H6 守卫是否真正只跳过 |

---

## 附录 A：复核命令（可直接重跑）

```bash
# 0. 基线
git log main --pretty='%h %ad %s' --date=iso -3
git merge-base main HEAD          # 空 → 无共同祖先

# 已修复
git grep -n "thread_local std::unique_ptr<WarpXRuntimeData>" main -- Source/Runtime/WarpXRuntimeData.cpp
git grep -c "WarpX::GetInstance" main -- Source/Runtime/Runtime.cpp            # 0
git grep -n "class HostServices" main -- Source/Runtime/HostServices.H        # 38
git grep -n "particles_mode" main -- tests/runtime/test_runtime_snapshot.cpp  # 451-453
git grep -n "NOT D STREQUAL 2" main -- tests/runtime/CMakeLists.txt           # 14
git grep -n -A4 "ReducedDiags::config" main -- Source/Diagnostics/ReducedDiags/ReducedDiags.cpp

# 未修复
git grep -n "branches" main -- .github/workflows/route-gates.yml              # 6,8
git grep -n "maxLevel" main -- Source/Diagnostics/ReducedDiags/Timestep.cpp   # 29,72
git grep -n "m_initialized" main -- Source/Runtime/Runtime.cpp                # 112,119（无 initialize 幂等）
git grep -n "sync_state" main -- Source/Runtime/Runtime.cpp                   # 113,123
git grep -n "reduced_diags_names" main -- tests/runtime/inputs_2d_particles   # 17,53
git grep -c "add_warpx_test" main -- tests/runtime/CMakeLists.txt             # 0
git grep -n "automation_command" main -- Tools/automation/route_tasks.py      # 180
git grep -n "A1.1\|A1.4e" main -- Docs/research_notes/ROUTE_STATUS.md         # 18,26 done
git show main:Tools/automation/route_tasks.json | python3 -c \
  "import json,sys;d=json.load(sys.stdin);print({k:v.get('status') for k,v in d['tasks'].items() if k in ('A1.1','A1.4e')})"
git grep -n "1.0e-14" main -- tests/runtime/test_runtime_snapshot.cpp          # 354-357,460
git grep -n "n_rz_azimuthal_modes" main -- Source/Runtime/RuntimeConfig.cpp   # 226-228
git grep -n "/tmp/header_catalog.json" main -- Tools/A0/parse_headers.py      # 90
```

---

## 附录 B：与既有文档的关系

| 文档 | 关系 |
|---|---|
| `CODE_REVIEW_REPORT.md`（`trae/agent-iGh8PB`） | 26 项问题的原始来源；本文件逐条对应，编号不变 |
| `ROUTE_AUDIT_REPORT.md` | 架构层缺口（F/S/C 系列）。A4.4b 已覆盖其中一部分（HostServices/DataBus/FieldRegistry），本文件不重复 |
| `ROUTE_STATUS.md`（`main`） | 进度声明；M12 仍与其任务图 JSON 冲突 |
| `最终的技术路线.md` | 技术要求基线（I1–I6、§1.3 P1） |

> 本文件只做复核。修复后请用附录 A 命令重跑，把 ❌/🟡 改为 ✅。
