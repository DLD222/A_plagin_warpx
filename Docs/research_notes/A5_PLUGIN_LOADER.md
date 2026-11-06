# A5.5 运行期插件加载

## C ABI

`Source/Runtime/PluginABI.H`：

```cpp
struct WarpXPluginDesc {
    std::uint32_t abi_version;
    std::uint32_t struct_size;
    const char* name;
    const char* version;
    void (*register_handlers)(PluginHost*);
};

extern "C" using WarpXPluginEntry = const WarpXPluginDesc* (*)();
```

插件导出：

```cpp
extern "C" const WarpXPluginDesc* warpx_plugin_entry();
```

## Loader

`PluginLoader::load(path, host)`：

1. `dlopen(RTLD_NOW | RTLD_LOCAL)`；
2. `dlsym("warpx_plugin_entry")`；
3. 校验 descriptor 非空、name/register 非空；
4. 校验 `struct_size == sizeof(WarpXPluginDesc)`；
5. 校验 `abi_version == 2`；
5. 调用 `register_handlers(&host)`；
6. 不 `dlclose`，handler 代码仍在库内。

失败抛 `std::runtime_error`，不 abort。

## 主机侧

Runtime 构造时：

```cpp
if (const char* path = std::getenv("WARPX_PLUGIN_PATH")) {
    PluginLoader::load(path, *m_plugin_host);
}
```

## 示例插件

`tests/plugins/dynamic_example_plugin.cpp`：

- 导出 `warpx_plugin_entry`；
- handler `dynamic_example_plugin` 在 `Stage::Finalize`。

`tests/plugins/test_plugin_loader.cpp`：

- 直接 `PluginLoader::load`；
- 校验 host 只有一个 handler 且 name 正确。

## 验证

```text
plugin_example_2 PASS
plugin_loader_2d PASS
WARPX_PLUGIN_PATH=<so> runtime_snapshot_particles_2d PASS
```

## 限制

- ABI 仅在同一编译器/标准库/构建配置内保证；
- 使用 `RTLD_NODELETE`，防止拒绝插件时 `dlclose` 卸载运行中的符号；
- 未实现插件卸载；
- 未实现签名/路径白名单；
- `PluginHost` 跨库传递依赖相同的 C++ ABI。
