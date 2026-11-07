# 三个相互啮合任务的技术路径

> 任务：
>
> 1. **独立化**：WarpX 源码完整进入项目，成为项目自带的 Core/Plugin，而不是外部原版；
> 2. **模块化**：按物理/逻辑分模块，一个单位子模块 = 一个功能 = 一个 CPP，一个完整模块 = 一个文件夹；
> 3. **插件化**：项目内一切要么是核心，要么是功能插件；插件可加/可换/可禁用。
>
> 维度要求：1D、2D、3D、RZ 四个维度都要覆盖，不能只验证 2D。

---

## 1. 目标定义

### 1.1 独立化

```text
Source/Core/       稳定内核
Source/Plugins/    功能插件
```

- 不链接外部 WarpX；
- 不依赖系统 WarpX 安装；
- 原 WarpX 源码复制/重组进本仓库；
- 删除某个插件后，内核仍能编译，其余插件仍能运行。

### 1.2 模块化

```text
Source/Plugins/PicEm/<Module>/
  <Module>Package.H/.cpp        // 只注册
  <Unit1>/<Unit1>.H/.cpp        // 一个单位 = 一个物理/逻辑功能
  <Unit2>/<Unit2>.H/.cpp
```

硬性判据：

- 一个单位模块一个 CPP；
- 单位模块只依赖 `StageContext`；
- 不持有其他模块指针；
- 读写字段/物种必须声明；
- 可禁用/替换；
- 参数属于模块命名空间。

### 1.3 插件化

- 内置实现也以插件方式注册；
- 外部 `.so` 通过 `WarpXPluginDesc` 注册；
- 内置模块可被 `WARPX_DISABLE_BUILTIN_MODULES` 禁用；
- 同 Stage 同字段双写必须被拒绝；
- 插件不互认，只通过 DataBus。

---

## 2. 当前基线（2026-11-07）

### 2.1 已完成

```text
Source/Core/
  Runtime/
  Utils/
  ablastr/
  main.cpp
Source/Plugins/PicEm/
  WarpX.H/.cpp
  Evolve / FieldSolver / Particles / Diagnostics / ...
  Modules/PicEm/...
```

- `WarpX.*` 与全部物理目录已移入 `Plugins/PicEm`；
- 没有外部 WarpX 依赖；
- 2D CPU：默认 76/87，HDF5 构建 159/159；
- 2D FFT / ASAN 通过；
- 1D/3D/RZ 统一构建（EB/QED ON，FFT OFF）已通过；
- 1D/3D/RZ 非 openPMD smoke 8/8 通过；
- L1 模块命名/选择/禁用已实现。

### 2.2 仍然存在

```cpp
// Source/Core/Runtime/Runtime.cpp
#include "WarpX.H"
m_warpx = WarpX::Create();
```

```text
Core/Runtime -> Plugins/PicEm/WarpX
```

即：

- 删除 `Plugins/PicEm/WarpX.H` 后 Runtime 不可编译；
- `WarpX` 仍是 AMReX 生命周期/物理容器的实际持有者；
- `particle_push` 仍是“推进 + 沉积”组合；
- `field_solve.*` 仍调用 `WarpX` 内部统一分派；
- 外部 `.so` 不能替换 `IAmrPolicy`；
- 1D/3D/RZ 的 FFT/openPMD 组合未验证。

---

## 3. 三个任务的耦合关系

```text
独立化
  └─ 需要 Core 不依赖 Plugins/PicEm/WarpX
       └─ 需要先有 IKernel / 模块接口
            └─ 需要模块化边界

模块化
  └─ 需要每个单位模块只依赖 StageContext
       └─ 需要 Core 提供 HostServices/DataBus/Registry
            └─ 需要插件化注册/禁用/替换

插件化
  └─ 需要内置模块与外部模块走同一注册路径
       └─ 需要模块化契约
            └─ 需要独立化后的稳定 ABI
```

不能单独做其中一个；顺序必须是：

```text
4D 构建稳定
  -> Core/Plugin 依赖倒置
  -> L1 单位模块拆分
  -> Kernel 替换
  -> 删除 WarpX
  -> 外部插件 ABI
```

---

## 4. 阶段计划

### P0：四维构建与测试基线

目标：

```text
1D / 2D / 3D / RZ × {CPU OMP}
```

验收：

- 四个维度都能编译 app + tests；
- 每个维度至少 3 个官方算例 run/analysis 通过；
- 2D openPMD+HDF5 159/159 保持；
- 记录 FFT/openPMD/QED/EB 组合矩阵。

已发现并修复：

- 3D `IntVectND<3>` 只给 2 分量；
- `WarpXRuntimeData` EB 头文件未守卫；
- `MomentsConfig.H` include 路径；
- Core/Utils 移动后 binary include path。

仍需：

- 1D/3D/RZ + FFT：需要 `blaspp`；
- 1D/3D/RZ + openPMD/HDF5：需要对应 HDF5 构建；
- RZ 轴、3D EB、3D QED 全量测试。

### P1：Core/Plugin 依赖倒置

目标：

```text
Core/Runtime -> IPicKernel
                  ^
                  |
Plugins/PicEm/WarpXKernel
```

`IPicKernel` 只暴露：

```cpp
initialize()
evolve(int)
finalize()
time()
fields()
particles()
data()
```

验收：

- `Core/Runtime` 不再 `#include "WarpX.H"`；
- `WarpXKernel` 适配器承担所有 WarpX 类型；
- 2D/1D/3D/RZ 回归通过。

