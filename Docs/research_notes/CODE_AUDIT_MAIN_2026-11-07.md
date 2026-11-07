# 全量代码审计报告（main @ 2050e6a）

<!-- SPDX-License-Identifier: BSD-3-Clause -->

> 审计日期：2026-11-07
> 审计对象：`main` 分支当前唯一提交 `2050e6a`（"feat(io): enable openPMD with parallel HDF5; fix PML field recreation on load balance"）的**完整源码树**
> 对照基准：`Docs/research_notes/` 技术路线（A0–A6 共 153 项任务，`ROUTE_STATUS.md` 声称全部完成）、`PLUGIN_RUNTIME_ARCHITECTURE_V2.md`、`STAGE_POINTS.md` 等设计文档
> 审计方式：全量人工/子代理代码审查 + 实机编译 + 实机运行测试

---

## 1. 验证基线（本次审计实做了什么）

| 验证 | 配置 | 结果 |
|---|---|---|
| 全量编译 | 2D / OMP / NOMPI / openPMD=OFF / FFT=OFF，GCC，C++20 | **0 错误**，仅 1 处测试警告（见 P3-7） |
| 运行时快照（fields 模式） | `runtime_snapshot_2` | **通过**（Runtime 装配、FieldRegistry、PluginHost、IAmrPolicy、on_regrid、reduced diags 数值断言全过） |
| 插件加载器 | `plugin_loader{,_reject,_missing_file}_2` | **全部通过** |
| 动态 regrid | `test_2d_dynamic_regrid{,_remake,_remove,_uniform_b}.run` | **4 个 .run 全部通过**（analysis 阶段因本机缺 `yt` 无法执行，非代码问题） |
| 碰撞链路 | `test_2d_collision_xz.run` | **通过**（CollisionHandlerAdapter → CollisionHandler::doCollisions 链路工作） |
| 运行时快照（particles 模式） | `runtime_snapshot_particles_2` | **失败**——根因已定位为测试输入文件 bug（见 P1-4） |

**验证限制**：仅编译了 2D 维度；未验证 MPI / RZ / 1D / 3D / openPMD / FFT / Python 绑定路径的编译；`yt` 缺失导致所有 plotfile analysis 阶段未执行。涉及这些路径的结论以代码审查为准。

---

## 2. 确认已修复/已落实的项（无问题）

