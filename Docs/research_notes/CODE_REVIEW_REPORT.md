# 代码审查报告：A1 阶段改动的逻辑 / 语法 / 技术要求符合性

<!-- SPDX-License-Identifier: BSD-3-Clause -->

> **文档性质**：只读审查报告。**未修改任何源码**。
> **审查日期**：2026-11-06
> **审查基线**：`Docs/research_notes/最终的技术路线.md`（唯一准绳，含 §1.2 不变量 I1–I6、§1.3 实施原则 P1）、`Docs/research_notes/AUTOMATION.md`（任务图与门禁约定）、`Docs/research_notes/ROUTE_AUDIT_REPORT.md`（已审计缺口 F1–F5 / S1–S8 / C1）、`AGENTS.md`（代码风格与测试规范）。
> **审查对象**：自根提交 `7bcb44a`（源码拷贝，对应上游 `0de0e26`）以来**全部被更改的文件**。
> **一句话结论**：这一批改动是**行为保持型（behavior-preserving）**的接口迁移，数值逻辑正确、P1 原则落实到位；但**它没有推进路线的核心目标**（去单例、内核不含物理、DataBus 命名状态），并且引入 **7 条高危问题**（2 条会导致进程级生命周期错误，3 条使测试/门禁失真，2 条使"验收已通过"的结论不成立）。

---

## 0. 结论摘要

### 0.1 判定

| 维度 | 结论 | 说明 |
|---|---|---|
| **数值/物理行为** | ✅ 保持不变 | 16 个 ReducedDiags 的替换均为等价取值（`warpx.X` → `data().X` / `config().X` / `frame().X`）；算术表达式、循环顺序、归约方式均未动；Strang 序列、checkpoint 写出顺序未被触碰 |
| **P1（能复制就复制、能小改就小改）** | ✅ 基本落实 | 没有重写任何内核函数；只在原文件里替换了取值入口；`WarpX` 仅 +7 行（3 个访问器 + 1 个成员） |
| **P1 的例外（契约层）** | ⚠️ 未推进 | 新增的 `RuntimeData` 是**类型化门面**，不是路线 §3.2/§3.3 要求的 `FieldKey/SpeciesKey` 命名总线；`Runtime` 仍是 `WarpX` 单例的消费者 |
| **I1 内核不含物理** | ❌ 未达成 | `RuntimeConfig` 持有 44 个物理枚举/边界/形状因子字段；`RuntimeData` 直接暴露 `MultiFabRegister` / `MultiParticleContainer` |
| **I2 插件不互认** | ⚠️ 无从谈起 | 本批没有插件对象，只有 `Services` 的**纯声明**（`nullptr` 注入，零实现） |
| **I3 生命周期归内核** | ✅ 未破坏 | `Alloc/Remake/FillBoundary/checkpoint` 一行未改 |
| **I5 逐位可回归** | ✅ 本批未引入新风险 | 唯一涉及 IO 顺序的改动是 A1.4e 的 `FieldProbe` 构造参数，不改变写出顺序 |
| **语法/编译** | 🟡 静态审查通过，**未实测编译** | 缺少 AMReX 头文件与构建环境（见 §1.3）。逐头核对未发现确定性编译错误，但有 3 处依赖传递包含的风险点 |
| **测试可信度** | ❌ 存在**假通过** | `test_runtime_snapshot` 在 particles 模式下依赖另一个测试留下的陈旧文件；多测试共享工作目录 |
| **门禁可信度** | ❌ 两处失效 | CI 只在 `main` 分支触发；`route_tasks.json` 未反映已声称完成的 A1.1–A1.4e |

### 0.2 问题计数

| 级别 | 数量 | 编号 |
|---|---|---|
| 🔴 高危 | 7 | H1–H7 |
| 🟡 中危 | 12 | M1–M12 |
| 🟢 低危 / 风格 | 7 | L1–L7 |

---

## 1. 审查范围与方法

### 1.1 被审查文件（共 47 个代码/构建/工具文件）

| 分组 | 文件数 | 清单 |
|---|---|---|
| 新增 Runtime 层 | 13 | `Source/Runtime/`：`Runtime.{H,cpp}`、`RuntimeConfig.{H,cpp}`、`RuntimeContext.H`、`TimeState.H`、`FrameState.{H,cpp}`、`RuntimeData.H`、`WarpXRuntimeData.{H,cpp}`、`Services.H`、`CMakeLists.txt`、`Make.package` |
| 内核改动 | 5 | `Source/main.cpp`、`Source/WarpX.H`、`Source/Evolve/WarpXEvolve.cpp`、`Source/Initialization/WarpXInitData.cpp`、`Source/Make.WarpX` |
| 诊断迁移 | 18 | `MultiReducedDiags.cpp`、`ReducedDiags.{H,cpp}`、`FieldEnergy`、`FieldMaximum`、`FieldMomentum`、`FieldPoyntingFlux`、`FieldProbe`、`FieldReduction.H`、`LoadBalanceEfficiency`、`ParticleEnergy`、`ParticleExtrema`、`ParticleHistogram`、`ParticleHistogram2D`、`ParticleMomentum`、`ParticleNumber`、`RhoMaximum`、`Timestep` |
| 测试 | 4 | `tests/runtime/CMakeLists.txt`、`test_runtime_snapshot.cpp`、`inputs_2d`、`inputs_2d_particles` |
| 构建 / 工具 / CI | 7 | `CMakeLists.txt`、`Tools/A0/parse_headers.py`、`Tools/A0/build_catalog.py`、`Tools/automation/route_tasks.py`、`Tools/automation/gates/{compliance,audit_check}.py`、`Tools/automation/gates/{build_2d,smoke_2d,full_gate}.sh`、`.github/workflows/route-gates.yml`、`.gitignore` |

另有 58 个 `.md` 文档改动（`Docs/research_notes/`、`Docs/A0/api/` 等），不在"代码逻辑/语法"审查范围内，仅在 §7 做一致性抽查。

### 1.2 审查方法

对每个文件执行四类检查：

1. **语法/编译可行性**：头文件完整性（`<memory>` / `<functional>` / `<stdexcept>` / `<string>`）、命名空间与前置声明一致性（`struct` vs `class`）、`const` 重载可解析性、枚举/容器类型匹配（`amrex::Array` vs `std::array`、`amrex::IntVect`、`AMREX_D_DECL`）。
2. **逻辑等价性**：逐处对比 `HEAD` 与被替换的原语句，确认取值来源、时间基准（`0.0_rt` vs `t_new`）、循环上界（`maxLevel()` vs `finestLevel()`）、条件编译分支（`WARPX_DIM_RZ`）是否一致。
3. **技术要求符合性**：对照 `最终的技术路线.md` §1.2（I1–I6）、§1.3（P1）、§4（A1 目标与验收）、`AUTOMATION.md`（`refactor` 必须 `--accept`、`--accept` 不能绕过 `verify_command`）、`AGENTS.md`（include 顺序、4 空格、`add_warpx_test`）。
4. **测试/门禁有效性**：测试是否真的能发现问题（而不是靠陈旧文件通过）、是否在所有 `WarpX_DIMS` 下可运行、CI 是否真的会执行这些门禁。

