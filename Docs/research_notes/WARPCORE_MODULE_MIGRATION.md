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

## 7. 第一个完整模块文件夹：Yee

已新增：

```text
Source/Plugins/PicEm/Modules/PicEm/FieldSolve/Yee/
  YeePackage.H/.cpp
  YeeFieldSolveHandler.H/.cpp
```

- `YeePackage` 只注册；
- `YeeFieldSolveHandler` 一个 CPP，一个单位功能；
- `PicEmPackage` 在 `field_solve.yee` 时调用 `YeePackage`；
- 其他算法仍走通用 `FieldSolveHandler`。

验证：

```text
默认 runtime + dynamic + plugin 23/23 PASS
```

这是“一个完整模块文件夹 + 单位 CPP”的第一个物理求解器骨架；
内部仍调用 `WarpX` 的 Yee 实现，尚未把 Yee kernel 复制进该文件夹。

## 8. FieldSolver 物理分组

`FieldSolver/` 已按算法族分组：

```text
Modules/FieldSolver/
  FiniteDifference/    // 有限差分（含 EvolveE/EvolveB、Yee/CKC/ECT/Nodal）
  Electrostatic/       // 静电
  PSATD/               // 谱方法
  Implicit/            // 隐式
  Magnetostatic/       // 静磁
```

`FiniteDifference/` 中的 `EvolveE.cpp` / `EvolveB.cpp` 按你的说明保留为文件夹内的
“多算法单位文件”，不再强行拆成 Yee 单独文件。

验证：

```text
2D runtime + dynamic + plugin 23/23 PASS
FFT runtime + dynamic + plugin 25/25 PASS
ASAN runtime + dynamic + plugin 23/23 PASS
1D/3D/RZ smoke 8/8 PASS
```

## 9. 当前模块化层级

```text
Modules/<Domain>/<Module>/
  一个 Module = 一个物理/算法族
  内部 CPP = 该族的一个实现文件（EvolveE/EvolveB 允许作为多算法单位）
```

下一步按同样方式整理 `Particles`、`Diagnostics` 的叶子目录。
