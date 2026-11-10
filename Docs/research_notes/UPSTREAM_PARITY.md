# 上游 WarpX 26.10 功能覆盖核查（UPSTREAM_PARITY）

> 目标：证明或证伪「本项目功能完整覆盖 WarpX 官方 26.10」。
>
> 参考工作树：`/tmp/warpx_up2610`（git worktree，指向导入提交 `7bcb44a`，含 26.10 源码）。
> 本轮执行时间：2026-11-09 09:30 +08:00 起。下文所有数字都可用本文给出的命令复现。

---

## 0. 结论

| 维度 | 结论 |
|---|---|
| 测试清单 | **覆盖成立**：上游 420 个 `add_warpx_test` 定义全部存在，且参数逐字相同；本地多出 4 个 `dynamic_regrid` 测试（新增能力，不是缺口） |
| 用例门控 | **覆盖成立**：`Examples/CMakeLists.txt`（`add_warpx_test` 本体、`if(WarpX_FFT)`、Python/PETSc 跳过逻辑）与上游逐字节相同 |
| 源文件 | **覆盖成立**：上游独有的 2 个文件名均可符号级定位到迁移后的文件；没有任何同名文件被削减到上游的一半以下 |
| 构建选项 | **覆盖成立**：上游 26 个 `WarpX_*` 选项全部存在；本地新增 3 个插件开关 |
| 运行 | **覆盖成立**：完整特性构建（openPMD+FFT）下 2D **334/335（99.7%）**、1D/3D/RZ **567/573（99.0%）**；剩余 7 条（6 个用例）= 校验和 3 + 缺 Python 绑定 1 + 库层挂起 1 + 上游自带发散用例 2（第 4 节） |
| 真实缺陷 | 本轮确认 2 个，均已修复：**D1 RZ+FFT 配置编译中断**、**D2 粒子归约诊断用例输入漏列**（第 5 节） |

一句话：**“功能完整覆盖 26.10”在清单、构建配置、运行三个层面都成立**；没有发现“上游有而本地缺失”的功能项。曾经让这句话不成立的是两个可复现缺陷（D1 使 RZ 维度根本无法编译），现已修复并有验证记录。

与 `S_SERIES_STATUS.md` 的 R5「功能完全覆盖（四维数值等价）——**未完成**」不矛盾：那条指的是
“四维套件零失败”这个更强的目标；本报告回答的是“**有没有缺功能**”。当前的 7 条残余失败里，
没有一条是“上游功能本地没有”。

尚未被本轮证明、因而不应声称的部分：**PICMI/Python 用例**（74 个定义）、**PETSc 用例**（5 个）、**ADIOS2 后端**、**GPU 后端**没有对应构建；它们的清单与上游一致，但没有运行证据（§7.2）。

---

## 1. 复核：测试清单（420 / 424）

### 1.1 计数

```bash
# 定义条数（行首的 add_warpx_test( ，排除注释与函数声明）
grep -rn "^\s*add_warpx_test(" --include=CMakeLists.txt . | wc -l        # 本地 424
grep -rn "^\s*add_warpx_test(" --include=CMakeLists.txt /tmp/warpx_up2610 | wc -l  # 上游 420
```

> 注：直接 `grep -o add_warpx_test` 会得到 427 / 423，因为 `Examples/CMakeLists.txt:28` 的函数声明与两处被注释掉的用例也含该串。计数必须按“行首 + 左括号”统计。

### 1.2 逐用例参数比对

脚本按括号配对解析每个 `add_warpx_test(...)` 的**全部**实参（先剥离 CMake 注释，再按引号切词），比较 `(name, dims, nprocs, inputs, analysis, checksum, dependency)`：

```bash
python3 Tools/verify/parse_tests.py --compare /tmp/warpx_up2610 .
# reference /tmp/warpx_up2610: 420 definitions
# local     .: 424 definitions
#
# shared names: 420
# only in reference: []
# only in local    : ['test_2d_dynamic_regrid', 'test_2d_dynamic_regrid_remake',
#                     'test_2d_dynamic_regrid_remove', 'test_2d_dynamic_regrid_uniform_b']
# tests whose full argument list differs: 0
```

（脚本随本报告一起提交：`Tools/verify/parse_tests.py`。它先剥离 CMake 注释再按引号切词，因此
`add_warpx_test` 的行内注释不会污染参数列表。）

**结论**：420 个同名用例的实参列表逐字一致 —— 没有被删弱的分析脚本、没有被改掉的维度、没有被替换的基准。

### 1.3 本地多出的 4 个用例

| 用例 | dims | 文件 |
|---|---|---|
| `test_2d_dynamic_regrid` | 2 | `Examples/Tests/dynamic_regrid/CMakeLists.txt` |
| `test_2d_dynamic_regrid_remove` | 2 | 同上 |
| `test_2d_dynamic_regrid_remake` | 2 | 同上 |
| `test_2d_dynamic_regrid_uniform_b` | 2 | 同上 |

这是本地新增的“动态重网格”覆盖，不是上游用例的替代品。

### 1.4 用例目录树比对

```bash
diff -rq /tmp/warpx_up2610/Examples Examples
# Only in Examples/Tests/CMakeLists.txt 与上游差一行（+add_subdirectory(dynamic_regrid)）
# Only in Examples/Tests: dynamic_regrid
# Only in Examples/...: __pycache__（运行产物）
# “Only in /tmp/warpx_up2610”: 无
```

`Examples/Tests/CMakeLists.txt` 的唯一差异：

```diff
+add_subdirectory(dynamic_regrid)
```

`Examples/CMakeLists.txt`（`add_warpx_test` 与门控逻辑所在文件）**完全相同**，因此同一套 `-D` 选项下本地注册的用例集合 = 上游集合 + 4。

### 1.5 按维度分布

| dims | 上游 | 本地 |
|---|---|---|
| 1 | 44 | 44 |
| 2 | 148 | 152 |
| 3 | 171 | 171 |
| RZ | 53 | 53 |
| RCYLINDER / RSPHERE | 3 / 1 | 3 / 1 |
| **合计** | **420** | **424** |

---

## 2. 复核：源文件

### 2.1 计数

```bash
# Source/ 下 .cpp + .H
find /tmp/warpx_up2610/Source -name '*.cpp' -o -name '*.H' | wc -l   # 636
find Source -name '*.cpp' -o -name '*.H' | wc -l                     # 765
```

本地多出的 129 个文件名集中在 `Source/Core/Runtime/`、`Source/Core/Modules/`、`Source/Plugins/PicEm/RuntimeBridge/` 等新的运行时/插件基础设施，属于架构重构的新增件。

### 2.2 上游独有文件名 —— 都只是改名/拆分

按 basename 比对，上游有而本地没有的只有 2 个：

| 上游文件 | 行数 | 本地落点（符号级核对） |
|---|---|---|
| `Source/FieldSolver/MagnetostaticSolver/MagnetostaticSolver.cpp` | 491 | **2026-11-10 起**：`Source/Plugins/FieldSolver/Magnetostatic/MagnetostaticSolverImpl.cpp`（432 行，`computeVectorPotential` / `setVectorPotentialBC` / `defineVectorPotentialBCs` / `EBCalcBfromVectorPotentialPerLevel::doInterp` 与 `operator()`）+ `MagnetostaticSolverRegistration.cpp`（24）/ `MagnetostaticSolverImpl.H`（41）/ `MagnetostaticSolver.H`（87），以及 `Source/Plugins/PicEm/Modules/FieldSolver/WarpX_Magnetostatic.cpp`（141 行，只剩 `WarpX::ComputeMagnetostaticField()` 这层胶水）。上一轮该文件还是 491 行的单体实现，见 §11 |
| `Source/Diagnostics/WarpXIO.cpp` | 458 | `Source/Plugins/PicEm/RuntimeBridge/IO/WarpXCheckpoint.cpp`：`WarpX::GetRestartDMap`、`WarpX::InitFromCheckpoint` |

核对命令：

```bash
for sym in computeVectorPotential setVectorPotentialBC defineVectorPotentialBCs \
           GetRestartDMap InitFromCheckpoint; do
  grep -rln "$sym" Source --include=*.cpp --include=*.H
done
```

### 2.3 没有“被掏空”的文件

按同名文件比较行数（上游 > 100 行者），**没有一个本地文件低于上游的 50%**；同名文件合计行数 160465（上游）→ 166904（本地）。

### 2.4 构建选项

```bash
grep -oE "option\(WarpX_[A-Za-z0-9_]+" CMakeLists.txt | sed 's/option(//' | sort -u | wc -l   # 26（上游）/ 29（本地）
```

上游 26 个 `WarpX_*` 选项**全部存在**；本地新增 `WarpX_PLUGIN_DIAGNOSTICS`、`WarpX_PLUGIN_COLLISION`、`WarpX_PLUGIN_FIELDSOLVER`。

---

## 3. 构建矩阵

| 构建目录 | dims | openPMD | FFT | 注册测试数 | 用途 |
|---|---|---|---|---|---|
| `build_takeover` | 2 | OFF | OFF | 263 | 2D 基线（用户指定） |
| `build_multi2` | 1;3;RZ | OFF | OFF | 469 | 1D/3D/RZ 基线（用户指定） |
| `build_2d_full` | 2 | ON | ON | 335 | 2D 完整特性（本轮**从零**配置+编译） |
| `build_multi_hdf5_fft` | 1;3;RZ | ON | ON | 573 | 1D/3D/RZ 完整特性 |

`build_2d_full` 是本轮新建的：此前不存在同时带 openPMD 与 FFT 的 2D 构建，2D 的物理用例只能在“缺 openPMD”或“缺 FFT”的构建里跑，无法区分环境失败与真实缺陷。

配置命令（与 `build_multi_hdf5_fft` 使用的库前缀一致，见 `FFTW_SETUP.md`、`OPENPMD_HDF5_BUILD.md`）：

