# 原 WarpX 源码进入项目模块

## 1. 目标

把原 WarpX 的物理源码变成项目自己的模块，而不是保留原版目录结构：

```text
Source/Plugins/PicEm/
  WarpX.H/.cpp        // 内核实现（仍待替换）
  Fields.H
  Modules/
    FieldSolver/
    Particles/
    Diagnostics/
    BoundaryConditions/
    EmbeddedBoundary/
    Evolve/
    Initialization/
    Laser/
    Filter/
    Fluids/
    AcceleratorLattice/
    NonlinearSolvers/
    MomentsSolver/
    Parallelization/
    Python/
    PicEm/            // 新的 L1 单位模块（Package/Handler）
```

所有原 WarpX 物理目录现在都在 `Source/Plugins/PicEm/Modules/` 下。

## 2. 当前模块

| 模块目录 | 原内容 | 当前单位化 |
|---|---|---|
| `FieldSolver/` | Yee/PSATD/CKC/ECT/Hybrid/静电/静磁 | 未拆到单个求解器 |
| `Particles/` | 推进、沉积、碰撞、QED、注入 | 未拆到单个单元 |
| `Diagnostics/` | Full/BTD/Reduced/OpenPMD | 未拆 |
| `BoundaryConditions/` | PML/PEC/Silver-Müller | 未拆 |
| `EmbeddedBoundary/` | EB cut-cell | 未拆 |
| `Evolve/` | 时间步/子循环 | 未拆 |
| `PicEm/` | 新 L1 handler | 已按单位模块拆分 |

`PicEm/` 下的单位模块：

```text
ParticlePush/ParticlePushHandler.H/.cpp
Deposition/DepositionHandler.H/.cpp
FieldSolve/FieldSolveHandler.H/.cpp
PicEmPackage.H/.cpp
```

## 3. 构建

CMake 路径改为：

```text
add_subdirectory(Source/Plugins/PicEm/Modules/FieldSolver)
add_subdirectory(Source/Plugins/PicEm/Modules/Particles)
...
```

include path 增加：

```text
Source/Plugins/PicEm
Source/Plugins/PicEm/Modules
```

原有 `#include "FieldSolver/..."`、`"Particles/..."` 仍可解析。

## 4. 验证

```text
2D runtime + dynamic + plugin 23/23 PASS
```

四维构建需要重新验证（CMake 路径已更新）。

## 5. 仍未完成

- `Modules/FieldSolver` 内部仍是原 WarpX 多文件集合，不是“一个单位一个 CPP”；
- `Modules/Particles` 仍混合推进/沉积/碰撞/QED；
- `Modules/Diagnostics` 仍混合 Full/BTD/Reduced；
- 只有 `Modules/PicEm` 是新 L1 单位模块。

## 6. 下一步

按模块逐层拆分：

```text
Modules/FieldSolver/Yee/
  YeePackage.H/.cpp
  UpdateE/UpdateE.H/.cpp
  UpdateB/UpdateB.H/.cpp
  Stencil/Stencil.H/.cpp
```

```text
Modules/Particles/Push/
  PushPackage.H/.cpp
  Boris/Boris.H/.cpp
  Vay/Vay.H/.cpp
```

每个单位模块一个 CPP，包只注册。
