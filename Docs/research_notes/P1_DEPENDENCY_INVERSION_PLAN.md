# P1：Core/Plugin 依赖倒置执行计划

## 1. 当前 Core 对 Plugin 的直接依赖

```text
Source/Core/Runtime/Runtime.cpp
  #include "WarpX.H"
  m_warpx = WarpX::Create();

Source/Core/Runtime/RuntimeConfig.cpp
  #include "WarpX.H"
  每个 getter 直接读 WarpX::*

Source/Core/Runtime/WarpXRuntimeData.*
  #include "WarpX.H"
  每个方法直接调用 WarpX

Source/Core/Runtime/WarpXHostServices.*
  #include "WarpX.H"
```

因此 Core 并不是真正独立。

## 2. 目标接口

```cpp
class IPicKernel
{
public:
    virtual ~IPicKernel () = default;

    virtual void initialize () = 0;
    virtual void evolve (int numsteps) = 0;
    virtual void finalize () = 0;
    virtual bool verbose () const = 0;

    virtual TimeState time_state (bool valid) const = 0;
    virtual FrameState frame_state () const = 0;
    virtual RuntimeConfig config_snapshot () const = 0;

    virtual ablastr::fields::MultiFabRegister& fields () = 0;
    virtual FieldRegistry& field_schema () = 0;
    virtual RuntimeData& data () = 0;
    virtual HostServices& host () = 0;

    virtual void set_plugin_host (PluginHost*) = 0;
    virtual void set_runtime_context (RuntimeContext*) = 0;
    virtual void register_collision (PluginHost&) = 0;
    virtual void allocate_collision_data () = 0;
    virtual void register_species (DataBus&) = 0;
    virtual void clear_runtime_context () = 0;
    virtual void reset () = 0;
};
```

## 3. 分步顺序

### 3.1 RuntimeConfig 快照

- 每个 getter 改为返回成员；
- `FromLegacy()` 移到 plugin，负责填充；
- Core 不再 include `WarpX.H`。

### 3.2 RuntimeData / HostServices 适配器

- `WarpXRuntimeData`、`WarpXHostServices` 移入 `Plugins/PicEm/RuntimeBridge/`；
- Core 只保留 `RuntimeData` / `HostServices` 抽象。

### 3.3 Collision

- `CollisionHandler` 创建移入 adapter；
- Runtime 只调用 `register_collision(host)` / `allocate_collision_data()`。

### 3.4 Runtime

- `Runtime` 持有 `std::unique_ptr<IPicKernel>`；
- 工厂 `make_pic_kernel(argc, argv)` 在 plugin 实现；
- `Core/Runtime` 不再出现 `WarpX` 类型。

## 4. 验收

```text
grep -R "WarpX" Source/Core 只允许注释与接口文档
删除 Source/Plugins/PicEm/WarpX.* 后 Core 库仍可编译
四维 + A/B 回归通过
```

## 5. 风险

- `RuntimeConfig` 有 50+ getter，需要逐项快照；
- `TimeState` / `FrameState` 也直接读 WarpX；
- collision、diagnostics、particles 都通过 WarpX 类型；
- 一次性替换会产生大量编译错误，必须按 3.1 → 3.4 逐步。

## 6. 当前未执行

本轮只完成四维构建与路径文档，没有开始 3.1。

在 `RuntimeConfig` 仍是 live WarpX 视图时，不能声称 Core 已独立。
