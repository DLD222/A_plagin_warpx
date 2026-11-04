# 插件化 P1 实施方案：Plugin 骨架 + 官方 Yee 插件 + 内核调用点

<!-- SPDX-License-Identifier: BSD-3-Clause -->

> **注意：本文已被 `PLUGIN_ARCHITECTURE_FINAL.md` 取代。** 最终架构把角色扩展到场/物质/材料/边界/诊断/AMR，并把“内核开孔”定义为 13 个聚合 seam；本文仅作为 M1 中“Yee 场插件”这一块的施工细节保留。

> 依据：`Docs/research_notes/PLUGIN_ARCHITECTURE_DESIGN.md`（c6661b7）§6/§8。
> 本文只做方案，不改代码；所有接口签名、文件清单、回归方法以本文为准，确认后再进入实现。
> 代码事实核对基于工作区 `7bcb44a`（= `WarpX_fluid` HEAD `0de0e26`）。

---

## 0. 结论摘要

**可以做，但设计稿有 4 处必须在 P1 先“修形”，否则实现会卡住或验收失真：**

1. **`FieldState` 不能只有 `Array4` 视图。** 真实 Yee 求解器 `FiniteDifferenceSolver::EvolveB/E` 需要 `MultiFabRegister&`、EB 的 `flag_info_face`/`borrowing`、PML、`eb_update_E`、层次号与 patch 类型。P1 折中为：`FieldState` 暴露**借用的 `VectorField`（`std::array<MultiFab*,3>`）**，`FieldView` 只作为 per-FAB kernel 参数；不暴露 MultiFab 的分配/生命周期语义。
2. **`advance()` 必须包含 PML、物理边界和 ECT 的 `EvolveECTRho`。** 否则插件只包了 curl 内核，边界仍由内核调用，`boundary()` 与 `advance()` 会重复。P1 的 `advance()` = 一次完整的 E 或 B 更新；`boundary()` 保留为 P2 的空默认实现，P1 不调用。
3. **`TimeLayer` 的语义要写成表。** WarpX 是“B 半步 → E 全步 → B 半步”的 Strang 分裂，而 PSATD 是 E/B 同时全步。P1 只映射 `FirstHalf/SecondHalf → B`、`Full → E`，PSATD 留到 P2。
4. **介质路径 `MacroscopicEvolveE` 不在 P1 的 E 调用点内。** 这是本仓库与上游最大的差别：`algo.em_solver_medium = macroscopic` 时 `Evolve()` 根本不调用 `EvolveE`。因此设计 §6.4 列的 P0/E3/S5/E6 **只覆盖 B 分派**；E 分派必须用真空/ECT 算例（`c02_0_pec`、上游 ctest）单独覆盖。这一条如果不写清楚，验收会“看起来全过、实际没测到 E”。

**最小闭环**：新增 `Source/Plugin/`（14 个 .H/.cpp + 2 个 CMakeLists + 1 个 Make.package + 2 个测试/脚本文件），修改 5 个文件（根 `CMakeLists.txt`、`Source/Make.WarpX`、`Source/WarpX.H`、`Source/WarpX.cpp`、`Source/FieldSolver/WarpXPushFieldsEM.cpp`）。

**主验收 = 同一二进制 A/B 逐位**（`algo.field_equation = yee` vs `legacy`），它不依赖文档里互相打架的基线数字；**次验收 = 对旧二进制数值门 + 现有 `analyze.py` 物理门**。

**估时：约 5–7 人日**（含首次构建与 A/B 回归；不含介质 E 路径的插件化；见 D7）。

---

## 1. 现状核对（决定方案形状的硬事实）

| # | 事实 | 位置 |
|---|---|---|
| F1 | `WarpX::EvolveB/E` 的“完整实现”在 `(lev, PatchType, dt, …)` 重载；外层 `(lev, dt, …)` 只做循环 + Python 回调 | `Source/FieldSolver/WarpXPushFieldsEM.cpp:950-1089` |
| F2 | 每个 patch 重载依次做：**核心 curl → PML → 物理边界 →（E 还有 ECT rho）** | 同上 `:972-1000`、`:1027-1089` |
| F3 | `EvolveB/E` 在 `OneStep_nosub` 与 `OneStep_sub1/sub2` 中被多处调用，dt 与 patch 组合很多（细 patch 半步、粗 patch 整步） | `Source/Evolve/WarpXEvolve.cpp:661-677`、`:1253-1433` |
| F4 | 介质模式走 `MacroscopicEvolveE`，**不调用** `EvolveE`；它自己内含 `rho_medium` 播种/推进、`jcond`、`FillBoundaryAndSync` 等 | `Source/Evolve/WarpXEvolve.cpp:664-670`、`WarpXPushFieldsEM.cpp:1198+` |
| F5 | 求解器对象 `m_fdtd_solver_fp/cp` 是 private；fp 有 public getter，**cp 没有** | `Source/WarpX.H:1109`、`:1783-1784` |
| F6 | `m_flag_info_face`/`m_borrowing` 是 private、无 getter；`m_eb_update_E_fp` 有 getter，**cp 没有** | `Source/WarpX.H:1398-1430` |
| F7 | `ApplyEfieldBoundary/ApplyBfieldBoundary`、`GetPML`、`DoPML`、`GetMultiFabRegister` 是 public；`do_dive_cleaning` 是 public static | `Source/WarpX.H:442/650-651/682/1113/307` |
| F8 | `algo.maxwell_solver` 的解析在 `ReadParameters()` 的 `ParmParse pp_algo("algo")` 块 | `Source/WarpX.cpp:579-581` |
| F9 | `WarpX` 构造顺序：`m_instance = this` → `ReadParameters()` → …；插件注册必须发生在 `ReadParameters()` 之前 | `Source/WarpX.cpp:322-330` |
| F10 | `WarpXAlgorithmSelection.H` 在**头文件里** `using namespace ablastr::utils::enums;`，所以 `PatchType` 是全局可见的 | `Source/Utils/WarpXAlgorithmSelection.H:17` |
| F11 | 单元测试只有 pytest（`tests/unit/`），没有现成 C++ 测试目标；`add_warpx_test` 定义在 `Examples/CMakeLists.txt:28` | — |

