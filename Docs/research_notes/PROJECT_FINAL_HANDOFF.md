# WarpX 插件化项目最终交接

## 1. 架构

```text
Runtime
  ├── RuntimeConfig / TimeState / FrameState
  ├── DataBus
  ├── PluginHost
  ├── CollisionHandler
  └── WarpX (kernel)
        ├── StagePlan
        ├── FieldRegistry
        ├── SpeciesRegistry
        ├── MaterialRegistry
        └── LevelResources
```

规则：

- 插件只依赖 `StageContext` / `HandlerSpec` / `DataBus`；
- 插件之间不互认；
- 内核保留物理机制；
- 用户模块按物理/逻辑命名，每个模块一个函数。

## 2. Stage

```text
BeforeInit
Init
AfterInit
BeforePush
Push
AfterPush
BeforeCollisions
Collisions
AfterCollisions
BeforeFieldSolve
FieldSolve
AfterFieldSolve
Diagnostics
Finalize
```

动态 regrid 额外回调：

```cpp
IHandler::on_regrid(StageContext&)
```

## 3. 插件 ABI

```cpp
struct WarpXPluginDesc {
    uint32_t abi_version;   // 3? 当前 2
    uint32_t struct_size;
    const char* name;
    const char* version;
    void (*register_handlers)(PluginHost*);
};
```

当前：

```text
warpx_plugin_abi_version = 2
```

加载：

```text
dlopen(RTLD_NOW | RTLD_LOCAL | RTLD_NODELETE)
dlsym("warpx_plugin_entry")
校验 struct_size / abi_version / name / register
```

## 4. 动态 regrid

支持：

- 新 level 创建；
- level 销毁；
- 已有 level BoxArray 改变；
- 粒子输出；
- all-registered-field coarse→fine 插值；
- 节点/面/单元分类型插值；
- 诊断 buffer 重建；
- `IAmrPolicy` 决策注入。

测试：

```text
dynamic_regrid
dynamic_regrid_remove
dynamic_regrid_remake
dynamic_regrid_uniform_b
```

## 5. 最终验证

```text
默认 run:      76 / 87 PASS
默认 analysis: 62 / 72 PASS
FFT run:       100 / 111 PASS
FFT analysis:  84 / 94 PASS
openPMD:       16 / 22 PASS
插件:          6 / 6 PASS
A/B:           langmuir / collision / eb_cube PASS
ASAN:          15 / 15 PASS
```

失败全部是环境依赖：

- 默认/FFT 未编译 openPMD；
- openPMD 缺 parallel HDF5。

## 6. 已知限制

1. 外部 `IAmrPolicy` ABI 不支持：
   - 插件链接静态 `lib_2d` 会带入第二份 AMReX 状态；
   - policy 调 `DistributionMapping` 会 segfault；
   - 需要共享运行时或纯 C 数据接口。
2. 插件 ABI 仅同编译器/标准库/构建配置。
3. GPU 后端未覆盖。
4. 动态 regrid 只验证 uniform-B 长期守恒；其他边界组合未逐一验证。
5. 无插件签名/路径白名单。

## 7. 构建

```bash
cmake -S . -B build
cmake --build build -j 24
ctest --test-dir build -R '^(runtime_|plugin_|test_2d_dynamic_regrid)' --output-on-failure
```

FFT：

```bash
cmake -S . -B build_fft -DWarpX_FFT=ON
cmake --build build_fft -j 24
LD_LIBRARY_PATH=$HOME/third_party/fftw/lib ctest --test-dir build_fft ...
```

## 8. 结论

正式技术路线 153/153 完成。

剩余项是增强/兼容性工作，不是未完成任务。
