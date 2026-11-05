# StagePoint 子位点表（审计 D2）

> 来源：`ARCHITECTURE_AUDIT_DETAILED.md` §1.3 / D2。
> 目标：18 个 Python 具名回调逐一落到 `StagePoint{stage, sub}`，不允许“只保留阶段名”的等价迁移。

## 1. 类型草案

```cpp
enum class Stage {
    InitData, AfterInitEsolve, AfterInitAtRestart,
    BeforeStep, AfterStep, BeforeParticles, AfterParticles,
    BeforeFieldSolve, AfterFieldSolve, BeforeCollisions, AfterCollisions,
    BeforeDeposition, AfterDeposition, BeforeDiagnostics, AfterDiagnostics,
    BeforeCheckpoint, AfterCheckpoint, OnBreakSignal,
};

struct StagePoint {
    Stage stage;
    std::string sub;   // 空 = 阶段末；例如 "AfterFieldSolve.afterBpush"
};
```

同一 `StagePoint` 上多个 handler 按 `priority` 升序执行；`sub` 位点由父 handler 显式调用
`Runtime::run_subpoints("AfterFieldSolve.afterBpush")` 触发，保证顺序语义写死。

## 2. 18 个回调映射

| # | Python 回调 | StagePoint | sub | 语义 |
|---:|---|---|---|---|
| 1 | `beforestep` | `BeforeStep` | — | 每步开始，`istep` 递增前 |
| 2 | `afterstep` | `AfterStep` | — | 每步结束，诊断之后 |
| 3 | `afterdiagnostics` | `AfterDiagnostics` | — | 所有诊断 flush 后 |
| 4 | `onbreaksignal` | `OnBreakSignal` | — | 收到中断信号、退出前 |
| 5 | `oncheckpointsignal` | `BeforeCheckpoint` | — | checkpoint 写出前 |
| 6 | `allocdata` | `InitData` | `allocdata` | 场/粒子分配前 |
| 7 | `loadExternalFields` | `InitData` | `loadExternalFields` | 外部场加载后、Esolve 前 |
| 8 | `beforeInitEsolve` | `InitData` | `beforeInitEsolve` | 初始 Esolve 前 |
| 9 | `afterInitEsolve` | `AfterInitEsolve` | — | 初始 Esolve 后 |
| 10 | `afterInitatRestart` | `AfterInitAtRestart` | — | restart 初始化后 |
| 11 | `afterBpush` | `AfterFieldSolve` | `afterBpush` | B 推进后、E 推进前 |
| 12 | `afterEpush` | `AfterFieldSolve` | `afterEpush` | E 推进后 |
| 13 | `beforeParticles` | `BeforeParticles` | — | 粒子推进前 |
| 14 | `beforeCollisions` | `BeforeCollisions` | — | 碰撞前 |
| 15 | `beforeEsolvePoisson` | `BeforeFieldSolve` | `beforeEsolvePoisson` | 静电 Poisson 求解前 |
| 16 | `afterEsolvePoisson` | `AfterFieldSolve` | `afterEsolvePoisson` | 静电 Poisson 求解后 |
| 17 | `beforeImplicitSolve` | `BeforeFieldSolve` | `beforeImplicitSolve` | 隐式求解器入口（含 substep 前） |
| 18 | `afterImplicitSolve` | `AfterFieldSolve` | `afterImplicitSolve` | 隐式求解器出口 |

## 3. 与现有 `Evolve` 的对应

- `BeforeStep`/`AfterStep` 对应 `WarpX::Evolve` 循环体首尾。
- `BeforeFieldSolve`/`AfterFieldSolve` 包含 `afterBpush`/`afterEpush` 两个子位点；FDTD 的 B/E 推进之间必须可由父 handler 显式触发。
- 隐式求解器自带 ≤8 substep，`beforeImplicitSolve`/`afterImplicitSolve` 必须包住整个 `ImplicitSolver::OneStep`，子步进由 composite handler 内部调度（A3）。
- `AfterDiagnostics` 在 `MultiDiagnostics::FilterComputePackFlush` 之后。
- `BeforeCheckpoint` 在 `Checkpoint::Write` 之前；`OnBreakSignal` 与 checkpoint 信号分开。

## 4. HandlerSpec 草案（D7/D8/D9）

```cpp
struct HandlerSpec {
    std::vector<FieldKey>    reads_fields, writes_fields;
    std::vector<SpeciesKey>  reads_species, writes_species;
    std::vector<ResourceKey> reads_resources, writes_resources;
    std::vector<std::string> checkpoint_fields;  // 必须持久化的内部状态
    CapabilitySet            supports;           // dim/grid_type/flags，构造期校验
    std::vector<StagePoint>  stages;
    int                      priority = 0;
    std::string              alias_of;           // 空 = 非别名
};
```

## 5. 验收

- 18 行映射与 `Source/Python/callbacks.H` 的回调名一一对应。
- `afterBpush`/`afterEpush` 不能退化成 `AfterFieldSolve` 的无序执行。
- 每个回调至少一个最小输入用例（A0.7 扩展性门 G3 的前置）。
