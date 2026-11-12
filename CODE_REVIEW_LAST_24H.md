# WarpX 插件化分支 — 近 24 小时代码正确性审计报告

**审计角色**：提交后正确性检查（post-commit correctness checker）
**审计时间**：2026-11-12
**报告版本**：v1

## 一、范围与方法说明

本仓库为 **浅克隆（shallow clone）**，`git log` 仅可见一个提交：

- `2d50df8` `docs(report): the pytest repair and the policy for every remaining failure`
- 提交时间：`2026-11-12 02:18:47 +0800`
- `git rev-list --count HEAD` = 1

因此**无法做逐提交 diff 审计**。本报告审计对象为该提交对应的**完整工作树快照**，重点审查：

1. 是否有代码违反项目根本目标——"插件相互独立，仅通过 core 接口 / 数据总线（DataBus）通信"；
2. 是否有代码逻辑违反物理理论；
3. 是否存在对 WarpX 上游功能的不完整支持（维度、场、species 等）。

### 严重程度定义

| 级别 | 含义 |
|---|---|
| **CRITICAL** | 导致整个仿真崩溃或核心功能完全不可用 |
| **HIGH** | 静默物理错误 / 系统性架构缺陷 / 上游功能对等性断裂 |
| **MEDIUM** | 潜伏缺陷，当前 in-tree 处理器不触发但会危害第三方插件 |
| **LOW** | 非物理影响的工程缺陷（测试、构建等） |

## 二、执行摘要