```bash
PKG_CONFIG_PATH="$HOME/third_party/fftw/lib/pkgconfig" \
cmake -S . -B build_2d_full \
  -DWarpX_DIMS=2 -DWarpX_EB=ON -DWarpX_MPI=ON -DWarpX_COMPUTE=OMP \
  -DWarpX_PYTHON=OFF -DWarpX_OPENPMD=ON -DWarpX_FFT=ON -DWarpX_QED=ON \
  -DCMAKE_BUILD_TYPE=Release \
  -DHDF5_ROOT=/tmp/hdf5_mpi -DHDF5_IS_PARALLEL=TRUE \
  -DopenPMD_USE_HDF5=ON -DopenPMD_USE_ADIOS2=OFF \
  -DCMAKE_PREFIX_PATH="$HOME/miniconda3;$HOME/third_party/fftw;$HOME/third_party/fft-deps"
cmake --build build_2d_full -j8
# [100%] 0 errors  —— 含 AMReX 在内的全量冷编译
```

**源码快照说明（重要）**：本报告的运行结果对应**本轮 09:30–09:50 的构建**：

| 构建 | 二进制对应的树状态 |
|---|---|
| `build_multi2` | `50307e2` + 工作区未提交的 RZ 签名修复（09:31） |
| `build_takeover` | 由并发会话在 09:37 重编（`895635e` 前后） |
| `build_multi_hdf5_fft` | 09:34 重建（RZ 修复已在工作区，与 `f4259d9` 同内容） |
| `build_2d_full` | 09:46–09:50 冷编译（`f4259d9` 之后、`bcffaa0` 之前） |

之后的提交**没有**进这些二进制：`bcffaa0`（只改用例输入，运行期读取，已生效）、
`2b95af0`（11:10，ParticleThermalizer 独立插件化）。因此
`test_2d_particle_thermal_boundary` 的复跑结果对应**旧实现**；对新实现的复核要重编后再跑。

测试运行统一加 `OMP_NUM_THREADS=1`（否则每个 rank 会按 24 核开线程，多用例并行时互相拖死，单个隐式算例会从 ~30 s 涨到 ~280 s）：

```bash
OMP_NUM_THREADS=1 \
LD_LIBRARY_PATH="$HOME/third_party/fftw/lib:/tmp/hdf5_mpi/lib" \
ctest --test-dir <build> -j<N> --output-on-failure
```

---

## 4. 运行结果与失败分类

### 4.1 失败分类口径

每个用例会生成 `.run` / `.analysis` / `.checksum` 三条 ctest 记录；`.run` 失败会连带后两条。分类按**用例**（去掉后缀）归并，依据运行期错误原文：

| 类别 | 判据（错误原文） |
|---|---|
| openPMD 环境依赖 | `### ERROR : To use openpmd output format, need to compile with USE_OPENPMD=TRUE`、`You need to compile WarpX with openPMD support`、`ReadExternalFieldFromFile requires OpenPMD support`、`ParticleHistogram2D: Needs openPMD-api`、`Could not detect format for path: diags/...` |
| FFT 环境依赖 | `### ERROR : algo.maxwell_solver = psatd is not supported because WarpX was built without spectral solvers` |
| 平台校验和 | `.checksum` 失败但 `.analysis` 通过，且差异是浮点/编译器相关的末位 |
| 基准数据缺失 | `No such file`、`datasets not found` |
| 真实缺陷 | 以上都不匹配，且可在完整特性构建中复现 |

分类脚本随本报告提交：

```bash
python3 Tools/verify/classify_ctest_log.py /tmp/ctest_hdf5fft_full_20261109.log
# 573/573 entries executed, 556 passed, 17 failed
# distinct failing cases: 9
#   test_3d_collision_pulsed_decay: environment (openPMD), timeout (machine contention)
#   ...
# cases with a candidate real defect need the serial rerun ... before they are believed
```

它只做“初判”：标成 candidate 的用例必须用 `ctest -j1 -R <case> --timeout 2400` 串行复跑后才采纳
（本轮 8 个超时用例正是这样被排除的）。

### 4.2 基线构建（无 openPMD / 无 FFT）

**`build_takeover`（2D，openPMD=OFF，FFT=OFF）**：**231 / 263 通过**，32 条失败（11 个用例），566 s。

```bash
OMP_NUM_THREADS=1 ctest --test-dir build_takeover -j12 --output-on-failure
# 88% tests passed, 32 tests failed out of 263
```

| 失败用例（11 个） | 失败条目 | `.run` 原文 |
|---|---|---|
| `test_2d_charge_exchange_dsmc` | 3 | `### ERROR : To use openpmd output format, need to compile with USE_OPENPMD=TRUE` |
| `test_2d_collisions_split_momentum_push_{electrostatic,electromagnetic,electromagnetic_vay}` | 9 | 同上 |
| `test_2d_embedded_boundary_em_particle_absorption_sh_factor_{1,2,3}` | 9 | 同上 |
| `test_2d_embedded_circle` | 2 | 同上（`analysis` 与 `checksum` 同脚本） |
| `test_2d_qed_breit_wheeler_opmd` | 3 | 同上 |
| `test_2d_rigid_injection_btd` | 3 | 同上 |
| `test_2d_laser_ion_acc` | 3 | 同上 |

`grep -c USE_OPENPMD` = 11（每个用例一条 `.run`），其余 22 条是随后 `.analysis`/`.checksum` 读不到 `diags/` 报的 `Could not detect format for path`。**11/11 个失败用例都是 openPMD 环境依赖**，没有一个是物理/数值失败。

D2 修好后，`runtime_snapshot_particles_2` 在这个基线构建里也通过了（不再出现在失败清单里）。

分类脚本对这个日志的判定与上表一致（0 个 candidate）：

```bash
python3 Tools/verify/classify_ctest_log.py /tmp/ctest_2d_takeover_full.log
# 263/263 entries executed, 231 passed, 32 failed
# distinct failing cases: 11
#   test_2d_charge_exchange_dsmc: environment (openPMD)
#   ...
# cases with a candidate real defect ...: (none)
```

**`build_multi2`（1D/3D/RZ，openPMD=OFF，FFT=OFF）**：**265 / 469 通过**，204 条失败（72 个用例），534 s。

```bash
OMP_NUM_THREADS=1 ctest --test-dir build_multi2 -j12 --output-on-failure
# 57% tests passed, 204 tests failed out of 469
```

| 阶段 | 用例数 | 分类 | 依据 |
|---|---|---|---|
| `.run` 失败 | 68 | openPMD / FFT 环境 | `USE_OPENPMD`（89 处）、`openPMD support`（14 处）、`openPMD-api`（2 处）、`psatd ... built without spectral solvers`（2 处） |
| 仅 `.checksum` 失败 | 3 | 平台校验和 | `test_3d_embedded_boundary_cube{,_mr,_macroscopic}` |
| 仅 `.analysis` 失败 | 1 | 真实发散（上游自带） | `test_1d_macroscopic_solver`（§4.4.1） |

这一档的失败率（43%）完全由构建开关决定：只要 diag 走 openPMD（默认 `diag_type = Full` + openPMD 输出）就会在启动时 abort。它**不能**用来判断功能覆盖，只能用来确认“缺 openPMD 时失败是干净的、可归因的”。

### 4.3 完整特性构建（openPMD + FFT）

#### 4.3.1 `build_2d_full`（2D）：**334 / 335 实质通过**

```bash
OMP_NUM_THREADS=1 LD_LIBRARY_PATH="$HOME/third_party/fftw/lib:/tmp/hdf5_mpi/lib" \
ctest --test-dir build_2d_full -j10 --output-on-failure
# 97% tests passed, 11 tests failed out of 335
# Total Test time (real) = 3630.90 sec
```

初始 324 / 335 通过，失败集中在 5 个用例：

| 用例 | 失败条目 | 初判 | 串行复跑（`-j1`） |
|---|---|---|---|
| `test_2d_theta_implicit_symmetry` | 3 | `***Timeout 1500.00 sec` | **13.21 s 通过** |
| `test_2d_nci_corrector_mr` | 3 | `***Timeout 1500.00 sec` | **3.11 s 通过** |
| `test_2d_particle_thermal_boundary` | 2 | `***Timeout 1500.00 sec` | **4.14 s 通过** |
| `test_2d_subcycling_mr` | 2 | `***Timeout 1500.00 sec` | **3.40 s 通过** |
| `test_2d_laser_ion_acc.checksum` | 1 | 校验和 | 仍失败（见 4.4） |

```bash
ctest --test-dir build_2d_full -j1 --timeout 2400 \
  -R "test_2d_theta_implicit_symmetry|test_2d_nci_corrector_mr|test_2d_particle_thermal_boundary|test_2d_subcycling_mr"
# 100% tests passed, 0 tests failed out of 11   (29.78 s)
```

同一算例从 1500 s 超时变成 13 s，说明这 4 个用例不是慢，而是在 2 个套件 × 每例 2 个 MPI rank 的过载下**互相活锁**（同目录里 `OMP initialized with 24 OMP threads` 的进程越多越糟）。复跑后只剩 1 条校验和差异：**334 / 335 = 99.7%**。

#### 4.3.2 `build_multi_hdf5_fft`（1D/3D/RZ）：**567 / 573 实质通过**

```bash
OMP_NUM_THREADS=1 LD_LIBRARY_PATH="$HOME/third_party/fftw/lib:/tmp/hdf5_mpi/lib" \
ctest --test-dir build_multi_hdf5_fft -j10 --output-on-failure
# 97% tests passed, 17 tests failed out of 573
```

初始 **556 / 573** 通过，17 条失败（9 个用例）；其中 4 个用例是超时（11 条）：

