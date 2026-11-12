# 目标完成度报告（2026-11-10）

本轮目标（60 轮预算）：
1. 功能与 WarpX 官方 26.10 完全相同；
2. 插件化；3. 模块化；4. 用户自定义插件接入无阻。

---

## 结论摘要

| 判据 | 状态 | 一句话（2026-11-12 最终） |
|---|---|---|
| 1 功能与上游相同 | **完成** | P0（1D PICMI）已修；覆盖 **405/407** 官方输入（余 2 条上游 CMake 无法注册）；四套件 **721/724**、335/336、570/573、232/232，残余全部是平台校验和或 `-j` 竞态；与上游二进制逐算例对照 **17 例**（16 例逐位相同）+ 3 例粒子级一致 |
| 2 插件化 | **主体完成** | 诊断/碰撞/**五种场求解器（含隐式）**/ParticleCore 均为独立插件、可关闭、缺失报错；审计的 5 个插件/总线缺陷已修；**粒子容器家族整体搬迁仍未做**（交接单见 `PLUGIN_REQUIREMENTS_AND_ROADMAP.md`） |
| 3 模块化 | **完成** | 四个检查器全 0；每次搬迁都以"失败集逐条一致"验收 |
| 4 用户插件 | **完成** | 仓库外 `.so`（裸 C 编译器、不链 WarpX）+ 非 PIC 内核，自动化 ctest **13/13** |

---

## 1. 功能与上游 26.10 相同

### 实测数字

| 套件 | 构建 | 通过/总数 | 残余 |
|---|---|---|---|
| 2D（无 openPMD） | `build_takeover` | **232/232** | 0（需要 openPMD 的测试不再注册，见 7.3） |
| 2D（全特性） | `build_2d_full`（openPMD+FFT） | **335/336** | 1 平台校验和（`test_2d_laser_ion_acc.checksum`） |
| 1D/3D/RZ | `build_multi_hdf5_fft`（openPMD+FFT+并行 HDF5） | **570/573** | 2 平台校验和 + 1 条 `-j` 竞态（单独重跑 3/3 通过） |
| 2D PICMI | `build_py_opmd` | **20/20** Python 单元测试 + 全部 PICMI 用例 | 平台校验和 |
| 四维（C++ + PICMI） | `build_py_4d`（1D/3D/RZ + Python） | **721/724** | 2 平台校验和 + 1 条 `-j` 竞态（单独重跑 3/3 通过） |

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

## 5. 诚实缺口（2026-11-12 更新）

1. **粒子容器家族未整体搬迁**（判据 2 的最后一块）：`Source/Plugins/ParticleCore/` 已存在并接
   `WarpX_PLUGIN_PARTICLECORE` 开关（承载 ParticleThermalizer），但约 1.17 万行的容器家族仍在
   `Source/Plugins/PicEm/Modules/Particles/`。它**不能逐文件搬**（继承链 + 内核 12 个调用方），
   六步交接单、缺失的契约清单与验收标准已写入 `PLUGIN_REQUIREMENTS_AND_ROADMAP.md`。
   **未完成**，目标因此不标记完成。
2. **四维 3 条残余**：2 条平台校验和（AGENTS.md 允许忽略）+ 1 条 `-j` 竞态（单独重跑 3/3 通过）。
   没有清零，但都不属于功能缺陷。
3. **`interface_function` 的 1D/3D 节点坐标**（审计发现 4）：代码确实是 XZ 专用，但**与上游逐字相同**；
   我实现过维度分支又**回滚**——没有任何测试覆盖该路径，我的 3D 自洽算例也没跑出非零场，
   **拿不出证据就不动物理**。正确做法已写入 `AUDIT_RESPONSE_2026_11_12.md`。
4. **PNG/ADIOS2/GPU 后端**：清单一致，无运行证据。

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

### 7.5 四维套件的最终结果（2026-11-12，781 条）

**更正**：这里原先写"本机单核、全量跑不完、只能分组抽样"——**那是错的**。
`nproc` 在导出 `OMP_NUM_THREADS=1` 时会直接返回 1，我把它当成了机器规格；
本机实际是 **24 核**（`nproc --all` = 24，`lscpu` 24，cgroup 无配额）。

全量分两段跑完：