### P2：L1 单位模块拆分

优先顺序：

```text
1. particle_push -> pusher.<algo> + deposition.current
2. field_solve.<algo> -> 独立实现
3. diagnostics -> Full / BTD / Reduced
4. collisions -> 每个模型一个 handler
5. boundaries -> PML / PEC / Silver-Müller
```

验收：

- 每个单位模块一个 CPP；
- 每个模块有 `Package` 注册；
- 删除任一模块后其余可编译；
- A/B 逐位。

### P3：Kernel 替换

目标：

```text
Core/Kernel
  -> TimeStep
  -> LevelLifecycle
  -> ParticleContainer
```

先替换生命周期，再替换物理模块。

验收：

- 无 WarpX 的 `Evolve` 可用；
- 同一算例与旧路径逐位；
- 1D/2D/3D/RZ 全部通过。

### P4：删除 WarpX

条件：

```text
Core 不 include Plugins/PicEm/WarpX.H
所有官方算例 A/B 逐位
四维构建通过
```

在此之前不能声称“删除原版后同结果”。

### P5：外部插件 ABI

- 共享运行时或纯 C 数据接口；
- 允许外部 `.so` 提供 `IAmrPolicy`；
- ABI 版本 + struct size；
- 失败路径测试。

---

## 5. 四维测试矩阵

| 组合 | 1D | 2D | 3D | RZ |
|---|---|---|---|---|
| CPU OMP | 待扩展 | 已通过 | 待扩展 | 待扩展 |
| + FFT | 待 | 已通过 | 需 blaspp | 需 blaspp |
| + openPMD/HDF5 | 待 | 159/159 | 待 | 待 |
| + EB | 待 | 已通过 | 待 | 待 |
| + QED | 待 | 已通过 | 待 | 待 |
| + Python/PICMI | 待 | 未构建 | 待 | 待 |
| GPU | 待 | 未构建 | 待 | 待 |

每个组合至少：

```text
langmuir / collision / embedded_boundary / ionization
```

---

## 6. 模块目录模板

```text
Source/Plugins/PicEm/FieldSolver/Yee/
  YeePackage.H/.cpp
  UpdateE/UpdateE.H/.cpp
  UpdateB/UpdateB.H/.cpp
  Stencil/Stencil.H/.cpp
  Boundary/Boundary.H/.cpp
```

```cpp
// YeePackage.cpp
void YeePackage::register_handlers(PluginHost& host)
{
    host.add(std::make_shared<UpdateEHandler>());
    host.add(std::make_shared<UpdateBHandler>());
}
```

用户新增：

```text
MySolver/
  MySolverPackage.cpp
  MyUpdateE.cpp
  MyUpdateB.cpp
```

只写 2–3 个 CPP + 注册，不改 Core。

---

## 7. 注意事项

1. **不要把目录移动当成独立化**；
2. **不要让 Core include 具体 Plugin**；
3. **一个 CPP 只做一个单位功能**；
4. **模块名是 API**；
5. **A/B 是硬门**；
6. **四维必须同时验证**；
7. **GPU 不是“有空再做”**，但应在 CPU 四维稳定后再做；
8. **openPMD/HDF5 是测试依赖，不是物理模块**；
9. **插件失败不能 abort 整个进程**；
10. **禁止用 `write_species=0` 绕过粒子输出缺陷**。

---

## 8. 风险与工作量

| 阶段 | 风险 | 估算 |
|---|---|---|
| P0 四维基线 | 3D/RZ 隐藏分支多 | 1–2 周 |
| P1 依赖倒置 | Runtime 接口宽 | 2–4 周 |
| P2 L1 拆分 | 物理 A/B 回归 | 2–3 月 |
| P3 Kernel 替换 | 生命周期复杂 | 3–6 月 |
| P4 删除 WarpX | 全功能等价 | 6–12 月 |
| P5 外部 ABI | 共享运行时 | 2–4 周 |

这不是一个迭代能完成的；必须按门禁推进。

---

## 9. 发现的问题记录

| 问题 | 状态 |
|---|---|
| Runtime 直接 `WarpX::Create()` | 未修复，P1 |
| 3D `IntVectND<3>` 2 分量 | 已修复 |
| `WarpXRuntimeData` EB 头未守卫 | 已修复 |
| `MomentsConfig.H` include 路径 | 已修复 |
| Core/Utils 移动后 binary include | 已修复 |
| `particle_push` 组合功能 | 未修复，P2 |
| `field_solve.*` 内部分派 | 未修复，P2 |
| 外部 `IAmrPolicy` ABI 静态链接 segfault | 未支持，P5 |
| 1D/3D/RZ + FFT 缺 blaspp | 环境/构建 |
| openPMD 默认构建未启用 | 已用 HDF5 构建解决 2D |
| PML 重复创建 | 已修复 |
| 硬编码 species | 已修复 |

---

## 10. 完成定义

三个目标同时满足才算完成：

```text
1. 删除 Source/Plugins/PicEm/WarpX.H 后项目仍可构建运行；
2. 每个物理功能是一个文件夹 + 一个单位 CPP；
3. 所有内置与外部模块走同一注册/禁用/替换路径；
4. 1D/2D/3D/RZ 官方算例 A/B 通过；
5. 输入输出与原版兼容。
```

当前状态：**未完成**，但源码归属和四维构建基线已经推进。
