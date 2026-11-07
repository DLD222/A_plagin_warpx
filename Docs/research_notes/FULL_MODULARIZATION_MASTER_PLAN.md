# 全量模块化 / 插件化 / 独立化总计划

## 0. 五个要求

1. 用户只写少量 CPP 插件即可接入新物理；
2. 一个单位子模块 = 一个物理/逻辑功能 = 一个 CPP；
3. 一个完整模块 = 一个文件夹，包含多个单位子模块；
4. 按物理/逻辑功能分模块；
5. 原 WarpX 源码完整纳入项目，项目独立于 WarpX，功能等价。

## 1. 当前盘点

原 WarpX 源码已移动到：

```text
Source/Plugins/PicEm/Modules/
```

盘点结果：

```text
241 个 .cpp
Diagnostics     62
FieldSolver     60
Particles       44
MomentsSolver   13
Initialization  12
Python           9
BoundaryConditions 7
PicEm            6
AcceleratorLattice 6
Parallelization  4
Fluids           4
EmbeddedBoundary 4
Laser            3
Filter           3
NonlinearSolvers 2
Evolve           2
```

逐文件清单：

- [MODULE_INVENTORY.md](MODULE_INVENTORY.md)
- 生成器：`Tools/Modularization/inventory.py`

## 2. 当前完成的“形式”

```text
Source/Plugins/PicEm/
  WarpX.H/.cpp
  Fields.H
  Modules/
    <Domain>/...
    PicEm/
      FieldSolve/Yee/
        YeePackage.H/.cpp
        YeeFieldSolveHandler.H/.cpp
```

`Modules/PicEm/FieldSolve/Yee/` 是第一个“完整模块文件夹 + 单位 CPP”的骨架。

## 3. 当前没有完成的“实质”

- 241 个 CPP 中只有 6 个在 `Modules/PicEm` 下按单位拆分；
- `FieldSolver` 60 个 CPP 仍混合 Yee/PSATD/CKC/ECT/Hybrid/静电/静磁；
- `Particles` 44 个 CPP 仍混合推进/沉积/碰撞/QED/注入；
- `Diagnostics` 62 个 CPP 仍混合 Full/BTD/Reduced/OpenPMD；
- Yee 的单位 handler 仍调用 `WarpX::fieldSolve`，没有自己的 UpdateE/UpdateB；
- Core/Runtime 仍 `#include "WarpX.H"`。

所以：

```text
目录已模块化 ≠ 实现已单位化
源码已收纳 ≠ 项目已独立
```

## 4. 全量单位化方法

### 4.1 语义分类

对每个 `.cpp` 判断：

```text
一个公开能力？
多个能力？
是否只是 helper？
```

- 一个能力 → 直接成为单位；
- 多能力 → 拆成多个 `.cpp`；
- helper → 归入所属单位，不单独注册。

### 4.2 目标结构

```text
Modules/<Domain>/<Module>/
  <Module>Package.H/.cpp
  Units/<Unit>/<Unit>.H/.cpp
```

例：

```text
Modules/FieldSolver/Yee/
  YeePackage.H/.cpp
  Units/UpdateE/UpdateE.H/.cpp
  Units/UpdateB/UpdateB.H/.cpp
```

### 4.3 CMake 自动注册

每个模块目录只暴露：

```text
<Module>Package.cpp
Units/*/*.cpp
```

CMake 不再逐文件列举，而是按模块目录收集，避免用户新增 CPP 时改内核构建。

### 4.4 插件注册

`<Module>Package::register_handlers(host)` 只做注册；
单位 handler 只依赖 `StageContext`。

### 4.5 独立实现

单位 handler 必须自己读写：

```text
ctx.bus->field("Efield_fp", lev)
ctx.bus->species("electrons")
```

而不是：

```text
ctx.host->fieldSolve()
```

当 Yee/PSATD/Boris/Deposition 都完成这一步，才谈删除 `WarpX`。

## 5. 迁移顺序

```text
1. FieldSolver/Yee
2. FieldSolver/PSATD
3. Particles/Push
4. Particles/Deposition
5. Particles/Collision
6. Diagnostics/Full
7. Diagnostics/Reduced
8. Boundary/PML
9. EB
10. Initialization / Laser / Fluids
```

每一步：

```text
拆分 -> 注册 -> 单元测试 -> A/B 逐位 -> 四维构建
```

## 6. 独立化顺序

```text
A. RuntimeConfig 快照化
B. RuntimeData/HostServices 移入 adapter
C. IPicKernel + WarpXKernel
D. Runtime 只持有 IPicKernel
E. 替换时间步/Level 生命周期
F. 删除 WarpX
```

`E` 之前不能声称“独立于 WarpX”。

## 7. 四维门禁

每个模块迁移后必须通过：

```text
1D / 2D / 3D / RZ
```

至少：

```text
langmuir
collision
EB
ionization
```

当前：

```text
1D/3D/RZ 构建通过
1D/3D/RZ openPMD smoke 通过
2D openPMD+HDF5 159/159
```

## 8. 完成定义

```text
1. 241 个 CPP 全部归入 Unit 文件夹，或拆成更细 Unit；
2. 每个 Unit 只有一个公开物理/逻辑功能；
3. 每个 Module 一个 Package；
4. 删除任一 Module 后其余可编译；
5. Core 不 include WarpX.H；
6. 删除 WarpX 后四维算例 A/B 通过；
7. 用户新增源项/求解器/推进器只写 2–3 个 CPP。
```

## 9. 结论

当前完成的是：

```text
源码收纳
Domains 目录
第一个 Yee 模块骨架
四维构建
```

尚未完成的是：

```text
241 CPP 的单位化
实现级模块拆分
Core 对 WarpX 的依赖倒置
删除 WarpX
```

这是一项多阶段工程，不能用目录移动冒充完成。
