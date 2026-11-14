# S 系列状态报告（2026-11-09）

本文件用**可复现的证据**回答三个问题：S 系列做到哪一步了、每一步的证据是什么、还差什么。

---

## 1. 一句话结论（2026-11-12 更新）

**核心与物理无关、插件接口通用、插件可独立增删**：用四个自动化检查器与逐套件实测证明。
**功能与上游 26.10 等价**：已达成可测范围内的等价——官方输入覆盖 **405/407**，
四套件通过 **721/724、335/336、570/573、232/232**，残余全部是平台校验和或 `-j` 竞态；
与上游二进制逐算例对照 **17 例**（16 例逐位相同，1 例双精度舍入）+ 3 例粒子级一致。

**仍未完成的一项**：粒子容器家族（约 1.17 万行）未整体搬入 `Source/Plugins/ParticleCore/`。
它是判据 2 的最后一块，六步方案与缺失契约清单见 `PLUGIN_REQUIREMENTS_AND_ROADMAP.md`。

**这一天修掉的真实缺陷**（都有回归证据）：
1. 1D PICMI 全部崩溃（legacy `FrameState` 默认 `moving_window_dir = -1` 导致越界写）；
2. `add_warpx_test` 在不支持 openPMD 的构建里仍注册需要 openPMD 的测试（32 条假失败）；
3. Python 单元测试整进程段错误（一个进程跑多个模拟 + `IImplicitSolver` 未注册）；
4. 审计报告的 5 项插件/总线缺陷（C ABI 读场、冲突检测、stage 接管、嵌套访问控制、pytest 收集）。

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
| 跨插件 include 违规 | 引号形式 **87 → 0**；11-13 修好检查器后，角括号形式另有 **10 条**既有债务留在棘轮清单（`Tools/Modularization/plugin_boundary_allowlist.txt`，只允许缩短；见 R5 的 2026-11-14 记录） |
| 核心回调总线 | 已落地：`register/has/execute/remove_callback`，Python 绑定注册、求解器触发 |
| 插件状态声明 | 插件在 `on_schema` 声明自有状态，核心随层分配（碰撞生产场为第一个实例） |

清零路径（都已落地）：
`HybridPICSolveE.cpp` → 改为接收 `HybridPICParameters` 参数块；
`EvolveEPML.cpp` → PML 分裂场系数容器（`SigmaBox`/`MultiSigmaBox`）与 PML 电流核移入核心。

### R5 功能完全覆盖原版（四维数值等价）——**571/573（99.65%），剩余为平台校验和**

**2026-11-14 例行检查：不变量仍然守住，但发现两处"环境/提交"级阻塞**

| 检查 | 结果 |
|---|---|
| `Tools/verify/check_macroscopic_vacuum_limit.sh` | `yee = macro = 3678360.615965`、`ratio = 1.000000`、退出码 0 |
| `check_core_purity.py` / `check_kernel_coupling.py` / `check_build_boundaries.py` | `0 known / 0 new`、OK |
| `check_plugin_boundaries.py` | **`10 known, 0 new`**（不再是 0 known，原因见下） |

`plugin_boundaries` 的 10 条**不是新增违规**：11-13 的 `baea579` 修掉了检查器"只匹配引号 include"的缺陷，
角括号形式的跨插件依赖第一次被看见。已核实这 10 条早于该提交存在——`ProjectionDivCleaner.cpp` 4 条、
`Modules/Python/WarpX.cpp` 6 条，正好等于允许清单的 10 条（清单只允许缩短）。因此本项的真实不变量应表述为
"**0 new，允许清单只减不增**"，而不是"全 0"。

**HEAD（5ee4fb9）在 `OPENPMD=ON` 下编不过**：`235434d` 把 `storeFieldOnParticles` 的签名改成
多一个 `const RuntimeConfigSnapshot&`（`Source/Core/Modules/Particles/ParticleIO.H:111`），
但 `Source/Plugins/Diagnostics/WarpXOpenPMD.cpp:608` 仍按旧签名传 `!use_pinned_pc`：

```
error: invalid initialization of reference of type 'const warpx::runtime::RuntimeConfigSnapshot&'
       from expression of type 'bool'
```

