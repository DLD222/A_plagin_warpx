# A7 构建与 ABI 审计

## 1. 构建层：Core 仍编译插件源

确认：

```text
Source/Core/Runtime/CMakeLists.txt
```

仍包含 18 个 PicEm 源文件：

```text
WarpXKernel.cpp
FieldSolveHandler.cpp
YeePackage.cpp
YeeFieldSolveHandler.cpp
DepositionHandler.cpp
ParticlePushHandler.cpp
PicEmPackage.cpp
WarpXRuntimeData.cpp
WarpXHostServices.cpp
RuntimeConfig.cpp
FrameState.cpp
LegacyRuntimeFactory.cpp
LevelResources.cpp
CollisionPackage.cpp
RuntimeConfigSnapshotBridge.cpp
```

后果：

```text
Core 库 lib_* 不是纯 Core
删除 PicEm 插件后 Core 无法构建
A7.1 的“Core 无 Plugins”只在 include 层成立
```

正确目标：

```text
warpx_core_<dim>
warpx_picem_<dim>
warpx_diagnostics_<dim>
warpx_io_<dim>
app_<dim> = core + plugins
```

## 2. 构建层修复步骤

```text
B1. 新建 warpx_core_<dim>，只含 Core 源
B2. 新建 warpx_picem_<dim>，含 PicEm 与 RuntimeBridge
B3. 新建 warpx_diagnostics_<dim>
B4. 新建 warpx_io_<dim>
B5. app_<dim> 链接 core + plugins
B6. 测试目标链接对应插件
B7. 删除 Core CMake 中所有 ../../Plugins/*
```

难点：

```text
Runtime.cpp 直接调用 create_pic_kernel
Core 静态库会留下未解析符号
```

修复：

```text
Core 只声明 KernelFactory
插件在静态初始化/显式 register 时设置 factory
Runtime 通过 factory 指针创建 kernel
```

## 3. ABI 层：C 名义、C++ 实质

确认：

```text
WarpXPluginDesc 是 POD
但以下跨边界类型是 C++：
  PluginHost*
  IHandler
  HandlerSpec
  std::string
```

后果：

```text
外部 .so 必须同编译器
必须同 STL
必须同 WARPX_DIMS
必须同 _GLIBCXX_USE_CXX11_ABI
不是稳定 C ABI
```

## 4. ABI v3 目标

### 纯 C 边界

```c
typedef struct WarpXPluginDesc {
    uint32_t abi_version;
    uint32_t struct_size;
    const char* name;
    const char* version;
    void (*register_handlers)(WarpXPluginHost*);
} WarpXPluginDesc;
```

### 不透明句柄

```c
typedef struct WarpXPluginHost WarpXPluginHost;
typedef struct WarpXStageContext WarpXStageContext;
```

### C handler 描述

```c
typedef struct WarpXHandlerDesc {
    const char* name;
    int stage;
    int priority;
    const char* const* reads_fields;
    const char* const* writes_fields;
    const char* const* reads_species;
    const char* const* writes_species;
    void (*apply)(WarpXStageContext*);
} WarpXHandlerDesc;
```

### 数据访问函数

```c
int warpx_stage_step(WarpXStageContext*);
double warpx_stage_time(WarpXStageContext*);
double warpx_stage_dt(WarpXStageContext*);
void warpx_stage_field(WarpXStageContext*, const char* name, int lev, void** data);
```

禁止跨边界：

```text
std::string
std::vector
std::function
IHandler vtable
PluginHost*
```

## 5. ABI 修复步骤

```text
A1. 新增 C ABI 头
A2. C 包装层：C -> C++ PluginHost
A3. 外部插件测试用纯 C 编译
A4. 保留 C++ 内置 handler 内部
A5. 内置插件也通过 C ABI 注册
A6. 验证不同编译器/STL 的 .so
```

## 6. 优先级

```text
1. 构建拆分（B1-B7）
2. KernelFactory 解耦
3. C ABI 包装（A1-A3）
4. 内置插件走 C ABI（A5）
```

## 7. 当前结论

```text
A7.1 “Core 无 Plugins” 只在 include 层成立
A7.8 “C ABI” 目前不成立
```

这两项不完成：

```text
不能宣布插件可独立分发
不能宣布插件可跨编译器加载
```
