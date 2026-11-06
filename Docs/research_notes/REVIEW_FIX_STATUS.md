# 代码改动复核：上一轮 Review 问题的修复状态

**审核基线**：分支 `trae/agent-GPG8wE`，HEAD `99ad292`（`feat(runtime): migrate FieldProbe/ParticleExtrema/histograms`）
**审核时间**：2026-11-06
**审核方式**：静态代码审查 + 三件快速门禁实跑（未编译、未跑 ctest，原因见 §5）
**工作区状态**：干净（`git diff` 为空，无未提交改动）

> 说明：本仓库当前 `origin/main` 指向初始导入提交 `bb0c164`，运行期改造代码位于
> `trae/agent-GPG8wE` 分支。本文按该分支的实际代码逐项核对。
> 本文只回答"问题是否被修复"，不评价架构方向。

---

## 1. 摘要

| 原编号 | 问题 | 状态 |
|---|---|---|
| C1 | `Runtime::finalize()` 不清理 `RuntimeContext`，留下悬空指针 | ❌ 未修复 |
| C2 | `WarpXRuntimeData::Legacy()` 静态对象持有 `WarpX&`，析构顺序未定义 | ❌ 未修复 |
| C3 | `ReducedDiags::config()` / `frame()` 用 `static const` 一次性快照 | ❌ 未修复 |
| C4 | 测试断言与输入不匹配、维度注册未过滤 | ❌ 未修复（定位更具体） |
| E1 | `ParticleHistogram2D` 局部变量 `data` 遮蔽成员函数 `data()` | ❌ 未修复（判定为 OPMD 构建不可编译） |
| M1 | `cellSize` 返回类型不统一 | ✅ 已一致 |
| M2 | `fluids()` 返回裸指针 | ❌ 未修复 |
| M3 | `Services` 接口无默认实现、恒为 `nullptr` | ❌ 未修复 |
| M4 | `ReducedDiags::WriteToFile` 仍有 `WarpX::GetInstance()` | ❌ 未修复 |
| M5 | `FieldEnergy` 的 `EffectiveRange` 分支仍有 `GetInstance()` | ❌ 未修复 |
| M6 | `FieldPoyntingFlux` 残留旧访问 | ✅ 已修复（0 处 `GetInstance`，已改用 `data()`） |
| M7 | `Timestep` 判空分支 | ⚠️ 部分（已有 `ctx` 判空，仍保留 `GetInstance` 回退） |
| M8 | `main.cpp` 无异常捕获 | ❌ 未修复 |
| M9 | particles 模式断言覆盖 | ⚠️ 部分（新增 5 组断言，但新增一处错位断言） |
| M10 | `lb_eff` 无任何断言 | ❌ 未修复 |
| M11 | 自动化脚本使用 `/tmp` 固定路径 | ✅ 不存在（本树无该脚本，`Tools/automation` 内无 `/tmp`） |
| M12 | CI 无构建 + 冒烟 | ❌ 未修复（仅 3 个快速门禁） |

**统计：18 项中，已修复 3 项，部分 2 项，未修复 13 项。**

另有 3 项新发现（见 §4），其中 1 项为**编译级**问题。

---

## 2. Critical 项逐条核对

### C1 — `finalize()` 不清理上下文（未修复）

期望：`Runtime::finalize()` 在最后执行 `SetRuntimeContext(nullptr)`。

实际：