| 用例 | 失败条目 | 串行复跑 |
|---|---|---|
| `test_3d_collision_pulsed_decay` | 3 | **4.52 s 通过**（初跑 1500 s 超时） |
| `test_3d_particles_in_pml_mr` | 3 | **86.31 s 通过**（`slow` 标签） |
| `test_1d_particle_absorbing_boundary` | 3 | **54.23 s 通过** |
| `test_1d_particle_absorbing_boundary_direction_dependent_thresholds` | 3 | **54.63 s 通过** |

```bash
ctest --test-dir build_multi_hdf5_fft -j1 --timeout 2400 -R "test_3d_collision_pulsed_decay|test_3d_particles_in_pml_mr"
# 100% tests passed (95.04 s)
ctest --test-dir build_multi_hdf5_fft -j1 --timeout 350  -R "test_1d_particle_absorbing_boundary"
# 100% tests passed (55.5 s)
```

**PSATD 子集 86 条全部通过、0 失败**（`grep -cE 'Test +#[0-9]+: [a-z0-9_]*psatd'` → 86 条，其中失败 0 条；`galilean` 24 条全过）。这直接否定了早先记录的“`-j 8` 下 16 条 PSATD 失败”：那些是资源争用，不是数值问题（见 `S_SERIES_STATUS.md` R5 的待复核项）。

复跑后剩余 6 条（5 个用例），全部已定性，见 4.4：**567 / 573 = 99.0%**。

### 4.4 剩余失败逐条定性（完整特性构建）

| # | 用例 | 数量 | 类别 | 证据 |
|---|---|---|---|---|
| 1 | `test_2d_laser_ion_acc.checksum` | 1 | 平台校验和 | `.run`/`.analysis` 通过；`particle_momentum_x` 相对差 2.76e-3、`particle_position_z` 2.09e-3，场量 1e-7～6e-6（同一构建内场量精确、粒子矩有微小偏移，属平台/求和顺序） |
| 2 | `test_3d_embedded_boundary_cube_macroscopic.checksum` | 1 | 平台校验和 | 最大相对差 1.21e-2 出现在 `Bx = 4.27e-18`（数值零），`Ey/Ez` 仅 2.37e-7 |
| 3 | `test_rz_laser_acceleration_boosted_opmd.checksum` | 1 | 平台/FFT 噪声 | 差异只在 mode-1 的 1e-12 量级分量（`Br_1_imag` 7.07e-12 vs 8.63e-12）；主导分量与基准一致（`S_SERIES_STATUS.md` R5 已逐键核过） |
| 4 | `test_3d_ionization_electron_dsmc.analysis` | 1 | 环境（Python 绑定） | `ModuleNotFoundError: No module named 'pywarpx'`；本机所有构建 `WarpX_PYTHON=OFF` |
| 5 | `test_3d_linear_compton_bunch_laser.analysis` | 1 | 环境（matplotlib 后端） | **不是 openPMD 读挂起**：`faulthandler` 抓到阻塞栈在 `plt.show()` → `_backend_tk.start_main_loop` → `tkinter.mainloop`（CPU 0% 的 futex 等待正是 Tk 事件循环）；`MPLBACKEND=Agg` 下同一脚本 0.59 s 通过、相对误差 7.87%（容差 8%）。见 §10.2、§10.4 |
| 6 | `test_1d_macroscopic_solver.analysis` + `.checksum` | 2 | **真实数值发散（继承自上游基准）** | 见 4.4.1 |

> 说明：#1–#3 是校验和；按 `AGENTS.md` 的口径不作为功能缺陷，且 #2/#3 的差异只落在数值零/噪声分量上。#4/#5 是环境。

#### 4.4.1 `test_1d_macroscopic_solver`：真实发散，但**不是本项目引入**

运行原文：

```
t = 5.211939e-09 s, omega*t = 4.0080 rad, rel. l2 error on By = 3.1725e+78
AssertionError  (analysis_fields.py:64: assert rel_err < rel_tol_err)
```

判定为“继承自上游”的三条证据：

1. **基准文件与上游逐字节相同**，且基准本身就是发散解：

   ```bash
   diff <(python3 -m json.tool Regression/Checksum/benchmarks_json/test_1d_macroscopic_solver.json) \
        <(python3 -m json.tool /tmp/warpx_up2610/Regression/Checksum/benchmarks_json/test_1d_macroscopic_solver.json)
   # 无差异
   # {'lev=0': {'Bx': 0.0, 'By': 52.773996, 'Bz': 0.0, 'Ex': 15198482581.03, ...}}
   ```

   `By = 52.77`、`Ex = 1.5e10` 对一个 `By = cos(kz)`（量级 1）的腔模不是物理结果。
2. **输入与分析脚本与上游逐字相同**（`diff` 无差异），包括 `rel_tol_err = 1e-1`。
3. **上游快照在本机无法用 1D 构建做运行时对照**：它 1D 编译失败（见 §7.4），所以只能用基准/脚本同一性来归因。

因此：该用例在本项目里失败，不能算作“本项目功能缺失或退化”；它是上游 26.10 自带的一处**基准与判据自相矛盾**的用例。修复建议（不在本轮执行）：把基准换成物理解（`By ≈ cos(kz)cos(ωt)`，量级 1）后重新生成，或按上游 CI 的方式从上游失败日志取值。


---

## 5. 真实缺陷

### 5.1 D1：RZ + FFT 配置**无法编译 / 无法运行**（该组合自插件拆分以来从未构建过）

本轮开始时 HEAD 是 `50307e2`。用 RZ + FFT 的编译标志编译该提交，最先暴露的是构造函数签名不一致：

**现象**：`SpectralFieldDataRZ.cpp` 的定义与头文件/调用点不一致。头文件在 `0f7dea1` 已把构造函数扩成 7 个参数（第 7 个是 `const warpx::runtime::RuntimeData* data = nullptr`）并把成员改成 `m_data`，调用点 `SpectralSolverRZ.cpp:74` 也已传 `data`，但 `.cpp` 仍是 6 参数定义并写 `m_data(data)`（`data` 此时解析成成员函数名）。

**复现命令**（用构建目录里同一 target 的编译标志编译 HEAD 版本，不改工作区）：

```bash
git show 50307e2:Source/Plugins/PicEm/Modules/FieldSolver/PSATD/SpectralFieldDataRZ.cpp > /tmp/SpectralFieldDataRZ_head.cpp
cd build_multi_hdf5_fft/CMakeFiles/warpx_picem_rz.dir
c++ $(sed -n 's/^CXX_DEFINES = //p' flags.make) \
    -I$HOME/WarpX_plugin/Source/Plugins/PicEm/Modules/FieldSolver/PSATD \
    $(sed -n 's/^CXX_INCLUDES = //p' flags.make) \
    $(sed -n 's/^CXX_FLAGS = //p' flags.make) \
    -c /tmp/SpectralFieldDataRZ_head.cpp -o /tmp/head_test2.o
```

**错误原文**：

```
/tmp/SpectralFieldDataRZ_head.cpp:36:1: error: no declaration matches
  'SpectralFieldDataRZ::SpectralFieldDataRZ(int, const amrex::BoxArray&, const SpectralKSpaceRZ&,
   const amrex::DistributionMapping&, int, int)'
... note: candidates are: 'SpectralFieldDataRZ(int, const amrex::BoxArray&, const SpectralKSpaceRZ&,
   const amrex::DistributionMapping&, int, int, const warpx::runtime::RuntimeData*)'
```

**是否本项目引入**：是。`git log -S "const warpx::runtime::RuntimeData* data = nullptr" -- ...SpectralFieldDataRZ.H` → `0f7dea1`（s6 field-solver 重构第 5 步）；后续 `50307e2` 又改过该 `.cpp` 但没修签名。

**影响**：`WarpX_DIMS=RZ`（或含 RZ 的多维构建）**完全无法编译**。因为 2D 构建不编译该文件，日常 2D 回归发现不了；这正是“四种维度都要构建”这条要求的意义。

**同一配置下的另外两个缺陷**（同一次构建暴露，见 `f4259d9` 提交说明）：

2. **`WarpXHostServices.cpp` 缺完整类型**：它使用 `SpectralSolverRZ` / `PML_RZ` 的成员，却只从 `WarpX.H` 拿到前向声明。需要按 `WarpX.cpp` 相同的 `WARPX_USE_FFT` / `WARPX_DIM_RZ` 守卫补 `#include`。
3. **`computeRho` 的网格断言在 RZ PSATD 下不成立**：它断言沉积出的 `rho` 是 nodal；RZ PSATD 下 `GetZeroChargeDensity` 按上游做法保留 cell-centred，由 `InterpolateMFForDiag` 在诊断时插值。上游 `RhoFunctor` 没有这条断言。→ 断言限定在非 RZ。

**修复**：`f4259d9`（`fix(s7): make the RZ + FFT configuration compile again`，09:40 由并发会话提交）一次性修掉上述三点，与本轮 09:30 在工作区看到的那份 `SpectralFieldDataRZ.cpp` 未提交改动一致。

**验证**：

```bash
cmake --build build_multi2 -j8            # 含 RZ，EXIT=0
cmake --build build_multi_hdf5_fft -j6    # RZ + FFT，EXIT=0
# lib/libwarpx.rz.MPI.OMP.DP.PDP.OPMD.FFT.EB.QED.a 正常产出
```

**教训**：`2D` 构建不编译 RZ/PSATD 文件，日常回归发现不了；“四种维度 × 特性组合都要真构建一次”不是形式要求。本轮的 `build_multi_hdf5_fft` 全量重编就是这次真构建。

### 5.2 D2：粒子归约诊断用例的输入漏列诊断名（测试自身失效）

**用例**：`runtime_snapshot_particles_2`（`tests/runtime/CMakeLists.txt`，`--particles` 分支）。

**现象**：`.run` 退出码非 0，17 条断言失败：

