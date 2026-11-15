# WarpX 官方 26.10 对照审计报告（2026-11）

> 审查对象：`$HOME/WarpX_plugin`（fork，HEAD `59be502` + 本报告第 7 节的修复）
> 参照物：官方 WarpX `26.10`（GitHub `BLAST-WarpX/warpX`，tag `26.10` = `ec1660d`）
> 本目录：`Docs/audit_2026_11/`（报告、脚本、逐用例记录）

---

## 0. 2026-11-15 基线更正（先读这一节）

本轮查清了一件影响**全部归因**的事：**本 fork 的上游基线不是 26.10**，而是 26.10 之后约三周的
`development` 快照（≈2026-10-30，`dependencies.json` 与上游 `ed30ada99` 一致），并且少了
4 个上游提交。证据、方法与逐文件结论见 `UPSTREAM_BASE.md`。

由此，本文下面所有"fork 代码差异"的说法都要按新口径读：

| 差异来源 | 是否 fork 缺陷 | 怎么判定 |
|---|---|---|
| (a) 上游演进（26.10 → 基线） | **否** | fork 与官方 26.11 一致、与 26.10 不一致 |
| (b) fork 重构（`Source/Core` + `Source/Plugins`） | 是 | 与 26.11 **也**不一致 |
| (c) fork 新功能（star/fluid 插件、plugin runtime） | 是 | 官方没有对应代码 |

本轮已按这个口径重做了三组对照（fork vs 26.10、fork vs 26.11、26.10 vs 26.11），见
`ATTRIBUTION.md` 的"三向归因"表；并据此**撤回两条此前的错误结论**：

