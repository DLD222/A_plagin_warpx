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
| 四维 PICMI | `build_py_4d`（1D/3D/RZ + Python） | **分组 100/100** | 见 7.5；另有 6 条超慢用例在本机跑不完，**未计入** |

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

### 7.5 四维 PICMI 分组的最终结果（2026-11-11）

`build_py_4d` 一共 724 个条目，在本机（**单核**）全量跑不完，因此按分组验证：

```
$ ctest --test-dir build_py_4d -R "picmi" -E "background_mcc|spacecraft_charging" -j 3 --timeout 300
95% tests passed, 5 tests failed out of 100
    5 条全部是 ***Timeout 300.00 sec（其中 2 条是 run 超时后没有输出、连带的 analysis/checksum）
```

把超时的 3 条 `run` 用 `--timeout 1800` 重跑：

```
100% tests passed, 0 tests failed out of 9
    test_rz_ohm_solver_em_modes_picmi.run    Passed   25.58 sec
    test_1d_ohm_solver_ion_beam_picmi.run    Passed  105.81 sec
    test_1d_dsmc_picmi.run                   Passed   58.99 sec
```

**PICMI 分组：100 / 100 通过。**

**排除项（没有计入、也没有声称通过）**：

| 用例 | 现象 |
|---|---|
| `test_1d_background_mcc_picmi.{run,analysis,checksum}` | 单核下 >10 分钟无进展 |
| `test_rz_spacecraft_charging_picmi.{run,analysis,checksum}` | 单核下 >10 分钟无进展 |

### 7.6 这一路上修掉的**环境**问题（不是代码，但影响结论可信度）

| 问题 | 症状 | 解决 |
|---|---|---|
| conda 的 PETSc 把 MPI 换成了 MPICH | 一批 `*_picmi.run` 报 `unsupported PMI version PMIx` | `Tools/verify/fix_mpi4py_for_openmpi.sh` 用系统 `mpicc` 重建 mpi4py 扩展模块（关键是强制链接阶段只用 OpenMPI 的库目录） |
| `ctest` 里记录的 MPI 启动器与构建时不一致 | 全部 C++ 测试"失败"而 `.analysis`/`.checksum` 反而通过 | 保持 `MPIEXEC_EXECUTABLE` 与构建时的 MPI 一致 |
| 测试工作目录残留 | `Couldn't open file: diags/.../Cell_H` | 只清各测试目录内的 `diags` 与 `*.old.*`（**不要**删 `build_py_4d/bin/*`，那里有可执行文件） |

**判读规则**：若一批 `*_picmi.run` 同时失败，先看错误里有没有 `PMI`/`Init_thread`；
若 `.run` 失败但 `.analysis`/`.checksum` 通过，先查启动器；若报 `Couldn't open file`，先查工作目录残留。

---

## 8. 从零复现清单

按顺序执行，每步都给出验证输出的样子。所有命令都在仓库根目录。

### 8.1 四个检查器（任何改动后都必须全 0/OK）

```bash
python3 Tools/Modularization/check_core_purity.py        # core purity OK (0 known references, 0 new)
python3 Tools/Modularization/check_plugin_boundaries.py  # plugin boundaries OK (0 known, 0 new)
python3 Tools/Modularization/check_kernel_coupling.py    # kernel coupling OK (0 known entries, 0 new)
python3 Tools/Modularization/check_build_boundaries.py   # build boundaries OK
```

### 8.2 空腔真空极限（宏观求解器，一条命令）

```bash
Tools/verify/check_macroscopic_vacuum_limit.sh   # 末行必须是 ratio 1.000000
```

### 8.3 构建与套件

见 `VERIFICATION_MATRIX.md` 的"复现命令"与"各构建的配置要点"。要点：

- **C++ 套件**用系统 OpenMPI，`MPIEXEC_EXECUTABLE` 保持配置时的默认值；
- **Python/PICMI 套件**要求 `mpi4py` 与 WarpX 用同一个 MPI：
  `python3 -c "import mpi4py.MPI as m; print(m.get_vendor())"` 必须显示 `Open MPI`，
  否则先跑 `Tools/verify/fix_mpi4py_for_openmpi.sh`；
- 每个套件前 `export MPLBACKEND=Agg OMP_NUM_THREADS=1`，并确认没有并发构建。

### 8.4 单个算例的对照（A/B）

```bash
export UP_BIN=<上游二进制> MINE_BIN=<本仓库二进制>
/tmp/ab_test/run_ab.sh <case> warp:<输入文件>
python3 /tmp/ab_test/compare_one.py <case>            # plotfile：逐字段 max|diff|/scale
python3 Tools/verify/compare_particles_openpmd.py <a>/diags/diag1 <b>/diags/diag1   # 纯粒子输出
```

### 8.5 插件开关的行为

```bash
cmake -S . -B build_nofs -DWarpX_PLUGIN_FIELDSOLVER=OFF && cmake --build build_nofs -j 4
build_nofs/bin/warpx.2d...<输入>   # 必须打印 "No finite-difference field-solver plugin is linked … rebuild with WarpX_PLUGIN_FIELDSOLVER=ON"
```

其余四个开关同样见 `VERIFICATION_MATRIX.md` 的"插件开关的实测行为"。

### 8.6 判据 4 的两个验收

```bash
ctest --test-dir build_takeover -R "external_plugin_2|nonpic_kernel_2"   # 2/2 Passed
```

### 7.7 第 52–53 轮：修掉 Python 单元测试的段错误

`tests/unit` 之前整进程段错误。两个独立根因、都已修复并验证：

| 根因 | 修复 | 验证 |
|---|---|---|
| `add_warpx_pytest` 把 20 个单元测试放进**一个进程**，而 WarpX 一个进程只能初始化一次（`ParmParse::Initialize(): already initialized!` / `MPI_Comm_dup() after MPI_FINALIZE`） | 配置时枚举 node id，**每个测试一个 ctest 条目** | 2D：**20/20**；四维（1D/3D/RZ）：**60/60** |
| `warpx.implicit_solver()` 返回核心接口 `IImplicitSolver*`，pybind11 没注册它，回退到具体类型后报 `Unregistered type : ThetaImplicitEM` | 把 `IImplicitSolver` 注册为基类，求解器方法通过 `dynamic_cast` 暴露在基类绑定上（绑定中不出现具体求解器类名） | 同上 |

### 7.8 关于"失败项"的口径

- **平台校验和**（`*.checksum`）：与上游录制的基准有浮点差异，AGENTS.md 明确允许忽略，**从未修改过任何容差/校验和/断言**；
- **`-j` 争用下的超时**：单条重跑即通过（如 `test_1d_ohm_solver_ion_beam_picmi` 108 s 通过）；
- **测试注册与构建能力不匹配**（缺 openPMD 的构建注册了需要 openPMD 的测试）：已通过
  `add_warpx_pytest`/`add_warpx_test` 的 `needs_openpmd` 标记消除，而不是把失败写进基线。

**没有一项是"已知失败所以不管"。**
