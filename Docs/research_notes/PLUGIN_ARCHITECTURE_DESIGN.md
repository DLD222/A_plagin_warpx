# 插件化架构设计：稳定内核 + 官方内置插件 + 外挂模块（建议稿）

> 状态：历史设计稿，已被 `PLUGIN_ARCHITECTURE_FINAL.md`（源码核对版）取代。本文保留作为讨论记录，不再作为实现依据。

> 工作区：`/home/douliangduo/WarpX_plugin`（`WarpX_fluid` 源码拷贝，HEAD `0de0e26`）
> 本文只写设计（按用户要求“现在只是写 .md”），不改代码；后续实现以此为准绳讨论。
> 用户确认的要点：
> 1. **原有内置功能全部下沉为「官方内置插件」**；内核只依赖抽象接口。
> 2. 第一个真实模块用**官方 Yee 求解器**验证。
> 3. **编译期静态注册**；**C++ 抽象类**；**每步一次虚调用 + FieldView/Array4 kernel**。

---

## 0. 定位

- 本文是**设计建议**，不是实现承诺；所有“建议/可选/待定”由负责人决定。
- 目标：内核（Kernel）稳定、只依赖抽象接口；所有物理能力（含原有 Yee/PSATD/PIC/冷流体/介质/PEC/PML/诊断）都以**插件**形式存在；用户新能力 = 外挂插件（1 个 `.cpp` + 注册）。
- 工作区：`WarpX_plugin/`（`WarpX_fluid` 的源码拷贝；本文在 `Docs/research_notes/`）。

---

## 1. 原则

```
Kernel 只认接口，不认具体实现
  ├─ 场方程（IFieldEquation）
  ├─ 物种（ISpeciesModel）
  ├─ 材料（IMaterialModel）
  ├─ 相互作用（IInteraction）
  ├─ 边界（IBoundary）
  ├─ 源项（ISource）
  └─ 诊断（IDiagnostic）

官方内置插件：把现有实现包成上述接口，注册成 "yee"/"psatd"/"pic"/"fluid_cold"/"medium_tensor"/...
外挂插件：用户实现同一接口，静态注册，输入里选名字
```

- **不删旧逻辑**：旧实现保留，作为官方插件的实现体；
- **内核调用点**：每个角色一个 no-op 默认调用；插件表为空 → 旧路径逐位/数值不变；
- **编译期静态注册**：`MOMENTS_REGISTER_PLUGIN(Type, "name")` 静态构造器注册到工厂表；
- **C++ 抽象类**：无 C ABI；运行期 dlopen 作为后续可选；
- **每步一次虚调用**：主机侧每步一次 `host.get(...)->advance(...)`；kernel 内用 `FieldView{Array4,Box,ncomp}` 裸视图，无 `std::function`、无每 cell 虚调用。

---

## 2. 三层结构

```
┌────────────────────────── Kernel（稳定内核）──────────────────────────┐
│ 状态（Fields/Particles/Fluids/Material）· 时间循环 · 诊断 · AMR · EB · IO │
│ + PluginHost（注册/查找/分发）                                          │
│ + 调用点：init / field advance / species advance / material / boundary / │
│           source / interaction / diagnostic                            │
└───────────────▲────────────────────────────────────────────────────────┘
                │ 标准插头（固定接口 + PluginInfo）
┌───────────────┴────────────────────────────────────────────────────────┐
│ 官方内置插件（把现有实现包成插件）                                       │
│  yee · psatd · ckc · electrostatic · pic · fluid_cold · moment5 ·         │
│  moment10 · medium_tensor · pec · pml · eb · laser · full_diag · em · …   │
└───────────────▲────────────────────────────────────────────────────────┘
                │ 同一接口
┌───────────────┴────────────────────────────────────────────────────────┐
│ 外挂插件（用户只写这里）                                                 │
│  my_field · my_species · my_material · my_boundary · my_source · …       │
└────────────────────────────────────────────────────────────────────────┘
```

---

## 3. 接口（插头）

