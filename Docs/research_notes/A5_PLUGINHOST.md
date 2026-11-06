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

## 尚未完成

- A5.3 内置插件适配的逐位 A/B；
- A5.4 外挂插件示例；
- A5.5 运行期 .so 加载；
- A5.6 A5 回归门。
