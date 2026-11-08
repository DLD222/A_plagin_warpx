# A7 路线重排（A7.2* / A7.3* / A7.4*）

## 0. 为什么重排

旧顺序：

```text
A7.2 Diagnostics/IO
A7.3 FieldSolver
A7.4 Particles
```

这个顺序没有先读清依赖方向，实际源码依赖是：

```text
FieldSolver -> Particles：14 处
Collision   -> Particles：171 处
Particles   -> FieldSolver：1 处（HybridPICModel）
Kernel      -> FieldSolver/Particles：21 处
```

结论：

```text
Particles 是基础层
FieldSolver 依赖 Particles
Collision/QED 依赖 Particles
Kernel 同时依赖两者
```

因此正确顺序必须是：

```text
A7.2  Core 契约与插件基础设施
A7.3  ParticleCore 插件
A7.4  FieldSolver 插件
```

旧 A7.3 FieldSolver 与旧 A7.4 Particles 对调。

---

## 1. 正确依赖图

```text
Core
  Runtime
  DataBus
  IParticleContainer
  IParticleTile / Iterator / Reduce
  IFieldSolver contracts
        |
        +--> Diagnostics (已完成)
        +--> IO (已完成)
        |
        +--> ParticleCore
                |
                +--> Collision
                +--> QED
                +--> Laser
                +--> ParticleIO
                |
                +--> FieldSolver
                        |
                        +--> Kernel integration
```

规则：

```text
ParticleCore 不 include FieldSolver
FieldSolver 只通过 Core 字段/物种接口访问粒子
Collision/QED 只通过 Core 粒子接口访问
```

---

## 2. A7.2 Core 契约与插件基础设施

### A7.2.1 Diagnostics 独立

状态：完成

### A7.2.2 IO 独立

状态：完成

### A7.2.3 插件边界检查

状态：完成

```text
Tools/Modularization/check_plugin_boundaries.py
```

### A7.2.4 粒子能力契约

状态：未完成

必须新增：

```text
IParticleTile
IParticleIterator
IParticleReduce
IParticlePositionAccess
IParticleSpeciesAccess
```

原因：

```text
Collision 171 处依赖
FieldSolver 14 处依赖
当前只有 IParticleContainer，不能迭代/归约/取位置
```

验收：

```text
Collision 可在不 include Particles/* 的情况下编译
FieldSolver 可只通过接口读取粒子
```

### A7.2.5 Kernel 契约

状态：部分

已有：

```text
IPicKernel
ISimulationKernel
IFieldSolverEngine
ICollisionHandler
```

仍缺：

```text
Kernel/TimeStep
Kernel/LevelLifecycle
Kernel/FieldState
Kernel/SpeciesState
Kernel/IO
```

A7.2 只要求契约，不要求拆分实现。

### A7.2.6 插件注册/禁用

状态：部分

已有：

```text
WARPX_DISABLE_DIAGNOSTICS
WARPX_DISABLE_BUILTIN_MODULES
```

仍缺：

```text
WARPX_DISABLE_COLLISION
WARPX_DISABLE_QED
WARPX_DISABLE_FIELD_SOLVER
```

### A7.2.7 验收

```text
Core 不 include Plugins
Plugin 之间无直接 include
Diagnostics/IO 可禁用
粒子能力契约可编译
```

---

## 3. A7.3 ParticleCore 插件

### A7.3.0 清单

```text
Particles 47 个 CPP
全部归属功能单元
```

### A7.3.1 ParticleCore 插件骨架

```text
Source/Plugins/ParticleCore/
  ParticleCorePackage
  Container/
  Push/
  Deposition/
  Gather/
  Boundary/
  Creation/
  Resampling/
  Thermalizer/
  Sorting/
```

规则：

```text
ParticleCore 不 include Collision/QED/Laser/FieldSolver
只通过 Core 字段/物种接口
```

### A7.3.2 容器拆分

`WarpXParticleContainer.cpp` 3254 行拆为：

```text
ParticleState
ParticleAttributes
ParticleIO
ParticleBoundary
ParticlePushDriver
ParticleDepositDriver
ParticleGatherDriver
```

验收：

