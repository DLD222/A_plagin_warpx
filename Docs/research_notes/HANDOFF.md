# 交接清单（2026-11-09）

本文件给接手者/定时任务一个**从上手到验证**的最短路径。

## 1. 当前不变量（必须保持，任何时候都要跑）

```bash
python3 Tools/Modularization/check_core_purity.py      # 核心不能出现具体 WarpX 粒子/迭代器类型
python3 Tools/Modularization/check_plugin_boundaries.py # 插件之间不得互相包含头文件
python3 Tools/Modularization/check_kernel_coupling.py   # 待拆模块的内核耦合只能减
python3 Tools/Modularization/check_build_boundaries.py  # 构建层次与层目标
```

四项当前全为 **0 已知 / 0 新增**。

## 2. 基线（改动前后必须逐条比对）

| 构建 | 命令 | 失败集 |
|---|---|---|
| 2D | `ctest --test-dir build_takeover -j 8` | 33（openPMD 用例 + 1 已知输入缺项） |
| 1D/3D/RZ（无 openPMD） | `ctest --test-dir build_multi2 -j 12` | 204（72 个 openPMD 用例） |
| 1D/3D/RZ（并行 HDF5 + openPMD） | `Tools/verify/build_and_run_4d.sh` | ≈25（仅剩环境前提） |

比对用 `build_*/Testing/Temporary/LastTestsFailed.log` 与 `/tmp/mh9_failed.log` 之类快照做 `diff`，
**不要**用 `ctest | tail` 截断（曾因此丢过失败条目）。

## 3. 剩余工作（按价值排序）

1. **环境前提**（不是代码问题）：
   - `*_boosted_opmd` 需 `-DWarpX_FFT=ON`；
   - `load_external_field*` 需 `openPMD-example-datasets` 数据；
   - `test_3d_particles_in_pml_mr` 并行下易超时（单独跑约 100 s）。
2. **粒子系统拆分（A7.3 剩余）**：从最独立的一块开始（ParticleCreation/AddPlasma、Deposition、
   Pushing/Gather、QED），按"核心建契约 → 插件实现并注册 → 内核只经契约 → 独立层目标与开关"推进。
3. **PSATD / 静磁 / 隐式迁移**：分别缺"谱求解器契约""静磁接口""内核服务层"（量化原因见
   `PLUGIN_REQUIREMENTS_AND_ROADMAP.md` §15b/§15）。
4. **非 PIC 内核契约收尾**：把 `IPicKernel` 正式改名为 `ISimulationKernel`，并把 `RuntimeData`
   的粒子访问器收进能力接口（当前已有可选默认实现，测试 `nonpic_kernel_2` 证明可用）。

## 4. 规则与踩过的坑

- **配置快照过期**：`InitData` 期间的代码必须读实时 `RuntimeConfig`，不能读 `RuntimeConfigSnapshot`
  （`ncomps` 要到层分配时才计算）。这一条已修四处，是四维核验里最后一批真实缺陷。
- **命名空间纪律**：不要在全局作用域写 `class Xxx;` 前向声明——会遮蔽 `warpx::runtime::Xxx`，
  表现为"无法转换/抽象类"。已踩三次。
- **静态归档**：任何"静态注册"的插件都必须有 `extern "C"` 锚点并加进 `_WARPX_PLUGIN_ANCHORS`，
  否则注册对象会被链接器丢掉（表现为"插件没注册"）。
- **磁盘**：聚变类测试每个约写 2.6 GB；跑套件前清理 `bin/test_*/diags`（**不要删测试目录本身**，
  它们是配置期创建的）。
- **并发会话**：多个会话同时改同一工作树会互相污染提交；本仓库曾因此有一次提交带入了他人文件
  （见 `S_SERIES_STATUS.md` §7）。建议用独立 worktree。

## 5. 关键文档

- `PLUGIN_REQUIREMENTS_AND_ROADMAP.md`：需求、路线、S0–S7 进度、§15b 现状、§15f 核验环境。
- `S_SERIES_STATUS.md`：R1–R5 的证据与缺口、已修缺陷清单、提交纪律说明。
- `external_plugin_demo/`：仓库外插件的最小示例（含复现命令）。
- `Tools/verify/`：核验环境脚本。


## 6. 为什么隐式求解器还没拆（以及要拆需要什么）

隐式求解器的公开入口是 `Define(WarpX*, bool)`：它拿内核对象直接驱动
"场推进 / 边界 / 粒子保存恢复 / 降维诊断"。实测耦合：

| 文件 | 内核引用 |
|---|---|
| `ImplicitSolver.cpp` | 70 |
| `SemiImplicitDarwin.cpp` | 60 |
| `ThetaImplicitEM.cpp` | 34 |
| `SemiImplicitEM.cpp` | 25 |

共 **40 个不同的内核成员**。其中 `m_fields`（109 次）可由 `RuntimeData` 提供，
但其余方法签名依赖内核侧类型（`WarpXSolverVec`、`ImplicitOptions`、`SubcyclingHalf`、
`WarpXParticleContainer`），因此**必须先把这些类型搬进核心或抽象掉**，才能定义
`IImplicitKernelServices`（约 40 个方法）并让求解器改走契约。

### 实测结论：还需要 6 个"粒子级循环"服务项

第一次尝试把 40 个调用点直接换成服务袋时，编译暴露了真实缺口：求解器里有 **5 处
`for (auto const& pc : GetPartContainer())` 循环**和 1 处全体沉积，循环体内直接操作
具体容器（`pti.GetAttribs()`、`GetRealSoANames()`、`PIdx::ux` 等），**无法经通用契约表达**。
正确做法是把每个循环体整体作为一个内核操作搬进服务袋：

| 待加服务项 | 来源 |
|---|---|
| `zero_implicit_velocity_correction` | `SemiImplicitDarwin.cpp:532` |
| `finish_velocity_update` | `SemiImplicitDarwin.cpp:569` |
| `convert_velocity_to_time_centered` | `SemiImplicitDarwin.cpp:314` |
| `deposit_current_all_species` | `SemiImplicitDarwin.cpp:369` |
| `count_suborbits` / `species_has_attrib` 已经加过 | `ImplicitSolver.cpp:34,115` |

**这不是"以后再说"，而是有明确前置条件的工程任务**：先把 `WarpXSolverVec` 抽象为
核心可用的"求解向量"，再按 40 个成员逐组定义服务，最后才搬文件。
任何尝试跳步的做法都会得到"假拆分"（接口存在但内部仍 `WarpX*`）。
