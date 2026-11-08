# S 系列状态报告（2026-11-09）

本文件用**可复现的证据**回答三个问题：S 系列做到哪一步了、每一步的证据是什么、还差什么。

---

## 1. 一句话结论

**核心与物理无关、插件接口通用、插件可独立增删**这三条已经**用自动化检查器与实测**证明；
**功能完全覆盖原版（四维数值等价）**这一条**尚未完成**，卡在两类问题上：
(a) 本机 openPMD 缺 HDF5 后端，72 类用例无法运行；
(b) 粒子系统、PSATD、静磁、隐式求解器仍在内核插件内（各有量化原因）。

---

## 2. 需求与证据对照

### R1 核心物理无关

| 检查器 | 结果 | 覆盖内容 |
|---|---|---|
| `check_core_purity.py` | `0 known references, 0 new` | 核心目录内**没有任何具体 WarpX 粒子/迭代器类型引用**（允许清单为空，零容忍） |
| `check_build_boundaries.py` | 通过 | 核心不编译插件源码；每层目标存在 |

核心保留的是**通用契约**：`IParticleContainer`、`IParticleContainerAccess`、`ParticleContext`、
`IFluidModel`、`IMacroscopicProperties`、`IFiniteDifferenceSolver`、`IElectrostaticSolver`、
`IPML`、`IPicKernel`、`Callback` 总线、`DataBus`/`StageRegistry`/`Scheduler`。

### R2/R4 插件接口通用 + 仓库外插件零改核心

**实测证据**（`Docs/research_notes/external_plugin_demo/`）：

- 在**仓库外**用普通 `gcc` 编写并编译插件，只包含公共 C ABI 头，不链接任何 WarpX 代码；
- 用 `WARPX_PLUGIN_PATH` 加载进真实算例，清单与处理器注册成功，处理器被调度执行：

```
[third_party] init stage handler ran
[third_party] handler call 1 at stage AFTER_FIELD_SOLVE, dt=1.17933e-10
[third_party] finalize stage handler ran
```

**该验证抓出并修复了两个真实缺陷**：
1. 阶段边界点（`AfterFieldSolve`/`AfterCollisions`/`Init`/`Finalize`）**从未被调度**；
2. 可执行文件未导出 C ABI 符号，外部插件 `dlopen` 直接失败。

### R3 插件只经总线耦合

| 指标 | 变化 |
|---|---|
| 跨插件 include 违规 | **87 → 0**（棘轮已清空：没有任何插件包含另一个插件的头文件） |
| 核心回调总线 | 已落地：`register/has/execute/remove_callback`，Python 绑定注册、求解器触发 |
| 插件状态声明 | 插件在 `on_schema` 声明自有状态，核心随层分配（碰撞生产场为第一个实例） |

清零路径（都已落地）：
`HybridPICSolveE.cpp` → 改为接收 `HybridPICParameters` 参数块；
`EvolveEPML.cpp` → PML 分裂场系数容器（`SigmaBox`/`MultiSigmaBox`）与 PML 电流核移入核心。

### R5 功能完全覆盖原版（四维数值等价）——**未完成**

| 构建 | 用例 | 通过 | 无法运行 | 失败性质 |
|---|---|---|---|---|
| 2D（OPENPMD=OFF） | 255 | **222** | 33 | openPMD 用例 + 1 已知输入缺项 |
| 1D/3D/RZ（OPENPMD=OFF） | 469 | **397** | 72 类 | 全部为 openPMD 输出依赖 |

**重构后回归检查（2026-11-09）**：S6 全部改动（插件拆分、契约抽取、PML/混合/流体搬移）完成后，
1D/3D/RZ 的失败集与重构前**逐条一致**（204 项，diff 为空），说明这轮结构性改动**没有改变任何
可运行用例的数值结果**。
| 1D/3D/RZ（OPENPMD=ON，**HDF5 无 MPI**） | 469 | 357 | — | 112 条失败，21 个运行失败报 `openPMD-api built without support for backend 'HDF5'` |
| 1D/3D/RZ（OPENPMD=ON，**并行 HDF5**） | 469 | **427** | — | **42 条失败**：12 运行 + 14 分析 + 16 校验 |

**2026-11-09 进展**：已用系统 OpenMPI 自建并行 HDF5（`/tmp/hdf5_mpi`，HDF5 1.14.6），
openPMD 的 HDF5 后端随之启用（配置日志中的"only serial version of HDF5"警告消失）。
四维失败集从 112 降到 **42**。剩余失败中 **12 个是真实运行失败**，错误为：

