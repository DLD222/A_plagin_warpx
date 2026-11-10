# 目标完成度报告（2026-11-10）

本轮目标（60 轮预算）：
1. 功能与 WarpX 官方 26.10 完全相同；
2. 插件化；3. 模块化；4. 用户自定义插件接入无阻。

---

## 结论摘要

| 判据 | 状态 | 一句话 |
|---|---|---|
| 1 功能与上游相同 | **99%（3 条已归因残余）** | 上游 420 个测试定义零缺失；四维 573 个测试 570 通过；本轮修掉一个真实数值缺陷 |
| 2 插件化 | **主体完成** | 诊断/碰撞/**五种场求解器（含隐式）**/ParticleCore 均为独立插件、可关闭、缺失报错；**粒子系统整体未拆**（已量化并给出路径） |
| 3 模块化 | **完成** | 四个检查器全 0，且每次搬迁都以"失败集逐条一致"验收 |
| 4 用户插件 | **完成** | 仓库外 `.so` 驱动真实算例 + 非 PIC 内核，均为自动化 ctest |

---

## 1. 功能与上游 26.10 相同

### 实测数字

| 套件 | 构建 | 通过/总数 | 残余 |
|---|---|---|---|
| 2D | `build_takeover`（openPMD=OFF） | 232/264 | 32，全部 openPMD 依赖 |
| 2D | `build_2d_full`（openPMD+FFT） | 335/336 | 1 平台校验和（`test_2d_laser_ion_acc.checksum`，宏观修复前后逐条一致） |
| 1D/3D/RZ | `build_multi_hdf5_fft`（openPMD+FFT+并行 HDF5） | **571/573**（导出 `MPLBACKEND=Agg`；未导出时 570/573） | **2**，均为平台校验和 |
| 2D PICMI | `build_py_opmd` | 72/75 | 3 平台校验和 |

### 剩余 2 条（四维）

| 条目 | 性质 | 实测幅度 |
|---|---|---|
| `test_3d_embedded_boundary_cube_macroscopic.checksum` | 平台校验和 | 最大相对误差 1.21e-2，落在数值零附近分量；`Ey/Ez` 仅 2.37e-7 |
| ~~`test_3d_linear_compton_bunch_laser.analysis`~~ | **已解决（环境）**：matplotlib 选 Tk 后端使 `plt.show()` 阻塞在 GUI 事件循环（faulthandler 抓到栈）；导出 `MPLBACKEND=Agg` 后 176 个分析测试 **全部通过**，无需改测试 | 已消除 |
| `test_rz_laser_acceleration_boosted_opmd.checksum` | FFT 噪声 | 差异只在 mode-1 的 1e-12 量级分量 |

### 本轮修复的真实缺陷（旗舰功能）

**宏观（张量介质）E 更新在真空极限下快了 2 倍**：

- 证据：同腔模同 CFL 下 Yee 与解析解四位吻合，宏观路径周期只有一半；单步 `max|Ex|` 比值**恰好 2.000000**；
- 定位：节点介电/电导填充的相邻单元索引按 XZ(2D) 写死，并从 `Domain().smallEnd(1)` 取界 —— 1D 构建下第 1 维不存在 ⇒ 越界读到未填充单元(0) ⇒ 算术平均把介电减半；
- 修复：索引按维度选取（1D 沿第 0 维，与 `DownwardDz` 一致）；
- 验收：真空极限比值 **2.000000 → 1.000000**；`test_1d_macroscopic_solver` 三条全过；四维失败 6 → 3（只减不增）；2D 32 项逐条一致。

**一键回归**：`Tools/verify/check_macroscopic_vacuum_limit.sh`（修复后 ratio = 1.000000、退出码 0）。

### 上游对照

见 `UPSTREAM_PARITY.md`：420 个上游测试定义全部存在且参数逐字相同；源文件可符号级对应；
构建选项 26 个全部存在 + 3 个插件开关。

---

## 2. 插件化

| 能力 | 位置 | 开关 | 缺失时 |
|---|---|---|---|
| 诊断 | `Plugins/Diagnostics` | `WarpX_PLUGIN_DIAGNOSTICS` | 跑完无输出 |
| 碰撞 | `Plugins/Collision` | `WarpX_PLUGIN_COLLISION` | 碰撞静默跳过 |
| 有限差分 | `Plugins/FieldSolver/FiniteDifference` | `WarpX_PLUGIN_FIELDSOLVER` | 可操作错误 |
| 静电 | `Plugins/FieldSolver/Electrostatic` | 同上 | 可操作错误 |
| PSATD | `Plugins/FieldSolver/PSATD` | 同上 + `WarpX_FFT` | 可操作错误 |
| 静磁 | `Plugins/FieldSolver/Magnetostatic` | 同上 | 可操作错误 |
| **隐式（四种）** | `Plugins/FieldSolver/Implicit` | 同上 + `WarpX_PLUGIN_IMPLICIT` | 可操作错误 |
| 粒子内核单元 | `Plugins/ParticleCore` | `WarpX_PLUGIN_PARTICLECORE` | 可操作错误 |