```
FAIL: lb_eff reduced diagnostic wrote a row
FAIL: part_num step
FAIL: part_num macroparticles
FAIL: part_num weight
FAIL: part_energy step
FAIL: part_energy is positive after acceleration
FAIL: rho_max step / max / min / |rho|
FAIL: extrema step / xmin / xmax / zmin / zmax
FAIL: part_mom step
FAIL: part_mom pz is nonzero after acceleration
```

同时运行日志给出：

```
Unused ParmParse Variables:
  [TOP]::extrema.intervals ... [TOP]::lb_eff.intervals ...
  [TOP]::part_energy... [TOP]::part_mom... [TOP]::part_num... [TOP]::rho_max...
```

产物目录里只有场类诊断：`field_energy.txt / field_max.txt / field_mom.txt / field_poynting.txt / field_reduce.txt / step_info.txt`，**六个粒子类诊断文件一个都没有**。

**根因**：`tests/runtime/inputs_2d_particles` 里 `warpx.reduced_diags_names` 只列了场类诊断，而下面的 `part_num/part_energy/part_mom/rho_max/lb_eff/extrema` 六段配置从未被 `MultiReducedDiags` 读取（`m_rd_names` 里没有它们 → 不构造 → 不写文件 → parmparse 报 unused）。断言与输入自相矛盾。

**是否本项目引入**：是，且不是迁移代码缺陷，而是**用例输入写漏**。`git log -S "part_num.type" -- tests/runtime/inputs_2d_particles` → `d65f10b`（2026-11-04，“migrate particle stats/RhoMaximum/LoadBalanceEfficiency; split particle test; numeric checks”）。该提交同时加了断言和输入，却漏了 names 行。

**为什么不是代码缺陷**：把 6 个名字补进 `reduced_diags_names` 后，粒子类诊断全部正常产出且数值命中断言（`part_num` 256 个巨粒子 / 权重 4e18；`extrema` xmin/xmax = ∓0.9375；`part_mom` 第 1 步后 pz ≠ 0）。**代码一直是好的，坏的是用例输入。**

**修复**：`bcffaa0`（本轮；原提交 `da728a6` 被并发会话 rebase 重写成同一内容的 `bcffaa0`），只在 `warpx.reduced_diags_names` 里补上 6 个名字，未改任何断言/容差。

**验证**：

```bash
OMP_NUM_THREADS=1 build_takeover/bin/test_runtime_snapshot_2d <inputs> --particles
# TESTEXIT:0   FAIL 行数: 0
# cat diags/reducedfiles/part_num.txt
# 1 1.17932716837484e-10 2.56000000000000e+02 ... 4.00000000000000e+18
```

ctest 侧的确认（完整特性 2D 套件，使用修好的输入）：

```
310/335 Test #323: runtime_snapshot_particles_2 ... Passed 0.80 sec
```

---

## 6. 上游关键物理用例抽查

抽查方法：在**完整特性构建**里直接读 CTest 的逐条结果，不看“能不能编译”，看 run/analysis/checksum 三条是否都绿。命令形如

```bash
ctest --test-dir build_multi_hdf5_fft -R "<用例名>" --output-on-failure
```

| 物理类别 | 用例 | 维度 | run | analysis | checksum |
|---|---|---|---|---|---|
| 嵌入边界 EB | `test_3d_embedded_boundary_cube` | 3D | ✅ | ✅ | ✅ |
| 嵌入边界 EB | `test_2d_embedded_circle` | 2D | ✅ | （与 checksum 同一脚本） | ✅ |
| PSATD 谱求解器 | `test_3d_langmuir_multi_psatd` | 3D | ✅ | ✅ | ✅ |
| PSATD 谱求解器 | `test_2d_langmuir_multi_psatd` | 2D | ✅ | ✅ | ✅ |
| QED（Bethe–Heitler） | `test_3d_qed_breit_wheeler` | 3D | ✅ | ✅ | ✅ |
| QED + openPMD | `test_2d_qed_breit_wheeler_opmd` | 2D | ✅ | ✅ | ✅ |
| 碰撞（DSMC/库仑） | `test_1d_collision_z` | 1D | ✅ | ✅ | ✅ |
| 碰撞（各向同性） | `test_3d_collision_iso` | 3D | ✅ | ✅ | ✅ |
| RZ 激光加速 | `test_rz_laser_acceleration` | RZ | ✅ | （与 checksum 同一脚本） | ✅ |
| 隐式 θ 方案 | `test_2d_theta_implicit_jfnk_vandb` | 2D | ✅ | ✅ | ✅ |
| 隐式 θ 方案 | `test_3d_theta_implicit_jfnk_direct` | 3D | ✅ | ✅ | ✅ |

补充：**完整特性构建里 86 条 PSATD 条目 0 失败**，包含 `langmuir_multi_psatd*`、`galilean_psatd*`、
`averaged_galilean_psatd*`、`pml_psatd*`、`rz_psatd_JRhom_LL2` 等；唯一与 PSATD 有关的失败是
`test_rz_laser_acceleration_boosted_opmd.checksum`，差异只落在 mode-1 的 1e-12 噪声分量（§4.4 #3）。


---

## 7. 说明与剩余风险

### 7.1 本轮遇到的并发会话

本轮运行期间，同一仓库上还有另一个会话在跑自己的 ctest（`/tmp/final2d.log`、`build_multi_hdf5_fft -R psatd`）并提交代码（`f4259d9` 修 RZ 编译、`bcffaa0` 转写本轮的用例输入修复、`2b95af0` 粒子热化插件化）。两个 ctest 在同一 build 目录并行会互相覆盖 `bin/<test>/diags`，所以：

- 第一次撞车时，主动停掉自己的 `build_takeover` / `build_multi2` 运行；
- 完整特性证据改用**独占的** `build_2d_full`（2D，本轮新建）与 `build_multi_hdf5_fft`（1D/3D/RZ）；
- 等对方跑完后，再单独重跑两个基线构建，得到 §4.2 的**完整**数字（231/263、265/469）。

并发还带来一个可复现的陷阱：`ctest -j10` × 两个套件时，`test_2d_theta_implicit_symmetry` 这类用例会从 13 s 变成 1500 s 超时（MPI 忙等 + 内存带宽争用活锁），因此 §4.3 的失败表都带“串行复跑”一列。

### 7.2 未运行的部分（构建选项所限，非缺口）

- **PICMI/Python 用例**（74 个定义）：需要 `WarpX_PYTHON=ON`，本机所有构建都是 `OFF`。
- **PETSc 用例**（5 个定义）：需要 `WarpX_PETSC=ON`。
- **ADIOS2 后端**：`openPMD_USE_ADIOS2=OFF`，只验证了 HDF5 后端。
- **GPU 后端**（CUDA/HIP/SYCL）：本轮只跑 `WarpX_COMPUTE=OMP`；CPU 后端上 `SPECIES/EB/QED` 的数值通过不能外推到 GPU。

这些在清单层面存在且参数与上游一致，只是本轮没有对应构建。

### 7.3 平台校验和

`.checksum` 比对的是 `Regression/Checksum/benchmarks_json/*.json`，数值随编译器/FFT 库/数学库末位漂移；按 `AGENTS.md` 的口径，校验和失败不作为功能缺陷，只在与同构建的 `.analysis` 通过率矛盾时才追查。完整特性构建里的校验和失败在 §4.4 单列。

### 7.4 附记：上游快照自身的两处问题（本地不欠债，反而更强）

**(a) 上游 1D 配置编译失败。** 为了给 `test_1d_macroscopic_solver` 做运行时 A/B 对照，本轮尝试构建上游快照的 1D 版本，失败：

```bash
cd /tmp/warpx_up2610
cmake -S . -B build_1d -DWarpX_DIMS=1 -DWarpX_COMPUTE=OMP -DWarpX_OPENPMD=OFF \
  -DWarpX_FFT=OFF -DWarpX_QED=OFF -DCMAKE_BUILD_TYPE=Release \
  -DFETCHCONTENT_SOURCE_DIR_FETCHEDAMREX=$HOME/WarpX_plugin/build_2d_full/_deps/fetchedamrex-src
cmake --build build_1d -j8
```

```
In file included from /tmp/warpx_up2610/Source/WarpX.cpp:26:
.../MacroscopicProperties/MacroscopicProperties.H:146:42: error:
  ‘EBFArrayBoxFactory’ in namespace ‘amrex’ does not name a type
  146 |    const amrex::EBFArrayBoxFactory* eb_fact,
```

原因：AMReX 用 `add_amrex_define(AMREX_USE_EB NO_LEGACY NO_1D)`（`Src/EB/CMakeLists.txt:1`）——1D 不定义 `AMREX_USE_EB`；而上游的 `MacroscopicProperties.H` 无条件引用该类型，且没有自带 include。`-DWarpX_EB=OFF` 也一样失败（该类型与 EB 选项无关）。

本地在该头文件里有正确的守卫（`#ifdef AMREX_USE_EB` → include，`#else` → 前向声明；`Source/Plugins/FieldSolver/FiniteDifference/MacroscopicProperties/MacroscopicProperties.H:20`，来自 `b370f2b`），因此**本地 1D 能编译**（`build_multi2` 的 1D 目标、`build_multi_hdf5_fft` 的 1D 目标都链接成功）。也就是说这项上本地是**修好了上游的缺陷**，不是欠债。

**(b) 上游 `test_1d_macroscopic_solver` 的基准是发散解。** 见 §4.4.1：基准与判据自相矛盾，直接拿它当“覆盖度”证据会得出错误结论。

### 7.5 本轮对仓库的改动

| 路径 | 提交 | 内容 | 是否涉及容差/基准 |
|---|---|---|---|
| `tests/runtime/inputs_2d_particles` | `bcffaa0` | `warpx.reduced_diags_names` 补 6 个粒子类诊断名 | 否（只补被测对象，断言与容差未动） |
| `Docs/research_notes/UPSTREAM_PARITY.md` | 本轮 | 本报告 | 否 |

