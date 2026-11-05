# 插件模块边界（按物理/逻辑单一功能）

<!-- SPDX-License-Identifier: BSD-3-Clause -->

> 本文是 [最终的技术路线](最终的技术路线.md) §1.3 的展开。原则：**一个模块 = 一件物理/逻辑上可独立命名的事**。
> 禁止把多个可独立演化的功能塞进一个“包”。

## 1. 模块清单

### 1.1 场求解（EM）

| 模块 | 单一职责 | 读 | 写 | 阶段 |
|---|---|---|---|---|
| `YeeMaxwellSolver` | 只推进 E/B | `E`, `B`, `J` | `E`, `B` | `FieldSolve` |
| `CkcMaxwellSolver` | 只推进 E/B（CKC） | `E`, `B`, `J` | `E`, `B` | `FieldSolve` |
| `PsatdMaxwellSolver` | 只推进 E/B（PSATD） | `E`, `B`, `J`, `rho` | `E`, `B` | `FieldSolve` |
| `ElectrostaticSolver` | 只解 Poisson/Phi | `rho` | `phi`, `E` | `FieldSolve` |
| `MagnetostaticSolver` | 只解 vector potential | `J` | `A`, `B` | `FieldSolve` |

### 1.2 粒子

| 模块 | 单一职责 | 读 | 写 |
|---|---|---|---|
| `ParticlePush` | 只推进粒子 | `E`, `B`, species | species |
| `CurrentDeposition` | 只沉积 J | species | `J` |
| `ChargeDeposition` | 只沉积 rho | species | `rho` |
| `FieldGather` | 只插值场到粒子 | `E`, `B` | species |
| `ParticleBoundary` | 只处理边界/吸收 | species | species |
| `ParticleResampling` | 只做重采样 | species | species |
| `Ionization` | 只做电离 | species, `E` | species |
| `QEDSynchrotron` | 只做同步辐射 | species, `B` | species, photons |
| `QEDPairProduction` | 只做对产生 | photons | pairs |

### 1.3 碰撞（独立插件）

| 模块 | 单一职责 | 读 | 写 |
|---|---|---|---|
| `BinaryCollision` | 一对物种的二元碰撞 | `electrons`, `ions` | same + products |
| `MCCCollision` | 背景气体 MCC | species, gas | species |
| `DSMCCollision` | DSMC | species | species |
| `NuclearFusion` | 聚变反应 | ions | products |
| `Bremsstrahlung` | 轫致辐射 | species | photons |

统一由 `CollisionPackage` 注册；核心只持有 `ICollisionModel` 注册表，不持有 `CollisionHandler`。

### 1.4 Hybrid

| 模块 | 单一职责 | 读 | 写 |
|---|---|---|---|
| `HybridExternalFields` | 外部 E/J | `B`, parser | `J_external`, `E_external` |
| `HybridAmpereCurrent` | curl B → J | `B` | `J_plasma` |
| `HybridOhmSolver` | Ohm 定律 → E | `B`, `J`, `rho`, `Te` | `E` |
| `ElectronPressureClosure` | Te/Pe 闭合 | `rho`, `Ke` | `Te`, `Pe` |
| `ElectronEnergyTransport` | 电子能量/QDSMC | `Te`, `rho` | `Ke`, `qdsmc` |
| `HybridBoundaryFilter` | 边界/滤波 | `E`, `B`, `Te` | same |

### 1.5 诊断

| 模块 | 单一职责 |
|---|---|
| `FieldDiagnostics` | 只输出场 |
| `ParticleDiagnostics` | 只输出粒子 |
| `BackTransformedDiagnostics` | 只做 BTD |
| `ReducedDiagnostics` | 只做标量 reduced diag |

### 1.6 边界/PML

| 模块 | 单一职责 |
|---|---|
| `PmlBoundary` | 只做 PML 吸收 |
| `PecBoundary` | 只做 PEC |
| `PeriodicBoundary` | 只做周期 |

### 1.7 流体

| 模块 | 单一职责 |
|---|---|
| `ColdFluid` | 冷流体推进 |
| `Moment5Fluid` | 五矩流体 |
| `QdsmcFluid` | QDSMC 闭合 |

## 2. 依赖规则

1. 模块之间**只能**通过 `DataBus` 键交互；
2. 一个模块不得 `#include` 另一个模块的实现头；
3. 共享类型只允许在 `Runtime/FieldSpec.H`、`Runtime/SpeciesSpec.H`、`Runtime/StagePoint.H`、`Runtime/HostServices.H`；
4. 内核只 include 接口，不 include 具体模块；
5. 模块参数在 `module.<name>.*` 命名空间。

## 3. 迁移映射

| 现有 | 目标模块 | 任务 |
|---|---|---|
| `FiniteDifferenceSolver` | `YeeMaxwellSolver` / `CkcMaxwellSolver` | A3.3 |
| `SpectralSolver` | `PsatdMaxwellSolver` | A3.3 |
| `ElectrostaticSolver` 家族 | `ElectrostaticSolver` | A1.8j 已去单例，A3 包装 |
| `MagnetostaticSolver` | `MagnetostaticSolver` | A1.8q 已去单例，A3 包装 |
| `HybridPICModel` | 6 个 Hybrid 模块 | **A3.3a** |
| `CollisionHandler` + models | `CollisionPackage` + 每个 model | **A3.3b** |
| `MultiParticleContainer` | `ParticlePush/Deposition/Gather` | A3.3 |
| `Diagnostics` | 4 个诊断模块 | A3.3 |
| `PML` | `PmlBoundary` | A3.3 |

## 4. 验收

- `rg "CollisionHandler" Source/Particles` 在核心侧为 0；
- 每个模块有 `HandlerSpec` 并能打印依赖图；
- 移除任一包后，其余功能可配置/运行；
- `test_2d_collision_xz` 数值保持 `4.66e-4`；
- 模块目录不含 `WarpX::GetInstance()` / 同伴模块指针。

## 5. 当前差距

- 没有 `PluginHost/HandlerSpec/SpeciesHandle`；
- 碰撞仍是核心 `CollisionHandler`；
- Hybrid 仍是单体；
- 诊断/场求解仍以核心类存在。

因此 A1.8 的去单例只是**前置条件**，模块化本身从 A3.3 开始。