```
what():  Chunk does not reside inside dataset (Dimension on index 0. DS: 1 - Chunk: 2)
```

集中在：RZ BTD、RZ EB 衍射、RZ/RZ load_external_field（含 restart）、RZ projection div cleaner、
RZ laser acceleration opmd。**定位进展**：`WarpXOpenPMDPlot::DumpToFile` 里 BTD 的尺寸/偏移逻辑（`ParticleFlushOffset`、
`is_first_flush_with_particles`、`is_resizing_flush`、`SetupPos/SetupRealProperties` 的 resizing 分支）
与本仓库导入的上游 26.10 源码**逐字节相同**，差异不在我们的重构里。该失败在 **1 个 MPI 进程**下同样出现，
因此也不是并行通信问题。当前最可能的解释是 openPMD 0.17.1 的 **HDF5 后端在数据集 resize 上的行为**
（上游 CI 的 openPMD 用例通常走 ADIOS2 或不同后端组合）。

**决定性对照实验（已完成）**：用上游 26.10 **自己的依赖 pin** + 同一并行 HDF5 构建出
`/tmp/warpx_up2610/build_up2/bin/warpx.rz`（配置日志无 "only serial version of HDF5"）。
用**同一输入、同一后端、同一 1 进程**运行 `test_rz_embedded_boundary_diffraction`：

- **上游 26.10：正常完成**（`AMReX ... finalized`）
- **本项目：失败**（`Chunk does not reside inside dataset (DS: 1 - Chunk: 2)`）

→ 结论：**这不是环境限制，而是我们分支与上游的差异**。

**差异定位进展**：写出路径的核心函数 `WriteOpenPMDParticles` / `DumpToFile` / `FlushBTDToDisk` /
`SetupPos` / `num_already_flushed` 与上游**逐行相同**（只有类型名和 `data()`/config 取值方式不同），
因此差异更可能在**送进写出器的数据**（每次 flush 的粒子数、要写的场/物种集合），而不是写出器本身。

**根因已定位并修复（2026-11-09）**：`SetupMeshComp` 用**配置快照**里的 `ncomps` 决定 RZ 数据集的
模式维长度，而快照是在 `InitData` 之前取的——`WarpX::ncomps` 要到层分配时才算出（`2*nmodes-1=3`），
第 0 步的诊断写出却发生在同一个 `InitData` 里，于是数据集按 1 个分量声明，写第 2 个分量就越界。
改为通过运行时上下文读**实时** `RuntimeConfig`（与上游直接读 `WarpX::ncomps` 同源）。
修复后 `test_rz_embedded_boundary_diffraction` 正常写出 4 个迭代（此前第 1 个就中止），
产物与上游 26.10 一致。

**12 个运行失败的分解**（本轮实测）：

| 子类 | 数量 | 原因 |
|---|---|---|
| `load_external_field`（RZ/3D，含 restart） | 7 | 外部数据集缺失：`openPMD::error::ReadError / Inaccessible`（`openPMD-example-datasets` 不在本机） |
| `test_rz_projection_div_cleaner` | 1 | 同上（ReadError） |
| `test_rz_btd`、`test_rz_embedded_boundary_diffraction`、`test_rz_laser_acceleration[_boosted]_opmd` | 4 | **本项目差异**（上游同环境通过），待定位到具体数据/计数逻辑 |

**原关键事实（现已解除）**：本机 conda 的 HDF5 是串行版，而 openPMD 按 MPI 构建 → openPMD 自动关闭 HDF5 后端
（配置日志有明确警告），也没有 ADIOS2。测试输入显式要求 `h5` 后端，因此这 72 类用例
**在本机无论如何都跑不起来**。

**非 PIC 内核（R2/R4 的一部分）**：内核工厂已改为中性命名（`SolverKernelPlugin` /
`register_solver_kernel_plugin` / `create_solver_kernel`），`IPicKernel` 的
`register_species`、`allocate_collision_data` 已变为**可选钩子（默认空实现）**；
`nonpic_kernel_2` 测试删掉这两个空实现后仍然通过，证明它们确实是可选的。

**要完成 R5 必须先做**：提供带 MPI 的 HDF5（如 conda-forge `hdf5=*=mpi_openmpi*`），
重建 openPMD → 重跑四维套件 → 建立权威基线。

---

## 3. 插件清单与删除矩阵（实测）

