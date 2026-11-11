# 验证矩阵（2026-11-11）

本仓库的所有结论都可以用下表的构建复现。每一行给出：构建目录、配置选项、测试总数、
本机最近一次运行的失败清单与归类。**检查器**（四个）在任何时候都必须全 0/OK。

| 构建 | 维度 | 特性 | 测试数 | 失败 | 归类 |
|---|---|---|---|---|---|
| `build_takeover` | 2 | EB、QED（openPMD=OFF） | **232** | **0** | 需要 openPMD 的测试已不再注册（见下） |
| `build_2d_full` | 2 | EB、QED、**FFT**、**openPMD** | 336 | 1 | `test_2d_laser_ion_acc.checksum`（平台校验和） |
| `build_multi_hdf5_fft` | 1、3、RZ | EB、QED、**FFT**、**openPMD**（并行 HDF5） | 573 | 3 | 2 条平台校验和 + 1 条需要 Python 绑定（本构建 PYTHON=OFF） |
| `build_py_opmd` | 2 | EB、QED、**FFT**、**openPMD**、**Python** | 426 | 1 | `test_2d_ionization_picmi.checksum`（平台校验和） |
| `build_py_4d` | 1、3、RZ | EB、QED、**FFT**、**openPMD**、**Python** | 724 | 6 | 4 条平台校验和 + 2 条 pytest（`libamrex_3d` 段错误：调试时只重建了 1D 目标造成的构建不一致） |
| `build_exotic` | RCYLINDER、RSPHERE | EB、QED、openPMD | 6 | 0 | — |
| `build_petsc` | 2、RZ、RCYLINDER、3 | EB、QED、**PETSc** | 680 | 4 | 全部 `*_petsc_*.checksum`（PETSc 版本/平台差异），run 与 analysis 全过 |

## 复现命令

```bash
# 检查器（任何时候都必须全 0/OK）
python3 Tools/Modularization/check_core_purity.py
python3 Tools/Modularization/check_plugin_boundaries.py
python3 Tools/Modularization/check_kernel_coupling.py
python3 Tools/Modularization/check_build_boundaries.py

# 跑某个构建（务必先确认没有别的会话在跑，并导出无头后端）
ps aux | grep -E "ctest|cmake --build"
export MPLBACKEND=Agg OMP_NUM_THREADS=1
ctest --test-dir build_multi_hdf5_fft -j 2 --timeout 2400
```

## 各构建的配置要点

```bash
# 四维（1D/3D/RZ）+ openPMD + FFT，用自建并行 HDF5
export LD_LIBRARY_PATH="$HOME/third_party/fftw/lib:/tmp/hdf5_mpi/lib:$LD_LIBRARY_PATH"
cmake -S . -B build_multi_hdf5_fft -DWarpX_DIMS="1;3;RZ" -DWarpX_OPENPMD=ON \
      -DWarpX_FFT=ON -DWarpX_COMPUTE=OMP -DHDF5_ROOT=/tmp/hdf5_mpi \
      -DCMAKE_PREFIX_PATH="$HOME/miniconda3;$HOME/third_party/fft-deps"

# PETSc：需要 conda 的 petsc 与配套编译器（mpicc/mpicxx）
export PATH="$HOME/miniconda3/bin:$PATH"
cmake -S . -B build_petsc -DWarpX_DIMS="2;RZ;RCYLINDER;3" -DWarpX_PETSC=ON \
      -DAMReX_PETSC=ON -DCMAKE_C_COMPILER=$HOME/miniconda3/bin/mpicc \
      -DCMAKE_CXX_COMPILER=$HOME/miniconda3/bin/mpicxx \
      -DCMAKE_PREFIX_PATH=$HOME/miniconda3

# 特殊维度
cmake -S . -B build_exotic -DWarpX_DIMS="RCYLINDER;RSPHERE" -DWarpX_OPENPMD=ON
```

## 外部依赖（非仓库内容）

| 依赖 | 位置 | 用途 |
|---|---|---|
| 并行 HDF5 1.14.6 | `/tmp/hdf5_mpi`（`Tools/verify/build_parallel_hdf5.sh`） | openPMD 输出、四维套件 |
| FFTW | `$HOME/third_party/fftw` | PSATD / FFT |
| blaspp / lapackpp | `$HOME/third_party/fft-deps` | RZ + FFT |
| openPMD 示例数据集 | `$HOME/openPMD-example-datasets` | `load_external_field` 与 gaussian_beam 类用例 |
| `lasy` 0.7.0 | conda 环境 | `*_lasy_file*` 用例（20 条） |
| PETSc 3.26.1 | conda 环境 | `*_petsc_*` 用例 |

