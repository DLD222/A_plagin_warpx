# 代码审计报告（2026-11-07，A4.4b + openPMD/PML 修复）

<!-- SPDX-License-Identifier: BSD-3-Clause -->

> **文档性质**：只读审计，**未修改任何源码**。
> **审计基线**：`main @ 2050e6a`（`feat(io): enable openPMD with parallel HDF5; fix PML field recreation on load balance`）
> **对照文档**：`Docs/research_notes/ROUTE_COMPLETE_2026-11-07.md`（153/153 完成）
> **沙箱限制**：无 AMReX / conda / `build/`，**未编译、未跑 ctest**；所有结论为静态取证。

---

## 0. 审计范围声明

本仓库含 764 个测试文件、184 个粒子源文件等，**不可能逐行通读**。本次审计聚焦：

1. `Source/Runtime/` 全部 43 个文件（用户新增的插件化 Runtime 层）；
2. `Source/Evolve/WarpXEvolve.cpp` 中 stage handler 接入逻辑；
3. `Source/Initialization/WarpXInitData.cpp` 的 PML 重建修复；
4. `tests/runtime/` 全部测试与输入；
5. 对前序报告 26 项问题逐条复核（见 §1）。

**未覆盖**：上游 WarpX 未改动文件、`Examples/`、`Regression/`、Python 绑定。这些不在本次改动范围。

---

## 1. 前序 26 项问题的当前状态

| 项 | 状态 | 证据（`main@2050e6a`） |
|---|---|---|
| H1 | ✅ | `WarpXRuntimeData.cpp:25-38` `thread_local`，不持引用 |
| H2 | ✅ | `Runtime.cpp` 中 `GetInstance` 0 命中 |
| H3 | 🟡 部分 | ReducedDiags 目标文件已 0；诊断子系统仍留 6 处 legacy 回落 |
| H4 | ✅ | `HostServices.H:38` + `WarpXHostServices`（53 个 override 全覆盖） |
| H5 | ✅ | `test:451-453` 守卫；CMakeLists 独立工作目录 |
| H6 | ✅ | `CMakeLists.txt:14` `if(NOT D STREQUAL 2) continue()` |
| H7 | ✅ | `route-gates.yml:6,8` → `branches: [main, development, "trae/*"]` |
| M1 | 🟡 部分 | `Timestep.cpp:29` 构造期仍 `maxLevel()`，ctx 路径用 `finest_level` |
| M2 | ✅ | `ReducedDiags.cpp` `thread_local` 每次重算 |
| M3 | 🟡 部分 | 配置复制已消除；`sync_time()` 仍每步复制 TimeState/FrameState |
| M4 | 🟡 部分 | `TimeState.valid` 标志已加；构造期 `dt` 仍为哨兵 |
| M5 | ✅ | `Runtime.cpp` `initialize()` 加 `if (m_initialized) return;` |
| M6 | ✅ | `RuntimeConfig` 为 stateless 实时 getter |
| M7 | ✅ | 4 文件 include 计数一致（23/23 等） |
| M8 | ❌ | include 分组字母序仍违规（见 §2-L1） |
| M9 | ✅ | `inputs_2d_particles` 仅一处 `reduced_diags_names` |
| M10 | 🟡 部分 | `step_info.txt`/`lb_eff.txt` 已断言；仍未用 `add_warpx_test` |
| M11 | ✅ | `route_tasks.py:180` 去掉 `or t.get("automation_command")` |
| M12 | ✅ | `route_tasks.json` 中 A1.1/A4.4b 均 `done`，与 `ROUTE_STATUS` 一致 |
| L1 | ✅ | `RuntimeContext` 拷贝已 `= delete` |
| L2 | ✅ | `~Runtime()` 用 `m_external_libraries_initialized` 守卫 |
| L3 | ✅ | `verbose()` 用 `m_warpx` |
| L4 | ❌ | 注释残留 `GetInstance`（`RuntimeData.H:44` 等） |
| L5 | ✅ | 测试不再用 `1.0e-14` 绝对容差 |
| L6 | ✅ | `RuntimeConfig.cpp:228` `#ifdef WARPX_DIM_RZ` |
| L7 | ✅ | `parse_headers.py` 不再硬编码 `/tmp` |