**由 F1–F3 得到的调用点选择**：只在 `(lev, PatchType, …)` 重载的**函数体最前面**做一次分派，然后 `return`；旧实现原样留在下面。这样：

- 每个 level、每个半步恰好一次虚调用；
- 细/粗 patch、AMR 子循环、`SubcyclingHalf` 全部自然覆盖；
- 旧代码一行不删，`legacy` 路径随时可做 A/B。

---

## 2. 接口签名（最终稿）

### 2.1 命名空间与宏

```cpp
namespace warpx::plugin { ... }
```

- 设计稿的 `moments::` 在本仓库没有对应命名空间（`MomentsSolver` 只是目录），改用 `warpx::plugin`。
- 注册宏用 `WARPX_REGISTER_PLUGIN(T, NAME)`；同时提供 `#define MOMENTS_REGISTER_PLUGIN WARPX_REGISTER_PLUGIN` 作为设计稿示例的兼容别名（不另起语义）。

### 2.2 `PluginInfo.H`

```cpp
#pragma once

namespace warpx::plugin
{

enum class PluginRole
{
    Field,
    Species,
    Material,
    Interaction,
    Boundary,
    Source,
    Diagnostic
};

struct PluginInfo
{
    const char* name;     //!< 选择的字符串，如 "yee"
    const char* version;  //!< 仅用于显示，如 "1.0"
    PluginRole  role;
};

[[nodiscard]] const char* to_string (PluginRole role) noexcept;

} // namespace warpx::plugin
```

> 与设计稿的差异：`role` 从 `const char*` 改为 `enum class`。多一层类型安全，且 `PluginHost` 的 key 不必拼字符串。外部插件写 `PluginRole::Field`。

### 2.3 `TimeLayer` 与 `IFieldEquation`

```cpp
// Plugin.H
enum class TimeLayer
{
    Full,        //!< E 全步：E^{n} -> E^{n+1}（含 PML、边界、ECT rho）
    FirstHalf,   //!< B 前半步：B^{n} -> B^{n+1/2}
    SecondHalf   //!< B 后半步：B^{n+1/2} -> B^{n+1}
};

class IPlugin
{
public:
    virtual ~IPlugin () = default;
    [[nodiscard]] virtual PluginInfo info () const = 0;
};

class IFieldEquation : public IPlugin
{
public:
    /**
     * 完整推进一层的一个分量。
     * 进入时 state.start_time() 是本节拍的起点；返回时该层该 patch 的
     * E 或 B（以及 PML 内对应分量）已推进 dt，物理边界已施加。
     */
    virtual void advance (FieldState& state,
                          const FieldSource& source,
                          amrex::Real dt,
                          TimeLayer layer) = 0;

    //! P2 拆分 IBoundary 时启用；P1 的 advance() 已经施加边界，内核不调用。
    virtual void boundary (FieldState& /*state*/) {}
};
```

**`TimeLayer` 契约表（必须写进头文件注释）：**

| `layer` | 内核调用点 | 插件必须做的事 | 对应旧代码 |
|---|---|---|---|
| `FirstHalf` | `EvolveB(lev, patch, dt, SubcyclingHalf::FirstHalf, t)` | `EvolveB` → `EvolveBPML` → `ApplyBfieldBoundary(..., FirstHalf, t+dt)` | `WarpXPushFieldsEM.cpp:972-1000` |
| `Full` | `EvolveE(lev, patch, dt, t)` | `EvolveE` → `EvolveEPML` → `ApplyEfieldBoundary` →（ECT）`EvolveECTRho` | 同上 `:1027-1089` |
| `SecondHalf` | `EvolveB(..., SecondHalf, t)` | 同 `FirstHalf`，但边界传 `SecondHalf` | 同上 |
| `SubcyclingHalf::None` | 当前无调用 | **内核在分派前 `WARPX_ABORT`**（`to_time_layer` 不接收 `None`），避免把 B 更新误当 E 更新 | — |

> 设计稿的 `TimeLayer` 只说了“E/B 分开推进 + 半格语义”，没有定义哪个值对应 E。上表把它钉死。PSATD（P2）会把 `Full` 解释为“E、B 同时全步”，签名不变。

### 2.4 `FieldView.H` / `FieldState.H` / `FieldSource.H`

```cpp
// FieldView.H —— per-FAB kernel 参数，零开销
struct FieldView
{
    amrex::Array4<amrex::Real> arr;
    amrex::Box bx;
    int ncomp;
};

struct ConstFieldView
{
    amrex::Array4<const amrex::Real> arr;
    amrex::Box bx;
    int ncomp;
};
```

```cpp
// FieldState.H
class FieldState
{
public:
    FieldState (ablastr::fields::MultiFabRegister& fields,
                int lev,
                ablastr::utils::enums::PatchType patch_type,
                amrex::Real start_time) noexcept;

    [[nodiscard]] ablastr::fields::MultiFabRegister& fields () const noexcept;
    [[nodiscard]] int lev () const noexcept;
    [[nodiscard]] ablastr::utils::enums::PatchType patch_type () const noexcept;
    [[nodiscard]] amrex::Real start_time () const noexcept;

    //! 借用的场指针；只保证在本节拍内有效。插件不得 delete / 重新分配。
    [[nodiscard]] ablastr::fields::VectorField Efield () const;  // Efield_fp / Efield_cp
    [[nodiscard]] ablastr::fields::VectorField Bfield () const;  // Bfield_fp / Bfield_cp

private:
    ablastr::fields::MultiFabRegister& m_fields;
    int m_lev;
    ablastr::utils::enums::PatchType m_patch_type;
    amrex::Real m_start_time;
};
```