## 官方算例覆盖

按 CTest 命令实际引用的输入文件统计：**405 / 407** 个官方 `inputs_test_*` / `PICMI_inputs_*`
至少被一个构建使用。剩余 2 个是 `gaussian_beam` 的 openPMD 用例，**上游自己的 CMake 无法注册**
（依赖的 `_prepare` 定义在其后，`if(dependency AND NOT TEST ${dependency}.run) return()` 必然为真），
本仓库该文件与上游逐字节一致。

## 插件开关的实测行为（判据 2 的"可关闭性"）

每个开关都用**关闭后的构建**跑一个**请求该能力**的输入验证过。
"必需能力"给出可操作错误并中止；"可选能力"记录高优先级警告并继续。

| 开关置 OFF | 构建 | 请求该能力的输入 | 实测行为 |
|---|---|---|---|
| `WarpX_PLUGIN_FIELDSOLVER` | `build_nofs` | 任意电磁输入 | **错误**：`No finite-difference field-solver plugin is linked … Rebuild with WarpX_PLUGIN_FIELDSOLVER=ON.` |
| `WarpX_PLUGIN_IMPLICIT` | `build_noimplicit` | `inputs_test_2d_theta_implicit_jfnk_vandb` | **错误**：`this input asks for an implicit evolve scheme, but this build has no implicit solver: rebuild with WarpX_PLUGIN_IMPLICIT=ON.` |
| `WarpX_PLUGIN_PARTICLECORE` | `build_nocoll` | 含 `particle_thermalizer.normal` | **错误**：`particle_thermalizer is set in the inputs but the particle-core plugin is not linked: rebuild with WarpX_PLUGIN_PARTICLECORE=ON.` |
| `WarpX_PLUGIN_COLLISION` | `build_nocoll` | `inputs_test_2d_collision_xz` | **警告**：`The input requests 3 collision(s) but this build has no collision plugin: they are skipped. Rebuild with WarpX_PLUGIN_COLLISION=ON …`，运行继续 |
| `WarpX_PLUGIN_DIAGNOSTICS` | `build_nodiag` | 含 `diagnostics.diags_names` | **警告**：`The input requests 1 diagnostic(s) but this build has no diagnostics plugin: no output will be written. Rebuild with WarpX_PLUGIN_DIAGNOSTICS=ON …`，运行继续 |

### 这两个警告是必要的，不是装饰

修复前，关闭碰撞/诊断插件时**输入照样跑完、什么也不发生、也没有任何警告**——
用户会得到与原输入描述**不同物理**的结果却毫无察觉。这是可关闭能力最危险的失败模式，
所以两处都改成了高优先级警告（`WarnPriority::high`）。

### 依赖关系

`WarpX_PLUGIN_IMPLICIT` 依赖 `WarpX_PLUGIN_FIELDSOLVER`（隐式求解器编译在场求解器插件里，
它需要有限差分求解器）。同时打开"场求解器 OFF + 隐式 ON"时 CMake 会强制关掉隐式并打印：

```
-- WarpX_PLUGIN_IMPLICIT forced OFF: the implicit solver requires WarpX_PLUGIN_FIELDSOLVER
```

否则内核会引用未编译的 `ImplicitSolver` vtable，在**链接期**失败（已实测）。

## 判据 4 的验收（用户自定义插件）

| 测试 | 内容 | 关键点 |
|---|---|---|
| `external_plugin_2` | 用**仓库外的** `third_party_plugin.c` 驱动一次真实 2D 模拟（`max_step=3`、16×16 网格、真实边界与求解器设置） | 只用公共 C ABI（`Runtime/PluginCApi.H`）；由**裸 C 编译器**构建：`${CMAKE_C_COMPILER} -shared -fPIC -std=c11`，不链接任何 WarpX 目标；插件自己声明场状态、在核心一无所知的阶段注册处理器、写出命名标量，测试断言插件自己的输出 |
| `nonpic_kernel_2` | 一个**完全没有粒子**的内核（十矩流体的替身），只使用 `Source/Core` 的头文件 | 证明内核可替换；`particle_capability()` 返回 `nullptr`，系统仍能运行 |