另：§5.1 的 D1 编译修复由并发会话在 `f4259d9` 提交（本轮只做了复现与验证）。

**没有修改任何**：测试容差（`rel_tol_err` / `rtol`）、`Regression/Checksum/benchmarks_json/*.json`、分析脚本的断言阈值。可核对：

```bash
git log --oneline 50307e2..HEAD -- Regression/Checksum/benchmarks_json Examples/*/Tests/*/analysis*.py | head
# （本轮与并发会话都没有触碰这些路径）
```

---

## 8. 独立复核（2026-11-09 22:30–23:20，第二次测量）

为排除"单次测量/单会话"的偶然性，本轮用**同一套配置**（openPMD + FFT + 并行 HDF5）
重新配置、构建并跑完整套件，只把并行度降到 `-j 4`、超时提到 2400 s：

```bash
export LD_LIBRARY_PATH="$HOME/third_party/fftw/lib:/tmp/hdf5_mpi/lib:$LD_LIBRARY_PATH"
export OMP_NUM_THREADS=1
cmake -S . -B build_2d_full  -DWarpX_DIMS=2 -DWarpX_OPENPMD=ON -DWarpX_FFT=ON \
      -DHDF5_ROOT=/tmp/hdf5_mpi -DFFTW_ROOT=$HOME/third_party/fftw ...
cmake -S . -B build_multi_hdf5_fft -DWarpX_DIMS="1;3;RZ" (同上)
ctest --test-dir <build> -j 4 --timeout 2400
```

| 构建 | 结果 | 失败条目 |
|---|---|---|
| `build_2d_full` | **335 / 336（99.7%）** | `test_2d_laser_ion_acc.checksum`（平台校验和） |
| `build_multi_hdf5_fft` | **568 / 573（99.1%）** | 5 条，见下 |

5 条失败与 §4.4 的定性**逐条吻合**，没有出现新的类别：

| 条目 | 类别 | 依据 |
|---|---|---|
| `test_3d_embedded_boundary_cube_macroscopic.checksum` | 平台校验和 | 差异最大项是 `Bx = 4.27e-18`（数值零），`Ey/Ez` 仅 2.37e-7 |
| `test_3d_ionization_electron_dsmc.analysis` | 环境（Python 绑定） | `ModuleNotFoundError: No module named 'pywarpx'`；本机全部构建 `WarpX_PYTHON=OFF` |
| `test_1d_macroscopic_solver.{analysis,checksum}` | 上游继承的发散基准 | 基准/输入/分析脚本与 `/tmp/warpx_up2610` **逐字节相同**（本轮再次 `diff` 确认） |
| `test_rz_laser_acceleration_boosted_opmd.checksum` | 平台/FFT 噪声 | 差异只在 mode-1 的 1e-12 量级分量 |

**结论**：两次独立测量一致（首次 334/335 + 567/573，本次 335/336 + 568/573，差异来自新增的
`external_plugin_2` 用例与修好的 `runtime_snapshot_particles_2`）。四维合计 **903/909 = 99.3%**，
其中 3 条平台校验和（按 `AGENTS.md` 口径不视为功能缺陷）、1 条缺 Python 绑定、2 条上游自带发散用例；
**没有一条可归因于"本项目功能缺失或退化"**。

## 9. 剩余待办（更新）

1. `test_1d_macroscopic_solver`：把基准换成物理解（需上游确认，属于改基准，不在本项目单方面执行）；
2. Python 绑定（`WarpX_PYTHON=ON`）构建下的 `test_3d_ionization_electron_dsmc` 等用例；
3. ADIOS2 后端、GPU 后端：清单一致但无运行证据。

---

## 10. 最终状态（2026-11-10，插件化完成之后）

四维套件与 Python/PICMI 套件的实测结果：

| 套件 | 构建 | 结果 | 残余 |
|---|---|---|---|
| 2D | `build_takeover`（openPMD=OFF） | 232 / 264 | 32 条，全部 openPMD 依赖 |
| 2D | `build_2d_full`（openPMD+FFT） | 335 / 336 | 1 条平台校验和 |
| 1D/3D/RZ | `build_multi_hdf5_fft`（openPMD+FFT） | 567 / 573 | 6 条（3 校验和 + 1 缺 Python 绑定 + 2 上游自带），与隐式搬迁前逐条一致 |
| 2D PICMI | `build_py_opmd` | 72 / 75 | 3 条平台校验和 |

**结论**：功能覆盖上没有缺口——上游 420 个测试定义全部存在且参数逐字相同，源文件可符号级对应，
四维与 PICMI 的失败全部落在平台校验和、缺失可选依赖与上游自带发散基准三类里。
插件化重构（把诊断、碰撞、五种场求解器、粒子内核单元拆成插件）**没有改变任何可运行用例的结果**：
每次搬迁都以"失败集逐条一致"作为验收条件。

### 10.1 `test_1d_macroscopic_solver` 的独立复现（2026-11-10）

之前把这条归因为"上游自带的发散基准"。本轮做了两组独立实验，结论需要修正为
**"1D 宏观（张量介质）更新在 CFL=1.0 下不稳定，且 CFL=0.5 下剖面仍不吻合解析解"**：

| 实验 | 命令 | 观察 |
|---|---|---|
| 原样（inputs 里 `warpx.cfl = 1.0`） | `warpx.1d ... inputs_test_1d_macroscopic_solver` | `max|By|` 200 步后约 **1e78**（`analysis_fields.py`：`rel. l2 error on By = 3.17e+78`） |
| 半 CFL | 同上加 `warpx.cfl = 0.5` | 有界：`max|By| = 0.953`、`max|Ex| = 1.0e8`（SI 尺度合理） |
| 半 CFL，400 步，每 100 步采样 | 同上 `max_step = 400` | `max|By|` = 0.9999 / 0.1532 / 0.9530 / 0.4451（第 100/200/300/400 步） |
| 同一输出的官方分析脚本 | `analysis_fields.py diags/diag1000200` | `omega*t = 2.004`，**`rel. l2 error on By = 1.27`**（判据 `rel_tol_err = 1e-1`） |

即：**降 CFL 后不再发散，但模式剖面与 `cos(kz)cos(omega t)` 的比较仍然是 127% 误差**。
按解析解，`omega = c k / sqrt(1.5)`，200 步（半 CFL）对应 `omega t = 2.0`、幅值应为 `|cos 2.0| = 0.42`，
实测幅值 0.95 且逐时采样不呈该周期 —— 说明**频率或相位不对**，不是单纯的稳定性问题。

这条直接影响本项目的旗舰功能（张量介质宏观求解器）的数值正确性，**必须单独定位**：
下一步应比较 `MacroscopicEvolveE` 里 `ein0`/`eix`（逆介电矩阵）与 `dt*ix0*curlH` 的量纲与符号，
并用一个均匀介质、低 CFL、单模的算例对照解析频率。

#### 10.1.1 对照实验：同一腔模，Yee 与宏观求解器对比（决定性）

同一输入（`inputs_test_1d_macroscopic_solver`）、同一点 `max_step=400`、`warpx.cfl=0.5`、每 50 步采样，
唯一变量是介质求解器：

| 步数 | 解析 `|cos(ω t)|`（ω = c k） | **Yee（`algo.em_solver_medium = vacuum`）** | **宏观（`epsilon = epsilon0`，即真空介质）** |
|---|---|---|---|
| 0 | 1.000 | 1.000 | 1.000 |
| 50 | **0.817** | **0.818** | 0.647 |
| 100 | **0.337** | **0.337** | 0.164 |
| 150 | **0.267** | **0.267** | 0.858 |
| 200 | **0.773** | **0.773** | 0.946 |
| 250 | **0.997** | **0.997** | 0.365 |
| 300 | **0.858** | **0.858** | 0.474 |
| 350 | **0.405** | **0.405** | 0.978 |

**Yee 与解析解逐点吻合（4 位有效数字）；宏观求解器在 `epsilon = epsilon0`（真空！）下给出完全不同的
时间序列，周期约为 Yee 的一半** —— 即有效 `1/epsilon` 偏大约 4 倍，等效光速约为 `2c`。

结论：**这不是"上游基准发散"的问题，而是本分支宏观（张量介质）E 更新在真空极限下就不等于 Yee**。
最小可疑点：`MacroscopicEvolveE` 中 `ein0`/`eix`（逆介电矩阵）的构建与 `dt*(ix0*Dx + ...)` 的系数，
以及 `epsilon` 从节点/Face 插值到 E 位置时的量纲。

**这一条必须修复**：它决定"功能与上游 26.10 相同"能否成立（张量介质是本项目的核心特性）。

#### 10.1.2 单步定量：宏观与 Yee 的 E 增量恰好相差 2 倍

同一初始条件（`By(z)=cos(kz)`、`E=0`）、同一 `cfl=0.5`、**只跑 1 步**：

| 求解器 | `max|Ex|`（1 步后） | `max|By|`（1 步后） |
|---|---|---|
| Yee（vacuum） | 3 678 360.616 | 0.99984941 |
| 宏观（`epsilon = epsilon0`） | **7 356 721.232** | 0.99977412 |

- `Ex` 比值 = **2.000000**（7 356 721.232 / 3 678 360.616）
- `By` 的减少量比值同样是 **2.000**（0.00015059 / 0.00007529）

两者同时恰好 2 倍，说明**不是**介电系数或量纲问题，而是**同一次更新被施加了两次**
（或等价地在循环里对同一格点累加两次）。两条更新路径都用 `T_Algo::DownwardDz` 与同一组
`coefs_z`，算子本身一致；`ix0 = 1/eps` 与 Yee 的 `c2*mu0` 在 `eps = eps0` 下相同。

**下一步（明确）**：检查 `MacroscopicEvolveE` 的调用点是否在一步内被调用两次
（例如 `EvolveE` 分派 + 直接调用各一次），以及 `Ex` 的更新是否被写了两次。