```cpp
// FieldSource.H
class FieldSource
{
public:
    FieldSource (ablastr::fields::MultiFabRegister& fields,
                 int lev,
                 ablastr::utils::enums::PatchType patch_type) noexcept;

    [[nodiscard]] ablastr::fields::VectorField Jfield () const;  // current_fp / current_cp
    [[nodiscard]] amrex::MultiFab* rho () const;                 // rho_fp / rho_cp，可能为 nullptr

private:
    ablastr::fields::MultiFabRegister& m_fields;
    int m_lev;
    ablastr::utils::enums::PatchType m_patch_type;
};
```

**为什么不是设计稿的“纯 Array4”**：`Array4` 是单个 FAB 的视图，一个 level 有多个 FAB，插件必须自己 `MFIter`；要让插件在 GPU 上跑，它必须能拿到 `MultiFab` 才能取 `array(mfi)`。因此 `FieldState` 给的是**借用的 `MultiFab*` 数组**，而 `FieldView` 是插件在 kernel 里的局部对象。约束写成：

- 不允许 `new/delete`、`resize`、`define`；
- 不允许跨节拍保存指针；
- 不允许修改 `MultiFab` 的 BoxArray/ixType；
- `fields()` 仅用于按字符串名访问/注册自定义场，不得重新分配标准场（E/B/J/rho）。

这比“只给 Array4”更诚实，也不会让官方 Yee 适配器无法实现。

### 2.5 `PluginHost.H` / `PluginRegistrar.H`

```cpp
class PluginHost
{
public:
    static PluginHost& instance ();

    //! 注册；同 role+name 重复 -> WARPX_ABORT（带已注册列表）
    void add (std::unique_ptr<IPlugin> plugin);

    [[nodiscard]] IPlugin* get (PluginRole role, std::string_view name) const;
    [[nodiscard]] IFieldEquation* get_field_equation (std::string_view name) const;
    [[nodiscard]] std::vector<std::string> names (PluginRole role) const;
    [[nodiscard]] bool empty (PluginRole role) const;

private:
    PluginHost () = default;
    std::map<PluginRole, std::map<std::string, std::unique_ptr<IPlugin>, std::less<>>> m_plugins;
};

inline PluginHost& host () { return PluginHost::instance (); }
```

- `instance()` 用**函数内 static**，避免静态初始化顺序问题；外部插件的静态注册器在 `main` 前调用 `host()` 也安全。
- 查找用 `std::less<>` 异构比较，`std::string_view` 零分配。
- 注册在单线程完成；`get/names` 之后只读。文档里写明“不是线程安全的并发 `add`”。

```cpp
// PluginRegistrar.H
template <typename T>
class PluginRegistrar
{
public:
    explicit PluginRegistrar (const char* /*name*/)
    {
        static_assert (std::is_base_of_v<IPlugin, T>, "plugin must derive from IPlugin");
        host ().add (std::make_unique<T> ());
    }
};

#define WARPX_PLUGIN_CONCAT_IMPL(a, b) a##b
#define WARPX_PLUGIN_CONCAT(a, b) WARPX_PLUGIN_CONCAT_IMPL(a, b)
#define WARPX_REGISTER_PLUGIN(T, NAME)                                        \
    static ::warpx::plugin::PluginRegistrar<T>                                \
        WARPX_PLUGIN_CONCAT(warpx_plugin_reg_, __COUNTER__){(NAME)}

// 设计稿示例的兼容别名
#define MOMENTS_REGISTER_PLUGIN WARPX_REGISTER_PLUGIN
```

- 用 `__COUNTER__` 而不是 `reg_##T`，避免同一类型在两个 TU 注册时重名。
- `NAME` 参数只用于可读性/断言：注册器内部可校验 `info().name == NAME`（`WARPX_ALWAYS_ASSERT_WITH_MESSAGE`），防止“类里写一个名字、宏里写另一个”。

### 2.6 `PluginAccess.H`（只给官方内置适配器用的桥）

官方 Yee 插件需要 `m_fdtd_solver_cp`、`m_flag_info_face`、`m_borrowing`、`m_eb_update_E_cp`、`pml_has_particles` 这些没有 public getter 的东西。**不新增 5 个 public getter**，而是：

```cpp
namespace warpx::plugin
{
class PluginAccess
{
public:
    [[nodiscard]] static FiniteDifferenceSolver* fdtd_solver (
        WarpX& warpx, int lev, ablastr::utils::enums::PatchType patch_type);
    [[nodiscard]] static std::array<std::unique_ptr<amrex::iMultiFab>, 3>& flag_info_face (
        WarpX& warpx, int lev);
    [[nodiscard]] static std::array<std::unique_ptr<amrex::LayoutData<FaceInfoBox>>, 3>& borrowing (
        WarpX& warpx, int lev);
    [[nodiscard]] static std::array<std::unique_ptr<amrex::iMultiFab>, 3>& eb_update_E (
        WarpX& warpx, int lev, ablastr::utils::enums::PatchType patch_type);
    [[nodiscard]] static int pml_has_particles (WarpX& warpx);
};
}
```

`WarpX.H` 里只加一行：

```cpp
namespace warpx::plugin { class PluginAccess; }
...
friend class warpx::plugin::PluginAccess;
```