**未完成**：粒子系统整体（约 1.17 万行、30 余文件，继承链要求整体搬迁）。
耦合已量化：22 处 `WarpX::` 静态量读取（`RuntimeConfig` 已可提供）、3 处 legacy 单例回退、
2 项需新契约（守护单元掩码、负载代价）。

---

## 3. 模块化

| 检查器 | 结果 |
|---|---|
| `check_core_purity.py` | 0（允许清单为空，零容忍） |
| `check_plugin_boundaries.py` | 0（插件之间零包含） |
| `check_kernel_coupling.py` | 0 |
| `check_build_boundaries.py` | OK |

核心契约：`IParticleContainer`、`IParticleContainerAccess`、`IParticleCapability`、`ISimulationKernel`、
`IFluidModel`、`IMacroscopicProperties`、`HybridPICParameters`、`IPML`、`ISpectralSolver`、
`IMagnetostaticSolver`、`ImplicitKernelServices`、`DataBus`、`Callback` 总线、`StageRegistry`/`Scheduler`。

---

## 4. 用户自定义插件

| 验收 | 内容 | 状态 |
|---|---|---|
| `external_plugin_2` | 仓库外 `.c` 用裸 gcc 编译的 `.so`，加载进真实算例、注册状态与阶段处理器并执行 | 通过 |
| `nonpic_kernel_2` | 流体内核（无粒子能力）替换内核并运行 | 通过 |

---

## 5. 诚实缺口

1. **粒子系统未拆**（判据 2）：路径已量化，需 6-10 轮；
2. **四维残余 3 条**（判据 1）：均已归因，但未清零；
3. **PNG/ADIOS2/GPU 后端**：清单一致，无运行证据。

因此**目标不标记为完成**。

---

## 6. 收尾复验（2026-11-10，本轮实测）

```
$ Tools/verify/check_macroscopic_vacuum_limit.sh
yee   max|Ex| = 3678360.615965
macro max|Ex| = 3678360.615965
ratio         = 1.000000   (expected 1.000000)      exit=0

$ ctest --test-dir build_takeover -R "external_plugin_2|nonpic_kernel_2|runtime_snapshot_particles_2"
nonpic_kernel_2 ................. Passed
runtime_snapshot_particles_2 .... Passed
external_plugin_2 ............... Passed
100% tests passed, 0 tests failed out of 3

$ python3 Tools/Modularization/check_{core_purity,plugin_boundaries,kernel_coupling,build_boundaries}.py
core purity OK (0 known references, 0 new)
plugin boundaries OK (0 known, 0 new)
kernel coupling OK (0 known entries, 0 new)
build boundaries OK
```

判据 3、4 与旗舰功能回归检查在收尾时**重新跑过**，不是引用旧数据。

---

## 7. 2026-11-11 更新（本轮目标）

### P0 已修复：1D PICMI

根因：Python/PICMI 路径的 legacy 运行时上下文给服务层的是**默认构造的 `FrameState`**
（`moving_window_dir = -1`）加一个空刷新 lambda；诊断用它做数组下标
（`new_lo[moving_window_dir]`），于是写坏栈、连带 `Geometry::prob_domain` 错位 16 字节。

修复：`WarpXInitData.cpp` 用 `FrameState::FromLegacy()` 初始化，并让 `refresh_time` 真正刷新。

验收：`test_1d_laser_acceleration_picmi`、`test_1d_plasma_acceleration_picmi` 的 run+checksum 全过；
`build_py_4d` 全套件失败 **31 → 6**（只减不增）。

### P1 覆盖：405 / 407

RCYLINDER/RSPHERE 构建、`lasy`、openPMD 示例数据集、PETSc（2D/RZ/RCYLINDER/3D）、
2D+FFT+Python 全部补齐。剩余 2 个是上游 CMake 无法注册的 gaussian_beam 用例。

### P2 A/B：13 个算例，12 个逐位相同

RZ 7 + 3D 4 + 2D 2；唯一差异 `d3_eb` 为 1.63e-11（双精度舍入）。上游无法构建 1D（EB 类型在 1D 不存在）。

### 验证矩阵

见 `VERIFICATION_MATRIX.md`：七个构建、选项、测试数、失败清单与归类、复现命令。