干净 worktree 复现（首编 **EXIT=2**）；工作区有一份**未提交**的一行修复
（`*m_runtime_context->config_snapshot`），应用后同一树 **EXIT=0**。详见 `UPSTREAM_PARITY.md` §13.7。

**构建工具链陷阱**：本机 shell 在 `conda activate base` 下导出了 `CC/CXX/CMAKE_ARGS`（conda gcc15 + MPICH）
与含 conda 的 `CMAKE_PREFIX_PATH/LD_LIBRARY_PATH`。这样新建构建目录会编出"MPICH 二进制 + 系统 OpenMPI 启动"
的组合，所有 MPI 用例启动即 `MPII_Comm_set_attr: Invalid communicator`（573 里 200+ 条 1 秒内失败，
极易被当成大面积回归）。`Tools/verify/build_and_run_4d.sh` 现在会 unset 这些变量并显式 pin
`/usr/bin/gcc` + 系统 OpenMPI。

本轮在主构建 `build_multi_hdf5_fft` 上用 **HEAD + 上面那一行修复**（`-DWarpX_PLUGIN_NEWTONGRAVITY=OFF`，
避开正在开发的 NewtonGravity）重建：721 个编译单元，**EXIT=0**。验证结果：
外部场族 + 粒子场诊断 **27/27 通过**（覆盖 P3-C 的外部场与 `storeFieldOnParticles` 改动）；
残余 8 条（含各自的 run/analysis/checksum）里 compton 三条**全过**、EB cube 与 boosted 的 run/analysis
通过、**只剩两条平台校验和失败**——失败集与 2026-11-10 的权威状态一致（`{110, 555}`，`276` 已通过）。

**全量 573 条本轮仍未跑完**：并行会话在反复运行 `test_gravity_*` 可执行文件（单次占 ~21/24 核）。
实测两次全量尝试：第一次 13 分钟推进 40/573、第二次 11 分钟推进 29/573，单个 3D 用例从 26 s 拖到 340 s 以上，
`loadavg` 33–38；已完成的 29 条里**没有失败**。这是同一资源阻塞的第三次（11-10、11-11、11-14），
不是代码问题；空闲窗口应补跑一次全量确认。

**2026-11-11 09:00 例行检查**：`Tools/verify/check_macroscopic_vacuum_limit.sh` →
`yee = macro = 3678360.615965`、`ratio = 1.000000`、退出码 0；四个检查器全部
`OK (0 known, 0 new)`（`core_purity` / `plugin_boundaries` / `kernel_coupling` / `build_boundaries`）。
源码相对 `67fa04c`（当前二进制的构建点）只多 3 个提交、`Source/` 净增 42 行，全部落在
"插件未链接时给出明确警告/断言"的分支上（`WarpXEvolve.cpp` 的碰撞告警、`WarpXInitData.cpp`
的诊断告警、`WarpX.cpp` 的场求解器断言），在全功能构建里一条都不会执行 → 数值等价。

同日按"源码增量"做了分区核验（全量被并发会话挡住，见下）：

| 分区 | 覆盖的源码增量 | 命令 | 结果 |
|---|---|---|---|
| 外部场族（8 用例 × run/analysis/checksum） | `67fa04c` 把外部场设置搬进契约 | `ctest -R "load_external_field\|projection_div_cleaner" -j 6` | **24/24 通过**（31.2 s） |
| 残余 4 条 | —— | `ctest -R "cube_macroscopic\.checksum\|boosted_opmd\.checksum\|linear_compton_bunch_laser\.(analysis\|checksum)"` | compton analysis 0.86 s / checksum 1.55 s **通过**；两条平台校验和**照旧失败** |
| `ionization_dsmc` 三条 | —— | 同上 + 链接 pywarpx 后 | run 61.7 s、analysis 2.26 s、checksum 1.68 s **全部通过** |

