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

## 尚未完成

- A5.3 内置插件适配的逐位 A/B；
- A5.4 外挂插件示例；
- A5.5 运行期 .so 加载；
- A5.6 A5 回归门。