```
$ ctest --test-dir build_py_4d -I 1,351   -j 12 --timeout 3000
99% tests passed, 1 tests failed out of 351      # test_3d_embedded_boundary_cube_macroscopic.checksum

$ ctest --test-dir build_py_4d -I 352,781 -j 12 --timeout 3000
99% tests passed, 2 tests failed out of 430
    test_3d_reduced_diags_load_balance_costs_timers_picmi.analysis   # 单独重跑 3/3 通过
    test_rz_laser_acceleration_boosted_opmd.checksum                 # 平台校验和
```

**合计 778 / 781 通过**；3 条残余中 2 条是平台校验和、1 条是 `-j` 竞态（单独重跑通过）。
`pytest.WarpX.{1d,3d,rz}` 三条条目共 60 个单元测试全部通过（修复前是整个进程段错误）。

原来的"分组 100/100 + 6 条超慢用例"已被这一节的实数取代。

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

### 7.9 审计修复后的最终全量（2026-11-12 晚）

审计的 5 项修复合入后重跑全部套件（`pytest` 条目改用 `tests/unit/run_tests.py`，
每个维度一条，因此套件条目从 781 收敛到 **724**——单元测试仍是一个不落地执行，只是不再各占一条）：

```
$ ctest --test-dir build_py_4d -I 1,351   -j 12
99% tests passed, 1 tests failed out of 351     # 3d_embedded_boundary_cube_macroscopic.checksum
$ ctest --test-dir build_py_4d -I 352,724 -j 12
99% tests passed, 2 tests failed out of 373
    test_3d_reduced_diags_load_balance_costs_timers_picmi.analysis   # 单独重跑 3/3 通过
    test_rz_laser_acceleration_boosted_opmd.checksum                 # 平台校验和
```

**合计 721 / 724**。

| 套件 | 结果 |
|---|---|
| `build_takeover`（2D 无 openPMD） | 232/232 |
| `build_2d_full`（2D 全特性） | 335/336 |
| `build_multi_hdf5_fft`（1D/3D/RZ） | 570/573 |
| `build_py_4d`（1D/3D/RZ + Python） | **721/724** |
| 插件验收（`external_plugin_2`、`nonpic_kernel_2`、`plugin_*`…） | **13/13** |
| `pytest.WarpX.{1d,3d,rz}` | **3/3**（各 20 个单元测试） |

全部残余均为**平台校验和**或 **`-j` 竞态（单独重跑通过）**，没有一条是已知功能缺陷。

---

## 9. 终局核对（2026-11-12，第 60 轮）

全部为实测，命令与输出如下。

| 项目 | 命令 | 结果 |
|---|---|---|
| 四个检查器 | `check_{core_purity,plugin_boundaries,kernel_coupling,build_boundaries}.py` | 全部 `OK / 0` |
| CI 三条 gate | `Tools/verify/run_gates.sh` | compliance / audit / route 全 OK |
| 与远端同步 | `git log origin/main -1` | `977c60b`，工作树仅有并发会话的 4 个未提交文件（未触碰） |
| **P0 验收** | `ctest -R "^test_1d_(laser\|plasma)_acceleration_picmi\."` | **4/4 通过**（两条 run + 两条 checksum） |
| **判据 4 验收** | `ctest -R "external_plugin_2\|nonpic_kernel_2"` | **2/2 通过** |
| 真空极限 | `Tools/verify/check_macroscopic_vacuum_limit.sh` | `ratio = 1.000000`（期望 1.000000） |

### 四条判据的终局判断

| 判据 | 判断 | 依据 |
|---|---|---|
| 1 功能与上游相同 | **达成（可测范围内）** | P0 4/4；覆盖 405/407；721/724、335/336、570/573、232/232；A/B 17 例（16 逐位）+ 3 粒子级 |
| 2 插件化 | **主体完成，未达"完成"** | 五类能力全部出场且开关行为实测；审计 5 项缺陷已修；**容器家族（1.17 万行）未搬入 ParticleCore** |
| 3 模块化 | **达成** | 四个检查器全 0 |
| 4 用户插件 | **达成** | 13/13（含仓库外裸 C 插件与无粒子内核） |

**目标不标记为完成**：判据 2 的最后一块未交付。这不是"做不到"，而是"在剩余轮次内无法安全完成"——
它是 10–20 轮的工程，且需要与正在该目录工作的并行会话协调。完整方案见
`PLUGIN_REQUIREMENTS_AND_ROADMAP.md` 的"P3 交接单"。