### 1.3 审查限制（必须声明）

- 本沙箱**没有 AMReX 头文件、没有 conda 环境、没有已配置的 `build/` 目录**（`find` 在 `/opt`、`/usr/local`、`/root` 均未找到 `AMReX.H`），因此**无法执行实际编译**。§3–§5 中的"编译风险"是**基于头文件与重载的静态推断**，不是编译器报错。
- 因此本报告**不对"能否编译通过"下最终结论**，只给出"确定性问题"与"需实测确认的风险点"，并在 §8 列出必须实测的项目。
- 三个快速门禁可以运行，实测结果见 §7.2。

---

## 2. 技术要求逐项对照

### 2.1 P1 实施原则（`最终的技术路线.md` §1.3）

| P1 要求 | 本批落实 | 证据 |
|---|---|---|
| 能复制就复制、能小改就小改 | ✅ | 16 个诊断只替换取值入口，函数体一行未重写；`FieldReduction.H` 模板保留 `WarpX::LowerCorner` 原调用 |
| 改接口壳、不改内核逻辑 | ✅ | `WarpX.H` 仅新增 3 个成员函数 + 1 个指针成员（`WarpX.H:1118-1119,1207`） |
| 改调用点、不改被调函数体 | ✅ | `WarpXEvolve.cpp` 只在步内插入 2 行 `refresh_time()`；`WarpXInitData.cpp` 同理；`ComputeDt`/`EvolveB`/`EvolveE` 零改动 |
| 契约层该重设计就重设计 | ❌ | 新增的 `RuntimeData` 选择了"类型化门面"而非路线的命名总线，见 H2 |

### 2.2 A1 阶段的验收（路线 §4 A1 / §9）

| 验收项 | 状态 | 证据 |
|---|---|---|
| `RuntimeConfig` 只读配置 | 🟡 形式达成 | 类型存在（`RuntimeConfig.H`），但它是**静态量的副本**，真值仍在 `WarpX` 静态成员里；两套真值会漂移（M6） |
| `TimeState` / `FrameState` | 🟡 形式达成 | 类型存在，但快照与 `WarpXRuntimeData` 的实时读形成**双通道**（M1） |
| `Services` 注入（FillBoundary/Reduce/Parser/RNG/Log） | ❌ 未达成 | `Services.H` 只有纯虚声明；`Runtime.cpp:49` 注入 `nullptr`；全仓库**无任何 `Services` 实现类** |
| ReducedDiags 不再出现 `WarpX::GetInstance()` | ❌ 未达成 | `ReducedDiags.cpp:150`、`Timestep.cpp:28,72`、`FieldEnergy.cpp:184`（RZ 分支）、`MultiReducedDiags.cpp` 仍有调用；`WarpXRuntimeData::Legacy()` 内部也在调用 |
| **A1 核心目标"去单例"** | ❌ 反向 | `Runtime.cpp` 新增 5 处 `WarpX::GetInstance()`（`:24,45,66,77,92`）——Runtime 本身成了又一个单例消费者 |

> **关键判断**：A1 的**形式**（类型齐备、诊断迁移）基本完成，A1 的**实质**（消除进程级全局状态、内核不依赖 WarpX 单例）**没有推进**。这不是本批的失误，而是策略选择：先建门面、后拆单例。但 `ROUTE_STATUS.md` 把 A1.1–A1.4e 标为 done，容易让后续阶段误以为"单例已拆"。

### 2.3 不变量

| 不变量 | 本批影响 |
|---|---|
| I1 内核不含物理 | `Runtime/` 目录本身持有大量物理枚举（`nox`、`current_deposition_algo`、`field_boundary_lo/hi`、`n_rz_azimuthal_modes`、`ect_conformal_E`…）。路线 §4 A1 允许 `RuntimeConfig` 承载算法选择，因此**不算违约**，但它使"内核不含物理"只剩字面意义（P1 的代价） |
| I2 插件不互认 | 无插件对象，未涉及 |
| I3 生命周期归内核 | 未改 `Alloc/Remake/FillBoundary`；`ClearLevel` 也未被本批触碰 |
| I4 旧输入不破坏 | ✅ 未新增/删除任何输入参数 |
| I5 逐位可回归 | ✅ 本批的取值替换不改变任何数值；但**测试无法证明**这一点（见 H5） |
| I6 GPU 不被抽象拖慢 | ✅ 未进入 kernel；`data()` 调用只出现在每个诊断一次/每步一次 |

---

## 3. 高危问题（H1–H7）

### 🔴 H1. `WarpXRuntimeData::Legacy()` 的静态对象在 `WarpX::Finalize()` 之后悬空