- **旧审计问题全部修复**：C1（finalize 先清 `RuntimeContext`，`Runtime.cpp:145-147`）、C2（`WarpXRuntimeData::Legacy()` 改为 thread_local 且每次调用刷新 `m_warpx`，`WarpXRuntimeData.cpp`）、C3（`RuntimeConfig` 活 getter + `FrameState::FromLegacy()`）、C4（`tests/runtime/test_runtime_snapshot.cpp` 断言位置正确、覆盖全面）、E1/N1（`ParticleHistogram2D.cpp:246` 局部 `data` 遮蔽成员 `data()` 的编译错误已消除）。
- **Handler/legacy 双路径逐位等价**：`RunPushStage/RunFinalizeStage/RunFieldSolveStage`（`WarpXEvolve.cpp:571-633`）在 handler 非空时走 `handler->apply(ctx)`，否则走 legacy；handler 内部经 `HostServices` → `WarpX::RunPicPush/RunPicFinalizeDeposition/RunPicFieldSolve`（`WarpXHostServices.cpp`、`WarpXEvolve.cpp:705-712` 等）最终调用同一 `OneStep_*` 实现。`RunPic*` 并非绕过 handler 的死代码，而是 host 委托终点。`Tools/automation/gates/a5_ab.py` 的 A/B 比较逻辑正确。
- **RemakeLevel 通用化**：`WarpXRegrid.cpp:179-239` 支持任意 BoxArray（拷贝旧字段 → ClearLevel → AllocLevelData → InitLevelData → InterpolateCoarseToFine → ParallelCopy 回拷 → RecreateLevelResources）；load-balance 同 ba 分支经 `MultiFabRegister::remake_level` 保留持久字段数据（`MultiFabRegister.cpp:275-328`）。
- **动态 regrid 主链路已接线**：`regrid_int` 读取（`WarpX.cpp:757`）、`AmrCore::regrid` 调用（`WarpXEvolve.cpp:407`）、`notify_regrid`（`WarpXEvolve.cpp:641-650`）、`MakeNewLevelFromCoarse` 有实现（`WarpX.cpp:2418-2427`）、`IAmrPolicy` 三处接线（tag `WarpX.cpp:3667`、makeDistributionMap `WarpX.cpp:3825`、postProcessBaseGrids `WarpXInitData.cpp:332`）。
- **PML load-balance 重建修复存在**：`WarpXInitData.cpp:1121-1136`（先 `pml.reset()` 再按名删除旧 `pml_*[level=N]` 字段，依赖 `alloc_init` 拒绝重复分配，已核实 `MultiFabRegister.cpp:47-64`）。
- **checkpoint/restart 与上游行为一致**：写侧（`FlushFormatCheckpoint.cpp:108-168`）与读侧（`WarpXIO.cpp:362-431`）显式覆盖同一字段集（E/B/J、cp、avg、E_old + PML + 粒子）。
- **碰撞所有权**：`CollisionHandler` 由 `Runtime` 以 `unique_ptr` 持有（`Runtime.cpp:83-87`），`WarpX`/PluginHost 只持非拥有指针；`CollisionSpeciesTable` 从 MPC 物种名构建（`CollisionSpeciesTable.cpp:14-23`）。
- **新建 level 的谱求解器与 costs 有保障**：两者都在 `AllocLevelMFs` 内创建（`WarpX.cpp:3006/3016/3237/3245`、`WarpX.cpp:3311-3315`），`MakeNewLevelFromCoarse` 路径可覆盖。
- **测试基础设施**：`test_runtime_snapshot.cpp` 断言完整；插件拒绝路径（bad_version/invalid_desc/no_entry/missing_file）测试真实验证失败分支。

---

## 3. 发现的问题

### P0 — 会产生错误结果或崩溃

#### P0-1 动态 regrid 新建的 fine level 没有 PML

`MakeNewLevelFromCoarse`（`WarpX.cpp:2418-2427`）只做 `resize + AllocLevelData + InitLevelData + InterpolateCoarseToFine`；而 PML 的创建点只有两处：`InitPML`（`WarpXInitData.cpp:1217`，仅初始化时）和 `RecreateLevelResources`（`WarpXRegrid.cpp:385-387`，仅既有 level 的 RemakeLevel 路径）。`regrid_int` 触发 `AmrCore::regrid` **新增** fine level 时，该 level 永远没有 PML——`do_pml` + PML 边界条件下，演化循环访问 `pmlPtr(lev)` 将解引用空指针崩溃，或（若有判空）静默无吸收边界。谱求解器无此问题（在 `AllocLevelMFs` 内创建）。

**建议**：在 `MakeNewLevelFromCoarse` 末尾补 `CreatePMLForLevel(lev, ba, dm)`（或复用 `RecreateLevelResources` 的相应部分），并补一个带 PML 的 dynamic-regrid 回归测试。

#### P0-2 动态 regrid 后未 redistribute 粒子边界 buffer

load-balance 路径有 `m_particle_boundary_buffer->redistribute()`（`WarpXRegrid.cpp:168`），但 Evolve 的 regrid 块（`WarpXEvolve.cpp:406-424`）只对粒子做 `SetParticleBoxArray/SetParticleBoxArray + AllocData + Redistribute + defineAllParticleTiles`，**漏掉了边界 buffer**。EB/边界刮削（scrape）暂存在 buffer 里的粒子仍挂在旧网格上，诊断 flush buffer 时按失效 BoxArray 写 plotfile，可能崩溃或静默丢粒子（当前 4 个 regrid 测试不含 EB 刮削场景所以未暴露）。

**建议**：regrid 块内补 `m_particle_boundary_buffer->redistribute()`，与 LoadBalance 路径对齐。

### P1 — 逻辑缺陷（当前不炸但是定时炸弹或导致 CI 红）

#### P1-1 `Runtime::finalize()` 销毁顺序两个隐患（`Runtime.cpp:139-158`）