```cpp
struct PluginInfo {
    const char* name;      // "yee"
    const char* version;   // "1.0"
    const char* role;      // "field" / "species" / "material" / ...
};

class IPlugin {
public:
    virtual PluginInfo info() const = 0;
    virtual ~IPlugin() = default;
};

// 场方程：官方 Yee 是第一个实现
class IFieldEquation : public IPlugin {
public:
    virtual void advance(FieldState&, const FieldSource&, Real dt, TimeLayer) = 0;
    virtual void boundary(FieldState&) = 0;
};

// 物种：PIC / 冷流体 / 5 矩 / 10 矩
class ISpeciesModel : public IPlugin {
public:
    virtual void advance(const FieldView&, Real dt) = 0;
    virtual void deposit(FieldSource&) = 0;
};

// 材料：物性 / 状态演化 / 源响应
class IMaterialModel : public IPlugin {
public:
    virtual void properties(Level&, const MaterialState&) = 0;
    virtual void advance(MaterialState&, const FieldView&, Real dt) = 0;
    virtual void respond(const FieldView&, FieldSource&, Real dt) = 0;
};

// 相互作用 / 边界 / 源 / 诊断
class IInteraction : public IPlugin {
public:
    virtual void deposit(const PlasmaState&, FieldSource&) = 0;
    virtual void gather(PlasmaState&, const FieldState&, Real dt) = 0;
    virtual FieldEquationId field_equation() const = 0;
};
class IBoundary   : public IPlugin { public: virtual void apply(FieldState&) = 0; };
class ISource     : public IPlugin { public: virtual void apply(State&, Real dt) = 0; };
class IDiagnostic : public IPlugin { public: virtual void evaluate(const State&) = 0; };
```

视图/时间层：

```cpp
struct FieldView { amrex::Array4<amrex::Real> a; amrex::Box bx; int ncomp; };
enum class TimeLayer { Full, FirstHalf, SecondHalf };
```

注册/宿主：

```cpp
PluginHost& host();                         // 全局注册表
host().add(std::make_unique<YeeFieldEquation>());
auto* eq = host().get_field_equation("yee"); // 输入选择
#define MOMENTS_REGISTER_PLUGIN(T, NAME) \
    static ::moments::PluginRegistrar<T> reg_##T(NAME);
```

---

## 4. 内核

```
状态：Fields/Particles/Fluids/Material（沿用现有 MultiFabRegister/容器）
时间循环：沿用现有 Evolve（只把“直接调用”换成“接口调用”）
PluginHost：注册表 + 输入 plugins= + 默认插件选择
调用点（每个角色一个，默认 no-op）：
  init：加载/注册
  field：IFieldEquation::advance(..., TimeLayer)
  species：ISpeciesModel::advance/deposit
  material：IMaterialModel::properties/advance/respond
  boundary：IBoundary::apply
  source：ISource::apply
  interaction：IInteraction::deposit/gather
  diagnostic：IDiagnostic::evaluate
兼容：插件表为空 → 旧路径逐位/数值不变
```

---

## 5. 官方内置插件（原功能下沉）

| 现有实现 | 接口 | 官方插件名 |
|---|---|---|
| `FiniteDifferenceSolver`（Yee） | `IFieldEquation` | `yee` |
| `SpectralSolver`（PSATD） | `IFieldEquation` | `psatd` |
| CKC / 静电 | `IFieldEquation` | `ckc` / `electrostatic` |
| `PhysicalParticleContainer`（PIC） | `ISpeciesModel` | `pic` |
| `WarpXFluidContainer`（冷流体） | `ISpeciesModel` | `fluid_cold` |
| `MomentsSolver`（5 矩 / 10 矩） | `ISpeciesModel` | `moment5` / `moment10` |
| `MacroscopicProperties`（ε/σ/μ、ρ_medium） | `IMaterialModel` | `medium_tensor` |
| PEC / PML / EB | `IBoundary` | `pec` / `pml` / `eb` |
| 激光 / 外部源 | `ISource` | `laser` / … |
| `MultiDiagnostics` | `IDiagnostic` | `full` / `bt` / … |
| 电磁 / 摩擦 / 引力 | `IInteraction` | `em` / `friction` / `gravity` |

**“下沉”的含义**：旧实现体保留，外面包一层适配器实现接口；内核的“直接调用”改成“接口调用”；旧行为不变（官方插件就是原实现）。这是一次**大重构**，按阶段推进（§8）。

---

## 6. 第一个里程碑：骨架 + 官方 Yee 插件

### 6.1 新增文件（只加，不改旧逻辑）

```
Source/Plugin/
├─ Plugin.H                 // IPlugin/PluginInfo/FieldView/TimeLayer
├─ PluginHost.{H,cpp}       // 注册表 + 输入 + 查找
├─ PluginRegistrar.H        // MOMENTS_REGISTER_PLUGIN
├─ FieldState.H             // E/B 视图
├─ FieldSource.H            // J/rho 视图
└─ Builtin/YeeFieldEquation.{H,cpp}   // 官方 Yee 插件（包现有 FiniteDifferenceSolver）
```

### 6.2 Yee 插件