| # | 严重级别 | 问题摘要 | 位置 |
|---|---|---|---|
| 1 | **CRITICAL** | C ABI 读场函数走写路径且无异常保护，读场直接 `std::terminate` | [PluginCAdapter.cpp:294](file:///workspace/Source/Core/Runtime/PluginCAdapter.cpp#L294-L301) |
| 2 | **HIGH** | 写冲突检测用精确字符串比较，排序用 `key_matches`，二者不一致；`writes_species` 完全不检测冲突 | [HandlerSpec.H:81-111](file:///workspace/Source/Core/Runtime/HandlerSpec.H#L81-L111) |
| 3 | **HIGH** | Stage 区间"接管"语义：区间内只要有任何 handler 注册就跳过核心物理，只读观察者会冻结粒子 | [WarpXEvolve.cpp:589-590](file:///workspace/Source/Plugins/PicEm/Modules/Evolve/WarpXEvolve.cpp#L589-L590) 等 |
| 4 | **HIGH** | 宏观介质节点填充硬编码为 XZ(2D)，1D/3D 下 `interface_function` 给出错误 ε/σ | [MacroscopicProperties.cpp:547](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicProperties/MacroscopicProperties.cpp#L547-L560) |
| 5 | **MEDIUM** | 嵌套 `Scheduler::run` 会清空外层 DataBus 活跃 handler，访问控制失效 | [Scheduler.cpp:25-32](file:///workspace/Source/Core/Runtime/Scheduler.cpp#L25-L32) |
| 6 | **LOW** | `add_warpx_pytest` 在 CMake 配置期收集测试，干净构建时 pywarpx 不可导入，全部 pytest 静默不注册 | [WarpXFunctions.cmake:348-351](file:///workspace/cmake/WarpXFunctions.cmake#L348-L351) |

---

## 三、详细发现

### 发现 1 — C ABI 读场函数走写路径 + 无异常保护 【CRITICAL】

**位置**：[PluginCAdapter.cpp:293-301](file:///workspace/Source/Core/Runtime/PluginCAdapter.cpp#L293-L301)

```cpp
extern "C" int32_t
warpx_stage_read_field (WarpXStageContext* ctx, const char* name, int32_t lev, void** multifab)
{
    if (ctx == nullptr || ctx->bus == nullptr || name == nullptr) { return 0; }
    const amrex::MultiFab* field = ctx->bus->field(std::string(name), lev);  // ← 非 const 重载
    ...
}
```

#### 根因

`ctx->bus->field(name, lev)` 因 `ctx->bus` 为非 const 指针，解析到 [DataBus::field 非 const 重载](file:///workspace/Source/Core/Runtime/DataBus.cpp#L108-L113)：

```cpp
amrex::MultiFab*
DataBus::field (const std::string& name, int lev)
{
    require_field_access(name, /*write=*/true);   // ← 要求写权限
    return m_fields.get(name, lev);
}
```

而函数语义是"读"，对应 handler 声明里只有 `reads_fields`，没有 `writes_fields`。`require_field_access(name, true)` 会抛出 `std::runtime_error`。

同时，**该函数没有 try/catch**。对比同一文件里的 [warpx_stage_scalar](file:///workspace/Source/Core/Runtime/PluginCAdapter.cpp#L322-L334)，它正确地用 try/catch 包住了 bus 调用并在异常时返回 0。`warpx_stage_read_field` 漏掉了这个保护，异常直接穿过 `extern "C"` 边界 → C++ 运行时调用 `std::terminate` → 整个仿真进程崩溃。

#### 执行轨迹

1. C 插件声明 `reads_fields = {"Efield_fp"}`，注册 `BeforePush` handler。
2. handler 内调用 `warpx_stage_read_field(ctx, "Efield_fp", 0, &mf)`。
3. 进入 `DataBus::field`（非 const）→ `require_field_access("Efield_fp", true)`。
4. `require_field_access` 检查当前活跃 handler 的 `writes_fields`，未命中 → 抛 `std::runtime_error`。
5. 异常未被捕获，穿越 C ABI → `std::terminate()`。

#### 次生问题：矢量场对 C API 完全不可见

`warpx_stage_has_field` / `warpx_stage_read_field` 均调用 `bus->has(name, lev)` / `bus->field(name, lev)`（无方向参数）。而 `Efield_fp`、`Bfield_fp`、`current_fp` 等矢量场在 `MultiFabRegister` 中按方向存储（`Efield_fp_x` 等）。`DataBus::has(name, lev)` 只查标量键，因此 C 插件**根本无法访问 E/B/J 场**——最基础的场读写在 C API 层缺失。

#### 对项目目标的违反

- **直接破坏插件可扩展性**：C 插件是插件化架构对外的主要扩展面。读场是最基础的操作，一调用就崩溃，等于插件生态的"Hello World"跑不起来。
- **违反总线契约**：`DataBus` 的读/写权限模型（`require_field_access` 的 write 标志）是核心安全机制，但 C 适配器没有正确使用 const 重载，使只读插件无法通过自己声明的权限读取数据。

#### 为什么现有测试没发现

in-tree 的 PicEm 处理器全部是 C++ 直接调用 `host->...` 服务，不走 C ABI。C ABI 路径没有端到端测试覆盖。

#### 复现步骤

1. 编写一个最小 C 插件，声明 `reads_fields = {"phi"}`，在任意 stage 调用 `warpx_stage_read_field`。
2. 设置 `WARPX_PLUGIN_PATH` 指向该插件，运行任意 WarpX 输入。
3. 进程在第一次调用 `warpx_stage_read_field` 时 `std::terminate`。

---

### 发现 2 — Handler 冲突检测与依赖排序的键匹配不一致 【HIGH / 系统性】

**位置**：[HandlerSpec.H:81-111](file:///workspace/Source/Core/Runtime/HandlerSpec.H#L81-L111)（冲突检测），[HandlerSpec.H:262-273](file:///workspace/Source/Core/Runtime/HandlerSpec.H#L262-L273)（排序依赖边）

#### 根因

**冲突检测**用精确字符串相等：

```cpp
for (const auto& w : spec.writes_fields) {
    for (const auto& ew : existing.writes_fields) {
        if (w == ew) {   // ← 精确相等
            throw ... "both write field '" + w + "'";
        }
    }
}
```

**排序**用 `key_intersects` → `key_matches`（见 [KeyMatch.H:26-41](file:///workspace/Source/Core/Runtime/KeyMatch.H#L26-L41)），支持聚合名与分量名匹配（`"Efield_fp"` 匹配 `"Efield_fp_x"`）。

`KeyMatch.H` 注释明确写道：

> This is the single definition used by the plugin host (ordering/conflict detection) and by the data bus (declared-access checks): if the two ever disagreed, a handler could be ordered against one key and rejected on another.

**注释自己声明了"单一来源"不变量，但 `add()` 里的冲突检测没有遵守它**——用了 `==` 而非 `key_matches`。这意味着：

- 两个 handler 在同一 StagePoint 分别写 `"Efield_fp"` 和 `"Efield_fp_x"` → 冲突检测通过（字符串不等）→ 排序时却认为它们有依赖关系。
- 更糟的是，**`writes_species` 完全没有冲突检测**（`add()` 里只检查了 `writes_fields` 和 `writes_scalars`）。两个 handler 在同一 StagePoint 写同一个 species 不会被拒绝。

#### in-tree 证据

[ParticlePushHandler.cpp:18-29](file:///workspace/Source/Plugins/PicEm/Modules/PicEm/ParticlePush/ParticlePushHandler.cpp#L18-L29)：

```cpp
return HandlerSpec{
    .name = "particle_push",
    .point = {Stage::Push, 0},
    .reads_fields = {},          // ← 空：未声明读取 E/B
    .writes_fields = {"current_fp", "rho_fp"},
    .reads_species = {},         // ← 空
    .writes_species = {},        // ← 空：未声明写入粒子 species
    .priority = 0};
```

粒子推动器实际**必须**读取 E、B 场并写入粒子位置/动量，但它的 spec 完全没有声明。这意味着：

1. 访问控制形同虚设（DataBus 的 `require_field_access` 查的就是这些声明）。
2. 如果有另一个 handler 也在 `Stage::Push` 写同一个 species，不会被检测到。

#### 影响

- **静默、不可复现、顺序依赖的物理错误**：两个写同一 species 的 handler 同时注册，排序没有写-写边（写-写不产生依赖边，只有写-读才产生），执行顺序由 priority/注册顺序决定 → last-writer-wins，物理结果不可预测。
- **架构不变量被破坏**：`KeyMatch.H` 承诺的"单一键匹配定义"被违反，冲突检测和排序对同一对键可能给出矛盾结论。

#### 对项目目标的违反

- **违反插件独立性契约**：插件通过声明 reads/writes 来让总线仲裁依赖与冲突。如果冲突检测与排序用不同的键定义，仲裁就不可靠，插件之间的独立性无法被系统保证。
- **违反"总线即契约"原则**：DataBus 的访问控制依赖 handler 声明的真实性，但 in-tree 处理器本身就没诚实声明（ParticlePushHandler 读写都为空），使整个声明-检查机制成为摆设。

#### 为什么现有测试没发现

现有测试只跑单个 PicEm handler 的 happy path，没有注册第二个写冲突 handler 的测试用例。

---

### 发现 3 — Stage 区间"接管"静默跳过核心物理 【HIGH / 静默物理】

**位置**：[WarpXEvolve.cpp:589-590](file:///workspace/Source/Plugins/PicEm/Modules/Evolve/WarpXEvolve.cpp#L589-L590)（Push）、[610-611](file:///workspace/Source/Plugins/PicEm/Modules/Evolve/WarpXEvolve.cpp#L610-L611)（FinalizeDeposition）、[632-633](file:///workspace/Source/Plugins/PicEm/Modules/Evolve/WarpXEvolve.cpp#L632-L633)（FieldSolve）、[695-696](file:///workspace/Source/Plugins/PicEm/Modules/Evolve/WarpXEvolve.cpp#L695-L696)（Diagnostics）

#### 根因

```cpp
warpx::runtime::Scheduler scheduler(*m_plugin_host);
if (scheduler.run(ctx, warpx::runtime::Stage::BeforePush,
                  warpx::runtime::Stage::AfterPush) > 0) { return; }   // ← 只要有 handler 执行就 return
OneStep_PushParticles(a_cur_time, a_dt, a_step);
```

`scheduler.run(...)` 返回区间内执行的 handler 数量。**只要 > 0**（区间内有任何 handler 被执行），就直接 `return`，跳过后面的 `OneStep_PushParticles`。

这不是"插件接管该 stage"的语义——`scheduler.run` 的区间是 `[BeforePush, AfterPush)`，里面包含 `BeforePush`、`Push`、`AfterPush` 等多个 StagePoint。一个在 `BeforePush` 注册的**只读观察者**（例如诊断插件）也会让 `run()` 返回 > 0，从而跳过核心粒子推进。

`RunAfterFieldSolveStage` 是个例外（[L678-682](file:///workspace/Source/Plugins/PicEm/Modules/Evolve/WarpXEvolve.cpp#L678-L682)），它用了 `amrex::ignore_unused(scheduler.run(...))`，注释说"接管标志被忽略，区间只让插件观察已解算的场"。这说明作者**知道**这个语义，但 Push / Finalize / FieldSolve / Diagnostics 四处没有做同样处理。

#### 可触发路径

结合 [Runtime.cpp:37](file:///workspace/Source/Core/Runtime/Runtime.cpp#L37) 和 [Runtime.cpp:47-49](file:///workspace/Source/Core/Runtime/Runtime.cpp#L47-L49)：

1. 设置 `WARPX_DISABLE_PLUGIN_HANDLERS=1` → PicEm 内置 handler 被禁用（`register_plugins` 的 `disable_handlers` 参数）。
2. 设置 `WARPX_PLUGIN_PATH=/path/to/external_plugin` → 加载一个外部插件，注册一个 `BeforePush` handler（只读场做诊断）。
3. 运行仿真 → 每个 step 的 `RunPushStage` 中 `scheduler.run(BeforePush, AfterPush)` 返回 1 → `return` → `OneStep_PushParticles` 永不执行。
4. **粒子完全冻结，但场仍在演化**（FieldSolve 同理可能被跳过或不被跳过，取决于该插件是否也注册 BeforeFieldSolve）。

#### 影响

- **静默灾难性物理破坏**：没有任何报错、没有任何 warning，粒子不动或场不更新，仿真结果完全错误且难以排查。
- 这是插件化架构最危险的一类 bug——"扩展点意外改变了核心行为"。

#### 对项目目标的违反

- **直接违反"插件独立、不影响核心物理"的根本目标**：一个只读诊断插件竟然能让核心粒子推进消失。插件本应通过 bus 观察/修改数据，而不是通过"存在性"来屏蔽核心算法。
- **stage 区间的语义设计错误**：`run(interval)` 的返回值被当作"是否接管"，但区间内可能同时存在观察者和替换者。正确的语义应该是只有当某个 handler 显式声明"接管"（例如 spec 里有 `replaces_core = true`）才跳过核心，而非"区间内有 handler 就跳过"。

#### 为什么现有测试没发现

- `WARPX_DISABLE_PLUGIN_HANDLERS` 只在禁用 PicEm handler 后才需要外部插件；正常测试中 PicEm handler 本身就在 `Push` stage 执行粒子推进（通过 `host->particleServices().pushParticlesAndDeposit`），所以核心物理实际上是由 handler 完成的，跳过 `OneStep_PushParticles` 不影响结果。
- 没有测试覆盖"只读观察者 + 核心 fallback"的场景。

---

### 发现 4 — 宏观介质节点填充硬编码为 XZ(2D) 【HIGH / 静默物理 + 上游对等性缺口】

**位置**：[MacroscopicProperties.cpp:546-560](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicProperties/MacroscopicProperties.cpp#L546-L560)

#### 根因

节点填充循环中，`z` 坐标始终用 `j` 索引计算：

```cpp
amrex::Real const zn = zlo_n + amrex::Real(j) * dz_n;   // L547
...
amrex::Real const gx0 = phi_n(xn + hh0, amrex::Real(0.0), zn)   // L551
                     - phi_n(xn - hh0, amrex::Real(0.0), zn);
amrex::Real const gz0 = phi_n(xn, amrex::Real(0.0), zn + hh0)   // L553
                     - phi_n(xn, amrex::Real(0.0), zn - hh0);
```

`phi_n(x, y, z)` 的 y 参数被硬编码为 `0.0`，z 参数用 `j*dz_n` 计算。

在 AMReX 索引约定中：
- **2D (XZ)**：`i` = x，`j` = z。此时代码正确。
- **3D**：`i` = x，`j` = y，`k` = z。z 索引应为 `k`，此处用 `j` 导致 z 坐标错误，且 y 被固定为 0。
- **1D**：只有 `i`（z 方向），`j` 不存在 / 为 0。

虽然代码在 [L510-532](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicProperties/MacroscopicProperties.cpp#L510-L532) 对 1D 的**邻居单元**做了维度处理（`iL/iR` 沿第一维），但**节点坐标 `zn` 和 `phi_n` 调用**完全没有做维度适配。`phi_n` 是用户通过 `interface_function` 提供的 level-set 函数，在 3D 中会收到错误的 (x, 0, y*dz) 坐标。

#### 影响

- 任何 1D 或 3D 仿真若使用 `interface_function` 定义介质界面，节点上的 ε（介电常数）和 σ（电导率）都会在错误位置求值 → 界面处的波速、阻尼完全错误。
- 这是**静默物理错误**：仿真正常运行、不报错，但电磁场在介质界面的反射/折射/吸收全错。
- **上游对等性缺口**：WarpX 上游的宏观介质 solver 支持 1D/2D/3D/RZ 全维度的 `interface_function`，此分支把它降格为仅 2D 正确。

#### 对项目目标的违反

- **违反 WarpX 上游功能对等性**：插件化重构不应降低原有功能的维度支持。此分支把一个全维度可用的物理模块变成了仅 2D 正确。
- **违反物理正确性**：level-set 界面定位错误直接导致介电常数分配错误，违反麦克斯韦方程在介质界面的边界条件。

#### 为什么现有测试没发现

宏观介质的测试（如有）可能只跑了 2D 案例。3D 界面测试会暴露此问题，但未被覆盖。

---

### 发现 5 — 嵌套 `Scheduler::run` 清空外层 DataBus 活跃 handler 【MEDIUM】

**位置**：[Scheduler.cpp:22-33](file:///workspace/Source/Core/Runtime/Scheduler.cpp#L22-L33)

#### 根因

```cpp
if (ctx.bus != nullptr) { ctx.bus->set_active_handler(&spec); }
try {
    handler->apply(ctx);
} catch (...) {
    if (ctx.bus != nullptr) { ctx.bus->clear_active_handler(); }
    throw;
}
if (ctx.bus != nullptr) { ctx.bus->clear_active_handler(); }
```

`set_active_handler` / `clear_active_handler` 没有保存/恢复上一个值。当 handler 的 `apply()` 内部再次触发 `Scheduler::run`（嵌套调度）时：

1. 外层 `run` 设置 `m_active = &outer_spec`。
2. 内层 `run` 设置 `m_active = &inner_spec`。
3. 内层 `run` 结束，`clear_active_handler()` → `m_active = nullptr`。
4. 外层 `apply` 继续执行，此时 `m_active` 为 `nullptr`，DataBus 的 `require_field_access` 不再有声明可检查 → **访问控制失效**。

#### 可触发路径

`RunPushStage`（外层 Scheduler，区间 `[BeforePush, AfterPush)`）→ `ParticlePushHandler::apply` → `host->particleServices().pushParticlesAndDeposit` → `RunPicPush` → `OneStep_PushParticles` → `RunCollisionStage`（内层 Scheduler，区间 `[BeforeCollisions, AfterCollisions)`）。

内层 `RunCollisionStage` 的 `scheduler.run` 执行完后 `clear_active_handler`，外层 `ParticlePushHandler` 在 `apply` 返回前的任何 DataBus 访问都不再受声明约束。

当前 in-tree handler 在内层返回后不再访问 bus，所以**目前不触发**。但任何第三方 handler 若在 `apply` 中调用会触发嵌套调度的 host 服务，且之后还访问 bus，就会绕过访问控制。

#### 影响

- **潜伏的访问控制失效**：插件独立性依赖 DataBus 基于声明的访问检查，嵌套调度会破坏这个保证。
- 属于"现在不炸、将来必炸"的架构债。

#### 对项目目标的违反

- **破坏总线的安全模型**：`set_active_handler` 是 DataBus 实现"按声明检查访问"的机制。没有 save/restore 就不是可重入的，与插件可组合的目标不兼容。

#### 为什么现有测试没发现

需要一个 handler 在嵌套调度后继续访问 bus 的测试场景，in-tree 没有这样的 handler。

---

### 发现 6 — `add_warpx_pytest` 在配置期收集测试，干净构建时静默失败 【LOW / 工程】

**位置**：[WarpXFunctions.cmake:344-351](file:///workspace/cmake/WarpXFunctions.cmake#L344-L351)，[tests/unit/conftest.py:53-58](file:///workspace/tests/unit/conftest.py#L53-L58)

#### 根因

`add_warpx_pytest` 在 **CMake 配置期**执行 `pytest --collect-only`：

```cmake
execute_process(
    COMMAND ${Python3_EXECUTABLE} -m pytest ${THIS_TEST_DIR} --collect-only -q
    ...
    RESULT_VARIABLE _pytest_collect_result
)
if(NOT _pytest_collect_result EQUAL 0)
    message(WARNING "add_warpx_pytest: could not collect tests from ${THIS_TEST_DIR}")
    return()
endif()
```

而 `tests/unit/conftest.py` 的 `pytest_configure` 中：

```python
available = available_dims()
if not available:
    raise pytest.UsageError(
        "No warpx_pybind_* module found. Build WarpX with -DWarpX_PYTHON=ON "
        "and make sure the package is importable, e.g. with "
        "`cmake --build build --target pip_install`."
    )
```

在**干净构建**流程中：先 `cmake --fresh -S . -B build`（配置），此时 `warpx_pybind_*` 尚未编译/安装 → `pytest --collect-only` 抛 `UsageError` → `add_warpx_pytest` 打印 WARNING 后 `return()` → **所有 pytest 对应的 ctest 条目都不注册**。

之后即使 `cmake --build build --target pip_install` 成功安装了 pywarpx，ctest 列表已经在配置期固定，pytest 测试仍然不可用，必须重新配置 CMake 才能恢复。

#### 影响

- 在 CI 的干净构建流水线中，Python 单元测试可能完全没被执行，且只在 CMake 输出里有一行 WARNING，容易被忽略。
- 开发者本地"配置-构建-安装-测试"流程中，pytest 静默缺失。

#### 对项目目标的违反

- 非物理影响，但降低了测试覆盖率的可信度，可能让其他 bug（如发现 1 的 C ABI 路径）长期不被发现。

#### 为什么现有测试没发现

因为测试本身就没被注册——这是一个"测试发现机制的 bug"，只有人工检查 ctest 列表或 CMake 日志才能发现。

---

## 四、对项目核心目标的系统性违反总结

本分支的根本目标是：**将 WarpX 重构为"插件相互独立，仅通过 core 接口 / 数据总线通信"的架构**。上述发现从不同层面破坏了这一目标：

| 目标维度 | 违反发现 | 具体表现 |
|---|---|---|
| **插件可扩展性** | #1 | C 插件读场直接崩溃，对外扩展面不可用 |
| **总线契约（声明-检查）** | #1, #2, #5 | C 适配器不走 const 读路径；冲突检测与排序键定义不一致；嵌套调度清空活跃 handler 使检查失效 |
| **插件不影响核心物理** | #3 | 只读观察者通过"存在性"静默跳过核心粒子推进/场求解 |
| **WarpX 上游功能对等** | #4 | 宏观介质界面在 1D/3D 下错误，上游全维度支持被降格 |
| **物理正确性** | #3, #4 | 粒子冻结、界面 ε/σ 错误，均为静默错误 |
| **声明诚实性** | #2 | in-tree ParticlePushHandler 的 reads/writes 全部为空，使声明机制沦为摆设 |

## 五、修复优先级建议

1. **立即修复**：#1（C ABI 读场崩溃）、#3（stage 接管跳过核心物理）——这两个任何一个都能让仿真完全错误或崩溃。
2. **尽快修复**：#2（冲突检测一致性）、#4（宏观介质维度）——静默物理错误 + 架构不变量破坏。
3. **计划修复**：#5（嵌套调度 save/restore）——架构债，第三方插件会触发。
4. **择机修复**：#6（pytest 收集时机）——工程质量。

## 六、审计局限

- 浅克隆只有一个提交，无法做逐提交归因；所有发现针对当前工作树快照。
- 未做编译/运行验证（审计角色限定为只读分析，不修改代码）；所有结论基于静态代码阅读。
- 行号基于当前快照，后续提交可能偏移。
