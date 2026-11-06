# A5.1/A5.2 PluginHost 与依赖拓扑

## A5.1 骨架

`PluginHost` 已提供：

```cpp
add(IHandler)          // 记录 handler 与 spec
declare(HandlerSpec)   // 仅声明
at(Stage)              // 取该阶段有序 handler
ordered_specs()        // 有序 spec
size()
```

`add` 会拒绝同一 `StagePoint` 上两个 writer 写同一字段。

## A5.2 依赖拓扑

`order_indices()`：

1. 先按 `StagePoint` + `priority` 稳定排序；
2. 每个相同 `StagePoint` 组内建依赖边：
   - `A.writes_fields ∩ B.reads_fields` → A 先于 B；
   - `A.writes_species ∩ B.reads_species` → A 先于 B；
3. Kahn 拓扑排序，同层按 priority/插入顺序稳定选择；
4. 有环时抛：

```text
PluginHost: dependency cycle at StagePoint (...)
```

`ordered()` / `ordered_specs()` / `at()` 都使用该顺序。

## 测试

- writer 先于 reader；
- 同一字段双 writer 抛错；
- 双向读写环抛错。

```text
默认 runtime 5/5 PASS
FFT runtime 7/7 PASS
ASAN runtime 5/5 PASS
```

### 聚合字段名匹配

spec 中可能声明聚合名（`Efield_fp`），registry 中存分量（`Efield_fp_x`）。
拓扑边现在把二者视为同一依赖：

```cpp
a == b
|| b starts with a + "_"
|| a starts with b + "_"
```

否则 `PicFieldSolveHandler` 的 `writes_fields={"Efield_fp"}` 无法和
读取 `Efield_fp_x` 的 handler 建立顺序。

测试覆盖该情况。

## A5.3b 诊断包

新增：

```text
DiagnosticsHandler   Stage::Diagnostics
DiagnosticsPackage   register_handlers()
```

执行：

```text
RunDiagnosticsStage(step)
  -> PluginHost::at(Diagnostics)
  -> DiagnosticsHandler
  -> HostServices::computeDiagnostics(step)
  -> WarpX::FlushDiagnostics(step)
```

`WarpXEvolve` 原来直接调用 `multi_diags->FilterComputePackFlush(step)`，
现在改走 `RunDiagnosticsStage`。

禁用 handler 时回退到原来的直接调用。

runtime / restart 测试通过。

## A5.3a 内置包注册集中化

Runtime 不再直接 `make_shared`：

```cpp
PicEmPackage::register_handlers(*m_plugin_host);
CollisionPackage::register_handler(*m_plugin_host, m_collision_handler.get());
```

`PicEmPackage` 拥有 PIC 三个 handler 的构造；
`CollisionPackage` 拥有 CollisionHandlerAdapter 的构造。
包之外只传 context/handler 指针。

A/B 结果仍为逐位一致（先前记录）。

## A5 收口

| 项 | 状态 |
|---|---|
| A5.1 PluginHost | ✅ |
| A5.2 拓扑/冲突 | ✅ |
| A5.3 内置适配 | ✅ PIC/Collision/Diagnostics |
| A5.3a 包注册 | ✅ |
| A5.3b 诊断包 | ✅ |
| A5.4 外挂示例 | ✅ |
| A5.5 动态加载 | ✅ |
| A5.6 A/B 门 | ✅ |

medium/PEC 已确认属于 FieldSolve 内部，不再强行作为独立 handler。

## A5.6 自动 A/B 门

新增：

```text
Tools/automation/gates/a5_ab.py
```

对以下算例在 `OMP_NUM_THREADS=1` + 固定 seed 下比较
启用/禁用 PluginHost：

```text
langmuir (diag1000080)
collision (diag1000150)
embedded_boundary_cube (diag1000114)
```

字段 `Ex/Ey/Ez/Bx/By/Bz` 必须全部 `array_equal`。

实跑：

```text
A/B PASS langmuir
A/B PASS collision
A/B PASS eb_cube
```