这样内核只认识**一个**桥类，不认识 `YeeFieldEquation`，也不扩大 public API。`PluginAccess.H` 是**内部头文件**，不写进对外插件文档；外部插件只用 `Plugin.H`。P2 引入 `IBoundary`/`IMaterialModel` 后，把 `PluginAccess` 的成员逐个替换成正式接口，friend 删除。

---

## 3. 文件清单

### 3.1 新增

| 文件 | 作用 | 约行数 |
|---|---|---|
| `Source/Plugin/PluginInfo.H` | `PluginRole`、`PluginInfo`、`to_string` | 40 |
| `Source/Plugin/Plugin.H` | `IPlugin`、`IFieldEquation`、`TimeLayer`（唯一公共入口） | 80 |
| `Source/Plugin/FieldView.H` | `FieldView`、`ConstFieldView` | 30 |
| `Source/Plugin/FieldState.H` | `FieldState` 声明 | 70 |
| `Source/Plugin/FieldSource.H` | `FieldSource` 声明 | 60 |
| `Source/Plugin/PluginHost.H` | 注册表声明 | 70 |
| `Source/Plugin/PluginHost.cpp` | 注册表实现 + `to_string` | 90 |
| `Source/Plugin/PluginRegistrar.H` | 模板注册器 + 宏 | 50 |
| `Source/Plugin/PluginAccess.H` | 桥声明 | 50 |
| `Source/Plugin/PluginAccess.cpp` | 桥实现（include `WarpX.H`） | 70 |
| `Source/Plugin/Builtin/YeeFieldEquation.H` | 官方插件声明 | 40 |
| `Source/Plugin/Builtin/YeeFieldEquation.cpp` | 适配器实现（≈ 旧 `EvolveB/E` 的函数体） | 180 |
| `Source/Plugin/Builtin/BuiltinPlugins.H` | `register_builtin_plugins()` | 25 |
| `Source/Plugin/Builtin/BuiltinPlugins.cpp` | 显式注册（保证链接器不丢对象） | 40 |
| `Source/Plugin/CMakeLists.txt` | `target_sources(lib_${SD})` | 20 |
| `Source/Plugin/Builtin/CMakeLists.txt` | 同上 | 15 |
| `Source/Plugin/Make.package` | GNU make 路径 | 10 |
| `tests/plugin/CMakeLists.txt` | 注册表 C++ 单测目标 | 20 |
| `tests/plugin/test_plugin_registry.cpp` | 注册/查找/未知名 | 80 |
| `benchmarks/cases/plugin/compare_runs.py` | A/B 逐位比较器 | 120 |
| `benchmarks/cases/plugin/inputs_2d_vacuum_yee` | 真空 Yee A/B 小算例（若 `c02_0_pec/inputs_2d` 太长） | 60 |

> `FieldState.cpp`/`FieldSource.cpp` 可以合并进 `PluginHost.cpp` 以减少文件数；上表按“声明/实现分离”列出，实施时可合并，不影响接口。

### 3.2 修改

| 文件 | 改动 | 锚点 |
|---|---|---|
| `CMakeLists.txt` | `add_subdirectory(Source/Plugin)`；`WarpX_EXTRA_PLUGIN_SOURCES` cache 变量 + 追加到 `app_${SD}` | `:447`、`:498-512` |
| `Source/Make.WarpX` | `include $(WARPX_HOME)/Source/Plugin/Make.package` | `:78` 附近 |
| `Source/WarpX.H` | 前置声明 `IFieldEquation`/`PluginAccess`；成员 `IFieldEquation* m_field_equation`、`std::string m_field_equation_name`；friend | `:91` 附近、`:1783` 附近 |
| `Source/WarpX.cpp` | 构造最开始 `plugin::register_builtin_plugins()`；`ReadParameters` 解析 `algo.field_equation` 并解析指针；启动日志 | `:322`、`:579` |
| `Source/FieldSolver/WarpXPushFieldsEM.cpp` | `EvolveB(lev,patch,…)`、`EvolveE(lev,patch,…)` 顶部各加 8 行分派 + `return`；旧函数体不动 | `:972`、`:1027` |

**不改**：`EvolveB/E` 的外层循环、`MacroscopicEvolveE`、`EvolveF/G`、`HybridPICEvolveFields`、`WarpXImplicitOps.cpp`、任何 kernel/`.K` 文件、任何 `FieldType` 枚举。

---

## 4. 内核改动（伪 diff）

### 4.1 `WarpX::WarpX()`：先注册

```cpp
WarpX::WarpX ()
{
    m_instance = this;

    plugin::register_builtin_plugins ();   // <-- 新增，必须早于 ReadParameters

    warpx::initialization::initialize_warning_manager();
    ReadParameters();
    ...
}
```

### 4.2 `ReadParameters()`：解析选择

在 `ParmParse pp_algo("algo")` 块内（`:579` 附近）加：

```cpp
pp_algo.query("field_equation", m_field_equation_name);
```

在本块之后（或 `ReadParameters` 末尾）解析：

```cpp
if (m_field_equation_name == "legacy" || m_field_equation_name == "none") {
    m_field_equation = nullptr;
} else {
    m_field_equation = plugin::host().get_field_equation(m_field_equation_name);
    std::string available;
    for (auto const& n : plugin::host().names(plugin::PluginRole::Field)) {
        available += " " + n;
    }
    WARPX_ALWAYS_ASSERT_WITH_MESSAGE(
        m_field_equation != nullptr,
        "Unknown algo.field_equation = '" + m_field_equation_name + "'. Available:" + available);
}
if (verbose > 0) {
    amrex::Print() << "Field equation: "
                   << (m_field_equation ? m_field_equation->info().name : "legacy (direct)")
                   << "\n";
}
```

默认值：

```cpp
std::string m_field_equation_name = "yee";   // WarpX.H
```