**位置**：[WarpXRuntimeData.cpp](file:///workspace/Source/Runtime/WarpXRuntimeData.cpp#L18-L21)

```cpp
WarpXRuntimeData& WarpXRuntimeData::Legacy ()
{
    static WarpXRuntimeData legacy(WarpX::GetInstance());
    return legacy;
}
```

**问题链**：
1. `legacy` 保存 `WarpX& m_warpx`（引用成员）；
2. `WarpX::Finalize()`（`WarpX.cpp:324`）执行 `m_instance = nullptr`，对象被销毁；
3. 函数内 `static` **不会重新初始化**，第二次进入 `Legacy()` 拿到的仍是旧引用；
4. 该路径只在 `m_runtime_context == nullptr` 时进入，即 **Python / pywarpx 路径**（`Runtime` 未安装上下文）。

**失败场景**：同一进程内先跑一次 pywarpx 仿真并 `finalize`，再初始化第二次仿真；或任何在 `Finalize` 之后仍调用 `data()` 的析构/收尾代码 → **use-after-free**（读到已释放的 `WarpX`，表现为随机崩溃或脏数据）。

**为什么本批测试发现不了**：C++ 的 `main` 路径全程有 `RuntimeContext`，永远不会走 `Legacy()`；测试也只测 `main` 路径。

**修复方向**（不实施，仅建议）：`Legacy()` 改为每次调用返回 `WarpXRuntimeData(WarpX::GetInstance())` 的**临时对象**（`data()` 返回引用，需改为返回 by-value 或改为在 `WarpX` 内持有适配器实例，随 `WarpX` 生命周期存亡）。

---

### 🔴 H2. `Runtime` 自身仍是单例消费者，A1 的"去单例"目标被反向

**位置**：[Runtime.cpp](file:///workspace/Source/Runtime/Runtime.cpp)：`:24`（`make_time`）、`:45`（构造函数）、`:66`（`initialize`）、`:77`（`evolve`）、`:92`（`verbose`）

**问题**：引入 `Runtime` 的动机（路线 §2 结论 1–2、§4 A1）是让主程序不再依赖 `WarpX::Evolve` 与全局单例。实际实现中：

```cpp
Runtime::Runtime (int argc, char* argv[])
{
    ...
    auto& warpx = WarpX::GetInstance();      // :45
    m_context = RuntimeContext{..., nullptr, [this] { sync_time(); }};
    warpx.SetRuntimeContext(&m_context);     // :51
}
```

`WarpX` 仍是进程级单例，`Runtime` 只是一个**栈上的包装器**；`Runtime` 析构后 `WarpX` 仍由 `WarpX::Finalize()` 销毁。因此"多实例 / 多包 / 多维度共存"（路线 §2 的 A1 前提）依旧不可能。

**后果**：`RuntimeData` 的抽象因此只能是"对单例的门面"，无法成为路线 §3.3 的 `DataBus`。后续 A5 调度器若基于此抽象实现，会在 A1 的债上再叠一层。

**判定**：方向可接受（渐进式），但 `ROUTE_STATUS.md` 与 `route_tasks.json` 不应把 A1 记成"已完成单例消除"。建议在路线文档中显式写明"A1 = 建门面，不拆单例；去单例属于 A2/A3"。

---

### 🔴 H3. 诊断里仍有 `WarpX::GetInstance()`，与 `ROUTE_STATUS.md` 的"done"矛盾

**位置**：

| 文件:行 | 代码 | 是否在 A1.4 声称的范围内 |
|---|---|---|
| [ReducedDiags.cpp:150](file:///workspace/Source/Diagnostics/ReducedDiags/ReducedDiags.cpp#L150) | `ofs << WarpX::GetInstance().gett_new(0);` | 基类（A1.4a） |
| [Timestep.cpp:28](file:///workspace/Source/Diagnostics/ReducedDiags/Timestep.cpp#L28) | `const auto& warpx = WarpX::GetInstance();` 在构造函数 | A1.4a "Timestep 试点" |
| [Timestep.cpp:72](file:///workspace/Source/Diagnostics/ReducedDiags/Timestep.cpp#L72) | 非 context 分支回退 | A1.4a |
| [FieldEnergy.cpp:184](file:///workspace/Source/Diagnostics/ReducedDiags/FieldEnergy.cpp#L184) | RZ 分支 `WarpX::GetInstance()` | A1.4b 声称"去 GetInstance" |
| `MultiReducedDiags.cpp` | `WarpX::GetInstance().GetRuntimeContext()` | A1.4a |

**问题**：`ROUTE_STATUS.md:20` 明确写 A1.4b = "FieldEnergy/FieldMaximum/FieldMomentum 去 GetInstance"。但 `FieldEnergy.cpp` 在 `#if defined(WARPX_DIM_RZ) || defined(WARPX_DIM_RCYLINDER) || defined(WARPX_DIM_RSPHERE)` 分支内仍保留 `WarpX::GetInstance()`。RZ 构建下该文件**未完成迁移**，而测试只跑 2D，永远暴露不了。

**另**：`Timestep` 的构造函数仍直接取单例——它恰恰是"试点"文件，说明迁移只覆盖了 `ComputeDiags` 的主力路径，构造期路径未覆盖。

**修复方向**：`Timestep` 构造期改用 `data().maxLevel()`；`FieldEnergy` 的 RZ 分支改用 `data().fields()`；`ReducedDiags.cpp:150` 改用 `data().t_new(0)`。

---

### 🔴 H4. `Services` 是"零实现的纯虚接口"，上下文里注入 `nullptr`

**位置**：[Services.H](file:///workspace/Source/Runtime/Services.H)（11 个纯虚函数）、[Runtime.cpp:49](file:///workspace/Source/Runtime/Runtime.cpp#L49-L50)

```cpp
m_context = RuntimeContext{&m_config, &m_time, &m_frame, m_data.get(), nullptr,
                           [this] { sync_time(); }};
```

**问题**：
1. 全仓库没有任何类继承 `Services`（`grep` 无实现类）；
2. `RuntimeContext::services` 因此恒为 `nullptr`；
3. `RuntimeData.H` 与 `Services.H` 的注释把二者当作"已提供的服务"（"FillBoundary/Reduce/Parser/RNG/Log"）；
4. **没有任何代码检查 `services != nullptr`**。

**失败场景**：A5 调度器或第一个插件按文档调用 `ctx->services->FillBoundary(mf, ng)` → **空指针解引用**，且错误发生在插件侧，堆栈很难指向 Runtime。这是"文档声称有、实际没有"的典型陷阱。

**判定**：作为 A1.3 的**接口占位**可以接受，但必须在类型层面禁止误用——要么注入一个 `UnavailableServices` 实现（每个方法 `abort("not implemented")`），要么把 `services` 改成 `std::optional`/抛异常访问器，并在文档标注"未实现"。

---

### 🔴 H5. `test_runtime_snapshot` 存在"陈旧文件假通过"，且两测试共享工作目录

**位置**：[test_runtime_snapshot.cpp:246-256](file:///workspace/tests/runtime/test_runtime_snapshot.cpp#L246-L256)、[tests/runtime/CMakeLists.txt:10,15](file:///workspace/tests/runtime/CMakeLists.txt#L10-L15)

**证据链（全部经核对）**：

1. `field_probe` 的校验块在**第 246 行，位于 `if (!particles_mode)` 之外**，即两个模式都会执行；
2. `field_probe` **只在 `inputs_2d` 注册**（`inputs_2d:17` 含 `field_probe`；`inputs_2d_particles:17/53` 均**不含**）；
3. 两个测试的 `WORKING_DIRECTORY` 都是 `${CMAKE_CURRENT_BINARY_DIR}`（`CMakeLists.txt:10,15`），输出路径都是 `diags/reducedfiles/field_probe.txt`；
4. 测试**不清理**旧文件，`last_line()` 只读取"最后一行以数字开头的行"，文件不存在时返回空串 → 变量保持 0 → `|0 - 1| > 1e-14` → **FAIL**。

**后果**：
- 顺序执行时：非 particles 测试先写出 `field_probe.txt`，particles 测试读到**别人的文件**并"通过" → **假通过**；
- `ctest -j`：两个测试并发写同一目录 → **竞态**，可能出现间歇性失败或读到半写文件；
- 单独运行 particles 测试（或新目录）→ **必然失败**；
- 反向也成立：若先前的运行用不同参数留下了旧文件，数值校验会对着**上次的物理结果**通过。

**这是本批最严重的测试问题**：它使 `A1.4a` 的 `verify_command`（`ctest -R '^runtime_snapshot_'`）不能作为有效门禁。

**修复方向**：把 `field_probe` 校验移入 `if (!particles_mode)`；两个测试各自使用独立 `WORKING_DIRECTORY`（如 `.../runtime_snapshot`、`.../runtime_snapshot_particles`）；测试开头清理 `diags/`。

---

### 🔴 H6. 测试输入硬编码 2D，却注册到所有 `WarpX_DIMS`

**位置**：[tests/runtime/CMakeLists.txt:7-15](file:///workspace/tests/runtime/CMakeLists.txt#L7-L15)

```cmake
add_test(NAME runtime_snapshot_${D}
         COMMAND test_runtime_snapshot_${SD} ${CMAKE_CURRENT_SOURCE_DIR}/inputs_2d)
```

`foreach(D IN LISTS WarpX_DIMS)` 会为 1D / 2D / 3D / RZ **每个维度**注册测试，但命令行永远传 `inputs_2d`，而该输入第 7 行写死 `geometry.dims = 2`。

**后果**：
- `WarpX_DIMS=1` / `3` 构建下：二进制是 1D/3D，输入声明 2D → WarpX 在 `ReadParameters` 早期中止（维度不匹配）→ 测试**必然失败**；
- `WarpX_DIMS=RZ`：输入未声明 RZ 相关参数（`n_rz_azimuthal_modes` 等），测不到本批唯一与 RZ 有关的改动（`FieldEnergy` 的 RZ 分支，见 H3）；
- CI（`.azure-pipelines.yml`）本来就是多维度矩阵，这会直接红。

**修复方向**：按维度提供 `inputs_1d` / `inputs_3d` / `inputs_rz`，或在 `if(D STREQUAL "2")` 内注册；不要在 1D/3D 构建里跑 2D 输入。

---

### 🔴 H7. CI 门禁只在 `main` 分支触发，本仓库的主分支是 `development`

**位置**：[.github/workflows/route-gates.yml:5-8](file:///workspace/.github/workflows/route-gates.yml#L5-L8)

```yaml
on:
  push:
    branches: [main]
  pull_request:
    branches: [main]
```

**问题**：
1. 按 `AGENTS.md`，WarpX 主分支是 `development`（"Main branch: development (not main)"）；
2. 本工作分支是 `trae/agent-iGh8PB`；
3. 因此该 workflow **在本仓库当前任何实际分支上都不会运行**；
4. 该 workflow 也只运行 3 个快速门禁（compliance / audit / task-graph），**不跑 build 和 smoke**——即使触发，也不覆盖本批改动最需要的编译与测试。

**后果**：`ROUTE_STATUS.md` 里"门禁通过"的结论只能来自本地手工执行，无法被 CI 复核；任何人 push 都不会被拦截。

**修复方向**：`branches: [development, main, trae/*]`（或直接用 `on: [push, pull_request]`），并在 PR 上至少跑 `build_2d.sh` + `smoke_2d.sh`。

---

## 4. 中危问题（M1–M12）

### 🟡 M1. `Timestep` 在构造期用 `maxLevel()`，在 `ComputeDiags` 用 `finest_level` —— 同一文件两种语义

**位置**：[Timestep.cpp:28-29](file:///workspace/Source/Diagnostics/ReducedDiags/Timestep.cpp#L28-L29) vs [Timestep.cpp:69](file:///workspace/Source/Diagnostics/ReducedDiags/Timestep.cpp#L69)

```cpp
// 构造期：按 AmrCore::maxLevel() 定 m_data 长度
const auto max_level = warpx.maxLevel();          // :29
// ComputeDiags（context 分支）：按 finestLevel() 循环
max_level = ctx->time->finest_level;               // :69
```

原实现两处都是 `maxLevel()`。在 WarpX 的静态 AMR 下 `finest_level == max_level`，所以**数值无差**；但这是两个不同概念：
- `maxLevel()` = 允许的最大层数（`amr.max_level`）；
- `finestLevel()` = 当前实际存在的最高层。

一旦后续 A6 引入动态 regrid，二者可能不等：循环上界变窄 → 输出列数变少；或 `m_data` 长度不足 → 若未来反向（finest > max，不可能，但重构时易错）会越界。

**修复方向**：`TimeState` 增加 `max_level` 字段，或 `Timestep` 直接使用 `data().maxLevel()`（`RuntimeData.H` 已声明该纯虚函数，改一行即可）。

---

### 🟡 M2. `static const RuntimeConfig legacy` / `static const FrameState legacy` 永不刷新

**位置**：[ReducedDiags.cpp](file:///workspace/Source/Diagnostics/ReducedDiags/ReducedDiags.cpp)（`config()` / `frame()` 的 fallback 分支）

**问题**：非 `Runtime` 路径（Python）下，`config()` / `frame()` 用**函数内 `static const`** 缓存第一次调用的快照。C++11 保证初始化线程安全，但：
1. 快照**永不过期**——若 Python 侧在 `WarpX` 构造后修改任何静态配置（例如通过绑定改 `WarpX::nox`、`field_boundary_hi`），诊断仍读到旧值；
2. 快照**先于其依赖对象构造**的风险：若某静态对象在 `WarpX` 之前调用 `frame()`，则 `WarpX::GetInstance()` 被提前构造，初始化顺序被打乱；
3. 与 H1 同源：静态生命周期 > `WarpX` 生命周期。

**修复方向**：fallback 返回**按需构造的临时对象**（`config()` 返回 by-value），或把适配器随 `WarpX` 实例持有。

---

### 🟡 M3. 每步 `sync_state()` 复制 6 个 `Vector` + 44 个标量；`make_time()` 每步分配

**位置**：[Runtime.cpp:133-137](file:///workspace/Source/Runtime/Runtime.cpp#L133-L137)（`sync_state` = `sync_config` + `sync_time`）、[Runtime.cpp:22-34](file:///workspace/Source/Runtime/Runtime.cpp#L22)（`make_time`）

`WarpX` 的 getter 全部**按值返回**（`getistep()`、`getdt()`、`gett_new()`、`gett_old()`、`getnsubsteps()` 各返回一个 `amrex::Vector` 副本）。`Runtime::evolve()` 每次调用后都 `sync_state()`，因此**每个 evolve 调用**（不是每步）至少 6 次堆分配 + 44 次标量赋值 + 1 次 `Vector` 赋值。当前 `evolve()` 是"整段循环"，代价可忽略；但若后续改成逐 step 调用 `evolve(1)`（调度器化的自然形态），就变成**每步 6+ 次分配**，在 GPU/大规模算例上是可测的回归。

**修复方向**：`TimeState` 只保留**标量**（`step`/`time`/`dt[0]`/`finest_level`），层次化向量按需查询；或把 `sync_*` 降为 `dirty` 标记 + 惰性刷新。

---

### 🟡 M4. 构造期快照的 `dt` 是 `numeric_limits<Real>::max()` 哨兵

**位置**：[Runtime.cpp:47-48](file:///workspace/Source/Runtime/Runtime.cpp#L47-L48)

`Runtime` 构造函数在 `WarpX` 构造后立刻 `sync_config(); sync_time();`。此时 `InitData()` 还没跑，`ComputeDt()` 还没算，`WarpX::dt` 仍是构造函数里赋的 `max()` 哨兵。因此 `runtime.time().dt` 在 `initialize()` 之前是**无意义值**。

本批靠两处 `refresh_time()` 补上：
- [WarpXInitData.cpp](file:///workspace/Source/Initialization/WarpXInitData.cpp)（`InitData` 内、诊断 flush 之前）；
- [WarpXEvolve.cpp:270](file:///workspace/Source/Evolve/WarpXEvolve.cpp#L270)（步内 `istep++`、`cur_time` 更新之后）。

**风险**：这是隐式契约——"任何读 `time()` 的代码都必须确保此前调过 `refresh_time()`"。没有断言、没有 `dt > 0` 的有效性检查，新增读点极易踩空。建议 `TimeState` 带 `bool valid` 或 `step` 哨兵，`refresh_time` 之外抛异常。

---

### 🟡 M5. `Runtime::initialize()` 无幂等保护，重复调用会二次 `InitData`

**位置**：[Runtime.cpp:62-69](file:///workspace/Source/Runtime/Runtime.cpp#L62-L69)

```cpp
if (m_finalized) { throw std::runtime_error(...); }   // :63
WarpX::GetInstance().InitData();                      // :66
m_initialized = true;                                 // :67
```

只挡"finalize 之后"，不挡"initialize 两次"。第二次会再跑 `InitData()`（重新分配 Level、重新构造诊断、重新注入粒子）→ 重复输出文件、内存翻倍、甚至 AMReX 断言失败。

**修复方向**：函数首行 `if (m_initialized) { return; }`（或抛 `logic_error`）。

---

### 🟡 M6. `RuntimeConfig` 的默认值与 `WarpX` 的默认值是两套真值

`RuntimeConfig.H` 给每个字段写了默认值（`nox = 3`、`galerkin_interpolation = true`、`ect_te_borrowing = true`、`filter_npass_each_dir{1,1,1}`…）。在 `FromLegacy()` 之前这些值是"第二套默认值"。

当前唯一构造路径是 `Runtime::sync_config()` → `FromLegacy()`，所以不会用到这些默认值。但只要有人构造 `RuntimeConfig{}`（测试、插件、未来的 A4 schema 默认值），就会拿到**与 WarpX 实际默认可能不一致**的值，而且编译器不会提示。

**修复方向**：要么删掉字段默认值（强制 `FromLegacy` 填充），要么加 `static_assert`/单元测试锁死两者一致。

---

### 🟡 M7. 4 个文件出现重复 `#include`

实测（`grep` + `sort | uniq -d`）：

| 文件 | 重复项 |
|---|---|
| `ParticleEnergy.cpp` | `Particles/MultiParticleContainer.H` |
| `ParticleExtrema.cpp` | `Particles/MultiParticleContainer.H` |
| `ParticleMomentum.cpp` | `Particles/MultiParticleContainer.H` |
| `ParticleNumber.cpp` | `Particles/MultiParticleContainer.H` |

新增那行与文件里已有的行**逐字相同**。虽然重复 include 有 include guard 不会报错，但会污染依赖图、让"这个文件需要哪些头"的判断失真，且 `clang-tidy`/IWYU 会告警。

---

### 🟡 M8. include 顺序违反 `AGENTS.md`（WarpX / AMReX / 标准库分组）

`AGENTS.md` 明确规定 `.cpp` 的 include 分组顺序（本文件头 → WarpX → AMReX → 第三方 → 标准库，组间空行、组内字典序）。

本批在 16 个诊断文件里把 `Runtime/RuntimeData.H` / `Runtime/RuntimeContext.H` / `Particles/MultiParticleContainer.H` 插在 `Utils/...` 之后、`WarpX.H` 之前，例如 [FieldProbe.cpp](file:///workspace/Source/Diagnostics/ReducedDiags/FieldProbe.cpp) 与 [RhoMaximum.cpp](file:///workspace/Source/Diagnostics/ReducedDiags/RhoMaximum.cpp)。这不是编译错误，但会让 `pre-commit`（clang-format 只管格式、不管顺序）漏过，长期造成组内乱序。

另外 [ReducedDiags.H:11-14](file:///workspace/Source/Diagnostics/ReducedDiags/ReducedDiags.H#L11-L14) 把 `namespace warpx::runtime { ... }` 前置声明**夹在 `#include <string>` 与 `#include <vector>` 之间**，虽然合法，但把声明块插进 include 组里，可读性差。

---

### 🟡 M9. `inputs_2d_particles` 重复定义 `warpx.reduced_diags_names`

**位置**：[inputs_2d_particles:17](file:///workspace/tests/runtime/inputs_2d_particles#L17) 与 [:53](file:///workspace/tests/runtime/inputs_2d_particles#L53)

```ini
17: warpx.reduced_diags_names = step_info field_energy field_max field_mom field_poynting field_reduce
53: warpx.reduced_diags_names = step_info field_energy field_max field_mom field_poynting field_reduce part_num part_energy part_mom rho_max lb_eff extrema
```

同一 key 在同一输入文件出现两次，行为取决于 AMReX `ParmParse` 对重复键的处理（追加 vs 取最后一次）。本仓库未 vendored AMReX 源码，**无法在本环境实证**：

- 若是**追加**：会注册 18 个条目，其中 6 个名字重复 → `MultiReducedDiags` 构造出重复对象，两个对象写同一个 `field_energy.txt` → 文件被互相覆盖、行序不确定 → 测试偶发失败；
- 若是**覆盖**：第 17 行纯属冗余。

**修复方向**：删除第 17 行，只保留一条完整定义。

---

### 🟡 M10. 测试没有按仓库规范接入，且覆盖存在空洞

| 问题 | 证据 |
|---|---|
| 未使用 `add_warpx_test()` | `AGENTS.md`："Adding a Test: Use `add_warpx_test()` in the test directory's `CMakeLists.txt`"；本测试用裸 `add_test` + 自定义可执行文件，没有 analysis 脚本、没有 checksum |
| 不参与回归体系 | 无 `Regression/Checksum` 条目 → 无法发现"数值微小漂移"（而 I5 是硬门） |
| `step_info` 从未断言 | 两个输入都注册了 `step_info`（`Timestep`），测试从不读 `step_info.txt` —— **偏偏这是唯一使用 `finest_level` 快照的诊断**（M1） |
| `lb_eff` 从未断言 | particles 输入注册了 `lb_eff`，测试不读 |
| 不能证明"诊断走了 context" | 测试只比较文件数值，而"走 context"与"走单例"在数值上完全相同 → 即使把 `m_runtime_context` 全置空，测试也会通过（除了 particles 模式的假通过问题） |
| 无法定位失败来源 | `check` 只打印一句话，不打印文件路径/行号 |

**结论**：该测试当前只能证明"数值没变"，不能证明"迁移生效"。

---

### 🟡 M11. `refactor` 任务可绕过 `--accept`（与 `AUTOMATION.md` 约定冲突）

**位置**：[route_tasks.py:180](file:///workspace/Tools/automation/route_tasks.py#L180)

```python
if ok and (t["kind"] != "refactor" or args.accept or t.get("automation_command")):
```

`AUTOMATION.md` 的约定是："`refactor` 任务必须 `--accept` 才标 done；`--accept` 不能绕过 `verify_command`"。

但该表达式里的 `or t.get("automation_command")` 意味着：**任何带 `automation_command` 的 `refactor` 任务，只要自动命令和 verify 都成功，就无需 `--accept` 即标 done**。这不是笔误，而是给自动任务开的后门——它削弱了"人工确认"这一唯一的人工闸门。

**修复方向**：若确实需要自动化豁免，应在任务定义里显式写 `auto_accept: true` 并单独审计；不要用 `automation_command` 的存在作为隐式豁免。

---

### 🟡 M12. `ROUTE_STATUS.md` 与 `route_tasks.json` 互相矛盾，且进度状态不在版本库

**证据**：

| 来源 | 说法 |
|---|---|
| [ROUTE_STATUS.md:15-23](file:///workspace/Docs/research_notes/ROUTE_STATUS.md#L15-L23) | A1.1 / A1.2 / A1.3 / A1.3a / A1.4a–A1.4e 全部 `done` |
| `Tools/automation/route_tasks.json` | 上述 9 项中只有 `A1.0`、`A1.3a` 为 done；A1.1–A1.4e 仍为 pending |
| 实测 `python3 Tools/automation/route_tasks.py next` | 输出 **A1.1**（而不是 A1.4 或 A1.5） |
| `.gitignore:105-106` | `.route_state.json`、`.route_logs/` 被忽略 → 真实完成状态**不在仓库里** |

**后果**：
1. 新克隆的人看到的"下一任务"是 A1.1，与所有文档矛盾；
2. `verify_command` 的执行记录不进仓库，无法审计"门禁到底跑没跑过"；
3. 门禁 `audit_check.py` 只统计文件/类数量，不校验任务图与代码的一致性，因此发现不了这种偏差。

**修复方向**：以 `route_tasks.json` 为唯一真值并把它提交进仓库（状态字段入库），`ROUTE_STATUS.md` 由脚本生成而非手写；或反过来，把 `route_tasks.json` 的 `status` 全部清空，只由 `.route_state.json` 驱动，并明确"状态不入库"是设计选择。

---

## 5. 低危 / 风格问题（L1–L7）

| # | 问题 | 位置 | 说明 |
|---|---|---|---|
| **L1** | `RuntimeContext` 是裸指针聚合，无所有权语义 | `RuntimeContext.H` | 结构可复制；复制后指向同一 `Runtime` 成员。建议 `= delete` 拷贝，或加注释 |
| **L2** | `~Runtime()` 无条件 `finalize_external_libraries()` | `Runtime.cpp:54-58` | 若 `Runtime` 构造成功但 `initialize()` 抛异常，析构仍会 `amrex::Finalize()`；单例场景无害，嵌入场景有风险 |
| **L3** | `Runtime::verbose()` 依赖 `WarpX::GetInstance()` | `Runtime.cpp:92` | 虽然在 `m_finalized` 后短路，但仍是 A1 目标外的单例依赖（并入 H2） |
| **L4** | 注释中的 `WarpX::GetInstance()` 干扰全局检索 | `RuntimeData.H`、`Services.H` | `grep -rc` 会把这些注释计入，自动核查脚本需排除注释 |
| **L5** | `field_max` 断言用绝对容差 `1e-14` 比对 `1.0` | `test:160-163` | 输出精度 `m_precision = 14`（`ReducedDiags.H:48`）→ 舍入误差可达 ~5e-15，余量只有 2 倍，属**边界容差** |
| **L6** | `RuntimeConfig` 在非 RZ 构建里也复制 `n_rz_azimuthal_modes` | `RuntimeConfig.cpp` | 无害，但不必要；建议 `#ifdef WARPX_DIM_RZ` |
| **L7** | `parse_headers.py` 硬编码 `/tmp/header_catalog.json`，无 shebang、无 CLI 参数 | `Tools/A0/parse_headers.py` | A0 目录生成不可复现（不同机器/并发会互相覆盖），建议改为仓库内路径 + argparse |

---

## 6. 逐文件审查结论

| 文件 | 语法 | 逻辑 | 技术要求 | 备注 |
|---|---|---|---|---|
| `Source/Runtime/Runtime.{H,cpp}` | ✅ | 🟡 | ❌ A1 目标 | 5 处单例、`services=nullptr`、无幂等（H2/H4/M5） |
| `Source/Runtime/RuntimeConfig.{H,cpp}` | ✅ | ✅ | 🟡 | 两套默认值（M6）；44 个字段全量复制 |
| `Source/Runtime/RuntimeContext.H` | ✅ | ✅ | ✅ | 前置声明与定义 `struct` 一致；`<functional>` 齐全 |
| `Source/Runtime/TimeState.H` | ✅ | 🟡 | 🟡 | 缺 `max_level`（M1）；`dt` 哨兵（M4） |
| `Source/Runtime/FrameState.{H,cpp}` | ✅ | ✅ | ✅ | 字段与 `WarpX` 静态一一对应 |
| `Source/Runtime/RuntimeData.H` | ✅ | ✅ | 🟡 | 纯虚接口完整；**已提供 `maxLevel()`**，但 `Timestep` 没用 |
| `Source/Runtime/WarpXRuntimeData.{H,cpp}` | ✅ | ❌ | 🟡 | `Legacy()` 静态悬空（H1） |
| `Source/Runtime/Services.H` | ✅ | ❌ | ❌ | 零实现、`nullptr` 注入（H4） |
| `Source/Runtime/CMakeLists.txt` / `Make.package` | ✅ | ✅ | ✅ | 两组构建都覆盖；`lib_${SD}` 存在的前提已核实（`CMakeLists.txt:169`） |
| `Source/main.cpp` | ✅ | ✅ | ✅ | `Runtime` 生命周期等价于原 `main`；`evolve()` 默认参等价于原 `Evolve()` |
| `Source/WarpX.H` | ✅ | ✅ | ✅ | +3 访问器 +1 成员；前置声明 `struct RuntimeContext` 与定义一致 |
| `Source/WarpXEvolve.cpp` | ✅ | ✅ | ✅ | 钩子在 `istep++`/`cur_time` 更新之后（`t_new` 已新） |
| `Source/WarpXInitData.cpp` | ✅ | ✅ | ✅ | 钩子在 diag flush 前 |
| `ReducedDiags.{H,cpp}` | ✅ | 🟡 | 🟡 | `static const` 缓存（M2）；`:150` 残留单例（H3） |
| `MultiReducedDiags.cpp` | ✅ | ✅ | 🟡 | 上下文注入顺序正确（构造后设置）；构造期一次单例读取 |
| `Timestep.cpp` | ✅ | 🟡 | 🟡 | `maxLevel`/`finest_level` 混用（M1）；构造期单例（H3） |
| `FieldEnergy.cpp` | ✅ | ✅ | 🟡 | RZ 分支未迁移（H3）；`cellSize` 由引用改值拷贝（等价） |
| `FieldMaximum/Momentum/PoyntingFlux/Reduction` | ✅ | ✅ | ✅ | 纯替换；`LowerCorner(box, lev, 0._rt)` 保持不变 |
| `FieldProbe.cpp` | ✅ | ✅ | ✅ | 构造期 `&data().amrCore()` 与原 `&WarpX::GetInstance()` 指向同一对象 |
| `LoadBalanceEfficiency.cpp` | ✅ | ✅ | ✅ | `data().costs(lev)` 与原 `warpx.getCosts(lev)` 等价 |
| `Particle{Energy,Extrema,Histogram,Histogram2D,Momentum,Number}.cpp` | ✅ | ✅ | ✅ | `const` 重载可解析（`GetParticleContainer(int) const` 返回非 const 引用）；4 处重复 include（M7） |
| `RhoMaximum.cpp` | ✅ | ✅ | ✅ | 等价替换 |
| `tests/runtime/*` | ✅ | ❌ | ❌ | 假通过（H5）、只支持 2D（H6）、未按规范接入（M10） |
| `Tools/A0/*` | ✅ | 🟡 | 🟡 | L7 |
| `Tools/automation/*` | ✅ | 🟡 | 🟡 | M11；门禁覆盖面弱（M12 附表） |
| `.github/workflows/route-gates.yml` | ✅ | ❌ | ❌ | 永不在实际分支触发（H7） |

---

## 7. 自动化门禁与状态的一致性

### 7.1 三个快速门禁的实测结果（本环境）

| 门禁 | 命令 | 实测 |
|---|---|---|
| Public-release compliance | `python3 Tools/automation/gates/compliance.py` | **exit 0**（通过） |
| A0 audit consistency | `python3 Tools/automation/gates/audit_check.py` | **exit 0**（通过） |
| Route task graph valid | `python3 Tools/automation/route_tasks.py next` | **exit 0**，但输出 `A1.1`（见 M12） |

### 7.2 门禁本身的覆盖缺口

| 门禁 | 缺口 |
|---|---|
| `compliance.py` | 只扫描 `git ls-files` → **未提交的新文件不参与检查**；新文件恰好都是本批的 Runtime 层 |
| `audit_check.py` | 只校验"头文件数 ≥ 372、类目录存在"等数量指标，**不校验新增 Runtime 类是否进入 catalog**，也不校验 `ROUTE_STATUS` 与代码一致 |
| `route_tasks.py --accept` | M11 的隐式豁免 |
| `full_gate.sh` | 串起 compile/smoke/sonar，但 CI 未调用（H7） |
| `smoke_2d.sh` | 正则 `^test_2d_langmuir_multi\.(run|analysis)$|^runtime_snapshot_` 能匹配到新测试——但一旦测试改名或 `WarpX_DIMS` 非 2，正则与实际名字就会失配 |

---

## 8. 已核对为"正确"的项目（正向结论）

这些是逐条比对后确认**没有问题**的点，列出来是为了避免后续重复怀疑：

1. **物理常数完全一致**：`Source/ablastr/constant.H:64` 的 `epsilon_0 = 8.8541878188e-12`、`:70` 的 `mu0 = 1.2566370612685e-6`，与测试里的 `constexpr`（`test_runtime_snapshot.cpp:144-145,169,182,251`）逐位相同。测试的容差是 `1e-12` 相对（不是 `1e-15`），大于 14 位输出的舍入误差（`ReducedDiags.H:48`），因此**常数与精度这两项不会导致失败**。
2. **`Runtime` 构造期不会越界读向量**：`WarpX` 构造函数已把 `istep`/`dt`/`nsubsteps`/`t_new`/`t_old` 全部 `resize` 到 `nvlevels_max`，所以 `make_time()` 在 `InitData()` 之前调用 `getistep(0)`/`gett_new(0)` 是安全的（值可能是哨兵，但不是 UB）。
3. **`main.cpp` 的执行序等价于原实现**：`initialize_external_libraries` → `WarpX::GetInstance()` → `InitData` → `Evolve` → `MaxStep` 打印 → `Finalize` → `finalize_external_libraries`，与 `7bcb44a` 版本逐句对应；`Runtime::evolve()` 的默认参数 `-1` 与 `WarpX::Evolve(int = -1)` 一致。
4. **不会出现重复 `main` 符号**：`Source/main.cpp` 只挂到 `app_${SD}`（`CMakeLists.txt:447`），不在 `lib_${SD}` 里；`tests/runtime` 链接 `lib_${SD}` 不会冲突。
5. **`lib_${SD}` 一定存在**：`CMakeLists.txt:169` 在 `WarpX_APP`/`WarpX_PYTHON` 时强制 `WarpX_LIB=ON`，所以 `tests/runtime/CMakeLists.txt:3` 的 `if(TARGET lib_${SD})` 守卫不会让测试"静默消失"（除非用户显式关掉 app 与 python）。
6. **`const` 重载可解析**：`MultiParticleContainer::GetParticleContainer(int) const` 返回 `WarpXParticleContainer&`，因此 `data().particles()` 返回 `const MultiParticleContainer&` 后，`auto& myspc = mypc.GetParticleContainer(...)` 仍可绑定非 const 引用（`ParticleHistogram`/`ParticleHistogram2D`/`ParticleExtrema` 均合法）。
7. **`MultiFabRegister::get()` 有 `const` 重载**，所以 `RuntimeData::fields() const` 返回 `const MultiFabRegister&` 后，`fields.get(...)` 仍可用（`FieldReduction.H`/`RhoMaximum` 合法）。
8. **枚举与容器类型匹配**：`FieldBoundaryType`/`ParticleBoundaryType`/`CurrentDepositionAlgo`/`LoadBalanceCostsUpdateAlgo`/`TimeDependencyJ`/`TimeDependencyRho`/`GridType` 均存在于 `Utils/WarpXAlgorithmSelection.H` 与 `ablastr/utils/Enums.H`，且都带 `Default` 枚举值；`amrex::Array<Real,3>` 与 `std::array<Real,3>` 是同一类型的别名（原代码已跨类型绑定，可反证）。
9. **Strang 序列零改动**：`WarpXEvolve.cpp:661-682` 未出现在 diff 中；本批只新增 `refresh_time()` 调用（`:270`、`InitData` 内）。
10. **checkpoint / IO 顺序零改动**：`IOSubstrate`、`MultiFabRegister` 的写序未碰；`FieldProbe` 的构造参数替换不改变 `WriteToFile` 的字段顺序。
11. **头文件自包含**：`Runtime.{H,cpp}` 有 `<memory>`/`<stdexcept>`；`RuntimeContext.H` 有 `<functional>`；`FrameState.H`/`TimeState.H` 有 `AMReX_REAL.H`/`AMReX_Vector.H`；`RuntimeData.H` 不依赖 `std::string`（本批未引入 `FieldSpec`）。未发现确定性编译错误。

---

## 9. 需实测确认的 4 项（本环境无法完成）

按优先级：

1. **`ctest -R '^runtime_snapshot_particles'` 在干净目录单独运行** —— 预期**失败**（H5 的复现实验）。这是验证 H5 的唯一硬证据。
2. **多维度构建**：`-DWarpX_DIMS="1;2;3;RZ"` 下 `ctest -R runtime_snapshot` —— 预期 2D 之外全部失败（H6）。
3. **`ctest -j 4 -R runtime_snapshot`**（同一构建目录，先 `rm -rf diags`）—— 预期出现竞态/间歇失败（H5）。
4. **RZ 构建编译** `Source/Diagnostics/ReducedDiags/FieldEnergy.cpp` —— 验证 H3 的 RZ 分支是否仍引用未迁移的单例（静态审查已确认引用存在，只需确认可编译）。

---

## 10. 建议的修复顺序（只建议，未实施）

不改变本批"行为保持"的定性，按"先保证门禁可信，再修生命周期"排序：

1. **H5 / H6 / H7**（测试与 CI）—— 先让门禁说真话，否则后续所有结论都不可信；
2. **H1 / M2**（静态悬空）—— 一个函数级改动即可消除 use-after-free；
3. **H4 / M5**（`Services` 与幂等）—— 防止插件侧空指针与二次初始化；
4. **H3 / M1**（残留单例与 `maxLevel`）—— 补齐 A1.4 的真实范围，特别是 RZ 分支；
5. **M3 / M4 / M6**（快照语义）—— 与 A2 的 Level 资源重构一起做更划算；
6. **M7 / M8 / L7**（卫生）—— 一次性批量清理。

---

## 附录 A：证据索引

### A.1 生命周期与单例

| 证据 | 位置 |
|---|---|
| `Legacy()` 静态对象 | `Source/Runtime/WarpXRuntimeData.cpp:18-21` |
| `WarpX::Finalize()` 清空单例 | `Source/WarpX.cpp:324`（`m_instance = nullptr`） |
| `Runtime` 的 5 处单例调用 | `Source/Runtime/Runtime.cpp:24,45,66,77,92` |
| 上下文注入 | `Source/Runtime/Runtime.cpp:49-51` |
| `WarpX` 持有裸指针 | `Source/WarpX.H:1207`；访问器 `:1118-1119` |
| `Services = nullptr` | `Source/Runtime/Runtime.cpp:49` |
| `ReducedDiags` 残留单例 | `Source/Diagnostics/ReducedDiags/ReducedDiags.cpp:150` |

### A.2 诊断迁移

| 证据 | 位置 |
|---|---|
| `Timestep` 构造期 `maxLevel()` | `Source/Diagnostics/ReducedDiags/Timestep.cpp:28-29` |
| `Timestep` context 分支 `finest_level` | `Source/Diagnostics/ReducedDiags/Timestep.cpp:69` |
| `FieldEnergy` RZ 分支保留单例 | `Source/Diagnostics/ReducedDiags/FieldEnergy.cpp:184` |
| `LowerCorner(..., 0._rt)` 未改 | `FieldReduction.H` / `FieldProbe.cpp` |
| `m_precision = 14` | `Source/Diagnostics/ReducedDiags/ReducedDiags.H:48` |

### A.3 测试与门禁

| 证据 | 位置 |
|---|---|
| `field_probe` 校验块无条件执行 | `tests/runtime/test_runtime_snapshot.cpp:246-256` |
| `field_probe` 只在 2D 输入注册 | `tests/runtime/inputs_2d:17`；`inputs_2d_particles:17,53` |
| 两测试共享工作目录 | `tests/runtime/CMakeLists.txt:10,15` |
| 所有维度都用 `inputs_2d` | `tests/runtime/CMakeLists.txt:7-15`；`inputs_2d:7`（`geometry.dims = 2`） |
| CI 只触发 `main` | `.github/workflows/route-gates.yml:5-8` |
| `refactor` 绕过 `--accept` | `Tools/automation/route_tasks.py:180` |
| 状态文件不入库 | `.gitignore:105-106` |
| 状态声明不一致 | `Docs/research_notes/ROUTE_STATUS.md:15-23` vs `Tools/automation/route_tasks.json` |

### A.4 构建接线

| 证据 | 位置 |
|---|---|
| `WarpX_APP/PYTHON` 强制 `WarpX_LIB=ON` | `CMakeLists.txt:169` |
| `main.cpp` 只挂 app 目标 | `CMakeLists.txt:447` |
| Runtime 源加入库 | `Source/Runtime/CMakeLists.txt`、`Source/Runtime/Make.package` |
| 测试守卫 | `tests/runtime/CMakeLists.txt:3` |

---

## 附录 B：复核命令

```bash
# 1. 本批改动的代码文件清单
git diff --numstat 7bcb44a..HEAD -- Source tests Tools CMakeLists.txt .github

# 2. 单例残留（排除注释后人工确认）
grep -rn "WarpX::GetInstance" Source/Runtime Source/Diagnostics/ReducedDiags

# 3. 静态悬空（Legacy 只初始化一次）
sed -n '16,24p' Source/Runtime/WarpXRuntimeData.cpp
grep -n "m_instance = nullptr" Source/WarpX.cpp

# 4. 测试假通过的两个前提（必须同时成立）
grep -n "field_probe" tests/runtime/inputs_2d tests/runtime/inputs_2d_particles
grep -n "WORKING_DIRECTORY" tests/runtime/CMakeLists.txt

# 5. 维度写死
grep -n "inputs_2d\|geometry.dims" tests/runtime/CMakeLists.txt tests/runtime/inputs_2d

# 6. CI 永不触发
grep -n "branches" .github/workflows/route-gates.yml
git branch --show-current

# 7. 状态不一致
python3 Tools/automation/route_tasks.py next
grep -n "A1\.4" Docs/research_notes/ROUTE_STATUS.md

# 8. 重复 include
for f in Source/Diagnostics/ReducedDiags/*.cpp; do
  d=$(grep -c '#include' "$f"); u=$(grep '#include' "$f" | sort -u | wc -l)
  [ "$d" -ne "$u" ] && echo "$f: $d/$u"
done

# 9. 快速门禁
python3 Tools/automation/gates/compliance.py; echo $?
python3 Tools/automation/gates/audit_check.py; echo $?
```

---

## 附录 C：与既有文档的关系

| 文档 | 关系 |
|---|---|
| [`最终的技术路线.md`](最终的技术路线.md) | **技术要求基线**。本报告 §2 全部结论以它的 I1–I6 与 P1 为准 |
| [`ROUTE_AUDIT_REPORT.md`](ROUTE_AUDIT_REPORT.md) | 架构层缺口（F1–F5 / S1–S8 / C1）。本报告是**其实现层**的补充：F 类缺口不在本批代码中体现，因为本批只做了 A1 的门面 |
| [`ROUTE_STATUS.md`](ROUTE_STATUS.md) | 进度声明。本报告 M12 指出它与 `route_tasks.json` 矛盾，且 H3 指出其中 A1.4b 的声明不成立 |
| [`AUTOMATION.md`](AUTOMATION.md) | 门禁约定。本报告 M11/H7 指出约定的两处失效 |
| `Docs/A0/api/*.md` | A0 目录。本批新增 Runtime 类**未进入**该目录（`audit_check.py` 也不校验），见 §7.2 |

> 本报告**不重新设计架构**，也不提出代码补丁；只列出问题、证据与修复方向。
> 与 `最终的技术路线.md` 冲突时，技术要求以路线为准；与实现现状冲突时，以实现现状为准（本报告逐条给出可复核命令）。