1. `m_host.reset()` 先于 `m_collision_handler.reset()`，而 `CollisionHandler` 持有 `HostServices*` 裸指针（`CollisionHandler.cpp:39-43`）。当前 `CollisionHandler` 及其碰撞模型的析构不使用 host，故无实害；但任何未来在析构链中调用 host 的改动都会立刻变成 use-after-free。顺序应反过来。
2. `m_plugin_host.reset()` 前**没有** `m_warpx->SetPluginHost(nullptr)`（对比：context 和 collision handler 都先置空再 reset）。`WarpX` 在销毁间隙持有悬空 `m_plugin_host`。当前 `~WarpX` 只调 `ClearLevel`（`WarpX.cpp:570-576`）不触碰它，属潜在风险。

**建议**：finalize 中按“先解除 WarpX 侧指针（含 SetPluginHost(nullptr)），再按依赖反序销毁成员”重排。

#### P1-2 `PluginHost` 冲突检测与拓扑排序的字段匹配规则不一致（`HandlerSpec.H`）

- `add()` 的写写冲突检测用**精确字符串相等**（`HandlerSpec.H:79`，且只查 `writes_fields`，不查 `writes_species`）；
- 拓扑排序用**聚合名前缀匹配** `field_matches`（`HandlerSpec.H:112-126`，且只建“写→读”边）。

后果：handler A 写 `"Efield_fp"`（聚合）、handler B 写 `"Efield_fp_x"`（分量），同 StagePoint 下冲突检查放行、拓扑也不排序（无写→写边）→ 两个 writer 对同一底层 MultiFab 的写入顺序由注册顺序隐式决定，违反设计文档“同阶段写同一键 → abort（冲突）”（`PLUGIN_RUNTIME_ARCHITECTURE_V2.md` §3.3）。

**建议**：`add()` 的冲突检测改用 `field_matches`，并覆盖 `writes_species`（可加 `species_matches`）；或按文档在冲突时 abort。

#### P1-3 checkpoint 双层元数据脱节，`write_checkpoints/read_restarts` 实为空调用

- `FieldRegistry` 侧：`AllocLevelField`/`AllocLevelFieldOnBA` 声明 `spec.checkpoint=true`（`WarpX.cpp:2472/2490`）；
- `MultiFabRegister` 侧：WarpX 的所有分配路径**从不传** `checkpoint_restart` 参数，恒为默认值 `false`（`MultiFabRegister.H:274`；`WarpX.cpp:2475/2494/2508` 均未传）。

于是 `fields().write_checkpoints(...)`（`FlushFormatCheckpoint.cpp:165`）和 `fields().read_restarts(...)`（`WarpXIO.cpp:428`）遍历不到任何字段，是**死调用**；实际 checkpoint 完全依赖 `FlushFormatCheckpoint` 内的**静态映射表**（`FlushFormatCheckpoint.cpp:36-47`，只认 E/B/J/cp/avg/E_old）。隐患：

1. 任何 `declare(checkpoint=true)` 但不在静态表里的字段会被**静默跳过**写入，而 manifest `field_schema.txt`（`FlushFormatCheckpoint.cpp:139-144`）却声称它被 checkpoint——元数据与磁盘内容可能不一致且无报警；
2. 注册表标志与 register 标志互相矛盾，后续谁若依赖 register 标志会踩空。

**建议**：二选一——要么让分配路径把 checkpoint 标志传进 register 并以 `write_checkpoints/read_restarts` 为准（删掉静态表），要么删掉 register 侧的 checkpoint 机制避免两套真相。

#### P1-4 测试输入 bug：`runtime_snapshot_particles_2` 红

`tests/runtime/inputs_2d_particles:17` 的 `warpx.reduced_diags_names` 只列了 `step_info field_energy field_max field_mom field_poynting field_reduce`；文件 53-65 行定义的 6 个粒子诊断（`part_num/part_energy/part_mom/rho_max/lb_eff/extrema`）**没有加入 names 列表**，永远不会被实例化。`test_runtime_snapshot.cpp` 的粒子分支（`:444-447`、`:522-566`）检查这些文件全部失败。本次实机运行复现：17 项 FAIL（`lb_eff` 未写行、各 `part_*`/`rho_max`/`extrema` 的 step 与数值全为 0）。