**汇总**：19 完全修复 / 5 部分修复 / 2 未修复（M8、L4）。

---

## 2. 本轮新发现的问题

### 高危（可能导致崩溃 / UAF / 错误结果）

#### **B1. `Runtime::finalize()` 销毁顺序——潜在 use-after-free**

`Source/Runtime/Runtime.cpp:127-143`：

```cpp
m_host.reset();                          // (1) 销毁 WarpXHostServices
if (m_warpx != nullptr) {
    m_warpx->SetCollisionHandler(nullptr);
}
m_collision_handler.reset();             // (2) 销毁 CollisionHandler
```

`CollisionHandler`（`Particles/Collision/CollisionHandler.H:51`）持有 `HostServices* m_host` 裸指针。当前先销毁 host、再销毁 collision handler。若 `CollisionHandler` 析构链中任何成员析构访问 `m_host`，即 UAF。当前 `CollisionHandler` 析构为默认则无影响，但该顺序**脆弱**——一旦未来在析构中加 host 调用即崩溃。

**修复**：调换顺序——先 `m_collision_handler.reset()`，再 `m_host.reset()`。

#### **B2. Stage 前置条件检查不一致**

`Source/Evolve/WarpXEvolve.cpp`：

- `RunPushStage` / `RunFinalizeStage` / `RunFieldSolveStage` / `RunCollisionStage`：检查 `host != nullptr && data != nullptr`
- `RunDiagnosticsStage`（`WarpXEvolve.cpp:655`）：**只检查 `host != nullptr`**，不检查 `data`
- `RunRegridStage`（`:644`）：**只检查 `host != nullptr`**，不检查 `data`

`DiagnosticsHandler::apply` 只检查 `host`，不检查 `data`；若 `ctx.data` 为空且 handler 内访问 data，即空指针解引用。

**修复**：统一所有 stage 的前置条件为 `host != nullptr && data != nullptr`，并在 `DiagnosticsHandler::apply` 中补 `ctx.data` 检查。

#### **B3. `PluginHost::ordered_specs()` 数据源不一致**

`Source/Runtime/HandlerSpec.H`：

- `add()` 把 spec 同时推入 `m_specs` 和 `m_handler_specs`，且冲突检测遍历 `m_specs`（含 `declare()` 的声明）。
- `ordered_specs()`：若 `m_handler_specs` 非空，**只用 `m_handler_specs` 排序**，忽略 `declare()` 声明但未绑定 handler 的 spec。

后果：通过 `declare()` 声明的依赖关系不参与拓扑排序，可能导致 handler 执行顺序与声明的 reads/writes 约束不一致。

**修复**：`ordered_specs()` 应合并 `m_specs` 与 `m_handler_specs`，或统一只用 `m_handler_specs` 并让 `add()` 的冲突检测也只看 `m_handler_specs`。

#### **B4. `PluginHost::add()` 冲突检测不完整**

`HandlerSpec.H` 的 `add()` 只检测**同一 StagePoint 上 `writes_fields` 的冲突**，不检测：

1. `writes_species` 冲突；
2. 同一 handler 被 `add()` 两次（会推入两份 `m_handlers`，执行两次）；
3. `reads_fields` 与 `writes_fields` 跨 handler 的真实依赖只在 `order_indices` 中处理，`add()` 时不预检。

**修复**：补充 `writes_species` 冲突检测；用 `name` 去重（同一 name 不允许重复 add）。

---

### 中危（逻辑 / 语义错误，可能导致静默错误）

#### **M1. `RuntimeConfig::FromLegacy()` 与 stateless 设计矛盾**

`RuntimeConfig` 是无状态实时 getter 类，但：

- `Runtime.cpp:221` `sync_config()` 仍执行 `m_config = RuntimeConfig::FromLegacy();`
- `RuntimeConfig::FromLegacy()`（`RuntimeConfig.cpp:314`）只返回默认构造的空对象

`m_config` 是空壳，`sync_config` 是无操作。若未来有人给 `RuntimeConfig` 加成员，这里会被悄悄覆盖。

