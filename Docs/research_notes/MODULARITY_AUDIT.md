# 模块化审计：现在到底算不算“按功能分模块”

> 结论：**没有。**
>
> 当前完成的是 A1 的“去单例/去上帝对象直连”，它让模块**有可能**通过 `RuntimeData/HostServices` 访问内核状态；但还没有
> `PluginHost`、`HandlerSpec`、`StagePoint` 执行器，也没有真正的模块边界。
>
> 碰撞尤其不是独立插件：它仍然是 `MultiParticleContainer` 直接持有的 `CollisionHandler`，核心编译单元的一部分。

## 1. 判断标准

路线文档定义的“模块”至少满足：

1. 有独立注册入口（`Package::register_handlers`）；
2. 有 `HandlerSpec`：reads/writes 字段/物种、stage；
3. 只通过 `DataBus` 命名状态 + `HostServices` 交互；
4. 不持有 `WarpX&`、`MultiParticleContainer*`、其他模块指针；
5. 可以整体启用/禁用，核心不 include 它的实现头。

按这个标准逐项检查。

## 2. 现状对照

| 功能 | 当前形态 | 是否模块 | 缺口 |
|---|---|---|---|
| PIC 粒子 | `MultiParticleContainer`/`PhysicalParticleContainer` | 否 | 仍是核心类，无 SpeciesHandle/HandlerSpec |
| EM 场求解 | `FiniteDifferenceSolver`/`SpectralSolver` | 否 | 无 IFieldSolver 包，直接调 `WarpX` |
| 静电 | `ElectrostaticSolver` 家族 | 部分 | 已走 data/host，但仍编译进核心，无注册 |
| Hybrid | `HybridPICModel` 单体 | 否 | 29 处单例，场/边界/QDSMC 混在一起 |
| 碰撞 | `CollisionHandler` + 多个 model | **否** | 核心直接持有，model 拿 `WarpXParticleContainer*` |
| 诊断 | `Diagnostics`/functor | 部分 | functor 走 context，但无独立包/阶段 |
| PML | `PML` | 部分 | context 已传入，但仍是核心对象 |
| QED | 粒子过程 | 否 | 直接依赖粒子容器 |

`Source/Runtime` 目前只有：

```
Runtime / RuntimeContext / RuntimeData / HostServices / DataBus(schema only)
```

没有：

```
PluginHost
HandlerSpec / StagePlan / StagePoint
PicEmPackage
SpeciesHandle
```

所以**还不能称为插件化模块架构**。

## 3. 碰撞为什么现在不是插件

调用链：

```text
MultiParticleContainer::doCollisions(step,time,dt)
  -> collisionhandler->doCollisions(step,time,dt,this)
    -> CollisionHandler 内部按 model 调用
      -> BinaryCollision / MCC / DSMC / ...
```

问题：

1. `MultiParticleContainer` 直接持有 `CollisionHandler`，构造在核心初始化里；
2. `CollisionHandler::doCollisions` 接收 `MultiParticleContainer*`，model 可直接拿粒子容器；
3. model 内部仍用 `WarpX::GetInstance()`（`BinaryCollision.H` 2 处、`NuclearFusionFunc.H` 2 处）；
4. 没有 `HandlerSpec` 声明“读哪些 species、写哪些 species/字段”，StagePlan 无法排序；
5. 没有注册目录，无法把碰撞整体禁用或替换成另一个碰撞包。

因此碰撞是“核心里的一个子系统”，不是“独立插件模块”。

## 4. 碰撞模块的目标形态

### 4.1 包

```cpp
class CollisionPackage {
public:
    void register_schema(DataBus& bus);
    void register_handlers(PluginHost& host);
};
```

### 4.2 模型接口

```cpp
class ICollisionModel {
public:
    virtual ~ICollisionModel() = default;
    virtual std::string name() const = 0;
    virtual HandlerSpec spec() const = 0;
    virtual void apply(StageContext&) = 0;
};
```

`HandlerSpec` 示例：

```cpp
HandlerSpec{
    .stage = Stage::Collisions,
    .reads_species = {"electrons", "ions"},
    .writes_species = {"electrons", "ions", "products"},
    .reads_fields = {},
    .writes_fields = {},
};
```

### 4.3 物种访问

model 不再拿 `WarpXParticleContainer*`，只拿：

```cpp
SpeciesHandle electrons = bus.species("electrons");
```

句柄提供：

```
numberOfParticles / positions / momenta / weights / mass / charge
createProduct / removeParticles / tile iteration
```

底层仍可以由 `WarpXParticleContainer` 实现，但**类型不出现在插件接口**。

### 4.4 阶段

```text
BeforeCollisions   （可选：准备截面/温度）
Collisions         （model 修改 species）
AfterCollisions    （可选：产物注册/诊断）
```

由 `StagePlan` 按 `reads/writes` 拓扑排序，而不是 `CollisionHandler` 内部顺序。

### 4.5 输入

碰撞参数移到包命名空间，例如：

```
collisions.models = binary
collisions.binary.pair = electrons ions
collisions.binary.cross_section = ...
```

不再散落在 `<species>.collision_*`。

## 5. 迁移顺序

1. **A3.3** 先有 `PluginHost/HandlerSpec/StagePoint`；
2. **A3.3b CollisionPackage**：
   - 注册 `collision` handler；
   - 把 `CollisionHandler` 改成包内实现；
   - 用 `SpeciesHandle` 替换 `MultiParticleContainer*`；
   - 删除核心对 `CollisionHandler` 的直接持有；
3. **A3.4** 回归门：
   - `test_2d_collision_xz` 数值不变；
   - 关闭 CollisionPackage 时核心仍能编译/运行（无碰撞）；
4. **A5** 允许包以共享库/动态注册方式加载。

## 6. 验收指标

- 核心 `Source/Particles` 不再 include `Collision/CollisionHandler.H`；
- `CollisionPackage` 可以单独注册/禁用；
- `rg "WarpX::GetInstance|MultiParticleContainer\\*" Source/Particles/Collision` 为 0（插件内也不允许）；
- `test_2d_collision_xz` 的定量误差保持 `4.66e-4`；
- `HandlerSpec` 能打印出碰撞阶段的数据依赖图。

## 7. 当前结论

- A1.8 的成果是“内核状态可注入”，**不是模块化完成**；
- 碰撞要成为独立插件，必须先有 `PluginHost/HandlerSpec/SpeciesHandle`，再把 `CollisionHandler` 整体迁入 `CollisionPackage`；
- 在此之前继续在碰撞类里加 context 参数，只是把单例换成参数，不能算模块化。