**建议**：把 6 个名字补进 `warpx.reduced_diags_names`。

### P2 — 与路线/设计文档的偏离（overclaim 或最小实现）

| # | 偏离 | 证据 |
|---|---|---|
| P2-1 | **18 个 Python 回调未映射到 StagePoint**：`STAGE_POINTS.md` 设计了 18 回调→带子位点（string sub）StagePoint 的映射；实现里 `Stage` 只有 14 个枚举、`sub` 是 `int`（`StagePoint.H:19-41`），没有 BeforeStep/AfterStep/OnCheckpoint 等阶段；回调仍在旧位置用字符串名 `ExecutePythonCallback`（如 `WarpXEvolve.cpp:201-205` beforestep、`:370-384` afterstep/afterdiagnostics、`WarpXPushFieldsEM.cpp:949-1012` afterBpush/afterEpush、`WarpXInitData.cpp:929-962` Esolve 系列）。语义保留，但文档声称的映射层不存在。 | 见左 |
| P2-2 | **PluginLoader 缺 V2 §4.4 的运行时校验**：只校验 `abi_version` + `struct_size`（`PluginLoader.cpp:22-59`）；设计要求的 WarpX 版本、`sizeof(Real)`、`WARPX_DIMS`、GPU 标志校验均未实现——错配维度/精度的插件 .so 不会被拒绝。 | `PluginLoader.cpp`；`PLUGIN_RUNTIME_ARCHITECTURE_V2.md` §4.4 |
| P2-3 | **RZ 谱求解器未注入 ctx**：`SpectralSolverRZ` 构造无 `RuntimeContext*` 参数、`is_pml` 硬编码 `false`（`SpectralSolverRZ.cpp:28-77`），A1.6c 的 context 链在 RZ 路径未覆盖。 | 见左 |
| P2-4 | **MacroscopicProperties 未注入 ctx**（`WarpX.cpp:468-471`），且 `refreshMaterials()` 只 declare 一个 `"default"` 材料、张量值取 parser 在 (0,0,0) 的取值（`MacroscopicProperties.cpp:291-307`）——空间变化的 ε/σ 张量在注册表里被压成单点值，A4.3 属最小实现，与“材料注册表反映真实材料”的目标有差距。 | 见左 |
| P2-5 | **诊断/求解器层保留大量 `GetInstance()` legacy 回退**：`Diagnostics.cpp:81-86/722-731`、`ComputeDiagFunctor.cpp:26-47`、`ComputeParticleDiagFunctor.cpp:20-28`、`ReducedDiags.cpp:168-202`、`FiniteDifferenceSolver.cpp:130-148`、`ElectrostaticSolver.cpp:593-615`、`PoissonBoundaryHandler.cpp:198-203`、`WarpXFluidContainer.cpp:1601-1621`、`QdsmcParticleContainer.cpp:652-664`。属文档认可的过渡形态，但与 A1.4/A1.5/A1.6 “完成迁移”的表述有出入；其中 `config()` 回退返回**空 `RuntimeConfig{}`** 的几处（FDS/静电/PoissonBoundaryHandler）在 legacy 路径下会让所有配置 getter 静默返回默认值，属隐患。`m_instance`/`GetInstance` 单例本体仍在（`WarpX.cpp:183/321-338`）。 | 见左 |

### P3 — 代码异味/小问题

