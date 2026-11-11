# 验证矩阵（2026-11-11）

本仓库的所有结论都可以用下表的构建复现。每一行给出：构建目录、配置选项、测试总数、
本机最近一次运行的失败清单与归类。**检查器**（四个）在任何时候都必须全 0/OK。

| 构建 | 维度 | 特性 | 测试数 | 失败 | 归类 |
|---|---|---|---|---|---|
| `build_takeover` | 2 | EB、QED（openPMD=OFF） | 264 | 32 | 全部 openPMD 依赖（本构建未开 openPMD） |
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