| 插件 | 层目标 | 开关 | 关闭后 |
|---|---|---|---|
| Diagnostics | `warpx_diagnostics_<dim>` | `WarpX_PLUGIN_DIAGNOSTICS` | 构建成功；运行完成且不产出输出（核心空服务兜底） |
| Collision | `warpx_collision_<dim>` | `WarpX_PLUGIN_COLLISION` | 构建成功；碰撞算例跑完，碰撞静默跳过 |
| FieldSolver | `warpx_fieldsolver_<dim>` | `WarpX_PLUGIN_FIELDSOLVER` | 构建成功；静电算例给出可操作错误 |
| ~~IO~~ | —— | —— | 唯一内容是通用打包工具，已并入核心，插件删除 |

---

## 4. 仍在内核插件内的模块（及阻塞原因）

| 模块 | 阻塞原因 |
|---|---|
| 隐式求解器 | 通过 `Define(WarpX*)` 驱动场推进/粒子快照/边界/诊断，约 198 处成员使用、11 类内核操作 |
| PSATD | 内核直接使用 `SpectralSolver::ForwardTransform/ApplyFilter/m_spectral_index`，缺谱求解器契约 |
| 静磁 | `WarpX` 持有 `VectorPoissonBoundaryHandler` 成员 |
| 混合模型 | 混合 E 更新需要 23 个参数 |
| 粒子系统（容器/沉积/推动/QED） | 尚未拆分（A7.3 剩余部分） |

---

## 5. 本系列修掉的真实缺陷（都有测试佐证）

1. **悬垂引用**：`WarpX::getdt()` 按值返回被适配器以 `const&` 转发 → 3D 加速器晶格路径崩溃（4 个用例）。
2. **RZ rho 校验过严**：沉积 rho 有 `2*nmodes-1` 分量而诊断要 1 个 → 所有 RZ rho 诊断中止（run+checksum 双验证修复）。
3. **阶段调度缺口**：9 个阶段点中 4 个（AfterFieldSolve/AfterCollisions/Init/Finalize）从未调度。
4. **插件注册被静态归档丢弃**（三次复现：诊断、碰撞、场求解器）→ 统一用链接锚点。
5. **空处理器抛异常**：无碰撞插件时包装空指针的适配器在碰撞阶段抛异常。
6. **平台/环境**：测试工作目录累积 277 GB 导致磁盘写满（已清理并记录规则）。

---

## 5b. 自动化任务（2026-11-09 创建）

| 任务 | 频率 | 目标 |
|---|---|---|
| 四维数值等价核验（openPMD/HDF5 环境） | 每天 09:00 | 建 MPI-HDF5 → 启用 openPMD 的 HDF5 后端 → 四维权威基线；不可行时确认 OPENPMD=OFF 失败集仍与基线逐条一致 |
| 上游 26.10 逐算例覆盖核验 | 每天 09:30 | 测试清单/源文件差异复核、按维度跑套件并分类失败、抽查关键物理用例，写入 `UPSTREAM_PARITY.md` |
| 粒子插件拆分（A7.3 续） | 每天 10:00 | 逐块把粒子系统拆成独立插件（先建契约→实现→注册→开关），每步保持构建与失败集 |

三个任务都写明了基线数字、检查器命令与纪律（不改测试容差、不提交他人路径）。

## 6. 后续优先级

1. **R5 前置**：建 MPI-HDF5 环境 → openPMD 编出 HDF5 并行后端 → 四维权威基线。
2. **逐算例核验**：对上游 26.10 的 420 个测试，逐项比对通过/失败与数值。
3. **收尾 S6**：PML 系数容器搬移（约 400 行）与混合模型参数块（23 字段）→ 跨插件依赖清零。
4. **非 PIC 内核**：**已用测试证明可行**（`tests/runtime/test_nonpic_kernel.cpp`，测试名
   `nonpic_kernel_2`）：一个没有粒子的内核（流体十矩的替身）只包含 `Source/Core` 头文件即可
   完整实现 `IPicKernel` 与 `RuntimeData`，通过回调总线触发事件；`IPicKernel` 里三处 PIC 词汇
   （`register_species`/`allocate_collision_data`）实现为空操作即可；`RuntimeData` 的粒子/混合/材料
   访问器已改为**可选默认实现**（无粒子内核无需伪造，访问时明确报告"本内核无粒子"）。
   仍需改进（非阻塞）：内核契约的命名与那三个 PIC 方法应拆成"通用内核 + 可选粒子能力"。