**修复**：删除 `sync_config()` 或让 `FromLegacy()` 明确返回 `RuntimeConfig{}` 并加注释说明无状态。

#### **M2. `DataBus` 双构造器导致 `m_owned_registry` 永远闲置**

`DataBus.H`：

```cpp
FieldRegistry m_owned_registry;          // 默认构造，从未使用
FieldRegistry* m_registry = &m_owned_registry;
```

`Runtime.cpp` 用 `DataBus(fields, warpx.fieldSchema())` 构造，`m_registry` 指向外部 registry，`m_owned_registry` 始终为空对象。浪费内存且易误导。

**修复**：若总是外部注入，删除 `m_owned_registry` 成员，构造器强制要求 `FieldRegistry&`。

#### **M3. `StageContext` const 不对称**

`HandlerSpec.H` 的 `StageContext`：`data` 是 `const RuntimeData*`，但 `bus` 是 `DataBus*`（非 const）。handler 可通过 `bus` 修改字段注册，却不能通过 `data` 读写字段——语义割裂，易让插件作者误解"哪些能改"。

**修复**：要么 `bus` 也 const（handler 不应改注册），要么明确文档说明 `bus` 是字段访问通道、`data` 是只读几何/时钟。

#### **M4. `WarpXPluginDesc` 非 C-compatible，却宣称 C ABI**

`PluginABI.H`：

```cpp
struct WarpXPluginDesc {
    std::uint32_t abi_version = 0;        // 默认成员初始化器（C 不支持）
    std::uint32_t struct_size = 0;
    const char* name = nullptr;
    const char* version = nullptr;
    void (*register_handlers)(PluginHost*) = nullptr;
};
```

该结构带默认成员初始化器，**不是 C 兼容 aggregate**。若插件用 C 编译器编译（"C ABI" 的本意），会编译失败。当前靠 `struct_size` 校验兜底，但 ABI 名不副实。

**修复**：去掉默认成员初始化器，或在文档中明确"插件必须用 C++ 编译，`extern "C"` 仅用于符号名"。

#### **M5. `Timestep` 构造期 `maxLevel()` 与运行期 `finest_level` 不一致（M1 残留）**

`Timestep.cpp:29` 构造期 `data().maxLevel()` 决定 `m_data` 大小与表头列数；`:69` 运行期用 `ctx->time->finest_level`。AMR 下 `maxLevel > finestLevel` 时，表头列数 > 数据行数，末尾列写零或越界。

**修复**：构造期也用 `finestLevel()`，或在运行期按 `max_level` resize `m_data`。

#### **M6. `Runtime::sync_state()` 每步全量刷新**

`evolve()` 后调 `sync_state()` → `sync_time()` 复制 `TimeState`（8 个 Vector）+ `FrameState`。每步都做，虽量不大但无必要（`refresh_time` 回调已在 step 边界触发）。

**修复**：`evolve()` 内不调 `sync_state`，依赖 `refresh_time` 回调；或改为只刷新确有变化的字段。

---

### 低危（风格 / 健壮性 / 可维护性）

#### **L1. include 分组顺序违反 `AGENTS.md`（M8 残留）**

- `Runtime.H`：`FrameState.H` 后直接 `<ablastr/fields/MultiFabRegister.H>`，再 `"Runtime/RuntimeConfig.H"`——`ablastr` 与 `Runtime/` 应同组按字母序。
- `RuntimeData.H`：`<ablastr/utils/text/IntervalsParser.H>` 在 `<ablastr/fields/MultiFabRegister.H>` 之前（`fields` < `utils`）；`<memory>` 插在 AMReX 头之间。

#### **L2. `Runtime::verbose()` 无显式 `m_warpx` 空检查**

`Runtime.cpp:149`：`return m_initialized && !m_finalized && m_warpx->Verbose();`。靠 `!m_finalized` 隐式保证 `m_warpx` 非空，脆弱。

**修复**：加 `m_warpx != nullptr &&`。

#### **L3. CMakeLists.txt 缩进不一致**

`Source/Runtime/CMakeLists.txt`：`LevelResources.cpp`、`SpeciesSpec.cpp` 未缩进，其余缩进。

