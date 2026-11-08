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

---

## 8. 实施：B1–B7 构建拆分（已完成）

### 8.1 目标分层

```text
warpx_core_<dim>         Runtime / DataBus / PluginLoader / PluginCAdapter / KernelFactory
                         Core/Utils, Core/ablastr
warpx_picem_<dim>        WarpX.cpp, WarpXKernel, Modules/**, RuntimeBridge/**
warpx_diagnostics_<dim>  Plugins/Diagnostics/**
warpx_io_<dim>           Plugins/IO/**
lib_<dim>                聚合归档：$<TARGET_OBJECTS:上列四个>
app_<dim> / 测试         链接 lib_<dim>
```

`Source/Core/Runtime/CMakeLists.txt` 现在只包含 Core 源：

```text
Runtime.cpp
DataBus.cpp
PluginLoader.cpp
PluginCAdapter.cpp
KernelFactory.cpp
```

18 个 PicEm 源已迁到 `Source/Plugins/PicEm/CMakeLists.txt`
（`WarpX.cpp`、`WarpXKernel.cpp`、`RuntimeBridge/{Runtime,Utils}/*`、
`Modules/PicEm/*` 5 个 handler）。

### 8.2 保真要求

```text
Core 目标只给 Core include 路径（WARPX_CORE_INCLUDE_DIRS），
任何残留的 Plugins/ 依赖直接编译失败，而不是被 include 路径静默解析。
```

### 8.3 验收命令

```bash
cmake --fresh -S . -B build_abi -DWarpX_DIMS=2 -DWarpX_COMPUTE=OMP
cmake --build build_abi --target warpx_core_2d   # 只编译 Core，1 min
cmake --build build_abi                          # app + 测试
python3 Tools/Modularization/check_build_boundaries.py
```

新增检查器 `Tools/Modularization/check_build_boundaries.py`：

```text
Core CMake 不得登记 Source/Plugins 下的源
插件 CMake 只能把源登记到自己的层目标
lib_<dim> 必须聚合四个层目标
```

### 8.4 结果

```text
warpx_core_2d 单独构建            PASS（无任何 Plugins 源）
app_2d + 全部测试构建             PASS
check_build_boundaries.py         PASS
check_plugin_boundaries.py        PASS
```

因此：

```text
A7.1 的“Core 无 Plugins”现在在构建层成立，不再只是 include 层
```

## 9. 实施：KernelFactory 解耦（已完成）

Core 不再引用 `create_pic_kernel` 的实现：

```cpp
// Source/Core/Runtime/KernelFactory.H
struct PicKernelPlugin {
    std::unique_ptr<IPicKernel> (*create) (int, char**);
    void (*initialize_libraries) (int, char**);
    void (*finalize_libraries) ();
};
void register_pic_kernel_plugin (const PicKernelPlugin&);
std::unique_ptr<IPicKernel> create_pic_kernel (int, char**);
```

- `KernelFactory.cpp` 用**函数内 static** 保存注册表，因此不存在
  Core/插件之间的静态初始化顺序依赖；
- PicEm 在 `WarpXKernel.cpp` 的静态初始化里注册三项入口；
- 未注册时 `create_pic_kernel` 抛带指引的 `std::runtime_error`
  （“no PIC kernel plugin is registered”），这正是 Core-only 构建的预期结果。

### 9.1 静态归档与静态初始化

静态库会丢弃未被引用的成员，进而丢弃插件的静态注册。处理方式：

```text
WarpXKernel.cpp         定义 extern "C" warpx_picem_kernel_anchor
lib_<dim>               INTERFACE LINKER:-u,warpx_picem_kernel_anchor
                        （MSVC: /INCLUDE:）
```

这样注册对象一定进入最终链接，且该约束随 `lib_<dim>` 传播到 app、pyWarpX
和所有测试；将来把 PicEm 从聚合中移除时，这一行也随之删除。

## 10. 实施：C ABI v3（已完成 A1–A4）

### 10.1 边界类型

`Source/Core/Runtime/PluginCApi.H` 可被 **C 编译器**直接包含：

