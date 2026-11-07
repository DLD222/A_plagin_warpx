# 模块目录与“一个单位模块一个 CPP”设计

## 1. 最终目标

用户新增物理能力时，只写少量 CPP：

```text
Source/Modules/<Domain>/<Module>/
  <Module>Package.H/.cpp        // 注册入口
  <Unit1>/<Unit1>.H/.cpp        // 一个单位模块 = 一个物理/逻辑功能
  <Unit2>/<Unit2>.H/.cpp
```

例：

```text
Source/Modules/PicEm/
  PicEmPackage.H/.cpp
  ParticlePush/ParticlePushHandler.H/.cpp
  Deposition/DepositionHandler.H/.cpp
  FieldSolve/FieldSolveHandler.H/.cpp
```

要求：

1. 一个单位模块只做一件事；
2. 单位模块只依赖 `StageContext`（DataBus + HostServices + Time）；
3. 模块包只负责注册，不含物理；
4. 内核不 include 具体实现；
5. 模块可被输入/环境变量禁用；
6. 删除该文件夹后，其余模块仍可编译/运行。

## 2. 当前已完成

### 2.1 目录

```text
Source/Modules/PicEm/
  PicEmPackage.H/.cpp
  ParticlePush/ParticlePushHandler.H/.cpp
  Deposition/DepositionHandler.H/.cpp
  FieldSolve/FieldSolveHandler.H/.cpp
```

### 2.2 单位模块

| 单位 | 文件 | Stage | 职责 |
|---|---|---|---|
| `particle_push` | `ParticlePushHandler.cpp` | Push | 推进粒子并沉积 |
| `deposition.charge_current` | `DepositionHandler.cpp` | AfterPush | 沉积收尾 |
| `field_solve.<algo>` | `FieldSolveHandler.cpp` | FieldSolve | 场推进 |

### 2.3 模块选择

`field_solve.<algo>` 由 Runtime 根据：

```cpp
electrostatic_solver_id()
electromagnetic_solver_id()
```

选择并注册。

### 2.4 禁用内置模块

```bash
WARPX_DISABLE_BUILTIN_MODULES=particle_push,field_solve.yee
```

用户插件可注册替代实现。

## 3. 用户新增模块的操作

### 3.1 内核内模块

1. 建目录；
2. 写 `<Unit>.H/.cpp`；
3. 写 `<Module>Package.H/.cpp` 注册；
4. 在 `Source/Runtime/CMakeLists.txt` 加 CPP；
5. 编译。

### 3.2 外部 `.so` 插件

1. 写 `extern "C" warpx_plugin_entry`；
2. 在 `register_handlers` 中 `host->add(...)`；
3. `WARPX_PLUGIN_PATH=<so>` 加载。

不需要改 `Runtime` 或 `WarpX`。

## 4. 注意事项

1. **一个 CPP 一个单位**：不要把多个物理动作塞进一个 `apply()`。
2. **不要持有其他模块指针**：只拿 `StageContext`。
3. **读写必须声明**：`HandlerSpec` 的字段/物种决定拓扑与冲突。
4. **双写拒绝**：同 Stage 同字段只能有一个 writer；替换内置模块时先禁用。
5. **A/B 必须逐位**：默认路径与旧实现一致。
6. **命名是 API**：模块名一旦发布，改名要兼容。
7. **文件夹是物理域，不是实现细节**：`FieldSolve/` 下再按算法分。
8. **当前 `particle_push` 仍是推进+沉积组合**：它是过渡模块，
   下一步需要把 `HostServices` 拆成 `pushParticles` 与 `deposit` 才能成为
   严格单一功能单位。

## 5. 当前限制

- 只有 `PicEm` 完成了文件夹化；
- `Diagnostics`、`Collisions` 仍分散在旧目录；
- `particle_push` 尚未拆成 pusher + deposition 两个单位；
- `field_solve.*` 仍调用 `WarpX` 内部统一分派，未拆成独立求解器实现；
- 外部 `.so` 目前只能注册 handler，不能替换 `IAmrPolicy`。

## 6. 下一步

```text
1. HostServices: pushParticles / depositCurrent 分离
2. particle_push -> pusher.<algo> + deposition.current
3. FieldSolve -> YeeMaxwellSolver 独立实现
4. Diagnostics/Collisions 文件夹化
5. 用户示例：新增一个源项模块，只写 2 个 CPP
```