**跨配置复验**（避免"只在某个构建里碰巧通过"）：

```
$ ctest --test-dir build_takeover -R "external_plugin_2|nonpic_kernel_2|runtime_snapshot_particles_2|plugin_"
100% tests passed, 0 tests failed out of 12

$ ctest --test-dir build_petsc -R "external_plugin|nonpic_kernel"
1/2 nonpic_kernel_2 ... Passed
2/2 external_plugin_2 ... Passed
```

即在默认 2D 构建与 PETSc 构建（2D/RZ/RCYLINDER/3D）中**都通过**。

## 回归记录

| 日期 | 构建 | 结果 | 说明 |
|---|---|---|---|
| 2026-11-11 | `build_takeover` | 264 个测试 / 32 失败 | 与本文件表格逐条一致 |
| 2026-11-11 | `build_2d_full` | **335/336** | 唯一失败 `test_2d_laser_ion_acc.checksum`，与基线一致 |
| 2026-11-11 | `build_multi_hdf5_fft` | **570/573** | 3 条：2 平台校验和 + 1 需 Python 绑定 |
| 2026-11-11 | `build_py_4d` | 见下 | P0 修复后 31 → 6 |

每次回归都先确认没有别的会话在跑（`ps aux | grep -E "ctest|cmake --build"`），并导出
`MPLBACKEND=Agg`、`OMP_NUM_THREADS=1`。

## 运行套件时的 MPI 选择（一次误判的教训）

本机有两套 MPI：系统 **OpenMPI**（`/usr/bin/mpiexec`，`libmpi.so.40`）与 conda 的 **MPICH**
（`$HOME/miniconda3/bin/mpiexec`，`libmpi.so.12`）。两者混用会在 `MPI_Init` 阶段直接失败。

2026-11-11 的一次回归里，`build_py_4d` 从 6 条失败**跳到 18 条**，且新增的 12 条全是
`*_picmi.run`。逐条查看后确认**不是代码回归**：

```
Abort: Fatal error in internal_Init_thread: Internal MPI error!
       check_MPIR_CVAR_PMI_VERSION: Runtime environment uses unsupported PMI version PMIx
```

原因是那次跑套件时 `PATH` 里 `mpiexec` 指向系统 OpenMPI，而 conda 的 `mpi4py` 是 MPICH 构建。
**用 conda 的 `mpiexec` 重跑，同样的测试立即通过。**

| 套件 | 应使用的启动器 | 命令 |
|---|---|---|
| C++ 套件（`build_multi_hdf5_fft`、`build_2d_full`、`build_takeover`、`build_petsc`…） | 系统 OpenMPI | 直接 `ctest` |
| **Python/PICMI 套件**（`build_py_4d`、`build_py_opmd`） | **conda 的 MPICH** | `export PATH="$HOME/miniconda3/bin:$PATH"` 后再 `ctest` |

**判读规则**：如果一批 `*_picmi.run` 同时失败且错误里有 `PMI`/`Init_thread`，先查 MPI 启动器，
不要急着怀疑代码。

### 2026-11-11：`build_py_4d` 的回归**尚未解释清楚**（进行中）

本轮把该套件重跑三次，结果分别是 **18 → 20 → 20** 条失败，而目标是 6 条。
逐步排除：

| 现象 | 结论 |
|---|---|
| 一批 `*_picmi.run` 同时失败，错误含 `PMI`/`Init_thread` | **MPI 启动器不对**：`ctest` 里记录的是配置时的 `/usr/bin/mpiexec`（OpenMPI），而 conda 的 `mpi4py` 是 MPICH。用 `-DMPIEXEC_EXECUTABLE=$HOME/miniconda3/bin/mpiexec` 重新配置后，这类错误消失 |
| 仍有 `*_picmi.run` 失败，日志为 `Couldn't open file: diags/field_diag000000/Level_0/Cell_H` | **尚未定位**：诊断输出没有产生 |

**本轮排查到的两个真实原因**（都不是代码改动）：

1. **stale 输出目录**：这些 PICMI 测试在**同一个工作目录**里反复写 `diags/`。之前因 MPI 启动器
   不匹配而中断的运行留下了不完整的 `diags/field_diag000000`（有目录、缺 `Level_0/Cell_H`），
   后续分析测试读到它便报 `Couldn't open file`。清理该目录后单独跑，
   `test_1d_ohm_solver_em_modes_picmi.run` **通过（17.17 s）**。