```text
单 CPP 不超过 500 行
```

### A7.3.3 Push 单位

```text
BorisPush.cpp
VayPush.cpp
HigueraCaryPush.cpp
ImplicitPush.cpp
PushSelector.cpp
```

### A7.3.4 Deposition 单位

```text
ChargeDeposition.cpp
CurrentEsirkepov.cpp
CurrentVillasenor.cpp
CurrentDirect.cpp
```

### A7.3.5 Gather 单位

```text
FieldGather.cpp
ExternalFieldGather.cpp
```

### A7.3.6 Boundary/Creation/Resampling/Thermalizer/Sorting

每个功能一个 CPP。

### A7.3.7 Collision 插件

```text
Source/Plugins/Collision/
```

依赖：

```text
Core IParticleTile/IParticleIterator
```

验收：

```text
删除 Collision 插件，ParticleCore 可编译运行
```

### A7.3.8 QED 插件

```text
Source/Plugins/QED/
```

依赖：

```text
Core IParticleTile/IParticleIterator
```

### A7.3.9 Laser/Rigid/Photon

```text
Plugins/Laser
ParticleCore species modules
```

### A7.3.10 验收

```text
ParticleCore 独立
Collision/QED 独立
PicEm 不 include 这些插件
四维 A/B
```

---

## 4. A7.4 FieldSolver 插件

前置条件：

```text
A7.3 ParticleCore 完成
A7.2 粒子能力契约完成
```

### A7.4.1 已完成契约

```text
IFieldSolver
IFiniteDifferenceSolver
IElectrostaticSolver
IImplicitSolver
IMacroscopicProperties
IFieldSolverEngine
```

### A7.4.2 FieldSolver 插件骨架

```text
Source/Plugins/FieldSolver/
  FieldSolverPackage
  FiniteDifference/
  Electrostatic/
  PSATD/
  Implicit/
  Magnetostatic/
```

### A7.4.3 FiniteDifference 单位

```text
EvolveElectricField
EvolveMagneticField
EvolveElectricFieldPML
EvolveMagneticFieldPML
EvolveF
EvolveG
HybridPICSolveE
MacroscopicEvolveE
```

### A7.4.4 Electrostatic 单位

```text
LabFrameExplicit
RelativisticExplicit
EffectivePotential
PoissonBoundary
```

### A7.4.5 PSATD 单位

```text
Galilean
Comoving
JRhom
PML
RZ/Hankel
```

### A7.4.6 Implicit 单位

```text
SemiImplicitEM
ThetaImplicitEM
StrangImplicitSpectralEM
SemiImplicitDarwin
```

### A7.4.7 Magnetostatic 单位

```text
SolveMagnetostatic
Boundary
```

### A7.4.8 去 WarpX 单例

```text
FieldSolver 内 WarpX:: 201 -> 0
```

### A7.4.9 验收

```text
FieldSolver 独立
PicEm 不 include FieldSolver
四维 A/B
禁用 FieldSolver 可编译
```

---

## 5. 命名规范

```text
A7.2
  IParticleTile
  IParticleIterator
  IParticleReduce
  IParticlePositionAccess
  IParticleSpeciesAccess

A7.3
  ParticleCorePlugin
  CollisionPlugin
  QEDPlugin
  BorisPush
  VayPush
  EsirkepovCurrentDeposition
  VillasenorCurrentDeposition

A7.4
  FieldSolverPlugin
  FiniteDifferenceFieldSolver
  ElectrostaticFieldSolver
  SpectralFieldSolver
  ImplicitFieldSolver
  MagnetostaticFieldSolver
  EvolveElectricField
  EvolveMagneticField
```

禁止：

```text
ParticleUtils
SmartUtils
AddParticles
EvolveE
EvolveB
```

---

## 6. 当前状态

```text
A7.2.1 Diagnostics   完成
A7.2.2 IO            完成
A7.2.3 边界检查      完成
A7.2.4 粒子能力契约  未完成
A7.2.5 Kernel 契约   部分
A7.2.6 禁用开关      部分
A7.2.7 验收          未完成

A7.3 未开始
A7.4 未开始（旧 A7.3 只是契约）
```

---

## 7. 下一步

