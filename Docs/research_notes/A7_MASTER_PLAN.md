# A7 总纲：全量模块化、插件化、独立化

> 本文件是 `THREE_TASK_TECHNICAL_PATH.md` 的 A7 细化，不推翻 A0–A6。  
> A0–A6 完成宿生与契约；A7 完成实现级拆分与去 WarpX。

## 0. 源码完整性

已验证：

```text
legacy Source 文件: 829
当前 Source 文件:   833
missing:            0
```

- 清单：`Docs/research_notes/legacy_source_manifest.txt`
- 校验：`python3 Tools/Modularization/check_legacy_coverage.py`

新增 4 个文件为模块接口/Yee 骨架，不替代任何旧文件。

## 1. 当前真实结构

```text
Source/Core/{Runtime,Utils,ablastr,main.cpp}
Source/Plugins/PicEm/
  WarpX.H/.cpp
  Fields.H
  Modules/
    AcceleratorLattice  6 cpp
    BoundaryConditions  7
    Diagnostics        62
    EmbeddedBoundary    4
    Evolve              2
    FieldSolver        60
    Filter              3
    Fluids              4
    Initialization     12
    Laser               3
    MomentsSolver      13
    NonlinearSolvers    2
    Parallelization     4
    Particles          44
    PicEm               6
    Python              9
```

## 2. 核心问题

### 2.1 Core 仍依赖 Plugin

```text
Core/Runtime/Runtime.cpp       #include "WarpX.H"
Core/Runtime/RuntimeConfig.cpp #include "WarpX.H"
Core/Runtime/WarpXRuntimeData.*
Core/Runtime/WarpXHostServices.*
```

### 2.2 接口仍是 WarpX 原生

`RuntimeData` 直接暴露：

```cpp
MultiParticleContainer& particles();
const amrex::EB2::IndexSpace* ebIndexSpace(int);
const Vector<unique_ptr<NCIGodfreyFilter>>& nciGodfreyFilter...;
```

`HostServices` 直接暴露：

```cpp
ParticleBoundaryBuffer& particleBoundaryBuffer();
amrex::AmrMesh* amrMesh();
```

### 2.3 241 个 CPP 仍是实现文件，不是单位

只有：

```text
Modules/PicEm/{ParticlePush,Deposition,FieldSolve/Yee}
```

是单位 handler。

## 3. A7 工作分解

### A7.0 源码完整性

- 已完成 829/829；
- 每次物理移动后重新运行校验脚本。

### A7.1 原生核心接口

目标：

```text
Core 只依赖抽象，不 include WarpX.H
```

工作：

1. `RuntimeConfig` 快照化；
2. `RuntimeData` 去掉 `MultiParticleContainer`、`NCIGodfreyFilter`、`EB2::IndexSpace`；
3. `HostServices` 拆成：
   - `IFieldAlgebra`
   - `IParticleServices`
   - `IDiagnosticsMetadata`
   - `IMovingWindow`
   - `IPML`
   - `ICheckpoint`
4. `IPicKernel` + `WarpXKernel` adapter；
5. `Core/Runtime` 不再出现 `WarpX` 类型。

### A7.2 Kernel 拆分

`WarpX.H/.cpp` 是 1,800+ 行上帝类，拆为：

```text
Kernel/TimeStep
Kernel/LevelLifecycle
Kernel/FieldState
Kernel/SpeciesState
Kernel/DiagnosticsDriver
Kernel/IO
Kernel/Checkpoint
Kernel/MovingWindow
Kernel/PML
```

每个单元一个 CPP。

### A7.3 FieldSolver

```text
FieldSolver/
  FiniteDifference/   (EvolveE/B 作为单位文件，不强制拆 Yee)
  Electrostatic/
  PSATD/
  Implicit/
  Magnetostatic/
```

进一步：

```text
FiniteDifference/Units/EvolveE
FiniteDifference/Units/EvolveB
FiniteDifference/Units/EvolveEPML
...
```

### A7.4 Particles

目标叶子：

```text
Particles/
  Container/
  Push/{Boris,Vay,HigueraCary,Implicit}
  Deposition/{Charge,Current}
  Collision/{Binary,MCC,DSMC,Fusion,Bremsstrahlung,...}
  QED/{Photon,Schwinger,Synchrotron}
  Injection/{Parser,OpenPMD,Flux}
  Boundaries
  Sort
  Resampling
  Filter
  Gather
  Thermalizer
```

### A7.5 Diagnostics

```text
Diagnostics/
  Full/
  BTD/
  ReducedDiags/<OnePerDiag>
  OpenPMD/
  IO/
  ParticleIO/
```

每个 `ReducedDiags/*.cpp` 本身已接近一个单位。

### A7.6 Boundaries / EB

```text
BoundaryConditions/
  FieldBoundary
  PML
  PEC
  SilverMueller
  ParticleBoundary
EmbeddedBoundary/
  Init
  Geometry
  ParticleInteraction
```

### A7.7 Initialization / Laser / Fluids / Lattice / Moments / Nonlinear / Parallel / Python

每个目录按类拆分：

```text
Initialization/{PlasmaInjector,ExternalField,TemperatureProperties,VelocityProperties,...}
Laser/{LaserProfile,LaserParticleContainer}
Fluids/{MultiFluidContainer,WarpXFluidContainer}
AcceleratorLattice/{Drift,Quadrupole,PlasmaLens}
MomentsSolver/{FV,Equations}
NonlinearSolvers/{Newton,Picard,MG}
Parallelization/{Regrid,LoadBalance,Communication}
Python/{Callbacks,PyWarpX}
Evolve/{ComputeDt,Step}
```

### A7.8 插件 ABI

- 单位模块统一 `IHandler` + `HandlerSpec`；
- 外部 `.so` 只能注册 handler；
- 后续提供纯 C 数据接口与共享运行时；
- `IAmrPolicy` 外部 ABI 单独设计。

### A7.9 删除 WarpX

条件：

```text
grep -R "WarpX" Source/Core 只允许注释
Core 不 include Plugins/PicEm/WarpX.H
所有模块不持有 WarpX 类型
1D/2D/3D/RZ 官方算例 A/B 通过
```

## 4. 四维门禁

每个 Unit 迁移后：

```text
1D / 2D / 3D / RZ
```

至少：

```text
langmuir / collision / EB / ionization
```

当前：

```text
1D/3D/RZ 构建 + openPMD smoke
2D 159/159
```

## 5. 风险

- `EvolveE/B` 是模板 + 多算法，不能按文件强拆；
- `Particles` 44 个 CPP 之间共享容器状态；
- `Diagnostics` 62 个 CPP 与 IO 格式耦合；
- `RuntimeData` 的 WarpX 类型会扩散到所有模块；
- 删除 WarpX 前必须先完成 A7.1/A7.2。

## 6. 完成定义

```text
A7.0 829/829
A7.1 Core 无 WarpX
A7.2 Kernel 拆分
A7.3-A7.7 每个物理功能一个 Unit CPP
A7.8 插件 ABI
A7.9 删除 WarpX 后四维 A/B
```

当前进度：

```text
A7.0 完成
A7.1 Core 无 WarpX 完成；插件独立未完成
A7.2 细化见 A7.2_SUBSTEPS；A7.3 细化见 A7.3_FIELD_SOLVER_PLAN；A7.4 细化见 A7.4_PARTICLE_SYSTEM_PLAN；A7.5-A7.7 仅目录/骨架
A7.8 部分
A7.9 未开始
```