1. **RZ `rho` 的两个残余缺陷（D-A/D-B）不是 fork 缺陷**，是上游 `ec99a479a`(#7341) 与
   `a180bc361`(#7331) 的改动被 fork 正确继承；其中后者是上游对"方位模在 r=0 处宇称"的
   物理修正。详见 `RZ_RHO_ROOT_CAUSE.md`。此前的"修复 #9"因此被撤销（见 `FIXES.md`）。
2. **"6 处测试容差被放宽"不成立**：`projection_div_cleaner` 等处的 `1e-12 → 2e-12/3e-12`
   是**上游 26.11 自己改的**（`Examples/Tests/projection_div_cleaner/inputs_test_3d_*_picmi.py`
   在 26.10 里是 `1e-12`，26.11 里是 `2e-12`），fork 与 26.11 逐字节一致。

## 1. 结论摘要

1. **官方 26.10 已就位**：`_upstream/warpX-26.10/`（shallow clone，tag `26.10`）。
   需要指出：本 fork 的实际基线**比 26.10 发布版新**（`dependencies.json` pin 的 AMReX 是
   `008b6737`，官方 26.10 pin 的是 AMReX tag `26.10`；fork 的 Examples 里还有 26.10 之后才
   进入上游的用例），因此本报告同时构建了两套官方参照，用来把“fork 改动”与“上游/AMReX 演进”
   分开（见 §2）。

2. **全量算例对比已完成**：官方 26.10 注册 **398** 个 ctest 用例，本 fork 注册 **411** 个；
   两者共享的 **398** 个用例全部逐一运行、逐一对比（fork 独有的 13 个用例单列）。
   在 398 个共享用例中（**全部 398 个两侧都能运行且通过**）：
   - **13 个逐字节完全相同**（连 `warpx_job_info` 都相同）；
   - **233 个数值逐位相同**，只有构建元数据（`warpx_job_info` 里的编译时间/路径/flags）
     或 `warpx_used_inputs` 的默认参数差异（AMReX 版本默认值 `amrex.handle_crt_reports`）；
   - **132 个存在可定位的数值差异**（plotfile/openPMD 数据集层面；另有 6 个仅二进制差异），
     另有 **20 个**只有日志文本差异。

3. **差异归因**（fork vs 官方 26.10，以及 fork vs “官方源码 + fork 同款 AMReX” 的隔离对照）
   见 §4。凡能归因到 AMReX 版本演进的，都不是 fork 的 bug；隔离对照后仍存在的差异才是
   fork 代码行为差异，逐条对应到 §6 的代码审查结论。

4. **代码审查**：`Source/` 842 个 `.cpp/.H/.py` 全部通读（其中 364 个与上游同路径不同内容
   做逐 hunk diff、238 个新文件逐行读），另有 `Python/`、`Examples/`、`Tools/`、`tests/`、
   `benchmarks/` 的分区审查。共记录 **189 条**问题：**12 critical / 62 major / 113 minor**
   （逐文件报告在 `reviews/`，汇总见 §6）。

5. **已修复并验证的缺陷**（§7）：1D/2D/RZ 维度编译中断（2 处 IntVect）、插件 `.so` 链接失败、
   外部 AMReX 构建路径失效、pywarpx 公共 API 断链（`WarpXParIter`）。
   每条都给出官方源码/语言规则依据与构建/运行证据。

6. **未修复的确认问题**（§8）：多数是自研插件（star/fluid、plugin runtime）的设计与物理缺陷，
   修复面大且会影响正在进行的架构迁移，本报告给出精确位置、证据与建议修法，交由你决策。

---

## 2. 环境与方法（可复现）

### 2.1 三套构建（参数完全一致）

| 构建 | 源码 | AMReX | pyAMReX | 用途 |
|---|---|---|---|---|
| `audit_build/fork` | 本 fork HEAD | `008b6737`（fork pin） | `72fe55f` | 被测对象 F |
| `audit_build/official` | 官方 tag `26.10` | tag `26.10`（官方 pin） | tag `26.10` | 官方参照 A |
| `audit_build/official_amrex` | 官方 tag `26.10` | `008b6737`（fork pin） | `72fe55f` | 隔离参照 B（同 AMReX） |

公共 CMake 选项（三套一致）：

```
-DCMAKE_BUILD_TYPE=Release
-DWarpX_DIMS="1;2;3;RZ" -DWarpX_COMPUTE=OMP -DWarpX_MPI=ON
-DWarpX_OPENPMD=ON -DWarpX_FFT=ON -DWarpX_QED=ON -DWarpX_EB=ON -DWarpX_PRECISION=DOUBLE
-DWarpX_PYTHON=ON -DWarpX_PYTHON_IPO=OFF -DpyAMReX_IPO=OFF
-DCMAKE_C_COMPILER=/usr/bin/gcc -DCMAKE_CXX_COMPILER=/usr/bin/g++
-DMPI_CXX_COMPILER=/usr/bin/mpicxx -DMPIEXEC_EXECUTABLE=/usr/bin/mpiexec
-DPython_EXECUTABLE=$HOME/miniconda3/bin/python3
```

- 编译器 GCC 13.3.0，MPI OpenMPI 4.1.6，FFTW 3.3.10，HDF5 1.14.6（并行）。
- 依赖源码冻结在 `audit_build/deps/`（AMReX 两版、openPMD 0.17.1、pyAMReX 两版、
  pybind11 v3.1.0、PICSAR 26.10），三套构建用**同一份**依赖源码，避免下载漂移。
- **必须清理 conda 环境污染**：会话 shell 里 conda base 的 `CFLAGS/LDFLAGS/CMAKE_PREFIX_PATH`
  会让 CMake 找到 conda 的 MPICH 与 GCC 15 头文件，导致 MPI 链接失败；
  `audit_build/cleanenv.sh` 负责清掉这些变量并固定 `PATH`。
- 逐字节对比要求同一 launcher：**必须用 `/usr/bin/mpiexec`（OpenMPI）**；
  miniconda 自带的是 MPICH/Hydra 的 `mpiexec`，与链接 OpenMPI 的 `warpx` 不兼容
  （表现为每个进程都以为自己是单进程、并发写同一文件，产生“随机”崩溃/损坏输出——不是 fork 的 bug）。
- PICMI 用例：fork 用 `picmistandard 0.35.0`（其 pin），官方 26.10 用 `picmistandard 0.34.0`
  （0.35 删掉了官方 26.10 `pywarpx/picmi.py` 依赖的 `_ClassWithInit`）。两套各按自身 pin 运行，
  差异在报告中显式标注。

### 2.2 用例集合与运行

- 用例来自两套构建的 `ctest --show-only=json-v1`；**共享 398 个**用例逐一对比。
- 运行命令与官方 CI 完全一致（ctest 注册的 command、`WORKING_DIRECTORY`、
  `AMREX_INPUTS_FILE_PREFIX`、`OMP_NUM_THREADS=1`、`amrex.abort_on_unused_inputs=1` 等运行时参数）；
  2-rank 用例按注册的 `-np 2` 运行。
- 运行日志与 JUnit：`audit_runs/ctest_{fork,official}.log`、`audit_runs/*_junit.xml`。
- 外部数据：`warpx-data/`、`openPMD-example-datasets/` 是指向本机数据副本的符号链接
  （WarpX 官方 CI 同样要求它们在仓库同级），用于 DSMC/电离截面与 `load_external_field*` 用例；
  没有它们时这 27 个用例会在两侧同样失败。

复现：

```bash
cd $HOME/WarpX_plugin
# 1) 构建（示例：fork）
DEPS=$PWD/audit_build/deps source audit_build/configure_common.sh
./audit_build/cleanenv.sh cmake -S . -B audit_build/fork "${COMMON[@]}" \
  -DWarpX_amrex_src=$DEPS/amrex-008b6737 -DWarpX_pyamrex_src=$DEPS/pyamrex-72fe55f \
  -Dblaspp_DIR=$HOME/third_party/fft-deps/lib/cmake/blaspp \
  -Dlapackpp_DIR=$HOME/third_party/fft-deps/lib/cmake/lapackpp
./audit_build/cleanenv.sh cmake --build audit_build/fork -j 16
# 2) 跑全套
Docs/audit_2026_11/scripts/run_suite.sh audit_build/fork audit_runs 8
# 3) 对比
Docs/audit_2026_11/scripts/compare_all.py --a audit_build/fork --b audit_build/official \
  --out audit_runs/compare_fork_vs_official --label-a fork --label-b official
Docs/audit_2026_11/scripts/analyze_results.py --dir audit_runs/compare_fork_vs_official \
  --out Docs/audit_2026_11/NUMERIC_DIFFS_fork_vs_official.md
```

### 2.3 对比判据

1. **第一层：逐字节**。对每个用例的工作目录递归比较全部文件（跳过 AMReX 的 `.old.<pid>`
   备份目录与 `Backtrace.*`）。
2. **第二层：分类**。
   - `warpx_job_info` → 构建元数据（编译时间、构建目录、C++ flags、git describe），预期不同，
     不计入数值差异；
   - `warpx_used_inputs` → 输入合成结果；差异说明默认参数不同（本审计中唯一系统性差异是
     AMReX 版本默认值 `amrex.handle_crt_reports`，属 AMReX 演进）；
   - 文本文件按去掉耗时/内存/时间戳后的行比较；
   - HDF5（openPMD）逐 dataset 比较；AMReX plotfile 用 yt 逐变量比较（粒子按 `particle_id`
     排序后比较），报告 `max_abs_diff` / `max_rel_diff` / 差异元素数。
3. **第三层：隔离归因**。对数值不同的用例，在 `official_amrex`（官方源码 + fork 同款 AMReX）
   上重跑同一用例，再与 fork 对比：
   - 与 A 不同、与 B 相同 → **AMReX/上游演进**；
   - 与 A、B 都不同 → **fork 代码行为差异**。

---

## 3. 结果总览（fork vs 官方 26.10）

共享用例 398 个：

| 类别 | 数量 | 含义 |
|---|---|---|
| 逐字节完全相同 | 13 | 连 `warpx_job_info` 都一致 |
| 仅元数据/默认输入差异 | 233 | 数值输出逐位相同；差异只在 `warpx_job_info` 与/或 `warpx_used_inputs`（`amrex.handle_crt_reports`） |
| 仅日志文本差异 | 20 | 数值输出相同，stdout/日志行不同（多为警告/打印差异，见 `NUMERIC_DIFFS_fork_vs_official.md`） |
| **非元数据差异** | **138** | 其中 **132** 个能定位到具体 plotfile/openPMD 变量（下表）；另 6 个只表现为二进制文件不同、未能定位到变量 |

两侧失败情况：

| 类别 | 数量 | 说明 |
|---|---|---|
| 两侧都失败 | **0** | 见 §3.1：补齐外部数据后，原先 27 个失败用例在两侧**全部通过** |
| 仅官方失败 | **0** | 官方 PICMI 用例在 `picmistandard 0.34` 下全部通过 |
| 仅 fork 失败 | **0** | **fork 没有出现“官方能跑、fork 崩”的用例** |

→ **398 个共享用例在两侧都能运行、都能通过**；对比覆盖 398/398。

### 3.1 曾因外部数据缺失而失败的 27 个用例（已补齐数据，全部通过）

这 27 个用例的输入引用 `../../../../warpx-data/...`（DSMC/电离截面表）与
`../../../../openPMD-example-datasets/...`（`load_external_field*` 的 openPMD 文件）——
WarpX 官方 CI 会把这两个独立仓库检出在 WarpX 同级目录。本审计在仓库根创建了两个
**符号链接**（`warpx-data`、`openPMD-example-datasets`，已 gitignore）指向本机已有的
数据副本，重跑后 27/27 在 fork 与官方两侧**全部通过**，并已纳入对比。
（此前把它们记为“环境失败”是当时数据未就位；现在没有任何用例因环境而不参与对比。）

原失败清单（供追溯）：

`test_1d_background_mcc_picmi`、`test_1d_charge_exchange_dsmc`、`test_1d_dsmc_picmi`、
`test_1d_ionization_neutral_dsmc`、`test_1d_photoneutralization_dsmc`、
`test_1d_two_product_reaction_dsmc`、`test_2d_background_mcc`、`test_2d_background_mcc_picmi`、
`test_2d_charge_exchange_dsmc`、`test_2d_embedded_circle`、
`test_3d_deuterium_deuterium_fusion_anisotropic_beam_target_uz_10pct`、
`test_3d_deuterium_tritium_fusion_anisotropic_beam_target_uz_10pct`、
`test_3d_ionization_electron_dsmc`、`test_3d_ionization_ion_dsmc`、
`test_3d_load_external_field_grid_picmi`、`test_3d_load_external_field_particle_multi_time`、
`test_3d_load_external_field_particle_multi_time_picmi`、`test_3d_load_external_field_particle_picmi`、
`test_3d_load_external_field_particle_time`、`test_3d_load_external_field_particle_time_picmi`、
`test_3d_load_external_field_particle_time_restart`、`test_3d_projection_div_cleaner_picmi`、
`test_rz_load_external_field_grid`、`test_rz_load_external_field_grid_restart`、
`test_rz_load_external_field_particles`、`test_rz_load_external_field_particles_restart`、
`test_rz_projection_div_cleaner`。

原因：① `Failed to open cross-section data file`——碰撞/电离用例的截面表在 `warpx-data`；
② `load_external_field*` 需要 `openPMD-example-datasets`。补齐后两侧均通过；
其数值对比结果也已并入 §3/§4 的统计。

### 3.2 fork 独有的 13 个用例（官方 26.10 未注册）

`test_1d_theta_implicit_planar_pinch_substeps`、`test_2d_dynamic_regrid[_remake|_remove|_uniform_b]`、
`test_2d_ohm_solver_{adaptive_checkpoint[_restart]|fixed_substeps_restart|external_field_picmi|external_field_segmented_picmi}`、
`test_2d_semi_implicit_pc_jacobi`、`test_2d_theta_implicit_planar_pinch_algmg`、
`test_3d_embedded_boundary_cube_mr`——**全部通过**（它们来自 26.10 之后的上游开发分支）。

逐用例明细：`Docs/audit_2026_11/CASE_RESULTS.md`、`NUMERIC_DIFFS_fork_vs_official.md`、
原始 JSON `audit_runs/compare_fork_vs_official/cases/`。

---

## 4. 差异归因

对 132 个数值差异用例，在 `official_amrex`（官方源码 + fork 同款 AMReX `008b6737`）
上重跑同一用例并再次对比（176 个用例已跑；剩余 3 个 >10 分钟的重算例未跑）：

| 归因 | 数量 | 含义 |
|---|---|---|
| **AMReX / 上游演进** | **50** | 换成同款 AMReX 后与 fork **数值一致**；差异来自 AMReX `008b6737` vs tag `26.10` 以及 fork 基线包含的 26.10 之后上游提交 |
| **fork 代码行为差异** | **79** | 同款 AMReX 下仍不同；需要看 fork 的改动 |
| 未归因 | 3 | `official_amrex` 未运行（3 个极重算例 + 4 个因环境失败） |

79 个 fork 代码差异按“与 official_amrex 的最差相对差”分布：

| 相对差 | 数量 | 解读 |
|---|---|---|
| < 1e-12 | 19 | 运算次序/求和顺序不同导致的舍入级差异，物理等价 |
| 1e-12 – 1e-8 | 13 | 极小；长期混沌放大（PIC 的位相/散粒噪声）可解释 |
| 1e-8 – 1e-4 | 8 | 小但非舍入级，需要逐案看 |
| 1e-4 – 1e-1 | 9 | 明显偏差 |
| ≥ 1e-1 | 30 | 显著差异（其中一部分是参考值≈0 导致相对差失真，应以绝对差/场量级判断） |

逐案表：`Docs/audit_2026_11/ATTRIBUTION.md`。

---

## 5. 重点差异案例

按“与官方 26.10 的最大绝对差”排序的前几类（完整表见 `ATTRIBUTION.md`）：

| 用例 | 最差字段 | 最大绝对差 | 归因 | 备注/候选根因 |
|---|---|---|---|---|
| `test_2d_energy_conserving_thermal_plasma` | jx/jy/jz（step 0） | 2.0e17 → **3.6e5**（rel 3.5e-9） | fork 代码（已修） | **根因已定位并修复**：诊断 J 走的 `depositCurrent` 服务额外做了 `WarpXSumGuardCells` + `FinalizeJOnLevel`，上游只做 `FillBoundary`。修复后 1D 用例逐位一致、2D 从 25% 降到 3.5e-9。见 §7.6 |
| `test_1d/2d_collision_xz(_global_debye)`、`test_3d_collision_xyz` | jx/jz | 1e7–1e8 → **0** | fork 代码（已修） | 原以为与碰撞 RNG 有关；实际同样是诊断 J 的沉积路径（§7.6），修复后**逐位一致** |
| `test_3d_acceleration_psatd(_time_avg)(_restart)` | jz | 2.4e5（rel 9e-5） | fork 代码 | PSATD 谱路径；与 `SpectralSolverRegistration`/PML 的 `lev`、`n_rz_azimuthal_modes` 参数缺失（FieldSolver_B F1）方向一致，尚需定点验证 |
| `test_2d_theta_implicit_jfnk_vandb(_picmi/_filtered)` | divE | 3.8e7（rel ~1e-10） | fork 代码 | 隐式求解器迭代/预条件差异；`WeightedJacobi` omega 每步被 Gershgorin 覆盖（FieldSolver_B F3） |
| `test_2d_laser_ion_acc(_picmi)` | E/j/rho | 1e14（rel 0.07–0.97） | fork 代码 | 电离 + 外场路径；与 `GetExternalEBField` 不传 context 的空指针/静默零场（ParticleCore_A F1）方向一致 |
| `test_rz_langmuir_multi_psatd_current_correction` | divE | 3.7e2 | fork 代码 | RZ + PSATD 电流修正 |
| `test_rz_ohm_solver_resistive_drag_species_fields_picmi` | `current_fp_ions[dir=z]`、`rho_ions` | 7.8e5 / 7.8（总量场一致） | fork 代码 | **逐格定位**：只在 `z` 上边界一排（第 15 列）以及个别角点不同，官方 `rho_ions=23.84`、fork `31.67`；总场 `rho`/`E` 一致 → 问题在**逐物种诊断量的边界处理**（`FinalizeRho`/per-species 场未做与总场一致的边界处理），不是求解器 |
| `test_2d_darwin_solver_em_modes_es_picmi` | Ez | 2.9e4 | fork 代码 | Darwin 求解器（预条件系数改动，FieldSolver_B F2） |

### 5.0 一个度量陷阱（已核实，避免误判）

`first_step.py`（只看最早 diag）与 `compare_case.py`（看所有 diag 取最大值）给出的数不是同一个量。
例如 `test_rz_magnetostatic_eb_picmi` 的 `Er`：首个 diag 上 max abs = 2.54e4，
全部 diag 上 max abs = 2.73e4。2026-11-15 曾把后者误当作“修复 #5 引起的回退”，复核后确认
首个 diag 的数值在修复前后**完全一致**（2.539e4），所以不是回退，只是度量不同。
后续比较必须同口径（`first_step.csv` 对 `first_step.csv`，或全量对全量）。

### 5.1 首个输出是否已不同（决定“根因在哪一层”）

对 79 个 fork 代码差异用例比较**最早 diag**（`scripts/first_step.py`，结果 `audit_runs/first_step.csv`）：

- **47 例首个输出完全相同** → 差异只来自后续演化（浮点求和顺序/迭代路径，在混沌的 PIC 里被放大），
  与初始化、诊断无关；
- 11 例首个输出只有 ≤1e-9 的差 → 舍入级；
- 21 例首个输出就有可测差异，其中**真正结构性**的集中在：
  RZ 的 `rho` 诊断（`test_rz_laser_acceleration*`：t=0 时粒子逐个属性完全相同、所有场为 0，
  只有 `rho` 的 343 个格点不同，最大 2.5%；差异集中在径向第 24 行与外侧 56–61 行）、
  `test_rz_ohm_solver_resistive_drag_species_fields_picmi` 的逐物种 `rho_ions`（z 上边界一排）、
  `test_2d_collisions_split_momentum_push_electrostatic` 的 ES 诊断 j（需用当前二进制重跑后再判）。
  其余（如 magnetostatic 的 `Er`、beam_beam 的 `E`）在**场量级**上只有 1e-8–1e-5，属求解器迭代差异。

**因此“79 个差异”里真正属于“fork 做错了物理/参数”的是少数**；多数是重构导致的运算次序差异。
要把后者也做到逐位一致，只能逐路径恢复与上游完全相同的运算次序（工作量大、且部分路径无法同时满足
新架构），这需要你确认是否值得。

重要结论：**没有一个是“fork 崩溃/官方正常”**；也没有发现“某类算例整体无法运行”。差异集中在
自研/重构触及的路径（碰撞 RNG、PSATD 谱参数、隐式迭代、电离+外场、J 诊断沉积）。

补充：`test_2d_dirichlet_bc(_picmi)` 的 phi 差 ~9.5e-7（rel 1e-9）被归因到 AMReX 演进的 MLMG
迭代路径，同款 AMReX 下消失。

---

## 6. 代码审查汇总

逐文件报告在 `Docs/audit_2026_11/reviews/`（17 份），协议见 `REVIEW_PROTOCOL.md`。
共 **189 条**：**12 critical / 62 major / 113 minor**（另有 2 条未分级）。

| 分区 | 覆盖文件 | C | M | m |
|---|---|---|---|---|
| `Source/Core/Runtime`（A+B） | 93 | 1 | 8 | 19 |
| `Source/Core/Misc`（Modules/Fields/IO/…） | 35 | 0 | 2 | 5 |
| `Source/Plugins/PicEm`（A+B） | 105 | 0 | 7 | 15 |
| `Source/Plugins/ParticleCore`（A+B） | 86 | 3 | 2 | 10 |
| `Source/Plugins/FieldSolver`（A+B） | 100 | 0 | 6 | 11 |
| `Source/Plugins/Diagnostics`（A+B） | 79 | 2 | 2 | 10 |
| `Source/Plugins/Collision`（A+B） | 48 | 0 | 2 | 10 |
| `Source/Plugins/{FluidMoments,NewtonGravity}` | 46 | 4 | 9 | 0 |
| `Python/` + PICMI 脚本 | 128 | 1 | 2 | 6 |
| `Tools/` + `tests/` | 103 | 1 | 15 | 15 |
| `benchmarks/` | 118 | 0 | 7 | 12 |

### 6.1 最严重的确认项（摘录）

- **C1 `Source/Plugins/ParticleCore/Particles/Gather/GetExternalFields.cpp`**：
  新的 `GetExternalEBField` 只从 context 取粒子容器，而 QED 成对产生 / 光子发射 / 场电离
  三条构造路径不传 context（`nullptr`）→ 同时开 `E_ext_particle_*` 时**空指针解引用**，
  或加速器晶格场被静默丢弃。
- **C2 `Source/Plugins/ParticleCore`（RZ hybrid 电荷密度）**：`DepositCharge` 去掉
  RZ/RCYLINDER/RSPHERE 的 `1/(2πr)` 反体积缩放后，`FinalizeRho()` 只处理 `rho_fp/rho_cp`，
  不覆盖 `rho_fp_<species>` → RZ + hybrid + per-species 场时电荷密度静默错标度。
- **C3 `Source/Plugins/Diagnostics/ComputeDiagFunctors/DivEFunctor.cpp`**：
  `divE/divB` 用 `config_snapshot().ncomps` 分配；legacy/PICMI 路径下快照的 `ncomps`
  在 `AllocLevelMFs` 前恒为 1 → RZ 多模时按 `2m-1` 写入 1 分量 MultiFab＝越界写。
- **C4 `Source/Plugins/{FluidMoments,NewtonGravity}`**（自研 star 插件，上游无对应）：
  ① 自引力势 φ 算出后**从未施加**（无 `computeAcceleration`，流体侧把 `g` 读入后丢弃），
  日志却打印 "g: yes"；② 1D/2D 下 47 处 `bx.smallEnd(1)/(2)` 越界取索引（release 下读到
  垃圾循环上界，`Array4` 写穿 FAB）；③ 单位混用（SSM 表 g/cm³ 灌进 SI 泊松、流体 cgs
  状态配 SI 的 `dx`）。这三条都属“功能未完成/未接线”，不是回归，但会让使用者误以为自引力生效。
- **C5 `Source/Core/Runtime/PluginCAdapter.cpp`**：`warpx_stage_field()` 是唯一没有 `try/catch`
  的 C ABI 入口，`DataBus` 的访问检查异常会越过 C 边界；且 `warpx_stage_geometry()` 只判
  `lev<0`，对不存在的层越界读 `Geom(lev)`。
- **C6 `Source/Core/Runtime/HandlerSpec.H`**：`on_level_created`/`on_checkpoint`/`on_restart`
  三个生命周期钩子有声明、有转发、有设计文档，但全树**零调用点**；checkpoint/restart 与
  动态加密时插件状态被静默丢弃。
- **C7 `Tools/verify/check_plugin_boundaries.py`**：前两条检查的结果被后一条整体覆盖，
  “重复头文件/依赖环”永远报 OK；实测合成反例 exit 0。类似的还有 `Tools/verify/parse_tests.py`
  把 `_upstream/` 当本地树（上游一致性门恒过）、`check_kernel_coupling.py` 监控目录不存在时
  静默跳过。**这四个“不变量检查器”目前不能证明它们声称的约束**，而仓库文档以它们为“0 已知/0 新增”的依据。
- **M 级**（62 条）集中在：诊断在 regrid 后清零 time-averaged 累加和、load-balance 每次重建 PML、
  `macroscopic_solver = laxpw` 被静默降级为一阶后向 Euler、ECT 共形路径丢掉散度清理项、
  Bremsstrahlung 饱和分支两条语句顺序与上游相反、`pulsed_decay` 删掉上游必填参数且无兼容保护、
  pywarpx `WarpXParIter` 断链、`ParticleTileIter` 改名未同步 Python、测试容差被悄悄放宽
  （`projection_div_cleaner` 1e-12→2e-12/3e-12 等 6 处）、benchmarks 的 Mie/指数电导解析参考公式
  与代码不一致（`mie_cylinder.rcs()` 多乘 `j^n`、`sigma_exponential_test.py` 用 Crank–Nicolson
  冒充 backward Euler）。

> 说明：`reviews/` 中每条都带 `文件:行`、严重度、置信度、依据（上游对照/公式/反例）与建议修法；
> 其中标注 `needs-verification` 的条目**未**作为结论使用。

---

## 7. 已修复的缺陷（含依据与验证）

### 7.1 1D/2D/RZ 维度编译中断（2 个文件）

- 位置：`Source/Plugins/NewtonGravity/StarGravityPlugin.cpp:52-53`、
  `Source/Plugins/FluidMoments/StarFluidPlugin.cpp:98-99,285`
- 现象：`amrex::IntVect{lo[0], lo[1], lo[2]}` 在 `AMREX_SPACEDIM=1/2` 下无匹配构造函数，
  **HEAD 只有 3D 能编译**（1D/2D/RZ 编译中断）。
- 依据：AMReX `IntVectND<dim>` 的变参构造要求参数个数等于 `dim`（`AMReX_IntVect.H:201-203`）；
  仓库自身的正确写法是 `AMREX_D_DECL`（如 `Source/Core/Numerics/*YeeAlgorithm.H:63`）。
- 修法：改为 `amrex::IntVect{AMREX_D_DECL(lo[0], lo[1], lo[2])}`（`bx.contains` 同理）。
- 验证：修复后 `cmake --build audit_build/fork` 在 `1;2;3;RZ` 四维下编译通过。

### 7.2 插件 `.so` 链接失败（2 个 CMakeLists）

- 位置：`Source/Plugins/NewtonGravity/CMakeLists.txt`、`Source/Plugins/FluidMoments/CMakeLists.txt`
- 现象：`OBJECT` 库 `star_*_objs_<SD>` 未开 PIC，却被包进 `MODULE`（`.so`）→
  `relocation R_X86_64_PC32 ... can not be used when making a shared object`。
- 依据：CMake 语义——目标文件进入共享库必须 `-fPIC`；仓库其他地方（`ablastr_${SD}`、
  `warpx_fluidmoments_static_${SD}`）都显式开 PIC。
- 修法：给两个 OBJECT 库加 `POSITION_INDEPENDENT_CODE ON`（MODULE 早已有）。
- 验证：链接通过，`lib/star_gravity_plugin.so`、`lib/star_fluid_plugin.so` 生成。

### 7.3 外部 AMReX 构建路径失效（2 个 CMakeLists）

- 位置：`CMakeLists.txt:533`、`Source/Plugins/Diagnostics/CMakeLists.txt:30`
- 现象：硬编码 `_deps/fetchedamrex-build/share/amrex/C_scripts`；使用受支持的
  `-DWarpX_amrex_src=...`（AMReX 外部源码）时该目录不存在 →
  `fatal error: AMReX_buildInfo.H: No such file or directory`。
- 依据：AMReX 的 CMake 已导出 `AMREX_C_SCRIPTS_DIR`（本构建缓存中为
  `.../amrex-008b6737/Tools/C_scripts`，含 `AMReX_buildInfo.H`）；上游 26.10 没有这条硬编码路径。
- 修法：改用 `${AMREX_C_SCRIPTS_DIR}`。
- 验证：fork 在 `-DWarpX_amrex_src` 下编译通过（本报告全部构建均用该路径）。

### 7.4 pywarpx 公共 API 断链（1 个 .cpp）

- 位置：`Source/Plugins/PicEm/Modules/Python/Particles/WarpXParticleContainer.cpp`
- 现象：C++ 侧只注册了 `ParticleTileIter`，而 `Python/pywarpx/particle_containers.py:166/211/250`
  仍用 `libwarpx_so.WarpXParIter` → `get_particle_real_arrays` 等 3 个公开 API 必抛
  `AttributeError`（子代理已实测 `hasattr(libwarpx_so,"WarpXParIter") == False`）。
- 依据：重命名属于 fork 内部重构，但 `WarpXParIter` 是 2021 年以来 pywarpx 的公开名字，
  不应在无 deprecation 的情况下消失。
- 修法：注册后加一行别名 `m.attr("WarpXParIter") = m.attr("ParticleTileIter");`（不改变新名字）。
- 验证：重编译后 `hasattr` 为 True，Python API 可用。

### 7.5 J 诊断的 guard-cell 填充（parity 修复，无数值影响）

- 位置：`Source/Plugins/Diagnostics/ComputeDiagFunctors/JFunctor.cpp`
- 现象：ES/无场求解器时诊断自行沉积电流；上游在沉积后对三个分量做
  `FillBoundary(Geom(lev).periodicity())`，fork 的重构把这步丢了。
- 依据：上游 26.10 `Source/Diagnostics/ComputeDiagFunctors/JFunctor.cpp:46-55`。
- 修法：补回三个分量的 `FillBoundary`。
- **诚实说明**：在 `test_1d/2d_energy_conserving_thermal_plasma` 上重跑后数值**没有变化**
  （该 `current_fp` 的 ngrow 为 0，FillBoundary 是空操作），所以它是行为对齐而不是已证实的修复；
  那些用例的根因仍未定（见 §5、§8）。

### 7.6 诊断 J 的电流沉积路径与上游不一致（1 个服务实现）

- 位置：`Source/Plugins/PicEm/RuntimeBridge/Runtime/WarpXHostServices.cpp::depositCurrent`
- 现象：该服务只被诊断 `JFunctor` 使用。它先 `mypc.DepositCurrent`，再做
  `WarpXSumGuardCells`，最后 `FinalizeJOnLevel`；而上游 26.10 的 `JFunctor` 只做
  `DepositCurrent` + `FillBoundary`（`_upstream/.../JFunctor.cpp:46-55`）。
  多出来的两步会改变诊断值，并且**修改了后续求解器要用的 live 电流**。
- 依据：上游对应实现逐行对照（上游没有 SumGuardCells/FinalizeJ）。
- 修法：服务里只保留 `FillBoundary`（三个分量、按 `Geom(lev).periodicity()`）。
- 回归证据（修复前 → 修复后，vs 官方 26.10 的最大绝对差）：

| 用例 | 修复前 | 修复后 |
|---|---|---|
| `test_1d_energy_conserving_thermal_plasma` | 1.75e17 | **0（逐位一致）** |
| `test_2d_energy_conserving_thermal_plasma` | 1.99e17 | 3.6e5（rel 3.5e-9） |
| `test_2d_collision_xz` | 1.15e8 | **0** |
| `test_2d_collision_xz_global_debye` | 8.64e7 | **0** |
| `test_3d_collision_xyz` | 8.88e7 | **0** |
| `test_1d_resample_velocity_coincidence_thinning(_cartesian)` | 5.3e306 / 7.7e282 | **0 / 0** |

  35 个带 j 诊断的用例全部重跑并通过；9 个的差异显著缩小或归零。

### 7.7 `divE`/`divB` 诊断用快照 `ncomps`（2 个文件）

- 位置：`Source/Plugins/Diagnostics/ComputeDiagFunctors/DivEFunctor.cpp:56`、`DivBFunctor.cpp:37`
- 现象：用 `config_snapshot().ncomps` 分配临时 MultiFab；legacy/PICMI 路径下快照的
  `ncomps` 在 `AllocLevelMFs` 之前恒为默认 1，而 RZ 的 `ncomps` 应为 `2m-1` →
  `ComputeDivECylindrical` 按 `2m-1` 写 1 分量 MultiFab，越界写。
- 依据：`RuntimeConfig::ncomps()` 是活配置（`RuntimeConfig.H:90`），
  `RuntimeConfigSnapshot::ncomps{1}` 是默认值（`RuntimeConfigSnapshot.H:48`）；
  仓库 HANDOFF 也明确记录“InitData 期间不能读 snapshot”。
- 修法：改用 `config().ncomps()`。
- 回归：`test_2d_theta_implicit_jfnk_vandb*`、`test_rz_psatd_JRhom_LL2`、
  `test_3d_divb_cleaning` 全部通过；2D 用例的 divE 残差为 rel~1e-9（舍入级），与修复前一致。

### 7.8 两个“验证工具”恒过（已修复，2 个脚本）

这两处直接推翻了仓库文档里“0 已知/0 新增”的证据链，因此按缺陷处理：

1. `Tools/Modularization/check_plugin_boundaries.py`
   - 现象：结尾 `errors = sorted(v for v in violations if v not in allowed)` **覆盖**了前面
     “重复头文件”和“依赖环”累积到 `errors` 的条目 → 两条检查永远不生效（合成反例实测
     `plugin boundaries OK`、exit 0）。
   - 修法：`errors = <violations> + errors`（保留已有条目）。
   - 验证：仓库上运行仍为 `OK (0 known, 0 new)`；重复头文件/环现在会进入 `errors` 并 exit 1。
2. `Tools/verify/parse_tests.py`
   - 现象：`SKIP_DIRS` 没有排除 `_upstream/`、`audit_build/`、`audit_runs/` 等目录，于是把
     官方参考树也当成“本地树”遍历：本地定义数翻倍（832 vs 408），上游一致性门恒过。
   - 修法：补齐跳过目录，并通用跳过 `build*`。
   - 验证：现在报告 `local 424` vs `reference 408`，与 §3.2 的 13 个 fork 独有定义一致。

---

## 8. 未修复 / 需要你决策的确认问题

以下均已确认但需要较大改动或涉及设计取舍，本报告只给出证据与建议（详见 `reviews/`）：

1. **star/fluid 插件整体不可用**（C4）：自引力未接线、1D/2D 越界、cgs/SI 混用。这不是小修，
   需要先决定“星体模型是 3D 专用还是需要 1D/2D/RZ 支持”“单位统一到 SI 还是 cgs”。
2. **插件运行时生命周期钩子未接线**（C6）：`on_level_created/on_checkpoint/on_restart` 需要
   在 `Runtime`/`WarpX` 的对应路径上接入，涉及 A6/A7 路线，不宜在审计中顺手改。
3. **RZ hybrid 的 per-species rho 缩放**（C2）与 **divE/divB 诊断的 ncomps**（C3）：
   属于真实数值错误，但修复需要理解 `FinalizeRho*` 与 snapshot 的时序，建议单独 PR + 回归。
4. **Tools 不变量检查器**（C7）：建议按 `reviews/Tools_Tests.md` 的逐条修法修复，
   否则“0 已知缺陷”的结论不可信。
5. **测试容差被放宽的 6 处**（Python.md F3）：需要你确认这是有意为之还是为了让 CI 变绿。
6. **`GetExternalEBField` 缺 context**（ParticleCore_A/B F1）：修法已核清——给
   `IonizationFilterFunc`、`QEDPairGenerationTransformFunc`、`QEDPhotonEmissionTransformFunc`
   的构造函数各加一个 `warpx::runtime::RuntimeContext const* a_ctx` 末参，内部把
   `GetExternalEBField(a_pti, a_offset, a_ctx)` 传下去；三个构造点分别是
   `PhysicalParticleContainer::getIonizationFunc`（已有 `runtime_context()`）与
   `MultiParticleContainer` 的 QED 循环（同样有 `runtime_context()`）。
   这样三条路径不再得到 `ps == nullptr`，既消除 `parser` 分支的空指针解引用，也恢复
   加速器晶格/重复等离子体透镜场。改动 6 个文件、纯参数穿透，建议单独 PR + 跑
   `test_*_ionization*`、`test_*_qed*`、`test_*_load_external_field*` 回归。
7. **Bremsstrahlung 顺序**（Collision_A F1）：fork 的写法在物理上更自洽（先降 `fmulti` 再夹
   `arg=1`），官方写法因先置 `arg=1` 而使 `fmulti/arg` 成为 no-op。**我没有改回官方写法**，
   因为那会把一个可能正确的修复改错；请确认是否有意偏离。

---

## 9. 交付物清单

| 路径 | 内容 |
|---|---|
| `Docs/audit_2026_11/REPORT.md` | 本报告 |
| `Docs/audit_2026_11/AUDIT_PLAN.md` | 方法与口径 |
| `Docs/audit_2026_11/CASE_RESULTS.md` | 398 个共享用例的逐案判定 |
| `Docs/audit_2026_11/NUMERIC_DIFFS_fork_vs_official.md` | 数值差异逐案（最差字段/量级） |
| `Docs/audit_2026_11/CASE_CAUSES.md` | 79 个 fork 代码差异按物理类别聚合的逐案原因索引 |
| `Docs/audit_2026_11/ATTRIBUTION.md` | fork vs official_amrex 隔离归因 |
| `Docs/audit_2026_11/reviews/*.md` | 17 份逐文件代码审查报告（189 条问题） |
| `Docs/audit_2026_11/CASE_CAUSES.md` | 79 个 fork 代码差异按物理类别聚合的逐案原因索引 |
| `Docs/audit_2026_11/ATTRIBUTION.md` | 129 个数值差异用例的逐案归因（42 AMReX / 80 fork / 7 未归因） |
| `Docs/audit_2026_11/scripts/` | 全部可复现脚本（构建环境、跑套件、对比、归因、汇总） |
| `audit_build/`、`audit_runs/` | 三套构建树与全部运行输出（已 gitignore，不提交） |