2. **运行时间不稳定**：同一条 `.run` 在清理后跑是 17 s 通过，在并发构建/其他作业争用时
   1.2 s 就失败 —— 说明还夹着资源争用，而不是确定性失败。

**本轮最终定位（第三条，也是真正的根因）**：我为了让 PICMI 测试用上 conda 的 MPI，
把这个构建的 `MPIEXEC_EXECUTABLE` 改成了 conda 的 `mpiexec`（MPICH）——而 `build_py_4d` 的**所有**
可执行文件都是**系统 OpenMPI** 链接的。于是 C++ 测试被 MPICH 启动，成批 `***Failed`，
而它们的 `.analysis`/`.checksum` 反而通过（输出其实写对了，是启动器错了）。

**验证**（改回 `/usr/bin/mpiexec` 后）：

```
1/5 test_rz_embedded_boundary_diffraction.run  Passed  1.00 sec
4/5 test_1d_laser_acceleration_picmi.run       Passed  0.60 sec
100% tests passed, 0 tests failed out of 5
```

**结论：`MPIEXEC_EXECUTABLE` 必须与构建时用的 MPI 一致（本机=系统 OpenMPI），不要按套件类型切换。**
原先的 6 条失败基线就是在这个配置下测得的。

需要在**干净状态 + 无并发**下重跑：

```bash
# 只清每个测试工作目录里的输出，绝不要删 build_py_4d/bin/*：
# 那里同时放着可执行文件与 CTest 建好的工作目录，删掉会让 724 个测试全部 "Not Run"（我犯过）
for d in build_py_4d/bin/test_*; do rm -rf "$d"/diags "$d"/*.old.* 2>/dev/null; done
export PATH="$HOME/miniconda3/bin:$PATH"          # conda 的 mpiexec（MPICH）
cmake -S . -B build_py_4d -DMPIEXEC_EXECUTABLE=$HOME/miniconda3/bin/mpiexec
export MPLBACKEND=Agg OMP_NUM_THREADS=1
ctest --test-dir build_py_4d -j 1 --timeout 2400   # 单线程，避免争用
```

得到干净数字之前，**不更新** `FINAL_REPORT` 里的 PICMI 结论。

### 2026-11-11 干净重跑的真结果：40 条失败，全部可解释（环境，不是代码）

在**原始配置**（`/usr/bin/mpiexec` = 系统 OpenMPI）、清空各测试输出、单机无并发下重跑：

```
724 个测试，40 条失败
```

**34 条是同一类**：PICMI 脚本里 `from mpi4py import MPI` 的用例（OHM 求解器、Darwin、
projection div cleaner、spacecraft charging、secondary ion emission、effective potential）。

**根因（已证实）**：

```
$ $HOME/miniconda3/bin/python3 -c "import mpi4py.MPI as m; print(m.get_vendor())"
('MPICH', (5, 0, 1))
```

conda 的 `mpi4py` 是 **MPICH** 构建——它是 **P1 阶段安装 conda PETSc 时被一并带进来的**
（`conda list` 显示 `mpich 5.0.1` + `petsc 3.26.1`）。而 WarpX 的可执行文件与 Python 扩展都是
**系统 OpenMPI** 链接的。于是 `/usr/bin/mpiexec` 启动一个 MPICH 的 `mpi4py` 进程：

```
Fatal error in internal_Init_thread: ... unsupported PMI version PMIx
```

**对照证据**：不使用 mpi4py 的 PICMI 用例（如 `test_1d_laser_acceleration_picmi`）在同一环境下**通过**。

**已尝试的修复（失败，已回滚）**：用 `MPICC=/usr/bin/mpicc pip install --no-binary mpi4py`
重编 mpi4py，结果编译时仍吃到 conda 的头文件、链到 conda 的 `libmpi.so.12`，
`ImportError: undefined symbol: ompi_mpi_logical8`。已恢复为可用的 wheel。

**结论**：这是**环境不一致**，不是本仓库的代码回归。要彻底解决需二选一：

1. 用 conda 的 MPI（MPICH）重建整个 `build_py_4d`（与 `Docs/source/install/cmake.rst` 的
   `warpx-cpu-mpich-dev` 环境一致）；
2. 在系统 OpenMPI 下**干净地**重建 mpi4py（隔离 conda 的 include/lib）。

在此之前，PICMI 的数字必须按"哪一类 MPI 环境"来读，不能与装 PETSc 之前的 6 条基线直接比较。

