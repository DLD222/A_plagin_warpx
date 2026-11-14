# 2026-11 审计计划：官方 WarpX 26.10 对照 + 全算例逐字节对比 + 代码审查

## 0. 结论口径（本目录所有报告共用）

| 术语 | 含义 |
|---|---|
| 官方 26.10 | `_upstream/warpX-26.10`，git tag `26.10` = commit `ec1660d`（GitHub BLAST-WarpX/warpX） |
| 本 fork | 工作区 HEAD（`WarpX_plugin`，origin `DLD222/A_plagin_warpx`） |
| 构建 A（official） | 官方源码 + 官方 pin（AMReX tag 26.10, pyAMReX 26.10） |
| 构建 B（official_amrex） | 官方源码 + fork 的 AMReX pin（`008b6737`）+ pyAMReX `72fe55f`，用于把“上游代码差异”与“AMReX 版本差异”分开 |
| 构建 F（fork） | 本 fork HEAD + 其自身 pin（AMReX `008b6737`, pyAMReX `72fe55f`） |
| 逐字节 | 同一用例、同一 rank 数、同一 `OMP_NUM_THREADS=1`、同一 ctest 运行时参数下，输出目录内每个文件先按字节比较；不同则逐数据集量化 |
| 差异归因 | `B vs F` 只含 fork 的源码改动；`A vs F` 含 fork 改动 + AMReX/上游演进 |

## 1. 构建（三套，参数完全一致）

- `-DWarpX_DIMS="1;2;3;RZ" -DWarpX_COMPUTE=OMP -DWarpX_MPI=ON -DWarpX_OPENPMD=ON -DWarpX_FFT=ON -DWarpX_QED=ON -DWarpX_EB=ON -DWarpX_PRECISION=DOUBLE -DWarpX_PYTHON=ON -DWarpX_PYTHON_IPO=OFF -DpyAMReX_IPO=OFF -DCMAKE_BUILD_TYPE=Release`
- 编译器 `/usr/bin/gcc` `/usr/bin/g++`（13.3.0）、MPI `/usr/bin/mpicxx`（OpenMPI 4.1.6）、`/usr/bin/mpiexec`
- 依赖源码预先冻结在 `audit_build/deps/`（AMReX 两版、openPMD 0.17.1、pyAMReX 两版、pybind11 v3.1.0、PICSAR 26.10）
- 环境：`audit_build/cleanenv.sh` 去掉 conda 的 CFLAGS/LDFLAGS/CMAKE_PREFIX_PATH 污染，避免 ctest 与编译期混入 conda 的 MPICH

## 2. 用例集合

- 官方 26.10 注册的全部 ctest 用例（`Examples/`，`add_warpx_test`），按注册的 dims 与 nprocs 跑；
- 二进制用例（`inputs*`）+ PICMI/Python 用例（`*.py`）；
- 逐用例记录：官方与 fork 各自的 run 结果、输出目录、比对结论。

## 3. 比对与产出

- `Docs/audit_2026_11/cases.csv`：每用例一行（dim、nprocs、官方/fork 退出码、字节一致文件数、差异文件数、最大绝对/相对差、归因）
- `Docs/audit_2026_11/diff_details/<test>.md`：差异用例的逐数据集明细
- `Docs/audit_2026_11/repro/`：可复现脚本 + 每用例的输入快照（`warpx_used_inputs`）与命令
- `Docs/audit_2026_11/CODE_AUDIT.md`：逐文件审查记录与缺陷清单
- `Docs/audit_2026_11/FIXES.md`：确认并修复的 bug（含官方源码依据、回归证据）

## 4. 代码审查协议

- 覆盖：`Source/` 全部 842 个 `.cpp/.H/.py`（通读）；其中 364 个与上游同路径但内容不同的文件做 diff 深审，238 个新文件逐行深审；
- 每文件记录：文件、行数、结论（OK / 可疑 / 确认缺陷）、依据（上游对应实现、物理方程、边界条件）；
- 重点：AI 生成代码常见幻觉——不存在的 API、错误单位/常数、静默吞错、错误的循环边界、拷贝粘贴变量、注释与代码不符、未初始化、竞态、维度(RZ/1D)特化遗漏；
- 修复原则：只改确认缺陷；每处修改附最小复现与回归（同用例 fork vs official 差异缩小或测试通过），不修改官方基准容差。

## 5. 已知环境事实

- 本 fork 的基线比 tag 26.10 新：fork 的 `dependencies.json` pin AMReX `008b6737`，且 Examples 里含 26.10 之后才有的用例（implicit、dynamic_regrid 等）。
- 官方 26.10 注册 398 个 ctest 用例（Python 开）；本 fork HEAD 注册 411 个（多 13 个）。
- 本 fork 增加了 `needs_openpmd` 第 8 参数（用例注册条件，不影响物理输入）。