```cpp
class YeeFieldEquation : public IFieldEquation {
public:
    PluginInfo info() const override { return {"yee", "1.0", "field"}; }
    void advance(FieldState& fs, const FieldSource& src, Real dt, TimeLayer layer) override {
        // 调现有 FiniteDifferenceSolver / WarpX::EvolveB/EvolveE（适配器）
        // layer ∈ {Full, FirstHalf, SecondHalf}
    }
    void boundary(FieldState& fs) override { /* 现有 PML/PEC/EB */ }
};
MOMENTS_REGISTER_PLUGIN(YeeFieldEquation, "yee")
```

### 6.3 内核调用点

```
WarpX::EvolveB/EvolveE：
  auto* eq = host().get_field_equation(field_equation_name); // 默认 "yee"
  eq->advance(fs, src, dt, layer);
插件表为空 / 名字缺省 → 旧 "yee" 官方插件 → 与旧路径逐位/数值一致
```

### 6.4 验收（数值门）

```
- 插件表为空：P0/E3/S5/E6/L4/L5/L6 逐位/数值不变
- 选 "yee" 官方插件：与旧路径逐位/数值一致
- 输入：field.equation = yee（默认）
- 单测：Yee 插件的 advance(Full/FirstHalf/SecondHalf) 与旧 EvolveB/E 同结果
```

---

## 7. 外挂模块（用户）

```cpp
// my_field.cpp —— 用户只写这一个文件
#include "Plugin/Plugin.H"

class MyField : public moments::IFieldEquation {
public:
    PluginInfo info() const override { return {"my_field", "1.0", "field"}; }
    void advance(FieldState& fs, const FieldSource& src, Real dt, TimeLayer tl) override { /* ... */ }
    void boundary(FieldState& fs) override { /* ... */ }
};
MOMENTS_REGISTER_PLUGIN(MyField, "my_field")
```

```
输入：field.equation = my_field
不改内核；编译期静态注册；可选单测（mock FieldState/FieldSource）
```

---

## 8. 迁移阶段

```
P1：Plugin 骨架 + 官方 Yee 插件 + 内核调用点（本里程碑）
P2：官方 PSATD/CKC/静电 → IFieldEquation；PIC/冷流体/5 矩/10 矩 → ISpeciesModel；
    MacroscopicProperties → IMaterialModel；PEC/PML/EB → IBoundary；诊断 → IDiagnostic
P3：内核的“直接调用”全部换成接口调用；旧实现 = 官方插件；旧行为不变
P4：清理；外挂模块（材料/边界/源/相互作用）验证
```

---

## 9. 性能

```
- 每步一次虚调用（主机侧）；kernel 内 FieldView{Array4,Box,ncomp} 裸视图
- 无 std::function；无每 cell 虚调用
- GPU：FieldView/Array4 直接可用；运行期 dlopen 时才需要类型擦除 launcher（后续）
- TimeLayer 保留 E/B 分开推进 + 半格语义
```

---

## 10. 风险

```
1. “官方内置插件下沉”是大重构：内核直接调用点很多（Evolve/场/粒子/流体/介质/边界/诊断）
2. 旧行为必须逐位/数值不变 → 每个官方插件都要有回归门
3. 抽象层性能 → 每步一次虚调用；kernel 内裸视图
4. 编译期静态注册的裁剪 → --whole-archive / 显式引用
5. 官方插件与内核的耦合（FieldState/FieldSource 的视图边界）
6. 测试：官方插件契约测试 + 现有回归
```

---

## 11. 决策点（建议值）

| # | 决策 | 建议 |
|---|---|---|
| 1 | 加载 | 编译期静态注册 |
| 2 | ABI | C++ 抽象类 |
| 3 | 内核调用点 | 每个角色一个 no-op 默认；默认插件 "yee" |
| 4 | 官方插件下沉范围 | 先场方程（Yee），再物种/材料/边界/源/诊断 |
| 5 | 视图边界 | FieldView/FieldSource 只读 Array4；不暴露 MultiFab 语义 |
| 6 | 回归 | 插件表为空 + 官方插件两条路径都要过 P0/E3/S5/E6/L4/L5/L6 |
| 7 | 运行期加载 | 后续可选（需类型擦除 launcher） |

---

## 12. 一句话

**内核只认抽象接口；原有 Yee/PSATD/PIC/冷流体/5 矩/介质/PEC/PML/诊断全部下沉为“官方内置插件”（旧实现保留、外包适配器）；用户新能力是同一接口的外挂插件（1 个 .cpp + 编译期静态注册）；第一个里程碑只做 Plugin 骨架 + 官方 Yee 插件 + 内核调用点，以 P0/E3/S5/E6/L4/L5/L6 为数值门，保证插件表为空和官方 Yee 两条路径都与旧行为一致。**