```text
A7.2.4 IParticleTile/IParticleIterator/IParticleReduce
```

不是 Collision 目录移动，也不是 FieldSolver 目录移动。

这些契约完成后：

```text
A7.3 ParticleCore
A7.4 FieldSolver
```

顺序才成立。

## 8. A7.2.4 进展

新增 Core 粒子能力契约：

```text
Core/Modules/Particles/IParticleTile.H
Core/Modules/Particles/IParticleIterator.H
Core/Modules/Particles/IParticleReduce.H
```

定义：

```cpp
IParticleTile      = IParticleContainer::ParticleTileType
IParticleTileData  = IParticleContainer::ParticleTileType::ParticleTileDataType
IParticleIterator  = WarpXParIter
particleReduce<ReduceData>(pc, op, reduce_op)
```

`IParticleContainer` 增加：

```cpp
using ParticleTileType = Base::ParticleTileType;
```

新增契约测试：

```text
tests/runtime/test_particle_contracts.cpp
```

验证：

```text
默认 11/11 PASS
plugin boundaries PASS
particle contracts 编译 PASS
```

仍未完成：

```text
Collision 仍 include Particles/*
FieldSolver 仍直接访问粒子
IParticlePositionAccess / IParticleSpeciesAccess 尚未独立
```

## 9. A7.2.4 位置/物种访问契约

新增：

```text
Core/Modules/Particles/IParticlePositionAccess.H
Core/Modules/Particles/IParticleSpeciesAccess.H
```

```cpp
using IParticlePositionAccess = GetParticlePosition<PIdx>;
particlePosition(p, x, y, z)
ParticleSpeciesInfo particleSpeciesInfo(const IParticleContainer&)
```

测试：

```text
tests/runtime/test_particle_access_contracts.cpp
```

验证：

```text
默认 11/11 PASS
plugin boundaries PASS
contracts 编译 PASS
```

## 10. CollisionSpeciesTable 迁移确认

尝试把表改为 `IParticleContainerAccess` 后，编译暴露：

```text
BinaryCollision.H
  Vector<WarpXParticleContainer*> product_species_vector
  doCollisionsWithinTile(WarpXParticleContainer&)
NuclearFusionFunc.H
  CollisionSpeciesTable(MultiParticleContainer&)
```

因此不能只改表：

```text
Collision 内部视图仍要求 WarpXParticleContainer
```

A7.2.4 的下一步是：

```text
1. 给 IParticleContainer 增加 collision 所需操作
2. 或把 Collision 内部 tile 访问改为 IParticleTile
```

不能通过 static_cast 隐藏。

## 11. A7.2.4 表构造迁移

`CollisionSpeciesTable` 构造函数已改为：

```cpp
const warpx::runtime::IParticleContainerAccess&
```

`BinaryCollisionUtils` 的入口也改为：

```cpp
IParticleContainerAccess const*
```

`CollisionSpeciesView` 仍保留具体容器：

```text
WarpXParticleContainer&
```

原因：

```text
BinaryCollision / SmartCopyFactory / ParticleCreation
仍要求具体容器
```

验证：

```text
runtime + plugin 12/12 PASS
plugin boundaries PASS
charge_exchange_dsmc PASS
```

仍未完成：

```text
Collision 视图接口化
Collision 单元无 Particles include
Plugins/Collision
```

## 12. Collision 视图接口化

`CollisionSpeciesView` 已改为：

```cpp
IParticleContainer&
IParticleContainer* operator->
```

新增 Core 粒子能力：

```cpp
isPhysicalSpecies(PhysicalSpecies)
getIonizationInitialLevel()
particleData()
config()
```

迁移的 Collision 单元：

```text
BinaryCollision
ParticleCreationFunc
PhotonCreationFunc
SplitAndScatterFunc
BackgroundMCC
InverseBremsstrahlung
PulsedDecay
```

验证：

```text
runtime + plugin 12/12 PASS
plugin boundaries PASS
charge_exchange_dsmc PASS
```

仍未完成：

```text
Collision 中 83 处 ParticleCreation/ParticleUtils 具体 include
Plugins/Collision 目录
```

不能宣布 A7.2 完成。
