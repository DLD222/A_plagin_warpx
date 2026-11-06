# 对四份 Review 文档的逐项回复

基线：`main @ f236800` 之后的本轮修复。

状态：
- ✅ 已修复/已确认不是 bug
- 🟡 部分修复
- ❌ 未修复

## H 级

| 项 | 状态 | 处理 |
|---|---|---|
| H1 Legacy 静态悬空 | ✅ | `WarpXRuntimeData` 改为非拥有 `WarpX*`；`Legacy()` 每次刷新指针 |
| H2 Runtime 仍是单例消费者 | ✅ | `WarpX::Create()` + `m_warpx`；Runtime.cpp 无 `GetInstance` |
| H3 诊断残留 GetInstance | 🟡 | 主路径已迁移；6 处 legacy fallback 保留给 Python/无上下文路径，属有意兼容 |
| H4 Services 空指针接口 | ✅ | 删除 `Services.H` 与 `RuntimeContext.services` |
| H5 测试陈旧文件/共享目录 | ✅ | 每个 runtime 测试独立 WORKING_DIRECTORY；独立干净目录单跑通过 |
| H6 2D 输入注册所有维度 | ✅ | `tests/runtime/CMakeLists.txt` 仅 2D |
| H7 CI 只挂 main | ✅ | `.github/workflows/route-gates.yml` 改为 `[main, development, "trae/*"]` |

## M 级

| 项 | 状态 | 处理 |
|---|---|---|
| M1 Timestep maxLevel/finestLevel 不一致 | ✅ | ComputeDiags 改用 maxLevel 并 clamp dt，表头/数据长度一致 |
| M2 fluids() 返回裸指针 | ✅ | 已确认不是 bug：无流体时 nullptr 是合法状态 |
| M3 Services 恒 nullptr | ✅ | 同 H4，删除接口 |
| M4 InitData 前发布哨兵 TimeState | ✅ | 新增 `TimeState.valid`；构造期 false，InitData 后 true |
| M5 initialize 无幂等 | ✅ | `if (m_initialized) return;` |
| M6 RuntimeConfig 两套真值 | ✅ | 实时 getter，不再复制 |
| M7 四个文件重复 include | ✅ | 删除重复 `MultiParticleContainer.H` |
| M8 include 分组顺序 | ✅ | 修正 `FieldEnergy.cpp` 的 Diagnostics/Fields/Runtime 顺序 |
| M9 输入重复 reduced_diags_names | ✅ | `inputs_2d_particles` 合并为一条 |
| M10 测试未断言 lb_eff/step_info | 🟡 | 已加 step_info/lb_eff 行断言；仍使用自定义 CTest 而非 add_warpx_test |
| M11 refactor 可绕过 --accept | ✅ | `route_tasks.py` 条件改为仅 `args.accept` |
| M12 进度真值冲突 | ✅ | `run` 同步写 JSON status；本轮已全量 sync |

## L 级

| 项 | 状态 | 处理 |
|---|---|---|
| L1 RuntimeContext 可复制 | ✅ | 删除拷贝构造/赋值，保留 move |
| L2 析构无条件 finalize 外库 | ✅ | 增加 `m_external_libraries_initialized` 标志 |
| L3 verbose 取单例 | ✅ | 已用 `m_warpx->Verbose()` |
| L4 注释残留 GetInstance | ✅ | 更新 RuntimeData/WarpXRuntimeData 注释 |
| L5 1e-14 绝对容差 | ✅ | runtime snapshot 改为 1e-12 |
| L6 非 RZ 读 n_rz | ✅ | `#ifdef WARPX_DIM_RZ`，否则返回 1 |
| L7 parse_headers 写 /tmp | ✅ | 改为仓库内 `Tools/A0/header_catalog.json` |

## 验证

```text
默认 runtime 5/5 PASS
FFT runtime 7/7 PASS
ASAN runtime 5/5 PASS
test_2d_ionization_lab_restart PASS
```

## 未关闭项

- H3：6 处 legacy fallback 需等 Python 也走 RuntimeContext；
- M10：runtime 测试仍是自定义 CTest，未改成 `add_warpx_test()`；
- M2：非 bug，保留可空指针。