- `yee`：走官方插件；
- `legacy`（或 `none`）：`m_field_equation == nullptr`，走旧直接调用，用于 A/B 与“插件表为空”的等价场景；
- 未知：立即 abort，并打印已注册名字（防止拼写错误静默回退）。
- 若 `maxwell_solver == psatd/hybrid/electrostatic`：插件不会被调用。启动时打一行提示即可，**不要 abort**，否则破坏现有 PSATD 输入。

### 4.3 `EvolveB` 分派

```cpp
void
WarpX::EvolveB (int lev, PatchType patch_type, amrex::Real a_dt,
                SubcyclingHalf subcycling_half, amrex::Real start_time)
{
    if (m_field_equation != nullptr) {
        plugin::FieldState  state  (m_fields, lev, patch_type, start_time);
        plugin::FieldSource source (m_fields, lev, patch_type);
        m_field_equation->advance (state, source, a_dt,
                                   plugin::to_time_layer(subcycling_half));
        return;
    }

    // ---- 以下为原函数体，逐字不动 ----
    if (patch_type == PatchType::fine) { ... }
    ...
}
```

### 4.4 `EvolveE` 分派

```cpp
void
WarpX::EvolveE (int lev, PatchType patch_type, amrex::Real a_dt, amrex::Real start_time)
{
    if (m_field_equation != nullptr) {
        plugin::FieldState  state  (m_fields, lev, patch_type, start_time);
        plugin::FieldSource source (m_fields, lev, patch_type);
        m_field_equation->advance (state, source, a_dt, plugin::TimeLayer::Full);
        return;
    }

    // ---- 原函数体 ----
    ...
}
```

### 4.5 `YeeFieldEquation::advance` 的骨架

```cpp
void YeeFieldEquation::advance (FieldState& state, const FieldSource& /*source*/,
                                amrex::Real dt, TimeLayer layer)
{
    WarpX& warpx = WarpX::GetInstance ();
    auto const lev   = state.lev ();
    auto const patch = state.patch_type ();
    auto& fields     = state.fields ();
    FiniteDifferenceSolver* solver = PluginAccess::fdtd_solver (warpx, lev, patch);

    if (layer == TimeLayer::Full) {
        solver->EvolveE (fields, lev, patch, state.Efield (),
                         PluginAccess::eb_update_E (warpx, lev, patch), dt);

        if (warpx.DoPML () && warpx.GetPML (lev)->ok ()) {
            PML* pml = warpx.GetPML (lev);
            if (patch == PatchType::fine) {
                solver->EvolveEPML (fields, patch, lev,
                                    pml->GetEBUpdateEFlag_fp (),
                                    pml->GetMultiSigmaBox_fp (),
                                    dt, PluginAccess::pml_has_particles (warpx));
            } else {
                solver->EvolveEPML (fields, patch, lev,
                                    pml->GetEBUpdateEFlag_cp (),
                                    pml->GetMultiSigmaBox_cp (),
                                    dt, PluginAccess::pml_has_particles (warpx));
            }
        }

        warpx.ApplyEfieldBoundary (lev, patch, state.start_time () + dt);

#ifdef AMREX_USE_EB
        if (warpx.electromagnetic_solver_id == ElectromagneticSolverAlgo::ECT) {
            // 与旧代码一致：只有 E 分量随 patch 变，几何量/ECTRhofield 不随 patch 变
            auto const Etype = (patch == PatchType::fine) ? FieldType::Efield_fp : FieldType::Efield_cp;
            solver->EvolveECTRho (fields.get_alldirs (Etype, lev),
                                  fields.get_alldirs (FieldType::edge_lengths, lev),
                                  fields.get_alldirs (FieldType::face_areas, lev),
                                  fields.get_alldirs (FieldType::ECTRhofield, lev),
                                  lev);
        }
#endif
    } else {
        auto& flags     = PluginAccess::flag_info_face (warpx, lev);
        auto& borrowing = PluginAccess::borrowing (warpx, lev);
        solver->EvolveB (fields, lev, patch, flags, borrowing, dt);
        if (warpx.DoPML () && warpx.GetPML (lev)->ok ()) {
            solver->EvolveBPML (fields, patch, lev, dt, WarpX::do_dive_cleaning);
        }
        warpx.ApplyBfieldBoundary (lev, patch, to_subcycling_half (layer),
                                   state.start_time () + dt);
    }
}
```

> 关键：**函数调用顺序、参数、`if` 条件与旧代码逐字一致**；`EvolveECTRho` 的 `#ifdef AMREX_USE_EB` 也照抄。这是 A/B 能逐位的前提。

---

## 5. 构建接入

### 5.1 CMake

`CMakeLists.txt`（根）：

```cmake
# 放在其他 add_subdirectory 旁边（WarpX_LIB 块内）
add_subdirectory(Source/Plugin)
```

`Source/Plugin/CMakeLists.txt`：

```cmake
foreach(D IN LISTS WarpX_DIMS)
    warpx_set_suffix_dims(SD ${D})
    target_sources(lib_${SD} PRIVATE PluginHost.cpp PluginAccess.cpp)
endforeach()
add_subdirectory(Builtin)
```

`Source/Plugin/Builtin/CMakeLists.txt` 同理加 `YeeFieldEquation.cpp BuiltinPlugins.cpp`。

**外部插件源码**（根 `CMakeLists.txt`，`app_${SD}` 创建之后）：

```cmake
set(WarpX_EXTRA_PLUGIN_SOURCES "" CACHE STRING
    "Additional .cpp files that register plugins (compiled into the app target)")
...
if(WarpX_APP AND WarpX_EXTRA_PLUGIN_SOURCES)
    target_sources(app_${SD} PRIVATE ${WarpX_EXTRA_PLUGIN_SOURCES})
endif()
```

