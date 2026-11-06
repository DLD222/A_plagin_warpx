# A6a 动态 regrid 试验记录

## 目的

验证 `amr.max_level=1`、`warpx.regrid_int=1` 时能否从单层动态加密到两层。

## 试验过的改动（已回滚）

1. `MakeNewLevelFromCoarse`：
   - `AllocLevelData`
   - `InitLevelData`
   - `InterpFromCoarseLevel` 填充 E/B/J/rho
2. `WarpX::ErrorEst` 增加 `refine_start_time`，避免初始建层
3. `Evolve` 在 `regrid_int` 到达时调用 `regrid(0,time)`
4. regrid 后 `mypc->Redistribute(); mypc->defineAllParticleTiles()`

## 实际错误

### 1. LevelResources 断言

```text
Assertion m_level_resources.size() == finest_level + 1 failed
```

regrid 过程中 `finest_level` 暂时为 0，而 resources 已按 `maxLevel()+1` 预分配为 2。
原断言只允许“恰好等于”，不允许预分配。

### 2. 粒子 tile 崩溃

```text
amrex::BATransformer::coarsen_ratio()
amrex::FabArrayBase::getTileArray()
PhysicalParticleContainer::Evolve()
```

新层创建后，粒子容器的 level 工厂/tile 没有按新 BoxArray 重建。

### 3. Geometry valid_domain

```text
Assertion valid_domain failed
```

`Redistribute + defineAllParticleTiles` 之后，新层几何与粒子 BoxArray 不一致。

## 结论

动态 regrid 不是只实现 `MakeNewLevelFromCoarse`：

还必须按顺序处理：

1. `LevelResources` 预分配 vs finest_level 断言；
2. 新层 field factory/EB/PML；
3. 粒子容器 `resizeData/Redefine`；
4. 粒子 tile 与 Geometry 一致；
5. 诊断/ReducedDiags 的 level 重建；
6. 插件 on_regrid 回调。

任何一步缺失都会崩溃或得到错误物理状态。

## 当前决定

试验代码全部回滚，主分支不保留未验证的 regrid 实现。

A6a 保持 pending。