#### **L4. `DataBus::speciesRegistry()` 与 `registry()` 命名不对称**

`DataBus.H`：`registry()` 返回 `FieldRegistry`，`speciesRegistry()` 返回 `SpeciesRegistry`。命名风格不一致（一个带 `species` 前缀，一个不带 `field`）。

#### **L5. `FieldRegistry::get()` 抛 `out_of_range` 而 `find()` 返回 nullptr**

两个接口语义不统一，调用方易错。建议统一为返回 `std::optional` 或都返回指针。

#### **L6. 测试未用 `add_warpx_test()`（M10 残留）**

`tests/runtime/CMakeLists.txt` 仍用裸 `add_test`，未走 `add_warpx_test` 封装（AGENTS.md 要求）。

#### **L7. 注释残留 `GetInstance`（L4 残留）**

`RuntimeData.H:44`、`WarpXRuntimeData.H:22` 注释仍写 `WarpX::GetInstance()`，干扰检索。

---

## 3. 本轮确认正确的设计点

| 点 | 证据 |
|---|---|
| PML 重建修复正确 | `WarpXInitData.cpp:1122` `CreatePMLForLevel`：重建前 `pml.reset()` + 擦除 `pml_*[level=..]` 字段，避免 `alloc_init` 重复 |
| Stage handler 无双执行 | `RunPushStage`→handler→`host.pushParticlesAndDeposit`→`RunPicPush`→`OneStep_PushParticles`；handler 路径 early return，不落回 legacy |
| 单例无重复实例 | `WarpX.cpp:360` 构造期 `m_instance = this`；`GetInstance` 只在 `m_instance` 空时创建 |
| `HostServices` 全覆盖 | `HostServices.H` 54 virtual（含析构）= `WarpXHostServices.H` 53 override + 析构 |
| `FieldSchema` 别名正确 | `FieldSchema.H:16` `using FieldSchema = FieldRegistry`，匹配 `DataBus` 构造器 |
| 插件加载安全 | `PluginLoader.cpp` 校验 `struct_size` + `abi_version`，`RTLD_NODELETE` 保 handler 存活 |

---

## 4. 建议修复顺序

1. **B1**（finalize 顺序）—— 1 行调换，消除 UAF 风险
2. **B2**（stage 前置条件统一）—— 补 `data` 检查
3. **B3 + B4**（PluginHost 排序与冲突检测）—— 插件执行正确性的基础
4. **M5**（Timestep maxLevel/finestLevel）—— 数据列数对齐
5. **M1 + M2**（stateless 语义清理）—— 防止未来回归
6. **L 系列**（风格/健壮性）—— 批量处理

---

## 5. 复核命令

```bash
# B1
git grep -n "m_host.reset\|m_collision_handler.reset" 2050e6a -- Source/Runtime/Runtime.cpp
# B2
git grep -n "m_plugin_host != nullptr" 2050e6a -- Source/Evolve/WarpXEvolve.cpp
# B3
git grep -n "m_handler_specs\|m_specs" 2050e6a -- Source/Runtime/HandlerSpec.H
# B4
git grep -n "writes_species\|existing.name" 2050e6a -- Source/Runtime/HandlerSpec.H
# M5
git grep -n "maxLevel\|finest_level" 2050e6a -- Source/Diagnostics/ReducedDiags/Timestep.cpp
# HostServices 覆盖
git grep -c "virtual" 2050e6a -- Source/Runtime/HostServices.H
git grep -c "override" 2050e6a -- Source/Runtime/WarpXHostServices.H
```

---

## 6. 与既有审计报告的关系

| 文档 | 关系 |
|---|---|
| `CODE_REVIEW_REPORT.md` | 26 项原始来源；本报告 §1 逐条更新其在 A4.4b 的状态 |
| `FIX_VERIFICATION_REPORT.md` | 上轮复核（v2 对照 main@9d06beb）；本轮对照更新的 main@2050e6a |
| `ROUTE_COMPLETE_2026-11-07.md` | 技术路线完成声明（153/153）；本报告对照其声称的完成度做代码层核验 |

> 本报告为只读审计。修复后请用 §5 命令重跑，把 ❌/🟡 改为 ✅。