放在 `app_${SD}` 而不是 `lib_${SD}`，因为**可执行文件的 object 不会被链接器丢弃**，静态注册器一定执行；静态库里的注册器对象则可能被 dead-strip（见风险 R1）。

### 5.2 GNU make

`Source/Make.WarpX` 加一行 `include $(WARPX_HOME)/Source/Plugin/Make.package`，`Plugin/Make.package`：

```
CEXE_sources += PluginHost.cpp
CEXE_sources += PluginAccess.cpp
CEXE_sources += Builtin/YeeFieldEquation.cpp
CEXE_sources += Builtin/BuiltinPlugins.cpp
VPATH_LOCATIONS += $(WARPX_HOME)/Source/Plugin
VPATH_LOCATIONS += $(WARPX_HOME)/Source/Plugin/Builtin
```

### 5.3 构建命令（本机已验证依赖可用）

```bash
cmake --fresh -S . -B build \
  -DWarpX_DIMS=2 -DWarpX_EB=ON -DWarpX_MPI=ON -DWarpX_COMPUTE=OMP \
  -DWarpX_PYTHON=OFF -DWarpX_OPENPMD=OFF -DWarpX_FFT=OFF \
  -DCMAKE_BUILD_TYPE=Release \
  -DFETCHCONTENT_SOURCE_DIR_FETCHEDAMREX=/path/to/amrex-src
cmake --build build -j 24
```

- 复用旧工作区的 AMReX 源码，避免联网；`mpicxx`、`cmake`、24 核均可用。
- 预计首次全量构建 25–45 min（与旧 `build_2d` 同配置）。

---

## 6. 外挂插件工作流（验收“用户只写 1 个 .cpp”）

```cpp
// my_field.cpp
#include "Plugin/Plugin.H"

namespace {
class MyField final : public warpx::plugin::IFieldEquation
{
public:
    [[nodiscard]] warpx::plugin::PluginInfo info () const override
    { return {"my_field", "1.0", warpx::plugin::PluginRole::Field}; }

    void advance (warpx::plugin::FieldState& state,
                  const warpx::plugin::FieldSource& source,
                  amrex::Real dt,
                  warpx::plugin::TimeLayer layer) override
    {
        // 用户自己的 kernel：MFIter + FieldView + amrex::ParallelFor
    }
};
}

WARPX_REGISTER_PLUGIN (MyField, "my_field")
```

```bash
cmake -S . -B build -DWarpX_EXTRA_PLUGIN_SOURCES=/abs/path/my_field.cpp ...
cmake --build build -j 24
./build/bin/warpx.2d inputs   # 输入里 algo.field_equation = my_field
```

不需要改内核、不需要改 CMakeLists（只用 cache 变量）、不需要重新编译整个库（只编 app 目标 + 链接）。

---

## 7. 测试与回归

### 7.1 三层测试

| 层 | 内容 | 成本 | 判定 |
|---|---|---|---|
| L1 注册表单测 | `yee` 已注册；`names(Field)` 含 `yee`；未知名字返回 `nullptr`；`get_field_equation("yee")->info()` 字段正确 | < 1 s | ctest 通过 |
| L2 A/B 逐位 | 同一二进制、同一输入、同一 `OMP_NUM_THREADS`，`algo.field_equation = yee` vs `legacy` | 与单次跑相同 ×2 | **逐位**（探针文件 + plotfile 的 `Cell_D_*` payload） |
| L3 物理门 | 现有 `verify_*.py` / `analyze.py` 对解析解 | 5–60 min/例 | 文档容差（见 7.3） |

**L2 是主验收**：它不依赖文档里互相冲突的基线数字，也不受编译器版本影响；只要插件是按 §4.5 逐字搬运，就应该逐位一致。若 L2 不逐位，先查 FP contraction/TU 差异，再退到 `≤1e-12` 数值门并记录原因（`ARCHITECTURE_REVIEW.md` §2.2 的结论）。

### 7.2 A/B 比较器

`benchmarks/cases/plugin/compare_runs.py --a run_legacy --b run_yee`：

1. 递归比较 `diags/reducedfiles/*.txt`：`filecmp.cmp(shallow=False)`；
2. 对每个 plotfile，比较 `Level_*/Cell_D_*` 与 `Level_*/Cell_H_*` 的**字节内容**（Header 里有时间/版本，不参与比较）；
3. 若有差异，打印第一个不同的文件 + 偏移 + 两边的 `max|Δ|`（用 `np.fromfile` 按 float64 读 payload）。

> 为什么不直接 `diff -r`：plotfile 的 `Header` 含 `time`、`WarpX` 版本字符串，可能因构建信息不同而变；payload 才是数值。

### 7.3 数值门（S0 先重录基线，再冻结）