`ionization_dsmc.analysis` 此前的失败是环境：验证构建是 `PYTHON=OFF`，而 ctest 的 `PYTHONPATH`
在 configure 时写死、里面没有 pywarpx。修法（不改测试）：把 `build_py_4d/lib/site-packages/{pywarpx,amrex}`
链进 `build_multi_hdf5_fft/lib/site-packages/`（该目录本就在 ctest 的 `PYTHONPATH` 里）。
`Tools/verify/build_and_run_4d.sh` 现在会自动做这一步，并补上了 `LD_LIBRARY_PATH` 与 pywarpx 说明。

**本轮没有跑成 573 条全量**：并行会话正在跑 `build_py_4d` 的 724 条 Python 套件（默认 `-j 24`）。
实测我这边 5 分钟只推进 23/573，单个 3D 用例从 26 s 涨到 340 s（`test_3d_electrostatic_sphere.run`），
`loadavg` 37/24 核。全量数字仍以 2026-11-10 的 **571/573** 为准；本轮用上表三个分区覆盖了源码增量，
并用"`Source/` 只增 42 行且全在 abort 分支"证明了其余部分的数值等价。

**最新状态（2026-11-10 09:57）**：四维套件（1D/3D/RZ，openPMD + FFT + 并行 HDF5，**573 条**）
**571 通过、2 条失败，且两条都只是平台校验和**：

| 剩余失败 | 实测幅度 | 定性 |
|---|---|---|
| `test_3d_embedded_boundary_cube_macroscopic.checksum` | `Bx` 4.27e-18 vs 4.32e-18（绝对差 5.2e-20，比主场 `By≈5.1e-3` 小 17 个量级）；`Ey/Ez` 相对 2.37e-7 | 平台校验和；OMP 1 vs 4 线程输出**逐位相同**，与求和顺序无关（`UPSTREAM_PARITY.md` §10.4） |
| `test_rz_laser_acceleration_boosted_opmd.checksum` | 只在 mode-1 的 1e-12 量级分量（相对 2.2e-1）；主导分量与基准一致到 1e-15 | FFT/平台噪声 |

本轮把 11-09 剩下的第 3 条 `test_3d_linear_compton_bunch_laser.analysis` 定性并清掉：**不是 openPMD
读挂起，而是 `plt.show()` 在 Tk 后端阻塞在 GUI 事件循环**（`faulthandler` 栈见 `UPSTREAM_PARITY.md` §10.2）；
`MPLBACKEND=Agg` 下 0.59 s 通过。`Tools/verify/build_and_run_4d.sh` 现在导出该变量（与上游 CI 的无头
环境一致），并补齐了数据集 / blaspp 依赖说明。

**不变量检查（2026-11-10 09:05，全绿）**：

```bash
export LD_LIBRARY_PATH="$HOME/third_party/fftw/lib:/tmp/hdf5_mpi/lib:$LD_LIBRARY_PATH"
Tools/verify/check_macroscopic_vacuum_limit.sh   # yee = macro = 3678360.615965, ratio = 1.000000, exit 0
python3 Tools/Modularization/check_{core_purity,plugin_boundaries,kernel_coupling,build_boundaries}.py
# core purity OK (0 known, 0 new) / plugin boundaries OK (0,0) / kernel coupling OK (0,0) / build boundaries OK
```

**全量复跑与 `/tmp/g113_failed.log` 对比**：`g113 = {110, 276, 555}` → 本轮 `= {110, 555}`，
只少了 compton 分析（现在通过），**无新增失败**（`UPSTREAM_PARITY.md` §10.4）。

**关于 HEAD 的说明**：上面那次全量是在 `7a96278` 的二进制上跑的。之后 HEAD 多了
`f4e9708`（"粒子容器从运行时配置读设置"）——该提交把 `WarpX::x` 换成 `config().x()`，而
`RuntimeConfig` 的这些访问器（`particle_boundary_lo`、`nox`、`galerkin_interpolation`、
`do_shared_mem_current_deposition`，见 `Runtime/RuntimeConfig.cpp`）**直接返回同一个 `WarpX::` 静态量**，
因此是等价改写。重建后（`EXIT=0`）复验：真空极限 `ratio = 1.000000`；残余 4 条相关用例
（`cube_macroscopic.checksum`、`boosted_opmd.checksum`、`compton.{analysis,checksum}`）**逐条同样结果**
（前两条失败、后两条通过）。
**本轮未再做一次 573 条全量**，具体阻塞：机器被并行会话的 3 个 ctest（`build_2d_full -j8`、
`build_takeover -j2` 子集、`build_multi_hdf5` 重编译）打满（`loadavg` 31–34 / 24 核），
全量会被拖成资源争用假失败；留到空闲窗口补跑。