### 2026-11-11 该问题已修复（并写成可复现脚本）

`Tools/verify/fix_mpi4py_for_openmpi.sh` 用系统 `mpicc` 重建 mpi4py 的扩展模块，
**关键是链接阶段**：若不加控制，conda 前缀自己的 `-L` 会排在前面，结果又链回 MPICH。
脚本强制 `LDSHARED` 只用 OpenMPI 的库目录，并把 MPICH 的模块备份到 `mpi4py/.mpich-backup/`。

```
$ python3 -c "import mpi4py.MPI as m; print(m.get_vendor())"
('Open MPI', (4, 1, 6))

$ ctest --test-dir build_py_4d -R "ohm_solver_em_modes_picmi|darwin_solver|projection_div_cleaner_callback|spacecraft_charging"
100% tests passed, 0 tests failed out of 14
```

**之前因 PMIx 失败的 14 条测试现在全部通过。** 全套件正在重跑以取得最终数字。

### 全套件重跑在本机不现实（实测）

修复 mpi4py 后，724 个测试的全量重跑**跑不完**：

| 观察 | 数据 |
|---|---|
| 正常测试 | 0.5–5 s |
| 个别测试 | `test_1d_particle_absorbing_boundary_direction_dependent_thresholds.run` **419 s** |
| 会卡住的测试 | `test_1d_background_mcc_picmi.run`（>10 min 无进展）、`test_rz_spacecraft_charging_picmi.run`（>10 min） |
| 进度 | 19 分钟才跑到第 37 个测试 |

本机只有 **1 个 CPU 核**，而这些 Python/PICMI 用例在 `-n 2` 下还要跑长时程物理。

**因此后续验证用"分组"而不是"全量"**：

```bash
# 只看某个物理族
ctest --test-dir build_py_4d -R "picmi" -E "background_mcc|spacecraft_charging" -j 2
# 只看某条具体用例
ctest --test-dir build_py_4d -R "^test_1d_laser_acceleration_picmi\.(run|checksum)$"
```

并在文档里**写明每组覆盖了哪些用例**，而不是笼统地说"724 个全过"。

### PICMI 分组验证结果（2026-11-11，mpi4py 修复后）

```
$ ctest --test-dir build_py_4d -R "picmi" -E "background_mcc|spacecraft_charging" -j 3 --timeout 300
95% tests passed, 5 tests failed out of 100
    失败项全部是 ***Timeout 300.00 sec（其中 2 条是 run 超时后没有输出、连带的 analysis/checksum）
```

把这三条超时的用例用 `--timeout 1800` 单独重跑：

```
$ ctest -R "^(test_1d_dsmc_picmi|test_rz_ohm_solver_em_modes_picmi|test_1d_ohm_solver_ion_beam_picmi)\."
100% tests passed, 0 tests failed out of 9
    test_rz_ohm_solver_em_modes_picmi.run   Passed   25.58 sec
    test_1d_ohm_solver_ion_beam_picmi.run   Passed  105.81 sec
    test_1d_dsmc_picmi.run                  Passed   58.99 sec
```

**结论：PICMI 分组 100 / 100 通过**（单核下个别用例 60–106 秒，`-j 3` 叠加 300 秒上限时会超时）。

**明确排除（未在本机跑完，需单独安排）**：

| 用例 | 情况 |
|---|---|
| `test_1d_background_mcc_picmi.{run,analysis,checksum}` | >10 分钟无进展 |
| `test_rz_spacecraft_charging_picmi.{run,analysis,checksum}` | >10 分钟无进展 |

这 6 条**没有**被计入上面的 100/100，也没有被声称通过。

### 粒子插件关闭：「无粒子输入仍可运行」已实测

`build_nocoll`（`WarpX_PLUGIN_PARTICLECORE=OFF`）跑一个**完全没有粒子物种**的输入：

```bash
# /tmp/nopar/inputs_noparticles：max_step=3、16x16、yee、periodic、无 particles.*
build_nocoll/bin/warpx.2d.MPI.OMP.DP.PDP.EB.QED inputs_noparticles
# exit=0, AMReX (...-dirty) finalized
```

这正是 P3 第 ④ 步的后半句（"关闭开关时……无粒子输入仍可运行"）的证据；
前半句（有粒子输入给出可操作错误）由 `particle_thermalizer` 那条断言覆盖（见"插件开关的实测行为"）。

