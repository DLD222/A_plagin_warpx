# 独立项目重构：Core / Plugin 物理归属

> 目标：项目自带核心与插件，不再把“原版 WarpX 源码”当作不可分类的整体。

## 1. 已完成的物理归属

```text
Source/
  Core/
    Runtime/         宿主、Stage、DataBus、Registry、PluginHost
    Utils/           WarpX 通用工具
    ablastr/         共享抽象层
    main.cpp         入口
  Plugins/
    PicEm/           原 WarpX 物理与应用层
      WarpX.H/.cpp
      Evolve/
      FieldSolver/
      Particles/
      Diagnostics/
      BoundaryConditions/
      EmbeddedBoundary/
      Initialization/
      Laser/
      Filter/
      Fluids/
      AcceleratorLattice/
      NonlinearSolvers/
      MomentsSolver/
      Parallelization/
      Python/
      Modules/        L1 模块目录
```

CMake 已改为：

```text
add_subdirectory(Source/Core/Runtime)
add_subdirectory(Source/Core/Utils)
add_subdirectory(Source/Core/ablastr)
add_subdirectory(Source/Plugins/PicEm/FieldSolver)
...
```

`Source/WarpX.cpp`、`Source/WarpX.H` 已移动到：

```text
Source/Plugins/PicEm/
```

因此：

- 原版 WarpX 物理源码现在明确属于 `Plugins/PicEm`；
- 核心目录不再直接包含物理实现文件；
- 项目没有外部 WarpX 依赖。

## 2. 当前依赖方向

```text
Source/Core/Runtime/Runtime.cpp
  #include "WarpX.H"
  m_warpx = WarpX::Create();
```

即：

```text
Core/Runtime  ->  Plugins/PicEm/WarpX
```

这是**当前仍然存在的反向依赖**：

- 删除 `Source/Plugins/PicEm/WarpX.H` 后 Runtime 无法编译；
- 还不能声称“删除原版后独立运行”。

## 3. 为什么还没删除 WarpX

`WarpX` 仍承担：

- AMReX 生命周期；
- 场/粒子分配；
- 时间步循环；
- 所有场求解器；
- 粒子容器；
- 诊断/IO；
- 边界/PML/EB。

这不是一次目录迁移能消除的，需要依赖倒置 + 模块实现替换。

## 4. 下一步（可执行）

### 4.1 依赖倒置

```text
Core/Runtime -> IPicKernel
                  ^
                  |
Plugins/PicEm/WarpXKernel
```

`Runtime` 只依赖 `IPicKernel`，不再 `#include "WarpX.H"`。

### 4.2 替换 kernel

先替换：

```text
时间步循环
Level 生命周期
粒子容器管理
```

再替换物理模块。

### 4.3 删除条件

只有满足：

```text
Core 不 include Plugins/PicEm/WarpX.H
所有算例 A/B 逐位通过
```

才能删除 WarpX。

## 5. 注意事项

1. **移动目录不是完成独立**：只是物理归属清晰。
2. **Core 仍依赖 Plugin**：必须在报告中明确，不能宣称已完成。
3. **输入输出**：移动不影响输入；CMake include path 已更新。
4. **A/B**：迁移后必须重新跑。
5. **Make 构建**：`Make.package` 路径仍需同步；本仓库以 CMake 为准。

## 6. 迁移后验证

```text
默认 runtime + dynamic + plugin 23/23 PASS
FFT runtime + dynamic + plugin 25/25 PASS
ASAN runtime + dynamic + plugin 23/23 PASS
openPMD + HDF5 11 个关键测试 21/21 PASS
```

## 7. 结论

已完成：

- `Core` / `Plugins/PicEm` 物理归属；
- 原 `WarpX.*` 与全部物理目录移入插件；
- CMake 与 include path 同步；
- 迁移后回归通过。

未完成：

- `Runtime` 仍直接 `WarpX::Create()`；
- 删除 `WarpX` 后仍不可编译；
- 物理实现仍未拆成独立 L1 模块。

当前是“**项目自带源码的 Core/Plugin 分层**”，不是“**已脱离 WarpX**”。