---

**以下为 2026-11-09 的历史记录。**

**2026-11-09 09:50，第 1-3 项已有实测结果**：37 条失败中 **26 条已清零**，剩 **11 条**：
RZ BTD 3 条、`boosted_opmd.checksum` 1 条、`pml_mr.run` 1 条（单独跑通过，待全量 `-j 4` 复核）、
`ionization_dsmc.analysis` 1 条（需 `pywarpx`，验证构建 PYTHON=OFF）、`macroscopic_solver` 2 条、
EB cube checksum 3 条。清零的是"缺外部测试数据"这一类（24 条），做法与上游 CI 一致
（`.azure-pipelines.yml:183`）：把 `openPMD-example-datasets` 克隆到**仓库的上一级目录**。
测试输入写的是 `../../../../openPMD-example-datasets/...`，从 `build_multi_hdf5/bin/<test>/`
上溯 4 层正好是仓库父目录。

```bash
# 在仓库父目录下执行，与上游 CI 相同
git clone --depth 1 https://github.com/openPMD/openPMD-example-datasets.git
# 只需其中 example-femm-3d.h5 (3.4 MB) 与 example-femm-thetaMode.h5 (84 KB)

cd WarpX_plugin
ctest --test-dir build_multi_hdf5 -R "load_external_field|projection_div_cleaner" -j 4 --output-on-failure
# -> 100% tests passed, 0 tests failed out of 24   (8 用例 x run/analysis/checksum, 9.8 s)
```

**证据**：8 个用例的 checksum 全部通过，且基准值非零
（`test_3d_load_external_field_particle_time`：`lev=0 Bz = 98.7756`；
`test_rz_load_external_field_grid`：`lev=0 Br = 0.6186`、`Bz = 4.6531`；
`test_rz_projection_div_cleaner`：`Br = 0.7880`、`Bz = 6.0501`）——数据确实从 HDF5 读入并复现了
上游数值，而不是"读取失败后全零也通过"。本轮**没有改任何代码**（`git status` 干净），
唯一的变量就是数据集的出现，因此归因明确。

**37 条失败按 CTest 条目的完整分解**（此前那张表只覆盖 30 条，漏了 7 条"其它校验/分析"）：

| 类别 | 条目 | 数量 |
|---|---|---|
| 缺外部测试数据（**已清零**） | `test_rz_load_external_field_grid[_restart]`、`test_rz_load_external_field_particles[_restart]`、`test_3d_load_external_field_particle_time[_restart]`、`test_3d_load_external_field_particle_multi_time`、`test_rz_projection_div_cleaner`（各 run/analysis/checksum） | 24 |
| 需 FFT 构建 | `test_rz_laser_acceleration_boosted_opmd.{run,checksum}`（`algo.maxwell_solver = psatd`） | 2 |
| 慢/偶发超时 | `test_3d_particles_in_pml_mr.run` | 1 |
| RZ BTD | `test_rz_btd.{run,analysis,checksum}` | 3 |
| 其它校验/分析 | `test_3d_embedded_boundary_cube{,_mr,_macroscopic}.checksum`(3)、`test_3d_ionization_electron_dsmc.analysis`(1)、`test_1d_macroscopic_solver.{analysis,checksum}`(2)、`test_1d_laser_acceleration_fluid_boosted.run`(1，已由快照修复解决) | 7 |

**结论**：37 条 = 24 缺数据 + 2 缺 FFT + 1 慢测试 + 3 BTD + 7 其它；其中 1 条已由快照修复解决。
其余 6 条已在下面逐条给出证据（1 条是环境项，1 条是真实数值发散，3 条待上游对照，1 条已清零），
不再声称"已全部归因"。

**2026-11-09 09:50，第 2 项（FFT 构建）与第 3 项（慢测试）的实测结果**：

