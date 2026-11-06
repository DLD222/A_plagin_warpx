# A5.4 外挂插件示例

## 目标

验证一个**独立 .cpp 模块**实现 `IHandler` 时，不需要修改内核源码。

## 文件

```text
tests/plugins/
  example_plugin.H
  example_plugin.cpp
  test_plugin_example.cpp
  CMakeLists.txt
```

`example_plugin.cpp`：

- 实现 `ExamplePluginHandler : IHandler`；
- `spec()` 返回 `example_plugin`，`Stage::Diagnostics`；
- `apply()` 只累加计数，不做物理。

`test_plugin_example.cpp`：

- 构造 `PluginHost`；
- `add(make_example_plugin_handler())`；
- 校验 `size()`、`ordered_specs().front().name`；
- 调用 `apply()`，校验计数为 1。

## 构建

```cmake
add_executable(test_plugin_example_${SD}
    test_plugin_example.cpp example_plugin.cpp)
target_link_libraries(... lib_${SD})
```

内核源码未增加任何分支或注册项。

## 运行

```text
plugin_example_2 PASS
```

## 限制

这是**编译期链接**的示例，不是运行期 `.so` 加载。
`dlopen`/C ABI/版本校验属于 A5.5。