1. **死代码且含假数据**：`PicEmPackage::register_specs`（`PicEmPackage.H:36-64`）与 `CollisionPackage::register_specs`（`CollisionPackage.cpp:22-31`）全仓库无调用点；内部硬编码物种名 `"electrons"/"ions"/"photons"/"gas"`，与任何真实输入无关，留着只会误导。同样地，`CollisionHandlerAdapter::spec` 硬编码同样的物种名（`CollisionHandlerAdapter.H`）——当前 Stage::Collisions 只有一个 handler 所以不影响排序，但属虚假声明。
2. **注释与实现矛盾**：`CollisionPackage.H:18-24` 注释称 “Execution handlers are intentionally not registered yet”，但 `CollisionPackage.cpp:17-20` 实际注册了执行 handler。
3. **误导性变量**：`JdispFunctor.cpp:38` `auto* hybrid_pic_model = host().hasHybridPICModel() ? this : nullptr;`——把 `this`（`const JdispFunctor*`）赋给名为 `hybrid_pic_model` 的变量，类型与名字完全不符；功能上只当布尔用，建议改为显式 bool。
4. **潜在越界**：`MultiFabRegister::remake_level` 里 `mf.tags()[0]`（`MultiFabRegister.cpp:294`）——若某 MultiFab 无 tag，`tags()` 为空 vector，下标 [0] 是 UB。当前所有 `alloc_init` 路径都设 tag，属潜在风险，建议加判空。
5. **硬编码配置值**：`RuntimeConfig::villasenor_mass_matrices_max_grid_crossings()` 返回字面量 3、`sync_nodal_points()` 返回字面量 `true`（`RuntimeConfig.cpp`）——与 `WarpX` 同名常量重复维护，上游改值时静默分叉。
6. **PML_RZ 分配风格不一致**：RZ 走 `FieldType` 重载（`PML_RZ.cpp:52-60`），笛卡尔走字符串 `alloc_init(..., false, false)`（`PML.cpp:859-924`）；功能等价（register checkpoint 标志本就恒 false，见 P1-3），但两套写法易误读。
7. **编译警告**：`test_runtime_snapshot.cpp:166` 对 `[[nodiscard]]` 的 `cyclic.ordered()` 丢弃返回值（本意是触发异常），建议 `(void)cyclic.ordered();`。
8. **`ordered()` 重复计算**：`PluginHost::ordered()`/`at()` 每次调用都重排 + 重跑 Kahn 拓扑排序（`HandlerSpec.H:128-210`），每个时间步每个 Stage 触发一次；handler 数少时无感，但属不必要的 O(n²)。可在 `add/declare` 后缓存排序结果。
9. **RTLD_NODELETE 与 dlclose 并存**：`PluginLoader.cpp` dlopen 用 `RTLD_NODELETE`（库永不可卸载），错误路径却调 `dlclose`（NODELETE 下为 no-op）——无害但自相矛盾，值得注释说明。
10. **RunRegridStage 守卫不完整**：`WarpXEvolve.cpp:641-650` 只判空 `plugin_host/runtime_context/host`，不判 `data`（其余 Stage 判空更全）。当前 `data` 恒非空，属一致性缺口。

---

## 4. 修复优先级建议

| 优先级 | 项 | 理由 |
|---|---|---|
| 立即 | P1-4（测试输入）、P3-7（警告） | CI 红，改动一行级 |
| 高 | P0-1（新 level PML）、P0-2（buffer redistribute） | 动态 regrid 正确性，P0-1 在 PML 配置下必崩 |
| 中 | P1-1（finalize 顺序）、P1-2（冲突检测）、P1-3（checkpoint 双轨） | 潜在崩溃/静默丢数据 |
| 低 | P2 各项、P3 其余 | 文档对齐与设计完整性 |

## 5. 附：实机验证命令与结果

```bash
# 编译（2D/OMP/NOMPI）：0 错误
cmake --fresh -S . -B build -DWarpX_DIMS=2 -DWarpX_COMPUTE=OMP \
      -DWarpX_PYTHON=OFF -DWarpX_OPENPMD=OFF -DWarpX_FFT=OFF \
      -DWarpX_MPI=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)   # 100% 完成，仅 P3-7 一处警告

# 运行
ctest --test-dir build -R "dynamic_regrid|runtime_snapshot|plugin_loader" --output-on-failure
# 通过：runtime_snapshot_2、plugin_loader{,_reject,_missing_file}_2、
#       test_2d_dynamic_regrid{,_remake,_remove,_uniform_b}.run
# 失败：runtime_snapshot_particles_2（P1-4，已定位根因）；
#       *.analysis/*.checksum 阶段因环境缺 yt 无法执行（非代码问题）
ctest --test-dir build -R "test_2d_collision_xz.run"   # 通过
```