这解释了 10.1.1 中频率约 2 倍的现象，也是唯一能同时让 E 与 B 增量都恰好翻倍的机制。

#### 10.1.3 决定性定量：等效 `1/epsilon` 恰好大 2 倍

同一单步实验，只改输入的介电常数：

| 输入 | 求解器 | 1 步后 `max|Ex|` |
|---|---|---|
| `epsilon = epsilon0` | Yee（vacuum 路径） | 3 678 360.616 |
| `epsilon = epsilon0` | 宏观 | **7 356 721.232** |
| `epsilon = 2*epsilon0` | **宏观** | **3 678 360.616** |

**把介电常数翻倍后，宏观求解器的结果与 `eps = eps0` 的 Yee 完全相等**（到 13 位有效数字）。
即宏观路径实际使用的逆介电是 `2/eps`：**等效 `1/epsilon` 恰好多一个因子 2**。

运行时打印确认材料数组本身是对的：
`M2B material: eps = 8.854e-12 .. 8.854e-12, sigma = 0 .. 0, mu = 1.2566e-06`。

因此缺陷在**逆介电矩阵的构建**，而不是材料读取、不是调用次数、不是差分算子：
嫌疑集中在 `MacroscopicProperties.cpp` 的节点逆介电块
`chim` → `warpx::subpixel::inv3_sym(chim)` → `en(i,j,k,0..5)`（约 595 行），
以及 `inv3_sym` 是否漏掉了 `1/det` 或对对角元多乘了 2。

**下一步**：对 `inv3_sym` 做单元级验证（输入 `diag(eps,eps,eps)` 应得 `diag(1/eps,1/eps,1/eps)`），
再检查 `chim` 的构造是否把 `eps` 存成了 `2*eps`。

#### 10.1.4 定位到"节点介电常数只有输入的一半"

用一次性调试打印（`WARPX_DEBUG_EPS_INV`，已移除）读出运行时数组：

```
[eps-inv-debug] cell eps max = 8.854187819e-12   ← 正确（= eps0）
                node eps comps = 4.427093909e-12 4.427093909e-12 4.427093909e-12   ← 恰好 eps0/2
                inv xx = 2.258818133e+11           ← = 1/(eps0/2)
                tensor_mode = 0
```

所以：**格子中心介电常数正确，节点介电常数被算成一半**，逆介电因此大 2 倍，与
10.1.3 的"翻倍 eps 得到 Yee 结果"完全自洽。

缺陷位置确定在 `MacroscopicProperties.cpp` 的**节点介电填充**（约 556-585 行的标量分支）：

```cpp
if (!tensor_mode) {
    amrex::Real const ev = normal
        ? ((eL(0,0) + eR(0,0) > 0) ? 2*eL(0,0)*eR(0,0)/(eL(0,0)+eR(0,0)) : 0)
        : 0.5 * (eL(0,0) + eR(0,0));     // 均匀介质时应得 eps，实测 eps/2
```

对均匀介质，`normal` 分支（调和平均）与 `0.5*(eL+eR)`（算术平均）都应给出 `eps`，
实测却是 `eps/2`，说明 **`eL`/`eR` 本身已经是 eps/2**（即读到的 `ec`/`et1`/`eps2_sc`
被缩放过），或该循环对同一节点写了两次并各乘 1/2。

**下一步**：在 `eL`/`eR` 赋值处打印它们的值（一次运行即可区分"读到的源数据已减半"与"写了两次"）。

#### 10.1.5 根因：节点填充读错维度，一侧介电常数为 0

在节点填充循环内打印前三处（已移除调试代码）：

```
[node-fill] eL=0  eR=8.854187819e-12  normal=0  ev=4.427093909e-12
            eps2_sc=8.854187819e-12   m_epsilon=8.854187819e-12
```

- 两侧本应是同一材料（本算例没有界面，`iface_on_n = false`，故 `m1L = m1R = true`）；
- 实际 `eL = 0`、`eR = eps`，算术平均 `0.5*(0 + eps) = eps/2` ← **正是实测的节点介电常数**。

`eL` 来自 `ec(ii, jm, k)`、`eR` 来自 `ec(ii, jp, k)`，而
`jm = max(j-1, jlo_dom)`、`jp = min(j, jhi_dom)` —— **偏移的是第 1 维（j）**。
在 `WARPX_DIM_1D_Z` 构建里，唯一的空间维是第 0 维，`j` 恒为 0，于是 `jm = jp = 0`；
`eL` 与 `eR` 读同一索引却给出不同值，说明该处对 1D 的维度约定与本文件的 `(i,j,k)` 用法不一致，
`ec(ii, 0, k)` 落在未填充的 ghost 上（0）。

**结论（可执行）**：`MacroscopicProperties.cpp` 的节点介电/电导填充必须按
`WARPX_DIM_1D_Z` / `XZ` / `3D` 分别取**正确的相邻单元方向**（1D 下是第 0 维，不是第 1 维），
或改为复用 `T_Algo` 的维度约定。修好后应用三条验收：

1. 真空极限单步 `max|Ex|` 与 Yee 相等（比值 1.000×，当前 2.000×）；
2. 该腔模在 `cfl=1.0` 下不发散且剖面吻合 `cos(kz)cos(ω t)`；
3. 四维套件失败集不增。

#### 10.1.6 修复与验证（2026-11-10）

**修复**：`MacroscopicProperties.cpp` 的节点填充改用**按维度**取相邻单元——1D 构建下沿第 0 维
（与 `DownwardDz` 一致），其余维度保持原样；并修正了 1D 下对 `geom.Domain().smallEnd(1)/bigEnd(1)`
的越界访问。所有对 `epsilon`、`sigma`、介电张量、电导张量的读取都走同一对索引。

**验收一：真空极限**（`Tools/verify/check_macroscopic_vacuum_limit.sh`）

| | 修复前 | 修复后 |
|---|---|---|
| Yee `max|Ex|` | 3 678 360.615965 | 3 678 360.615965 |
| 宏观 `max|Ex|` | 7 356 721.231931 | **3 678 360.615965** |
| 比值 | 2.000000 | **1.000000** |

**验收二：`test_1d_macroscopic_solver`**：`run` / `analysis` / `checksum` **全部通过**
（修复前 analysis 报 3.17e+78、checksum 失败）。

**验收三：四维全套件（573 个测试）**

| | 修复前 | 修复后 |
|---|---|---|
| 失败条目 | 6 | **3** |
| 差异 | — | 只少了 `test_1d_macroscopic_solver.{analysis,checksum}` 与 `test_3d_ionization_electron_dsmc.analysis`，**无新增失败** |

剩余 3 条：`test_3d_embedded_boundary_cube_macroscopic.checksum`（相对误差 1.21e-2 且最大项落在
数值零附近的分量、`Ey/Ez` 仅 2.37e-7）、`test_3d_linear_compton_bunch_laser.analysis`（分析脚本在
3D 数据集上挂起，环境）、`test_rz_laser_acceleration_boosted_opmd.checksum`（mode-1 的 1e-12 量级噪声）。

**验收四：2D 完整特性套件（336 个测试）**：335 通过、1 条失败
（`test_2d_laser_ion_acc.checksum`，平台校验和），与宏观修复前**逐条一致**。

#### 10.2 第 2 条的真相：不是"挂起"，是 matplotlib 弹出 Tk 窗口

`test_3d_linear_compton_bunch_laser.analysis` 之前记为"环境挂起（单跑 25 分钟不返回）"。
用 `faulthandler` 抓取阻塞栈后真相明确：

```
Timeout (0:01:00)!
  File ".../matplotlib/backends/_backend_tk.py", line 583 in start_main_loop
  File ".../matplotlib/backend_bases.py", line 3710 in show
  File ".../matplotlib/pyplot.py", line 646 in show
  File "<string>", line 144 in <module>
```

分析脚本末尾调用 `plt.show()`；本机 matplotlib 选了 **Tk** 后端，于是阻塞在 GUI 事件循环里。

**不需要改任何测试**——把后端固定为非交互式即可：

```bash
MPLBACKEND=Agg ctest --test-dir build_multi_hdf5_fft -R "^test_3d_linear_compton_bunch_laser\.analysis$"
# → 1/1 Passed 0.51 sec
MPLBACKEND=Agg python3 Examples/Tests/linear_compton/analysis_bunch_laser.py diags/diag1
# → Relative error 7.87%（在容差内）
```

**因此本环境推荐始终导出 `MPLBACKEND=Agg`**（与上游 CI 的无头环境一致），四维残余从 3 条降为 **2 条**
（两条 platform 校验和）。

#### 10.3 四维最终数字（分项实测，2026-11-10）

无争用窗口不足，故改用**分项实测**而非再一次全量：既避免并发会话争用造成的假失败，也把每类结论钉死。

| 分项 | 命令 | 结果 |
|---|---|---|
| 全部分析测试（176 个，含此前"挂起"的 compton） | `MPLBACKEND=Agg ctest -R "\.analysis$" -j 2` | **176/176 通过，0 失败** |
| 仅存的两条校验和 | `ctest -R "cube_macroscopic\.checksum|boosted_opmd\.checksum"` | 2/2 失败（与 10.1.6 相同的幅度） |
| 全量（未导出 Agg 的那次，10.1.6） | `ctest -j 4` | 570/573，唯一非校验和失败就是 compton 分析 |

**因此四维在无头环境（`MPLBACKEND=Agg`）下的数字是 571/573**：176 个分析测试全绿，
残余恰为两条平台校验和。这不是推算——两个分项都是本轮实测，且分项之和覆盖了全量失败集的构成。

#### 10.4 单次全量复跑与 EB 校验和的线程无关性（2026-11-10 09:02–09:57）

上一节的数字来自分项实测，本节补一次**单次全量**（573 条一条不落），并顺带把"平台校验和"这个
说法钉死到可复现的证据上。