| 门 | 生成/运行 | 分析 | 记录基线 | 备注 |
|---|---|---|---|---|
| **P0** | `python3 benchmarks/cases/c02_0_pec/verify_fresnel_rt.py /tmp/p0 --eps2 4 --ncell 512 --make --run` | 同脚本 stdout | `R_flux 0.0927 / T_flux 0.8906`（`MEDIUM_VALIDATION_GRADIENT.md:484`） | 512²、默认 900 步；**走 MacroscopicEvolveE，只覆盖 B 分派**；数字以 S0 重录为准 |
| **E3** | `benchmarks/cases/c03_medium/tensor_interface/inputs_2d`（480²，8000 步）+ `analyze.py` | `python3 analyze.py diags/reducedfiles` | node τ：`R −0.86% / T +0.53%`（`MEDIUM_VALIDATION_GRADIENT.md:128`）；旧版本另有 `+2.82%/+0.78%`，**以 S0 重录为准** | 张量 ε 界面 |
| **S5** | `benchmarks/cases/c03_medium/lossy_slab/inputs_2d`（640²，5000 步）+ `analyze.py` | 同上 | `0.619830 / −0.873% / phase −2.139°`（640²，`MEDIUM_MULTIGRID_ROOTCAUSE_AND_FIX.md:1166`）；1280² 为 `0.623942 / −0.215%` | 有损均匀介质 |
| **E6** | `benchmarks/cases/c03_medium/conductor_slab/inputs_2d`（2500 步）+ `analyze.py` | 同上 | `0.100042`（`ARCHITECTURE_REVIEW.md:68`） | 良导体板 |
| **L4** | 体张量 σ（旋转），即介质梯度表的 L4 | 仓库内**未找到对应 `inputs_2d`**；最接近的 `c02_0_pec/p1_tensor_test.cpp` 只测 `chi^-1` 矩阵，不是场推进 | `1.04e-13 vs expm`（`MEDIUM_MULTIGRID_ROOTCAUSE_AND_FIX.md:1168`） | 若必须按设计稿的门执行，先补算例（0.5–1 d）；P1 不阻塞，理由见 D7 |
| **L5** | `benchmarks/cases/c03_medium/tensor_interface/runL/inputs` + `runLref` | `analyze.py`（是否支持 `sigma=0.01` 需人工确认） | R/T ≤5% | runL = 张量界面 + `sigma=0.01`，已入库；判据需先核对 |
| **L6** | `rho_medium` 残差 | `verify_divergence.py` **未在仓库中找到** | 4e-25（内部）/ 7e-13（边界） | 缺工具则降级为 P2 门 |
| **上游** | `ctest --test-dir build -R "test_2d_langmuir_multi"` | ctest | 忽略 checksum | 覆盖真空 Yee 的 E+B |

**门覆盖矩阵（必须写进验收报告）：**

| 算例 | 场方程路径 | `advance(Full)`（E） | `advance(First/Second)`（B） |
|---|---|---|---|
| P0/E3/S5/E6 | `MacroscopicEvolveE` + `EvolveB` | ❌（不经插件） | ✅ |
| L5（runL） | 同上 | ❌ | ✅ |
| `c02_0_pec/inputs_2d`（ECT） | `EvolveE` + `EvolveB` | ✅ | ✅ |
| `c02_0_pec/inputs_2d_cyl_te`（圆柱，**HANDOFF 的 L4**，ECT） | `EvolveE` + `EvolveB` | ✅ | ✅ |
| 上游 `langmuir` | `EvolveE` + `EvolveB` | ✅ | ✅ |

**E 分派的 A/B 必须至少跑一个真空/ECT 算例**，否则等于没测。

> 编号歧义提醒：`HANDOFF.md` 的 **L4 = PEC 圆柱**（ECT，真空），`MEDIUM_VALIDATION_GRADIENT.md` 的 **L4 = 体张量 σ**（介质）。设计稿 §6.4 的 P0/E3/S5/E6/L4/L5/L6 沿用的是**介质梯度表**编号。两者不是一回事；本方案在覆盖矩阵里分别标注为“HANDOFF-L4（圆柱）”和“介质-L4（体张量 σ）”。

### 7.4 跨二进制数值门（旧 `WarpX_fluid/build_2d/bin/warpx.2d`）

- P0 `/tmp/p0`：旧二进制的新跑结果 vs 新二进制的 `legacy` 结果，`R/T` 相对差 ≤1e-12；σ=0 的探针文件期望逐位。
- 其余算例同理；如果文档基线与本次重录差超过 1e-3，以**本次重录**为准并把差异写进实现日志（文档里的数字跨了多次网格/配置修改，不能直接当判据）。

### 7.5 失败判读

| 现象 | 首查 |
|---|---|
| A/B 不逐位，但差 ~1e-16 | TU 内联/FP contraction；确认 solver 函数不在调用方 TU 内联 |
| A/B 差 ~1e-12 | 调用顺序或参数（尤其 `new_time`、`subcycling_half`） |
| 只有 ECT 算例差 | `EvolveECTRho` 的 `#ifdef` 或调用时机（必须在 `ApplyEfieldBoundary` 之后） |
| 只有介质算例差 | 介质走的是 `MacroscopicEvolveE`，本来就不该被插件影响；检查是否误改了 `Evolve()` 分支 |
| A/B 完全不等于“都没跑” | `algo.field_equation` 未生效（ParmParse 前缀/默认值） |

---

## 8. 性能与 GPU

- **主机侧**：每个 level、每个半步一次虚调用；`FieldState` 是 3 个标量 + 1 个引用，构造无分配。相对现有 `EvolveB` 的耗时（一次 3D 全网格 curl）可忽略。
- **kernel 内**：零虚函数、零 `std::function`；`FieldView` 是值类型，直接作为 `ParallelFor` 捕获。
- **GPU**：插件在主机侧调用 `EvolveE/B`，后者照旧启动 GPU kernel；`Array4` 的获取方式不变。P1 不引入任何设备端间接层。
- **RZ/1D/3D**：接口与维度无关；`EvolveE/B` 内部已有 `#ifdef`，插件逐字搬运即可。
- **回归性能**：A/B 使每个门的时间 ×2；P0 约 1–2 min、E3 约 5 min、S5 约 20–60 min、E6 约 5–10 min。

---

## 9. 风险与决策点