## 更正：本机是 24 核；`nproc` 报 1 是因为我导出了 `OMP_NUM_THREADS=1`

**这是一个自造的约束，必须纠正。** GNU `nproc` 在设置了 `OMP_NUM_THREADS` 时会**返回该值**：

```
$ nproc                      # 在导出了 OMP_NUM_THREADS=1 的 shell 里
1
$ nproc --all
24
$ lscpu | grep '^CPU(s)'
CPU(s): 24
$ cat /sys/fs/cgroup/cpu.max
max 100000                   # 无 CPU 配额
$ python3 -c "import os; print(len(os.sched_getaffinity(0)))"
24
```

我为了不给测试超订 CPU 而长期 `export OMP_NUM_THREADS=1`，随后把 `nproc` 的输出当成机器规格，
于是得出"本机 1 核、跑不完套件"的错误结论，并据此把大量验证缩成分组抽样。
**正确做法**：`nproc` 要配 `--all` 或用 `lscpu`；`OMP_NUM_THREADS=1` 只该限制每个测试进程内的 OpenMP 线程，
不应限制 `ctest -j` 的并发度。

## 更正：`build_takeover` 不能当作 2D 套件基线

那张表里的"32 失败，与基线逐条一致"是**错误的表述**。查证结果：

```
$ ctest -R "^test_2d_charge_exchange_dsmc\.run$" --output-on-failure
### ERROR : To use openpmd output format, need to compile with USE_OPENPMD=TRUE
```

`build_takeover` 配置时 `WarpX_OPENPMD=OFF`，而**这 32 条测试的输入里有 `diag1.format = openpmd`**。
它们不是"已知失败"，而是**该构建根本不具备这些测试所需的能力**；上游同样无条件注册这些测试，
所以换任何只关 openPMD 的构建都会得到同样的 32 条。

**结论与处置**：

- `build_takeover` **不再作为套件基线**。它的价值只剩"2D 无 openPMD 的快速编译检查"。
- 2D 的权威数字用 `build_2d_full`（`WarpX_OPENPMD=ON`）：**335/336**，唯一失败是平台校验和。
- 同理，任何套件的数字都必须与**能支撑其全部测试的配置**绑定，否则"与基线一致"毫无意义。

### 已修复：`WarpX_OPENPMD=OFF` 的构建不再注册需要 openPMD 的测试

**问题**：`build_takeover` 里曾有 32 条"失败"，全部是
`### ERROR : To use openpmd output format, need to compile with USE_OPENPMD=TRUE`。
把它们写进"264 个测试、与基线一致"是用错误的分母自证——那个构建**根本跑不了**这些测试。

**修复**：`add_warpx_test` 增加可选的第 8 个参数 `needs_openpmd`：

```cmake
    set(needs_openpmd OFF)
    if(ARGC GREATER 7)
        set(needs_openpmd ${ARGV7})
    endif()
    if(needs_openpmd AND NOT WarpX_OPENPMD)
        return()
    endif()
```

判定依据是**输入文件的实际内容**（`<diag>.format = openpmd`），并且**跟随 `FILE =` 包含**——
共享的碰撞基文件正是通过包含把 `diag1.format = openpmd` 带进来的，只扫顶层文件会漏掉。
共标记 89 处注册。

**验证**：

```
$ ctest --test-dir build_takeover -j 12            # openPMD=OFF
100% tests passed, 0 tests failed out of 232

$ ctest --test-dir build_2d_full -j 12             # openPMD=ON
99% tests passed, 1 tests failed out of 336
    唯一失败：test_2d_laser_ion_acc.checksum（平台校验和）
```

同一个测试集在开 openPMD 的构建里**仍然注册并且通过**，说明不是把测试藏起来，而是让注册与构建能力一致。

## 全量重跑（24 核，2026-11-12）

之前用"分组抽样"是因为我把 `nproc` 的自造限制当成了机器规格。纠正后用 `-j 12` 跑全量：

```
$ ctest --test-dir build_py_4d -j 12 --timeout 2400
99% tests passed, 7 tests failed out of 724
```