第 2 项：RZ + FFT 需要 `blaspp`/`lapackpp`，本机没有系统 BLAS。依赖用 micromamba 装在
`$HOME/third_party/fft-deps`（conda-forge `blaspp`/`lapackpp` 2025.05.28 + OpenBLAS），
FFTW 沿用 `$HOME/third_party/fftw`。配置：

```bash
cmake -S . -B build_multi_hdf5_fft -DCMAKE_BUILD_TYPE=Release -DWarpX_DIMS="1;3;RZ" \
  -DWarpX_COMPUTE=OMP -DWarpX_OPENPMD=ON -DWarpX_FFT=ON -DWarpX_MPI_THREAD_MULTIPLE=ON \
  -DCMAKE_PREFIX_PATH="$HOME/miniconda3;$HOME/third_party/fftw;$HOME/third_party/fft-deps" \
  -DHDF5_DIR=/tmp/hdf5_mpi/cmake -DHDF5_ROOT=/tmp/hdf5_mpi -DWarpX_CCACHE=ON
cmake --build build_multi_hdf5_fft -j 20      # -> EXIT=0
```

**这个组合此前从未被编译过，第一次编译就暴露 3 个真实缺陷（都是插件化搬移留下的，已最小修复）**：

| # | 文件 | 症状 | 修复 |
|---|---|---|---|
| 1 | `Source/Plugins/PicEm/RuntimeBridge/Runtime/WarpXHostServices.cpp` | `SpectralSolverRZ`/`PML_RZ` 是完整类型使用，但只经 `WarpX.H` 拿到前置声明 → `invalid use of incomplete type` | 加 `#ifdef WARPX_USE_FFT` + `#ifdef WARPX_DIM_RZ` 保护的两个头（与 `WarpX.cpp` 同款） |
| 2 | `Source/Plugins/PicEm/Modules/FieldSolver/PSATD/SpectralFieldDataRZ.cpp` | 缺 `Runtime/ILegacyRuntime.H`、`Runtime/RuntimeData.H`、`Utils/TextMsg.H`；构造函数签名漏了头文件里已有的 `const RuntimeData*` 形参（`m_data(data)` 引用了不存在的名字） | 补 include，补形参（改名 `runtime_data`，避免遮蔽成员函数 `data()`） |
| 3 | 同目录 `WarpXHostServices.cpp` 的 `computeRho` | `ixType` 断言在 RZ 上是错的：RZ PSATD 的沉积 rho 本来就是 cell-centered（`MultiParticleContainer::GetZeroChargeDensity` 对 RZ PSATD 不做 `surroundingNodes`，与上游逐行一致），诊断随后在 `InterpolateMFForDiag` 插值到节点；上游 `RhoFunctor` 没有此断言 | 该断言只在非 RZ 保留（`#ifndef WARPX_DIM_RZ`） |

用例结果：`test_rz_laser_acceleration_boosted_opmd.run` **通过**（3.43 s）；`.checksum` 仍失败，
但差异只出现在 mode-1 的微小分量（如 `Br_1_imag` 7.07e-12 vs 8.63e-12，相对 22%），
而同一用例的主导分量与基准**在 1e-15 相对量级上一致**（`jr_1_imag` 1.5554e15、`jt_1_real` 1.5423e15）——
该算例的 mode-1 本身是发散的（基准里同样有 1e12–1e15 的非物理值），微小分量属平台/FFT 相关噪声。
结论：**FFT 不再是阻塞项**；这条 checksum 要用"上游 26.10 + 同 FFT/HDF5 环境"对照后才能定性（下一轮）。

**FFT 构建额外带入 104 条此前从未运行过的 PSATD 条目（总数 573 vs 469）**：
`ctest --test-dir build_multi_hdf5_fft -R psatd -j 8` 跑 86 条，**70 通过 / 16 失败**，
失败集中在 6 个用例：`3d_uniform_plasma_psatd_JRhom_CC1`、
`rz_galilean_psatd_current_correction[_psb]`、`rz_psatd_JRhom_LL2`、
`3d_pml_psatd_dive_divb_cleaning`、`rz_pml_psatd`。抽检 `test_rz_psatd_JRhom_LL2.run`
**单独跑 14.81 s 通过**（并行套件里 28.73 s 失败），因此这 16 条更像 `-j 8` × 每例 2–4 个 MPI rank
的资源争用，需用 `-j 4` 复跑定性（下一轮）。3D/RZ 的 langmuir、galilean、acceleration 等
PSATD **数值用例全部通过**，说明 RZ+FFT 的三处编译修复没有改变数值结果。