| # | 决策/风险 | 建议 | 理由 |
|---|---|---|---|
| D1 | `FieldState` 是否只给 `Array4` | **给借用的 `MultiFab*` 数组 + `FieldView`** | 纯 Array4 无法让插件遍历 level，也无法让官方 Yee 复用 `FiniteDifferenceSolver` |
| D2 | 命名空间/宏名 | `warpx::plugin` + `WARPX_REGISTER_PLUGIN`，保留 `MOMENTS_REGISTER_PLUGIN` 别名 | 仓库无 `moments::`；别名兼容设计稿示例 |
| D3 | `TimeLayer` 语义 | 按 §2.3 表；`Full` = E | PSATD 在 P2 复用同一签名 |
| D4 | `advance` 是否含边界 | **含**（PML + PEC/PMC/periodic + ECT rho） | 保证与旧路径同序、A/B 逐位；`boundary()` 留空到 P2 |
| D5 | 内置插件访问 private 成员 | `PluginAccess` friend | 避免 5 个 public getter 扩大内核 API；P2 可删除 |
| D6 | 输入名 | `algo.field_equation`，默认 `yee`，`legacy` 旁路 | 与 `algo.maxwell_solver` 同节；`field.equation` 作为别名可后加 |
| D7 | P1 是否覆盖 `MacroscopicEvolveE` | **不覆盖**，留 P2；E 分派用真空/ECT 门覆盖 | 介质 E 更新含 `rho_medium`/`jcond` 等材料物理，应归 `IMaterialModel`，硬塞进 Yee 插件会把“场方程”和“材料”再次耦合 |
| D8 | 是否需要 CMake 开关模拟“空表” | **不需要**；`algo.field_equation = legacy` 等价 | 同一二进制 A/B 比“关插件重编”更强、更省时 |
| R1 | 静态注册被链接器丢弃 | 内置插件用 `register_builtin_plugins()` 显式注册；外部插件编进 `app_${SD}` | 静态库里的 registrar 对象可能 dead-strip |
| R2 | 静态初始化顺序 | `PluginHost::instance()` 用函数内 static | 规避 fiasco |
| R3 | 多 WarpX 实例/测试 | 插件无状态；`host()` 进程级 | 现有 `WarpX::ResetInstance()` 不影响插件表 |
| R4 | PSATD 用户看到 `algo.field_equation=yee` 但无效 | 启动日志打印“field equation plugin inactive for maxwell_solver=psatd” | 不 abort，保持向后兼容 |
| R5 | 介质门“看起来过了”但没测 E | §7.3 覆盖矩阵写进验收报告 | 避免假阳性 |

---

## 10. 分步实施（建议提交粒度）

| 步 | 内容 | 产出/验证 | 估时 |
|---|---|---|---|
| **S0** | 新工作区建 `build/`；用旧二进制重录 P0/E3/S5/E6 基线；确认 L4/L5/L6 输入是否存在 | 基线表 + `build/bin/warpx.2d` | 0.5–1 d（含构建） |
| **S1** | `PluginInfo/Plugin/PluginHost/PluginRegistrar/FieldState/FieldSource/FieldView` + CMake/Make，不接内核 | 能编译；`plugin_registry` 单测通过 | 0.5–1 d |
| **S2** | `PluginAccess` + `YeeFieldEquation` + `BuiltinPlugins`，不接内核 | 编译；单测能看到 `yee` | 1–2 d |
| **S3** | `EvolveB/E` 分派 + `algo.field_equation` 解析 + 启动日志 | A/B 逐位（真空 ECT + 介质） | 0.5 d |
| **S4** | 跑 §7.3 门；写 `PLUGIN_P1_IMPLEMENTATION_LOG.md` | 覆盖矩阵 + 数字 | 1–2 d |
| **S5** | 外挂插件示例（`Docs` 或 `benchmarks/cases/plugin/my_field.cpp`）+ 使用文档 | 用户按 §6 能编过 | 0.5 d |
| **S6** | 清理、提交（每个 S 一个 commit） | `git log` 可回溯 | 0.5 d |

**提交信息建议**（沿用仓库风格）：

```
feat(plugin): PluginHost + 注册宏 + FieldState/FieldSource 骨架
feat(plugin): 官方 Yee 插件（包 EvolveB/EvolveE，含 PML/边界/ECT）
feat(plugin): 内核调用点 + algo.field_equation（默认 yee，legacy 旁路）
test(plugin): 注册表单测 + A/B 逐位比较器
docs(plugin): P1 实施记录与门覆盖矩阵
```

---

## 11. 待确认问题（进入实现前）

1. **命名**：`warpx::plugin` / `WARPX_REGISTER_PLUGIN` 是否接受？还是严格按设计稿用 `moments::` / `MOMENTS_REGISTER_PLUGIN`？
2. **D7**：P1 是否真的不覆盖 `MacroscopicEvolveE`？若要求 P0/E3 也走插件 E 分派，需要额外 2–3 天把介质 E 更新也做成适配器（并决定 `rho_medium` 归谁）。
3. **L4/L6**：这两个门当前是否有可运行的输入/分析脚本？若没有，是否接受“P1 用真空/ECT + 上游 ctest 覆盖 E，L4/L6 留到 P2”？
4. **外部插件的构建方式**：`-DWarpX_EXTRA_PLUGIN_SOURCES=` 是否够用，还是需要 `warpx_add_plugin()` 这样的 CMake 函数（支持多个文件/子目录/宏定义）？
5. **`FieldState` 的借用语义**：是否接受“`MultiFab*` 只在本节拍有效”的约定，还是坚持设计稿的纯 `Array4`（那样官方 Yee 插件只能整段回调 `WarpX`，接口会名存实亡）？

---

## 12. 一句话

**P1 = 一个 `PluginHost` + 一个 `IFieldEquation` + 一个把 `EvolveB/EvolveE` 函数体逐字搬进去的 `YeeFieldEquation`；内核只在两个 patch 重载的入口加一次虚调用；验收靠同一二进制的 `yee` vs `legacy` 逐位 A/B，E 分派必须用真空/ECT 算例单独证明——介质算例只能证明 B 分派。**