| 失败项 | 性质 | 处置 |
|---|---|---|
| `test_1d_ohm_solver_ion_beam_picmi.{run,analysis,checksum}` | `***Timeout 2400`（`-j 12` 争用下） | **单独重跑通过**：`run 108.18 s`，`100% tests passed, 0 failed out of 3` |
| `test_3d_embedded_boundary_cube_macroscopic.checksum` | 平台校验和 | 与上游基准的浮点差异 |
| `test_rz_laser_acceleration_boosted_opmd.checksum` | 平台校验和 | 同上 |
| `pytest.WarpX.1d`、`pytest.WarpX.3d` | **段错误（未修）** | 见下 |

### 未修复项：`tests/unit` 的 pytest 段错误

```
Fatal Python error: Segmentation fault
  File ".../pywarpx/_libwarpx.py", line 171 in initialize
```

- 位置：`LibWarpX.initialize()` 内部（`amrex_init` → `get_instance` → `initialize_data`），
  调用方是 `tests/unit/test_charge_deposition.py` 的 `sim.initialize_warpx()`；
- **PICMI 端到端用例不受影响**（PICMI 分组 100/100 通过），说明崩溃与单元测试的启动路径有关；
- 在 `build_py_opmd`（2D Python 构建）里同样崩溃，**不是某个维度特有**；
- 该条目在本项目早期基线里就已在失败清单中，**不是本轮改动引入**。

**已定位根因（本轮实测），仍未修复**：

```
# 同一进程里跑第二个模拟：
amrex::Error::0::ParmParse::Initialize(): already initialized! !!!
# 若先 amrex_finalize()，则第二个模拟：
*** The MPI_Comm_dup() function was called after MPI_FINALIZE was invoked.
```

即 **WarpX/AMReX 只能在一个进程里初始化一次**：

| 做法 | 结果 |
|---|---|
| 两次 `initialize_warpx()`（不 finalize） | `ParmParse::Initialize(): already initialized!` |
| 先 `finalize()`（默认 `finalize_mpi=1`）再初始化 | `MPI_Comm_dup() after MPI_FINALIZE` → 整个 pytest 进程段错误 |
| `finalize(finalize_mpi=0)` | 仍然失败：AMReX 的 `Finalize()` 自身会终结 MPI（本 AMReX 版本没有"保留 MPI"的重载） |
| 装 `pytest-forked` 用 `--forked` 每测试独立进程 | 子进程以 **status 3** 退出（WarpX 的 Python 回调失败路径 `std::exit(3)`） |

而 `cmake/WarpXFunctions.cmake` 的 `add_warpx_pytest` 正是把 **20 个单元测试放进同一个进程**：

```cmake
add_test(NAME pytest.WarpX.${SD}
         COMMAND ${Python_EXECUTABLE} -m pytest -s -vvvv -p no:cacheprovider ${THIS_TEST_DIR})
```

**因此这是测试编排（一个进程跑多个模拟）与 WarpX 单次初始化约束冲突，不是数据/数值问题。**
正确的修法二选一：让每个单元测试在独立子进程中运行（并修掉 status 3 的那个回调失败），
或让 AMReX/WarpX 支持在同一进程内多次初始化。**两者都超出本轮可安全完成的范围，故如实留为未修复项。**

### 进一步收窄（同一轮实测）：**即使只有第一个测试，在 pytest 里也失败**

为排除"多测试同进程"这一层，做了三组对照：

| # | 做法 | 结果 |
|---|---|---|
| 1 | 纯脚本：设 `pywarpx.geometry.dims='2'`，建 sim、`initialize_inputs()`、`initialize_warpx()` | **成功**，并正常 `AMReX ... finalized` |
| 2 | 同上，但把 `sys.argv` 伪装成 pytest 的（`-m pytest -x -q <path>`） | **仍然成功** —— 排除 argv 解析 |
| 3 | `pytest -x -q test_charge_deposition.py`（`WARPX_TEST_DIMS=2`，只有一个测试） | **在 `initialize_warpx()` 段错误** |
| 4 | 同上加 `--forked`（每测试独立进程） | 子进程 `status 3`（WarpX 回调失败路径），且捕不到回调报错文本 |
| 5 | 同上加 `-p no:mpi` | 仍段错误 —— 排除 pytest-mpi 插件 |

**结论**：崩溃与"一个进程跑多个模拟"无关，是**在 pytest 进程里第一次初始化就崩**，
而等价的独立脚本不崩。已排除 `argv` 与 `pytest-mpi`。下一步应对比 conftest 的 fixture 链
（`warpx_lifecycle` 的 `monkeypatch.chdir(tmp_path)` 等）与脚本的差异，或用一个最小
`conftest.py` 二分。**未修复。**