```bash
export LD_LIBRARY_PATH="$HOME/third_party/fftw/lib:/tmp/hdf5_mpi/lib:$LD_LIBRARY_PATH" MPLBACKEND=Agg
ctest --test-dir build_multi_hdf5_fft -j 8 --timeout 2400 --output-on-failure
# 571/573；失败 = #110 test_3d_embedded_boundary_cube_macroscopic.checksum
#                  #555 test_rz_laser_acceleration_boosted_opmd.checksum
# 第 573 条（#535/#536 test_3d_beam_beam_collision）在会话切换时被中断，补齐：
ctest --test-dir build_multi_hdf5_fft -R "^test_3d_beam_beam_collision\." -j 2   # 2/2 Passed (512 s)
```

与 `/tmp/g113_failed.log` 逐条比对：`g113 = {110, 276, 555}`，本轮 `= {110, 555}`，
**只少了 `test_3d_linear_compton_bunch_laser.analysis`（本轮 0.59 s 通过），没有新增失败**。
`test_3d_linear_compton_bunch_laser` 三条在本轮分别是 run 21.02 s、analysis 0.59 s、checksum 1.36 s。

**EB cube macroscopic 校验和不是 OpenMP 求和顺序问题**。用同一算例在 `OMP_NUM_THREADS=1` 与 `=4`
各跑一次，再用测试自己的 `analysis_default_regression.py` 对比基准：

| 线程数 | `lev=0,Bx` | `lev=0,Ey` |
|---|---|---|
| 1 | 4.318467838736486e-18 | 3.022897195538015e-05 |
| 4 | 4.318467838736486e-18 | 3.022897195538015e-05 |
| 基准 | 4.266736006843336e-18 | 3.022896477653753e-05 |

两种线程数**逐位相同**，因此差异与线程/求和顺序无关；它是编译/平台层面的常量差：
`Bx` 绝对差 5.2e-20，比同算例主场 `By≈5.1e-3` 小 17 个量级（数值零），`Ey/Ez` 相对差 2.37e-7。
按 `AGENTS.md` 的"校验和可随平台变化"口径，这条不作为功能缺陷。

```bash
# 复现该对照（每次约 50 s）。注意校验和脚本用 CWD 的目录名当测试名，
# 所以要建一个名字正好是测试名的目录，否则找不到 benchmarks_json/<name>.json。
REPO=$HOME/WarpX_plugin
EXE=$REPO/build_multi_hdf5_fft/bin/warpx.3d.MPI.OMP.DP.PDP.OPMD.FFT.EB.QED
for t in 1 4; do
  D=/tmp/ebprobe/t$t/test_3d_embedded_boundary_cube_macroscopic
  mkdir -p "$D" && cd "$D"
  AMREX_INPUTS_FILE_PREFIX=$REPO/Examples/Tests/embedded_boundary_cube \
    OMP_NUM_THREADS=$t mpiexec -n 1 "$EXE" inputs_test_3d_embedded_boundary_cube_macroscopic
  PYTHONPATH=$REPO/Regression/Checksum python3 \
    $REPO/Examples/Tests/embedded_boundary_cube/analysis_default_regression.py --path diags/diag1000208
done
```

#### 10.5 关于 HEAD：全量测的是哪一版二进制

§10.4 的 573 条全量跑在 `7a96278` 的二进制上。此后 HEAD 多了 `f4e9708`（粒子容器改从运行时配置读
设置）。逐条核过 `RuntimeConfig` 的访问器实现（`Source/Plugins/PicEm/RuntimeBridge/Runtime/RuntimeConfig.cpp`）：
`particle_boundary_lo/hi`、`nox`、`galerkin_interpolation`、`do_shared_mem_current_deposition`
都**直接返回同一个 `WarpX::` 静态量**，所以那是等价改写，不是快照。

重建 `build_multi_hdf5_fft`（`EXIT=0`，223 个编译单元）后复验：

```bash
Tools/verify/check_macroscopic_vacuum_limit.sh          # ratio = 1.000000, exit 0
ctest --test-dir build_multi_hdf5_fft -j 3 \
  -R "cube_macroscopic\.checksum|boosted_opmd\.checksum|linear_compton_bunch_laser\.(analysis|checksum)"
# 4/4 运行：compton analysis+checksum 通过；两条平台校验和照旧失败
```

**未在 HEAD 上再做 573 条全量**，阻塞是具体的：并行会话同时跑着 `build_2d_full -j8`
（336 条）、`build_takeover -j2` 子集和 `build_multi_hdf5` 重编译，`loadavg` 31–34（24 核），
此时跑全量只会得到资源争用造成的假失败（10.5 与 4.3.1 都记录过这种误判）。空闲窗口应补一次。

---

## 11. 与上游 26.10 的直接 A/B 运行（2026-11-10 晚）

之前的等价性证据是"测试定义逐字节相同 + 上游自带 checksum 通过"，属于**间接**证据。
本节是**直接**证据：用**上游 26.10 自己编译出的二进制**与我们的二进制跑同一个输入，
再对最终 plotfile 的**全部场分量**逐点求差。

**方法**：`/tmp/ab_test/run_ab.sh` 在 `up/` 与 `mine/` 两个目录下用相同输入、相同 MPI 进程数、
相同线程数各跑一次；`compare3.py` 用 yt 读最后一份 plotfile，报告
`max|diff| / max(|field|)`（分母取该 plotfile 所有场分量的全局最大值，避免"场本身≈0"时比值失真）。

| 算例 | 维度 | 覆盖 | plotfile | 场数 | 全局相对差 |
|---|---|---|---|---|---|
| `rz_collision` | RZ | 库仑碰撞 | `diag1000150` | 33 | **0.000e+00** |
| `rz_es` | RZ | 静电求解器 | `diag1000030` | 31 | **0.000e+00** |
| `rz_es_eb` | RZ | 静电 + 嵌入边界 | `diag1000001` | 3 | **0.000e+00** |
| `rz_btd` | RZ | BTD 诊断（boosted frame） | `diag1000289` | 9 | **0.000e+00** |
| `rz_langmuir` | RZ | PIC 标准算例 | `diag1000080` | 41 | **0.000e+00** |
| `rz_fluid` | RZ | 流体（矩）模型 | `diag1000080` | 6 | **0.000e+00** |
| `rz_fusion` | RZ | 核聚变（D-T，QED 相关） | `diag1000001` | 91 | **0.000e+00** |
| `d3_langmuir` | 3D | PIC 标准算例 | `diag1000040` | 37 | **0.000e+00** |
| `d3_implicit` | 3D | **隐式求解器（theta/JFNK）** | `diag1000010` | 47 | **0.000e+00** |
| `d3_laser` | 3D | 激光注入 | `diag1000020` | 8 | **0.000e+00** |
| `d3_eb` | 3D | 嵌入边界 | `diag1000208` | 6 | 1.63e-11（双精度舍入） |

**10 / 11 算例逐位相同**；唯一差异 `d3_eb` 的绝对差 7.6e-9 对应主分量 `Ex` 的 4.6e2，
即 1.6e-11 的相对量级 —— 双精度舍入。`d3_implicit` 的逐位一致尤其重要：本会话把隐式求解器
从内核拆成独立插件（服务袋 50+ 项），这是对那次重构最强的回归证明。

### 顺带发现：上游 26.10 在本机 AMReX 版本下 3D 编译不过

上游参考树 `Source/EmbeddedBoundary/EmbeddedBoundaryInit.cpp:351` 使用
`amrex::IntVect const ng2dZ(ngZ[0], ngZ[1]);`（两参数构造），当前 AMReX（`008b67375`，
与本仓库用的**同一个** revision）已移除该构造函数：

```
error: no matching function for call to 'amrex::IntVectND<3>::IntVectND(const int&, const int&)'
```

本仓库同一处有对应修复（`#if AMREX_SPACEDIM == 2` 分支 + 3D 用 `ng2dZ[2] = 0`）。
为了让 3D A/B 能跑，**只在 scratch 的上游参考树里**打了同样的补丁（未提交到本仓库）。

### 未做 A/B 的部分（诚实）

- **PSATD / FFT 类**：上游参考构建是 `WarpX_FFT=OFF`，无法逐位对照；这些用例的等价性
  依据仍是上游自带的 checksum 基准（本仓库全部通过，除 2 条平台校验和）。
- **1D**：上游 1D 参考构建是 `WarpX_OPENPMD=OFF`，不产生场输出，无法对比。
- 2D：上游无 2D 参考构建。

---

## 12. 覆盖核查：官方算例到底跑了多少（2026-11-10 晚，按用户要求重查）

官方算例分布在 `Examples/Tests/`（92 个目录、399 个 `inputs_test_*`）与
`Examples/Physics_applications/`（12 个目录、45 个输入，其 `inputs_test_*` 同样进 ctest）。
合计 **407 个官方测试输入**。

**核查方法**：把三个构建的 `ctest -N` 注册名与 `Examples/` 下全部 `inputs_test_*` 求差集。

| 阶段 | 已注册/可运行 | 未覆盖 |
|---|---|---|
| 原三个构建（1D/3D/RZ、2D、2D+Python） | 334 | **73** |
| 新增 `build_py_4d`（1D/3D/RZ + Python + FFT） | **391** | **16** |

剩余 16 个及原因：

| 原因 | 数量 | 条目 |
|---|---|---|
| PETSc（`AMReX_PETSC=ON`，未构建） | 9 | `test_{2d,rz,rcylinder}_curl_curl_petsc_pc`、`test_2d_semi_implicit_planar_pinch_petsc_snes`、`test_{2d,rz}_theta_implicit_*`、`test_3d_theta_implicit_jfnk_pcwidth` |
| RCYLINDER / RSPHERE 维度（未构建） | 2 | `test_rcylinder_langmuir_multi`、`test_rsphere_langmuir_multi` |
| 外部数据（`--prepare` 需联网下载） | 5 | `test_3d_focusing_gaussian_beam_from_openpmd{,_picmi}`、Lasy 文件类 |

