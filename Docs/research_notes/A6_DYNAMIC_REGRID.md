# A6a 动态 regrid 实现

## 已实现

### 1. 触发

`warpx.regrid_int > 0` 时，在每步末尾：

```cpp
regrid(0, cur_time);
multi_diags->ResizeLevels(finest_level);
mypc->AllocData();
mypc->Redistribute();
mypc->defineAllParticleTiles();
```

`warpx.refine_start_time` / `warpx.refine_stop_time` 控制何时允许 tagging，
用于“先单层、后加密、再撤层”的测试。

### 2. 新层

`WarpX::MakeNewLevelFromCoarse`：

- `AllocLevelData`
- `InitLevelData`
- `InterpFromCoarseLevel` 填充 E/B/J/rho
- `m_level_resources` 按 `max(maxLevel(), lev)+1` 扩展

### 3. 诊断

新增：

```cpp
Diagnostics::ResizeLevels(int new_finest)
FullDiagnostics::ResizeLevels
MultiDiagnostics::ResizeLevels
```

`FullDiagnostics` 为新 level：

- `InitializeBufferData`
- `InitializeFieldFunctors`
- 更新 `nlev/nlev_output`

### 4. 资源断言

`RescaleCosts` 从：

```cpp
size == finest_level + 1
```

改为：

```cpp
size >= finest_level + 1
```

因为 regrid 期间资源可能预分配到 `maxLevel()+1`。

## 官方测试

新增：

```text
Examples/Tests/dynamic_regrid/
  inputs_test_2d_dynamic_regrid
  inputs_test_2d_dynamic_regrid_remove
  analysis.py
  analysis_removed.py
```

测试 1：0 层 -> 1 层，最终 `max_level == 1`。

测试 2：1 层 -> 0 层，最终 `max_level == 0`。

## 验证

```text
test_2d_dynamic_regrid.run/analysis PASS
test_2d_dynamic_regrid_remove.run/analysis PASS
默认 runtime 5/5 PASS
FFT runtime 7/7 PASS
ASAN runtime + dynamic regrid 9/9 PASS
```

## 限制

- 当前只验证 FullDiagnostics；
- BTD 的 `ResizeLevels` 仍是 no-op；
- `MakeNewLevelFromCoarse` 只插值 E/B/J/rho，未覆盖所有 aux/PML 场；
- 未覆盖 GPU。
