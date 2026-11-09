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
| `Source/FieldSolver/MagnetostaticSolver/MagnetostaticSolver.cpp` | 491 | `Source/Plugins/PicEm/Modules/FieldSolver/WarpX_Magnetostatic.cpp`：`WarpX::computeVectorPotential`、`WarpX::setVectorPotentialBC`、`MagnetostaticSolver::VectorPoissonBoundaryHandler::defineVectorPotentialBCs`、`MagnetostaticSolver::EBCalcBfromVectorPotentialPerLevel::doInterp` |
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
| 5 | `test_3d_linear_compton_bunch_laser.analysis` | 1 | 环境/库层挂起 | `.run` 通过、输出完整；分析脚本在 `OpenPMDTimeSeries` 读 73 MB 3D 数据集时停在 futex（CPU 0%），单独跑 25 min 不返回；在 `build_multi_hdf5`（无 FFT）的同一输出上同样挂起 → 与本地改动无关 |
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
    -I/home/douliangduo/WarpX_plugin/Source/Plugins/PicEm/Modules/FieldSolver/PSATD \
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
  -DFETCHCONTENT_SOURCE_DIR_FETCHEDAMREX=/home/douliangduo/WarpX_plugin/build_2d_full/_deps/fetchedamrex-src
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
