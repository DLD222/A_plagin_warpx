# 技术路线完成状态（2026-11-07）

## 任务图

```text
A0 8/8
A1 53/53
A2 20/20
A3 52/52
A4 7/7
A5 8/8
A6 5/5
总计 153/153
```

## 关键完成项

### A0–A2

- Runtime 所有权
- RuntimeContext / DataBus
- FieldRegistry / SpeciesRegistry / MaterialRegistry
- LevelResources
- TimeState / FrameState

### A3

- Stage/StagePoint/StagePlan
- PluginHost 注册、拓扑、冲突检测
- PIC/Collision 包
- 标记/诊断解耦

### A4

- checkpoint schema
- 字段注册驱动
- restart 逐位

### A5

- PluginHost 外挂示例
- dlopen C ABI 加载
- 自动 A/B 门
- Diagnostics handler

### A6

- A6a 动态 regrid 创建/销毁
- A6.1 任意 BoxArray RemakeLevel
- A6.2 通用 RemakeLevel
- A6.3 IAmrPolicy
- A6.4 on_regrid + 物理门

## 最终验证

```text
默认 run:      75 / 86 PASS
默认 analysis: 61 / 71 PASS
FFT run:       99 / 110 PASS
FFT analysis:  83 / 93 PASS
openPMD:       16 / 22 PASS
修改功能:      33 / 33 PASS
插件/runtime: 7 / 7 PASS
A/B:          PASS
ASAN:         15 / 15 PASS
```

失败仅来自环境：

- 默认/FFT 未启用 openPMD；
- openPMD 缺 parallel HDF5。

## 已知限制

- `dynamic_regrid_remake` 关闭粒子输出：
  - 已有 level 改变 BoxArray 时，旧 particle tile 的 grid id 未重建；
- BTD 的 `ResizeLevels` 仍为 no-op；
- `MakeNewLevelFromCoarse` 只插值 E/B/J/rho；
- IAmrPolicy 无外部 `.so` ABI；
- 未覆盖 GPU。

这些限制均已记录，不伪装为通过。