[Runtime.cpp](file:///workspace/Source/Runtime/Runtime.cpp) 的 `finalize()` 全部内容为：

```cpp
WarpX::Finalize();
m_finalized = true;
```

上下文由 [Runtime.cpp:51](file:///workspace/Source/Runtime/Runtime.cpp#L51) 在构造期注入：

```cpp
warpx.SetRuntimeContext(&m_context);
```

而 [WarpX.H:1118](file:///workspace/Source/WarpX.H#L1118) 只保存裸指针：

```cpp
void SetRuntimeContext (warpx::runtime::RuntimeContext* context) { m_runtime_context = context; }
```

`m_context` 是 `Runtime` 的成员（[Runtime.H:72](file:///workspace/Source/Runtime/Runtime.H#L72)），
`WarpX` 是进程级单例、生命周期更长。因此 `Runtime` 析构后，
`WarpX::m_runtime_context` 仍指向已释放对象——任何在 `finalize()` 之后读取上下文的路径都是 use-after-free。

修复量：1 行。

### C2 — 静态 Legacy 对象持 `WarpX&`（未修复）

[WarpXRuntimeData.cpp:20](file:///workspace/Source/Runtime/WarpXRuntimeData.cpp#L20)：

```cpp
static WarpXRuntimeData legacy(WarpX::GetInstance());
```

[WarpXRuntimeData.H:50](file:///workspace/Source/Runtime/WarpXRuntimeData.H#L50)：

```cpp
WarpX& m_warpx;
```

该静态对象首次使用时构造，持有单例引用，析构顺序相对 `WarpX` 单例未定义。
若 `WarpX` 先析构，静态对象后续被再次访问即悬空。

### C3 — `config()` / `frame()` 仍是一次性快照（未修复）

[ReducedDiags.cpp:180](file:///workspace/Source/Diagnostics/ReducedDiags/ReducedDiags.cpp#L180)：

```cpp
static const warpx::runtime::RuntimeConfig legacy =
    warpx::runtime::RuntimeConfig::FromLegacy();
```

[ReducedDiags.cpp:191](file:///workspace/Source/Diagnostics/ReducedDiags/ReducedDiags.cpp#L191)：

```cpp
static const warpx::runtime::FrameState legacy =
    warpx::runtime::FrameState::FromLegacy();
```

与之相邻的 `data()`（[:167-173](file:///workspace/Source/Diagnostics/ReducedDiags/ReducedDiags.cpp#L167-L173)）
每次调用都重新取 `WarpXRuntimeData::Legacy()`。三个访问器语义不一致：
`data` 实时、`config`/`frame` 冻结在首次调用时刻。
运行期变化（`moving_window_v`、`dt`、`finestLevel`、`n_step`）在无上下文路径下不可见。

### C4 — 测试断言与输入不匹配（未修复，且已定位到具体行）

两个独立问题叠加：

1. **断言位置错误**：[test_runtime_snapshot.cpp:246-256](file:///workspace/tests/runtime/test_runtime_snapshot.cpp#L246-L256)
   的 `field_probe` 断言位于 `if (!particles_mode)`（137-198）与 `if (particles_mode)`（200-244）**之外**，
   因此在两种模式下都会执行。
   而 [inputs_2d_particles:17](file:///workspace/tests/runtime/inputs_2d_particles#L17) 的
   `reduced_diags_names` 不含 `field_probe`，该文件不会被生成。
   `last_line` 对不存在的文件返回空串（[test_runtime_snapshot.cpp:126-135](file:///workspace/tests/runtime/test_runtime_snapshot.cpp#L126-L135)），
   随后解析得到全零，`check_eq(step, 1)` 必然失败。

2. **维度未过滤**：[tests/runtime/CMakeLists.txt:1-16](file:///workspace/tests/runtime/CMakeLists.txt#L1-L16)
   对 `WarpX_DIMS` 中每个维度都注册同一份 `inputs_2d` 输入，仅以 `if(TARGET lib_${SD})` 作为存在性判断。
   若配置了 1D 或 3D 构建，测试将以 2D 输入运行。

---

## 3. Major 项核对

### 已修复 / 不存在

| 项 | 证据 |
|---|---|
| M1 `cellSize` | 接口 [RuntimeData.H:56](file:///workspace/Source/Runtime/RuntimeData.H#L56) 与实现 [WarpXRuntimeData.H:38](file:///workspace/Source/Runtime/WarpXRuntimeData.H#L38) 均为 `amrex::Array<amrex::Real, 3>`，类型一致 |
| M6 `FieldPoyntingFlux` | `grep GetInstance` 在该文件为 0 处；已改为 `data().geom/data().lowerCorner/data().fields()` |
| M11 `/tmp` 固定路径 | `grep -rn "/tmp" Tools/automation` 为空；原报告中的 `build_catalog` 脚本在本树不存在 |

### 未修复

| 项 | 证据 | 影响 |
|---|---|---|
| M2 `fluids()` 裸指针 | [WarpXRuntimeData.cpp:29](file:///workspace/Source/Runtime/WarpXRuntimeData.cpp#L29) 返回 `MultiFluidContainer*`，同接口其余 50 项均返回引用 | 调用方需自行判空，风格与其它访问器不一致 |
| M3 `Services` 死接口 | [RuntimeContext.H:36](file:///workspace/Source/Runtime/RuntimeContext.H#L36) `Services* services = nullptr;`；全仓库无赋值点，`Services.H` 无默认实现 | 接口存在但永不生效 |
| M4 `WriteToFile` | [ReducedDiags.cpp:150](file:///workspace/Source/Diagnostics/ReducedDiags/ReducedDiags.cpp#L150) `WarpX::GetInstance().gett_new(0)` | 绕过了 `data()` 抽象 |
| M5 `FieldEnergy` | [FieldEnergy.cpp:184](file:///workspace/Source/Diagnostics/ReducedDiags/FieldEnergy.cpp#L184) `auto const & warpx = WarpX::GetInstance();` | 同上 |
| M8 `main.cpp` | [main.cpp](file:///workspace/Source/main.cpp) 无 `try/catch` | `Runtime` 构造/初始化抛异常时直接 `std::terminate` |
| M10 `lb_eff` | `grep -n "lb_eff" tests/runtime/test_runtime_snapshot.cpp` 无结果；仅 [:98](file:///workspace/tests/runtime/test_runtime_snapshot.cpp#L98) 校验了配置字段 `load_balance_costs_update_algo` | `LoadBalanceEfficiency` 的输出文件从未被断言 |
| M12 CI | [.github/workflows/route-gates.yml:13-28](file:///workspace/.github/workflows/route-gates.yml#L13-L28) 只有 compliance / audit_check / route_tasks 三步 | 无构建、无冒烟，代码级回归不会被 CI 捕获 |

### 部分修复

| 项 | 现状 |
|---|---|
| M7 `Timestep` | [Timestep.cpp:65-70](file:///workspace/Source/Diagnostics/ReducedDiags/Timestep.cpp#L65-L70) 已有 `ctx != nullptr && ctx->time != nullptr` 判空；[:72](file:///workspace/Source/Diagnostics/ReducedDiags/Timestep.cpp#L72) 仍回退到 `GetInstance()` |
| M9 particles 断言 | 新增 `part_num`、`part_energy`、`rho_max`、`extrema`、`part_mom` 五组断言（:200-244）；但同文件新增了错位的 `field_probe` 断言（见 C4） |

---

## 4. 新发现

### N1（编译级）`ParticleHistogram2D` 局部变量遮蔽成员函数

[ParticleHistogram2D.cpp:299](file:///workspace/Source/Diagnostics/ReducedDiags/ParticleHistogram2D.cpp#L299)：

```cpp
auto data = f_mesh[io::RecordComponent::SCALAR];
```

同一作用域内随后调用成员函数（[:317](file:///workspace/Source/Diagnostics/ReducedDiags/ParticleHistogram2D.cpp#L317)）：

```cpp
auto const time = data().t_new(0);
```

局部变量 `data` 遮蔽了 `ReducedDiags::data()`（[ReducedDiags.H:60](file:///workspace/Source/Diagnostics/ReducedDiags/ReducedDiags.H#L60)）。
按 C++ 名字查找规则，此处 `data` 是对象而非函数，`data()` 非良构。
该段位于 `#if defined(WARPX_USE_OPENPMD)` 内，**在启用 openPMD 的构建中会编译失败**。
最小修复：改变量名（如 `component`），或调用改为 `this->data()`。

> 说明：本环境未保留构建目录，未能用编译器直接确认；该判断来自静态审查。
> 建议以一次 `WarpX_OPENPMD=ON` 的构建验证。

### N2 测试用例共享工作目录

[tests/runtime/CMakeLists.txt](file:///workspace/tests/runtime/CMakeLists.txt) 中
`runtime_snapshot_${D}` 与 `runtime_snapshot_particles_${D}` 的 `WORKING_DIRECTORY` 均为
`${CMAKE_CURRENT_BINARY_DIR}`，两个用例写入同一个 `diags/reducedfiles/`。
在 `ctest -j` 并行或顺序不固定时，一个用例可能读到另一个用例残留的 `*.txt`，
从而把"文件缺失"掩盖成"数值不符"，或反之。建议每个用例独立工作目录。

### N3 `finalize()` 后上下文仍可被读取

`Runtime::context()` 在 `finalize()` 之后仍返回 `m_context` 的引用，
而 `WarpX::m_runtime_context` 也仍指向它（C1 的直接延伸）。
`Runtime::~Runtime()` 先 `finalize()` 再 `finalize_external_libraries()`，
若 `finalize()` 抛异常，外库不会释放；析构函数未声明 `noexcept`。

---

## 5. 验证能力与限制

| 项 | 结果 |
|---|---|
| 工作区 | 干净，无未提交改动 |
| `python3 Tools/automation/gates/compliance.py` | PASS（2705 个跟踪文件） |
| `python3 Tools/automation/gates/audit_check.py` | PASS（379 头文件 / 349 类 / 2418 公共方法） |
| `python3 Tools/automation/route_tasks.py next` | 正常输出 A1.1 任务（图有效） |
| 构建 | **未执行**：本工作树无 `build/` 目录，未配置 |
| `ctest` | **未执行**：无可执行文件 |
| 已知构建异常 | 早前后台构建失败于 `ranlib: BackTransformParticleFunctor.cpp.o: file truncated`，属陈旧目标文件截断，非源码错误 |

因此：本文所有"是否修复"的判断均基于源码静态核对；
需要一次干净的 `WarpX_DIMS=2` 配置 + 构建 + `ctest -R '^runtime_'` 才能确认运行时行为。

---

## 6. 建议的最小修复顺序（仅为选项，不含时间估计）

| 顺序 | 项 | 改动性质 |
|---|---|---|
| 1 | C1：`finalize()` 末尾 `SetRuntimeContext(nullptr)` | 1 行 |
| 2 | N1：`ParticleHistogram2D` 局部变量改名 | 1 行 |
| 3 | C4 / N2：`field_probe` 断言移入 `if (!particles_mode)`；每个测试独立工作目录；维度过滤 | 测试文件 |
| 4 | C3：`static const` → 每次调用刷新（或统一走 `WarpXRuntimeData::Legacy()`） | 2 处 |
| 5 | C2：`Legacy()` 不再持有 `WarpX&`（改为每次 `GetInstance()` 取用） | 接口内部 |
| 6 | M10：补 `lb_eff.txt` 断言 | 测试文件 |
| 7 | M8：`main.cpp` 包 `try/catch` | 1 处 |
| 8 | M12：CI 增加 build + smoke job | workflow |

其中第 1、2 项各自只需一行，且直接消除"悬空指针"与"编译失败"两类硬问题，
与其它改动无耦合，可先单独提交。

---

## 附录：本文数据的复现命令

```bash
# 基线
git log --oneline -1
git status --short

# C1
sed -n '/Runtime::finalize/,/^}/p' Source/Runtime/Runtime.cpp
grep -n "SetRuntimeContext" Source/WarpX.H Source/Runtime/Runtime.cpp

# C2 / C3
grep -n "static WarpXRuntimeData\|static const warpx::runtime" \
  Source/Runtime/WarpXRuntimeData.cpp Source/Diagnostics/ReducedDiags/ReducedDiags.cpp

# C4 / N2
grep -n "if (!particles_mode)\|if (particles_mode)\|field_probe" \
  tests/runtime/test_runtime_snapshot.cpp
grep -n "reduced_diags_names" tests/runtime/inputs_2d tests/runtime/inputs_2d_particles
grep -n "WORKING_DIRECTORY\|inputs_2d" tests/runtime/CMakeLists.txt

# E1 / N1
sed -n '290,325p' Source/Diagnostics/ReducedDiags/ParticleHistogram2D.cpp

# M2 / M3 / M4 / M5
grep -n "GetInstance" Source/Diagnostics/ReducedDiags/ReducedDiags.cpp \
  Source/Diagnostics/ReducedDiags/FieldEnergy.cpp
grep -n "services" Source/Runtime/RuntimeContext.H

# M10 / M12
grep -n "lb_eff" tests/runtime/test_runtime_snapshot.cpp
sed -n '1,40p' .github/workflows/route-gates.yml

# 门禁
python3 Tools/automation/gates/compliance.py
python3 Tools/automation/gates/audit_check.py
python3 Tools/automation/route_tasks.py next
```

> 本文所有判断对应 HEAD `99ad292`。代码更新后需重新核对。
</content>