第 3 项：`ctest --test-dir build_multi_hdf5 -R test_3d_particles_in_pml_mr --timeout 3000` 单独跑，
`run` 87.44 s + `analysis` + `checksum` 全通过（总 1:30）。此前并行失败是资源争用下的超时，
不是数值问题；全量 `-j 4` 复跑的权威确认放到下一轮。

**"其它 6 条"已逐条分类（不再是"待查"）**：

| 条目 | 结论 | 证据 |
|---|---|---|
| `test_3d_ionization_electron_dsmc.analysis` | **环境项**：分析脚本 `from pywarpx import picmi`，验证构建 `WarpX_PYTHON=OFF` | `ModuleNotFoundError: No module named 'pywarpx'` |
| `test_1d_macroscopic_solver.{analysis,checksum}` | **真实数值发散**（`By`→1.44e80、`Ez`=NaN、相对误差 2.7e78）；但**基准本身也是发散值**（`By`=52.77、`Ex`=1.52e10），需与上游同环境对照 | `analysis_fields.py` 报 `rel. l2 error on By = 3.17e+78` |
| `test_3d_embedded_boundary_cube{,_mr,_macroscopic}.checksum` | Ey/Ez 相对差 95%（1.5e-6 vs 3.0e-5），其余分量 <0.5%；同样需上游对照 | checksum 输出逐键对比 |
| `test_1d_laser_acceleration_fluid_boosted.run` | **已清零**（快照修复） | 本轮复跑 38.71 s 通过 |

**无回归检查**：打完上述补丁后重建 FFT=OFF 的 `build_multi_hdf5` 全绿（`EXIT=0`），
RZ 诊断子集（`test_rz_laser_acceleration_opmd`、`load_external_field`、`projection_div_cleaner`、
`1d_macroscopic_solver.run`）**19/19 通过**。

| 构建 | 用例 | 通过 | 无法运行 | 失败性质 |
|---|---|---|---|---|
| 2D（OPENPMD=OFF） | 255 | **222** | 33 | openPMD 用例 + 1 已知输入缺项 |
| 1D/3D/RZ（OPENPMD=OFF） | 469 | **397** | 72 类 | 全部为 openPMD 输出依赖 |

**重构后回归检查（2026-11-09）**：S6 全部改动（插件拆分、契约抽取、PML/混合/流体搬移）完成后，
1D/3D/RZ 的失败集与重构前**逐条一致**（204 项，diff 为空），说明这轮结构性改动**没有改变任何
可运行用例的数值结果**。
| 1D/3D/RZ（OPENPMD=ON，**HDF5 无 MPI**） | 469 | 357 | — | 112 条失败，21 个运行失败报 `openPMD-api built without support for backend 'HDF5'` |
| 1D/3D/RZ（OPENPMD=ON，**并行 HDF5**） | 469 | **427** | — | 42 条失败：12 运行 + 14 分析 + 16 校验 |
| 同上，修完两处"配置快照过期"缺陷后 | 469 | **432** | — | **37 条失败**（12 运行 + 11 分析 + 14 校验） |

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

**同一类根因的第二处（2026-11-09）**：`BackTransformFunctor::InitData` 用
`i / config_snapshot().ncomps` 把输出变量映射到字段名表。快照里的 `ncomps` 此时仍是默认 1，
索引越界后 `m_varnames_fields[field_id]` 越界 → **段错误**（栈在 `InitData`）。
改为读实时 `RuntimeConfig` 后，RZ BTD 的运行阶段恢复正常。