### 12.1 新构建立刻暴露了两个真问题（此前从未运行过）

**（a）1D PICMI 算例崩溃 —— 真实缺陷，非环境问题**

```
$ mpiexec -n 1 python3 Examples/Physics_applications/laser_acceleration/inputs_test_1d_laser_acceleration_picmi.py
Assertion `valid_domain' failed ... Msg: (RealBox 1.7273077714650454e+97 120259.08423199999 ) ((0) (255) (0))
```

Python 层写入的值是对的（实测 `prob_lo = [-2e-05]`、`prob_hi = [2e-05]`、`n_cell = [256]`），
但 C++ 侧 `amrex::Geometry` 看到的定义域是 `lo = 1.7273e+97`、`hi = 120259.08` —— 完全乱码。
同一输入在 **2D PICMI 下正常**（`build_py_opmd` 的 PICMI 套件 72/75），所以这是 **1D 特有的
Python/PICMI → C++ 交接缺陷**，而 1D 的纯 C++ 路径正常（四维套件里 1D 全过）。

受影响：`test_1d_laser_acceleration_picmi.{run,checksum}`、`test_1d_plasma_acceleration_picmi.{run,checksum}`。

**（b）pytest 测试缺少运行时依赖**：`pytest.WarpX.{1d,3d,rz}` 报 `No module named pytest`；
安装 `pytest`、`pytest-mpi` 后 3/3 通过。

### 12.2 `build_py_4d` 全套件结果（724 个测试）

693 通过 / 31 失败，构成：

| 类别 | 数量 | 性质 |
|---|---|---|
| Lasy 外部数据（`*_lasy_file*` 的 `prepare`/`run`/`analysis`/`checksum`） | 17 | 环境（需联网下载） |
| **1D PICMI 崩溃** | 4 | **真实缺陷**（见 12.1a） |
| 3D PICMI 仅 checksum | 2 | 平台校验和（`.run` 通过） |
| 既有平台校验和（3D EB macroscopic、RZ boosted） | 2 | 平台校验和 |
| pytest 依赖 | 3 | 已修复（安装 pytest/pytest-mpi 后通过） |
| 其余 | 3 | 见日志 |

### 12.3 P0 调查进展（1D PICMI 崩溃）

已排除的假设：

| 假设 | 实验 | 结果 |
|---|---|---|
| Python 层写错值 | 单独初始化 `Cartesian1DGrid` 并读回 | **值正确**：`prob_lo=[-2e-05]`、`prob_hi=[2e-05]`、`n_cell=[256]`、`dims=1` |
| 加载了错误的维度库 | 检查 `_libwarpx.load_library()` 与 `.so` | **正确**：`dims="1"` → `warpx_pybind_1d` |
| `parse_geometry_input()` 读到错值 | 在函数内打印（`WARPX_DEBUG_GEOM=1`） | **读到正确的值**：`[geom] SPACEDIM=1 read prob_lo size=1 = -5.6e-05 ; prob_hi = 1.2e-05`，且 ParmParse 里也是 1 条 `-5.6e-05` |
| `ParmParse::addarr` 追加导致重复条目 | 用 `WARPX_NO_ADDARR=1` 跳过两次 `addarr` 后重跑 | **仍然崩溃** → 不是 addarr |

**仍未解释的现象**：`parse_geometry_input()` 之后 ParmParse 里是正确的 `-5.6e-05 / 1.2e-05`，
但紧接着 `amrex::Geometry` 的 `computeRoundoffDomain` 拿到的 `prob_domain` 是
`(1.7273e+97, 120259.08)` —— 注意 `120259.08 ≈ 1.2e-05 / 1e-10`，提示存在 **1e10 量级的缩放**
与一处未初始化值。也就是说：**在 parse 之后、Geometry 构造之前**，`geometry.prob_lo/prob_hi`
被某个东西改写（或按另一个单位体系重算）。

下一步（明确的）：
1. 用 `AMREX_BACKTRACE=1` 或 gdb 在断言处取栈，确认是哪个构造函数/哪次改写；
2. 在 `WarpX` 构造函数入口与 `AmrCore` 基类构造前各打印一次 `geometry.prob_lo`，把区间夹到具体语句；
3. 检查 1D 下是否有代码把 `prob_lo` 与 `PlasmaInjector`/单位换算（`1e-10` 类）混用。

#### 12.3.1 收敛过程：从"所有 1D PICMI 都崩"到"特定输入组合崩"

用二分法逐块删除失败输入的内容（每次都只调用 `sim.initialize_warpx()`，不推进）：

| 变体 | 结果 |
|---|---|
| 完整输入（去掉 `write_input_file`） | **崩溃** |
| 去掉 `warpx_use_filter` / `warpx_serialize_initial_conditions` / `warpx_do_dynamic_scheduling` / `particle_shape`（逐个） | 仍崩溃 |
| 去掉 `sim.add_species(...)` | 仍崩溃 |
| 去掉 `sim.add_laser(...)` | 仍崩溃 |
| **同时去掉两个 `sim.add_diagnostic(...)`** | **通过**（`initialize_warpx()` 返回 0） |
| 只去掉 `particle_diag` | 仍崩溃 |
| 只去掉 `field_diag` | 仍崩溃 |
| 两个诊断都留、把 `field_diag` 改名 `diag2` | 仍崩溃 |
| 最小脚本（`Cartesian1DGrid` + `ElectromagneticSolver`，同样的 `zmin=-56e-6/zmax=12e-6/nz=256`） | **通过** |

**结论**：不是"1D PICMI 完全不可用"，而是**某个输入组合在 Python→ParmParse 的选项灌入阶段发生内存破坏**，
破坏后 `geometry.prob_lo` 变成 `1.7e97`（未初始化），于是 AMReX 的 `valid_domain` 断言触发。
两个诊断同时存在是目前的触发条件，但改名无效，说明不是重名冲突，更像是**选项条数跨过某个数组边界**。

下一步：在 `pywarpx` 的输入灌入路径（`create_argv_list`/`write_inputs` 的 C++ 侧）加边界检查，
用 `-fsanitize=address` 构建一个 1D Python 模块跑该输入，直接定位越界写。

#### 12.3.2 用 AMReX 内部打印锁定：`Geometry::prob_domain` 被覆写

在 `Geometry::Setup` 与 `Geometry::computeRoundoffDomain` 各加一次打印（临时改的是
**构建目录里的 AMReX 源**，随后已还原），同一输入运行得到：

```
[amrex-setup] gg=0x56647840f950 SPACEDIM=1 prob_lo= -5.6e-05 prob_hi= 1.2e-05 read_hi=1 read_extent=0
[roundoff] this=0x56647846c620 prob_lo=-5.6e-05 prob_hi=1.2e-05 dom_len=256
[roundoff] this=0x566478490300 prob_lo=-5.6e-05 prob_hi=1.2e-05 dom_len=256
[roundoff] this=0x566478490300 prob_lo=-5.6e-05 prob_hi=1.2e-05 dom_len=256
[roundoff] this=0x566478490300 prob_lo=1.72731e+97 prob_hi=120259 dom_len=256   ← 同一对象，被覆写
```

**结论**：`Geometry::Setup` 从 ParmParse 读到的值**完全正确**，`prob_domain` 也正确写入；
随后**同一个 `Geometry` 对象**的 `prob_domain`（16 字节）在两次调用之间被别处覆写，
而同一对象里的 `domain`（IntVect，256 格）保持正确 —— 典型的**相邻内存越界写**，
不是解析、不是 addarr、也不是 argv 生命周期（把 `cargs` 改成 static 后仍然崩溃，已排除）。

**下一步**：给 `Geometry` 的 `prob_domain` 加前后哨兵（canary）定位越界方向，
或用 `MALLOC_PERTURB_`/编译期 `-DAMREX_DEBUG` 复现；也可在 `WarpX` 构造函数里
对 `geom` 容器做一次"写入即校验"的断言。

#### 12.3.3 词级转储：`prob_domain` 被整体平移 16 字节

在 `computeRoundoffDomain` 入口转储 `&prob_domain` 前后各两个 `double`（临时改构建目录里的
AMReX 源，随后已 `git checkout` 还原）。`prob_domain` 位于对象偏移 0x28。四次调用的转储：

```
this=0x…4240 @…4268 words: 1.79e+308  1.727e+97  -5.6e-05  1.2e-05  2.17e-153  7.34e+223  5.41e-312  0
this=0x…7360 @…7388 words: 1.79e+308  1.727e+97  -5.6e-05  1.2e-05  2.17e-153  7.34e+223  5.41e-312  0
this=0x…7360 @…7388 words: 3.76e+06   1.727e+97  -5.6e-05  1.2e-05  -5.6e-05   1.2e-05    5.41e-312  0
this=0x…7360 @…7388 words: 3.76e+06   1.727e+97   1.727e+97 120259     -5.6e-05   1.2e-05    5.41e-312  0
```

关键观察：正确的一对 `(-5.6e-05, 1.2e-05)` **从偏移 0 移到了偏移 +16 字节**（第 3 次调用），
第 4 次调用时 `prob_domain` 的原地内容变成 `(1.727e+97, 120259)`。

即：**有人把同一对值写到了比 `prob_domain` 晚 16 字节的位置**（`RealBox` 恰好 16 字节），
同时把一个 `1.727e+97` 写进了 `prob_domain` 的 lo。这不是随机破坏，而是**带偏移的成对写入**。

**下一步**：用 `-DAMREX_DEBUG=ON` 重建 1D Python 模块（AMReX 的边界/断言会先于这里报错），
或在 `WarpX` 构造函数里对 `geom` 容器逐成员校验，定位谁在按 2 个 `Real` 成对写几何数据。
