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
| 跨插件 include 违规 | **87 → 3**（棘轮，新增即失败） |
| 核心回调总线 | 已落地：`register/has/execute/remove_callback`，Python 绑定注册、求解器触发 |
| 插件状态声明 | 插件在 `on_schema` 声明自有状态，核心随层分配（碰撞生产场为第一个实例） |

剩余 3 处（都在 `plugin_boundary_allowlist.txt` 里记录）：
`EvolveEPML.cpp` → `PML.H`、`PML_current.H`（PML 系数容器与电流核，约 400 行辅助函数需一并搬移）；
`HybridPICSolveE.cpp` → `HybridPICModel.H`（混合 E 更新需要 23 个参数，应做成参数块契约）。

### R5 功能完全覆盖原版（四维数值等价）——**未完成**

| 构建 | 用例 | 通过 | 无法运行 | 失败性质 |
|---|---|---|---|---|
| 2D（OPENPMD=OFF） | 255 | **222** | 33 | openPMD 用例 + 1 已知输入缺项 |
| 1D/3D/RZ（OPENPMD=OFF） | 469 | **397** | 72 类 | 全部为 openPMD 输出依赖 |
| 1D/3D/RZ（OPENPMD=ON） | 469 | 357 | ？ | 112 条失败，其中 21 个运行失败报 `openPMD-api built without support for backend 'HDF5'` |

**关键事实**：本机 conda 的 HDF5 是串行版，而 openPMD 按 MPI 构建 → openPMD 自动关闭 HDF5 后端
（配置日志有明确警告），也没有 ADIOS2。测试输入显式要求 `h5` 后端，因此这 72 类用例
**在本机无论如何都跑不起来**。

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

## 6. 后续优先级

1. **R5 前置**：建 MPI-HDF5 环境 → openPMD 编出 HDF5 并行后端 → 四维权威基线。
2. **逐算例核验**：对上游 26.10 的 420 个测试，逐项比对通过/失败与数值。
3. **收尾 S6**：PML 系数容器搬移（约 400 行）与混合模型参数块（23 字段）→ 跨插件依赖清零。
4. **非 PIC 内核可行性**：`IPicKernel` + `KernelFactory` 已是插件注册制，但契约里仍有
   `register_species`/`allocate_collision_data` 等 PIC 词汇 → 需拆分"通用内核契约 + 可选粒子能力"，
   并写一个非 PIC（流体十矩）空内核冒烟测试证明零改核心可替换。