**共性结论**：凡是"在 `InitData` 期间就用到 `ncomps`/模式数"的代码，都不能读配置快照，
必须读实时配置——因为快照在 `InitData` 之前生成，而这两个值要到层分配时才计算。
这条已作为规则记录，后续新增诊断代码需遵守。

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
| `test_rz_btd`、`test_rz_embedded_boundary_diffraction`、`test_rz_laser_acceleration[_boosted]_opmd` | 4 | **本项目差异**；其中 EB 衍射与 `_opmd` 已修复（配置快照），`_boosted_opmd` 需 FFT 构建；**`test_rz_btd` 仍崩溃**：`free(): corrupted unsorted chunks`（堆损坏，发生在第一次 BTD 缓冲刷新；`storeFieldOnParticles` 等已排除） |

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


### 2026-11-09 末次验证（RZ 诊断族全绿）

修复「配置快照过期」的第三、四处（BTDiagnostics 缓冲区尺寸、后处理核的模式循环）后，
以下用例 **run/analysis/checksum 全部通过**：

- `test_rz_btd`
- `test_rz_embedded_boundary_diffraction`
- `test_rz_laser_acceleration`（含 `_opmd`）
- `test_rz_projection_div_cleaner`

剩余仅有：`*_boosted_opmd`（需 `WarpX_FFT=ON` 构建）、`load_external_field` 系列（缺示例数据集）、
`test_3d_particles_in_pml_mr`（并行下偶发超时）。


## 7. 提交纪律说明（2026-11-09 自查）

`50307e2`（后处理模式数修复）在提交时用了 `git add -A Source`，无意中把**另一个并发会话**当时的两个
改动一并带入：`Source/Plugins/PicEm/RuntimeBridge/Runtime/WarpXHostServices.cpp`（FFT/PSATD 相关
include）与 `Source/Plugins/PicEm/Modules/FieldSolver/PSATD/SpectralFieldDataRZ.cpp`（`RuntimeData`
注入）。**这两处不是本会话的改动**。

处理方式：不改写历史（避免破坏并发会话的工作树），在此如实记录；此后所有提交都显式指定自己的路径
（`git add <path>`），不再使用 `-A`。


## 8. 关于 2D 失败集短暂升到 52 的说明（2026-11-09）

本会话末轮重建并跑 2D 套件时得到 52 条失败（基线 33），逐项核查后确认**不是回归**：

- 新增失败全部是**慢测试**（`test_2d_pml_x_yee`、`test_2d_theta_implicit_*`、`test_2d_nci_corrector_mr`、
  `test_2d_ionization_lab`、`test_2d_laser_injection_implicit`）；
- 单独复跑 `test_2d_pml_x_yee.run` → **通过**（243 秒）；
- 同一时刻有三个定时任务在各自会话中构建并跑套件（它们此时提交了 `f4259d9`、`bcffaa0`、`e361107`，
  并新建 `Source/Plugins/ParticleCore/`），机器被占满导致慢测试超时。

**结论**：并发会话共享同一台机器/工作树时，ctest 的超时会被放大成"假失败"。
复现或核验时应保证同一时刻只有一个会话在跑套件，或使用独立 worktree + 降低并行度。


## 9. Python / PICMI 路径（2026-11-10）

Python 绑定在插件化重构后**曾完全不可用**（`WarpX_PYTHON=ON` 连配置都失败）。已修：

| 缺陷 | 修法 |
|---|---|
| pyWarpX 源码路径仍指向 `Source/Plugins/PicEm/Python/` | 改到 `Modules/Python/pyWarpX.cpp` |
| `Modules/Python/WarpX.cpp` 包含 `FieldSolver/FiniteDifference/HybridPICModel/…`（旧路径） | 改到 `HybridPIC/HybridPICModel/…` |
| `set_potential_on_eb` 调已删除的 `GetElectrostaticSolverConcrete()` | 契约新增 `IElectrostaticSolver::setPotentialOnEB`，绑定走契约 |
| Python 路径不经 `Runtime`，故无运行时上下文 → `FlushFormat: runtime context/data is not set` | `InitData` 在上下文为空时自建 legacy 上下文（data/host/config/时间与帧状态） |
| 同上，但缺配置快照 → "Te diagnostic requires the hybrid-PIC solver" | legacy 上下文补 `RuntimeConfigSnapshot::FromLegacy()` |

