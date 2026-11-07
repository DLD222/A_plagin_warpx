# L1 物理/逻辑模块修正报告

## 1. 目标

把隐式的“枚举 + if/else”模块层提升为**显式 L1 模块**：

- 一个模块 = 一个物理/逻辑能力；
- 模块名可读、可枚举、可测试；
- 阶段、读写字段/物种由 `HandlerSpec` 声明；
- 注册表不依赖实现头；
- 替换算法时不改内核分派。

目标不是立即重写 WarpX，而是先建立**正确的模块边界与命名**，再逐步替换实现。

## 2. 修正前

```text
pic_push
pic_finalize_deposition
pic_field_solve
```

问题：

- `pic_field_solve` 一个名字覆盖 Yee/CKC/PSATD/ECT/Hybrid/静电；
- `pic_finalize_deposition` 不说明是电荷还是电流；
- `pic_push` 同时包含推进、沉积、当前；
- 名字是“实现阶段”，不是“物理能力”。

## 3. 本次修改

### 3.1 模块命名

| 旧名 | 新名 | 物理/逻辑定位 |
|---|---|---|
| `pic_push` | `particle_push` | 粒子推进 |
| `pic_finalize_deposition` | `deposition.charge_current` | 电荷/电流沉积收尾 |
| `pic_field_solve` | `field_solve.<algo>` | 场求解，按算法命名 |

场求解模块：

```text
field_solve.yee
field_solve.ckc
field_solve.psatd
field_solve.ect
field_solve.hybrid_pic
field_solve.electrostatic
field_solve.none
```

### 3.2 选择机制

`Runtime` 根据：

```cpp
config.electrostatic_solver_id()
config.electromagnetic_solver_id()
```

选择 `field_solve.<algo>`，再注册对应 handler。

### 3.3 测试

```text
默认 Yee:  field_solve.yee
PSATD:    field_solve.psatd
```

`runtime_snapshot` 检查选中的模块名存在。

## 4. 实现方法

```cpp
PicEmPackage::register_handlers(host, field_solver_module)
  -> particle_push
  -> deposition.charge_current
  -> PicFieldSolveHandler(field_solver_module)
```

`PicFieldSolveHandler` 的 `HandlerSpec.name` 由构造参数决定，
不再硬编码。

## 5. 注意事项

1. **名称是 API**：外部插件若引用旧名会失效；当前无外部稳定 API，允许改名。
2. **行为未变**：`field_solve.*` 仍调用同一个 `HostServices::fieldSolve`，
   内部仍分派到 WarpX。命名层先正确，实现层后续拆。
3. **不要声称已拆实现**：本次只完成 L1 命名/选择。
4. **A/B 必须保持**：改名不影响数值，A/B 继续通过。
5. **物种依赖**：`particle_push` 不再写死 `electrons/ions/photons`。

## 6. 验证

```text
默认 runtime + dynamic + plugin 23/23 PASS
FFT runtime + dynamic + plugin 25/25 PASS
ASAN runtime + dynamic + plugin 23/23 PASS
runtime_psatd_2 PASS
```

## 7. 下一步

继续按物理功能拆：

```text
particle_push
  -> pusher.boris
  -> pusher.vay
  -> pusher.higuera_cary

deposition.charge_current
  -> deposition.charge
  -> deposition.current

field_solve.yee
  -> 独立 YeeMaxwellSolver 实现
```

每一步都必须：

- 保持 A/B；
- 保持输入参数；
- 有独立测试；
- 不在内核里增加新的 if/else。