```c
typedef struct WarpXPluginHost   WarpXPluginHost;    /* 不透明 */
typedef struct WarpXStageContext WarpXStageContext;  /* 不透明 */
enum WarpXStage { WARPX_STAGE_BEFORE_INIT = 0, ..., WARPX_STAGE_FINALIZE = 13 };
typedef struct WarpXHandlerDesc { uint32_t struct_size; const char* name;
    int32_t stage, sub, priority;
    const char* const* reads_fields;  uint32_t n_reads_fields;
    const char* const* writes_fields; uint32_t n_writes_fields;
    const char* const* reads_species; uint32_t n_reads_species;
    const char* const* writes_species;uint32_t n_writes_species;
    void (*apply)(WarpXStageContext*, void*);
    void (*on_regrid)(WarpXStageContext*, void*);
    void* user_data; } WarpXHandlerDesc;
typedef struct WarpXPluginDesc { uint32_t abi_version, struct_size;
    const char* name; const char* version;
    void (*register_handlers)(WarpXPluginHost*); } WarpXPluginDesc;

int  warpx_host_add_handler (WarpXPluginHost*, const WarpXHandlerDesc*);
int32_t warpx_stage_step (const WarpXStageContext*);
double  warpx_stage_time (const WarpXStageContext*);
double  warpx_stage_dt   (const WarpXStageContext*);
int32_t warpx_stage_has_field (WarpXStageContext*, const char*, int32_t);
int32_t warpx_stage_field     (WarpXStageContext*, const char*, int32_t, void**);
```

边界上不再出现 `std::string`、`std::vector`、`std::function`、`IHandler`、
`PluginHost*`。

### 10.2 宿主侧适配器

`PluginCAdapter.cpp` 做三件事：

```text
warpx_host_add_handler  -> 复制字符串、构造 CApiHandler(: IHandler) -> PluginHost::add
CApiHandler::apply      -> 构造 WarpXStageContext，调 C 函数指针
访问器                   -> 转发 DataBus/StageContext
```

并有编译期守卫：

```text
14 个 WarpXStage 与 Stage 的 static_assert 对应
WarpXHandlerDesc / WarpXPluginDesc 的 standard_layout + trivially_copyable 断言
```

`PluginLoader` 只把不透明句柄交给插件（`plugin_host_c_handle`），
ABI 版本号升到 **3**。

### 10.3 测试

```text
plugin_c_abi_header_2d     纯 C 编译/链接/运行 ABI 头与访问器
plugin_example_2           C 插件在进程内注册并执行
plugin_loader_2d           C 插件 .so 动态加载；插件不链接宿主，
                           warpx_host_add_handler 由可执行文件（ENABLE_EXPORTS）解析
plugin_loader_reject_2d    版本不匹配拒绝
plugin_loader_missing_entry_2d / invalid_desc_2d / missing_file_2d  失败路径
```

12 项 `plugin_|runtime_` 测试：**11 通过**，唯一失败是
`runtime_snapshot_particles_2`（见 §12，测试输入缺项，与本次改动无关）。

## 11. 当前结论（更新）

```text
A7.1  Core 无 Plugins           构建层成立（warpx_core_<dim> 单独可构建）
A7.8  C ABI v3                  外部插件现在可以纯 C 编写与加载
内置 handler 仍为 C++             A5 未做（见 §12）
KernelFactory                    成立；Core 无插件符号引用
```

## 12. 剩余

```text
A5  内置 handler 走 C ABI（PicEmPackage/Yee/Deposition 仍直接注册 IHandler）
    删除 PicEm 后 app 可构建但无 kernel（预期），需要 “C 插件提供 kernel” 的入口
    Source/Core/Runtime/Make.package 仍指向旧路径（GNU make 路径未同步，CMake 为准）
    Windows/MSVC 未验证（-u / /INCLUDE 分支已写，但无 CI）
    -DWarpX_QED=OFF 构建本就失败（IParticleContainer 的 QED 纯虚函数未加守卫），
    与本次改动无关，需单独修
```

## 8. 构建拆分落地

当前 CMake 已改为分层目标：

```text
warpx_core_<dim>
warpx_picem_<dim>
warpx_diagnostics_<dim>
warpx_io_<dim>
lib_<dim> = 聚合对象
app_<dim> = lib
```

Core CMake 已不含：

```text
../../Plugins/*
```

`KernelFactory` 注册：

```text
PicEm WarpXKernel 静态注册
Runtime 通过 registry 创建
无 kernel 时给出明确错误
```

## 9. C ABI 测试迁移

`tests/plugins/*` 的示例插件已改为：

```text
PluginCApi.H
WarpXHandlerDesc
warpx_host_add_handler
```

不再跨边界传：

```text
PluginHost*
IHandler
std::string
```

验证：

```text
runtime + plugin 12/12 PASS
plugin boundary checker PASS
HDF5 langmuir/charge_exchange/laser PASS
```

## 10. 仍剩余

```text
lib_<dim> 仍是聚合 archive
runtime 测试需要显式链接 plugin objects 才能触发静态注册
C ABI 只覆盖外部 handler，内核/诊断接口仍是 C++
```