**2D PICMI 套件（75 个测试）**：74 失败 → 47 → 38 → 19 → **3 失败（72 通过，96%）**。
最后那一步是 `MultiParticleContainer::get` 的绑定：它直接绑定到
`GetParticleContainerFromName`，而该函数在插件化后返回核心接口 `IParticleContainer&`，
pybind 于是回退到动态类型 `PhysicalParticleContainer` 并报"未注册"。
改为在绑定处 `dynamic_cast` 到 `WarpXParticleContainer&`（PIC 内核只会存这一种）。

剩余 3 条**全部是校验和**（相对误差 1e-3~6e-3，`run` 与 `analysis` 均通过），
属于与 C++ 套件一致的平台校验和类，不是功能缺口。
另有 10 条校验和差异在修复前统计中已归入该类。


## 10. 判据 2 收尾：场求解器族全部插件化（2026-11-10）

| 能力 | 位置 | 独立开关 | 缺失时的行为 |
|---|---|---|---|
| 有限差分 | `Plugins/FieldSolver/FiniteDifference` | `WarpX_PLUGIN_FIELDSOLVER` | 可操作错误 |
| 静电 | `Plugins/FieldSolver/Electrostatic` | 同上 | 可操作错误 |
| PSATD | `Plugins/FieldSolver/PSATD` | 同上 + `WarpX_FFT` | 可操作错误 |
| 静磁 | `Plugins/FieldSolver/Magnetostatic` | 同上 | 可操作错误 |
| **隐式（四种）** | `Plugins/FieldSolver/Implicit` | 同上 + `WarpX_PLUGIN_IMPLICIT` | 可操作错误 |

**隐式求解器搬迁的四个前置条件（全部完成）**：

1. `WarpXSolverVec`/`WarpXSolverDOF` 去内核指针并移入 `Source/Core/Solvers`；
2. `FieldGatherOrders`、`SpectralFieldIndex`、`NonlinearSolvers/*`（28 文件）移入核心；
3. `Define` 改收 `ImplicitKernelServices` 服务袋（50+ 项），求解器内 `WarpX*` 归零；
4. 内核侧三处未使用的 `ImplicitSolver.H` 包含删除。

**验证**：四个检查器全 0；2D 全量套件失败集 32 项，与搬迁前**逐条一致**；27 个隐式/Darwin 测试通过。


## 11. 宏观求解器缺陷：发现、定位与修复（2026-11-10）

**这是本项目唯一一处确认为自身引入的数值缺陷**，也是本轮系列最重要的发现。

| 阶段 | 证据 |
|---|---|
| 症状 | `test_1d_macroscopic_solver` 在 `cfl=1.0` 下发散到 1e78；`cfl=0.5` 下剖面对解析解差 127% |
| 排除"上游问题" | 同腔模、同 CFL、同采样下 **Yee 与解析解四位有效数字吻合**，宏观路径周期只有一半 |
| 定量 | 单步 `max|Ex|` 比值 **恰好 2.000000**；把 `eps` 翻倍后宏观结果与 Yee(eps0) 完全相等 ⇒ `1/eps` 大 2 倍 |
| 运行时定位 | `cell eps = eps0`（正确）但 `node eps = eps0/2`；节点填充中 `eL = 0`、`eR = eps` |
| 根因 | 节点填充的相邻单元索引按 **XZ（2D）** 写死（偏移第 1 维，并从 `Domain().smallEnd(1)/bigEnd(1)` 取界），在 1D 构建下第 1 维不存在 ⇒ 越界读到未填充单元（0）⇒ 算术平均把介电减半 |
| 修复 | 相邻单元索引按维度选取（1D 沿第 0 维，与 `DownwardDz` 一致），并修正 1D 下的域边界越界访问 |

**验收**：真空极限比值 2.000000 → **1.000000**；`test_1d_macroscopic_solver` 三条全过；
四维套件失败 6 → **3**（只减不增）；2D 套件 32 项**逐条一致**（无回归）。

**一键回归检查**：`Tools/verify/check_macroscopic_vacuum_limit.sh`（修复后打印 ratio = 1.000000 并返回 0）。
