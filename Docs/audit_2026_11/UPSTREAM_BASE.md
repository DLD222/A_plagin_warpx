# 本 fork 的上游基线（2026-11 审计）

本文回答一个此前没有答案、却决定了整个差异归因的问题：

> 这个 fork 的 `Source/` 到底对应官方 WarpX 的哪个版本？

结论先说：**它不是官方 26.10，而是 26.10 之后约三周的 `development` 快照**（大致
2026-10-30），并且少了 4 个上游提交。因此"与官方 26.10 不一致"这件事本身并不能
说明 fork 有缺陷——其中相当一部分是**上游 26.11 的改动被 fork 正确继承了**。

## 1. 方法

`Docs/audit_2026_11/scripts/upstream_provenance.py`

fork 把 `Source/` 重构成了 `Source/Core` + `Source/Plugins/*`，并把 `WarpX::` 静态配置
换成运行时上下文，所以逐行 `diff` 全是噪声。脚本改为比较**归一化后的行集合**：

- 去掉注释、`#include`、空行、行尾空白，折叠连续空白（不受 clang-format 影响）；
- 以"行集合的 Jaccard 相似度"代替逐行对齐（不受改名、重排影响）；
- 用**文件名**把 fork 的文件映射到上游同名文件（唯一同名才算数）。

上游三个参照树：`26.10`（tag）、`26.11`（tag）、`origin/development`。

```
python3 Docs/audit_2026_11/scripts/upstream_provenance.py \
    --fork Source \
    --upstream 26.10=/tmp/up_26.10 26.11=/tmp/up_26.11 dev=/tmp/up_dev \
    --csv /tmp/provenance.csv
```

结果：553 个文件可按唯一同名映射（其余 247 个是 fork 独有的运行时桥接文件、
或上游有多个同名文件而无法唯一定位）。

## 2. 关键数字

| 事实 | 数值 |
|---|---|
| 可映射文件 | 553 |
| 与 26.11 逐行一致（相似度 > 0.999） | 296 |
| 在 26.10 与 26.11 之间**上游改过**的可映射文件 | 43 |
| 其中 fork 用的是 **26.11 版** | 28 |
| 其中 fork 用的是 **26.10 版** | 15 |
| 优于 26.11、匹配 `development` 的文件 | 0 |

`dependencies.json` 一手佐证：fork 里是

```json
"version_warpx": "26.10",
"commit_amrex": "008b67375045d2154a6b1295994a330eef4ad822",
"commit_picmi": "89b27cee07ec78f8b7c76f5be63b581a757256fa"
```

这正是上游提交 `ed30ada99`（2026-10-27，"Dependencies: weekly update (#7475)"）
的内容——比 26.10 发布（`ec1660d96`，2026-10-04）晚三周。

## 3. fork 已经继承的 26.11 改动（按上游提交分组）

| 上游提交 | 日期 | 内容 | fork 采用的 26.11 文件 |
|---|---|---|---|
| `c80705c2b` | 10-30 | Darwin 线性算子减少 guard cell 交换 | `DarwinLinearFieldOperator.H` |
| `3523ff7e9` | 10-30 | 单线程编译 `parse_field_function` | `LaserProfileFieldFunction.cpp`, `LaserProfiles.H` |
| `b1539eda5` | 10-28 | C++20 constraints | `IsIn.H`, `Serialization.H` |
| `0d1a86f1a` | 10-26 | Hybrid-PIC 重启动保持自适应子步 | `FlushFormatPlotfile.H` |
| `9b3f2351c` | 10-21 | GPU 上周期性重建 PETSc | `LinearSolver.H`, `NonlinearSolver.H`, `WarpXInit.H`, `WarpX_PETSc.H` |
| `915662b09` | 10-21 | 用 `std::ranges::fill` | `WarpXAMReXInit.cpp` |
| `327cf813a` | 10-21 | 稀疏预条件矩阵移出 `MatrixPC` | `Preconditioner.H` |
| `d54b95590` | 10-17 | 修复 AMR+EB+EM 的段错误 | `ExternalVectorPotential.cpp` |
| **`ec99a479a`** | **10-17** | **电荷/电流密度 finalization 统一（Part 1/3）** | `WarpXFieldBoundaries.cpp` |
| `bebb39701` | 10-17 | C++20 `std::ranges` | `MsgLogger.cpp`, `TextMsg.cpp` |
| `66f5d358e` | 10-14 | 非线性求解器输入参数重构 | `AMReXGMRES_Wrapper.H`, `PicardSolver.H` |
| `323722410` | 10-14 | MLMG 使用 `NoGpuSync` | `DarwinMLMGPC.H` |
| `0d01990a9` | 10-14 | `WarpXSumGuardCells` 显式传 `icomp`/`ncomp` | `WarpXSumGuardCells.H` |
| `5f2a2e6b7` | 10-10 | Maxwell-Jüttner 改用 `temperature_in_eV` | `GetTemperature.H/.cpp`, `InjectorMomentum.H`, `TemperatureProperties.H` |
| `f976bdcb7` | 10-09 | BTD species header 读整型分量名 | `BTD_Plotfile_Header_Impl.cpp` |
| `321fb9bf2` | 10-09 | ThetaImplicitEM 子步 | `FieldPoyntingFlux.H` |
| `14b6fa791` | 10-09 | ES 求解器暴露 MLMG bottom solver | `EffectivePotentialPoissonSolver.H`, `PoissonSolver.H` |

## 4. fork 落后于 26.11 的 4 个上游提交

| 上游提交 | 日期 | 内容 | 受影响文件 |
|---|---|---|---|
| `7970fb639` | 10-22 | finalization 统一（Part 2/3） | `JFunctor.cpp` |
| `eb9115c3a` | 11-04 | finalization 统一（Part 3/3） | `WarpX_PEC.cpp`（去掉了反射边界里的径向 dV 权重 `rscale`） |
| `85fba0911` | 11-04 | 禁止 1D RSPHERE 里非物理的电磁项 | `ApplySilverMuellerBoundary.cpp`, `ComputeLaplacian.cpp`, `CurrentDeposition.H`, `EvolveB/BPML/E.cpp`, `ExternalField.cpp`, `FieldGather.H`, `FiniteDifferenceSolver.H`, `GetExternalFields.H`, `HybridPICSolveE.cpp`, `ImplicitPushPX.cpp` |
| `f2c086e33` | 11-05 | MoveWindow 交换 RZ PML 角点 guard cells | `WarpXMovingWindow.cpp` |

这 4 个提交里没有一个是**修正 fork 自身重构引入的 bug**，它们是上游的后续演进；
fork 在这 15 个文件上停在 26.10 的状态，因此在这些文件覆盖的算例上，fork 与官方
26.11 之间会存在差异，而与官方 26.10 一致。

是否把 4 个提交前移（= 与 26.11 对齐、但与 26.10 产生新差异），是一个需要用户
拍板的方向问题，见 `Docs/audit_2026_11/ATTRIBUTION.md` 末尾的决策矩阵。

## 5. 对归因的后果

1. 之前把 132 个"有差异"的算例简单分成"50 个 AMReX 漂移 / 79 个 fork 代码 / 3 个未定"，
   其中"fork 代码"这个词是不准确的：fork 与 26.10 的差异有三个不同来源，必须分开：
   - **(a) 上游演进**：fork 采用 26.10→基线（约 10-30）之间的上游改动；
   - **(b) fork 重构**：`Source/Core` + `Source/Plugins` 拆分与运行时上下文改造；
   - **(c) fork 功能**：`star_gravity` / `star_fluid` 插件等 fork 新增代码。
   只有 (b)(c) 才是"fork 作者的代码"，才需要当作缺陷来审。
2. 新增了第四个参照构建 `audit_build/official_2611`（官方 26.11 源码 + fork 同款
   AMReX `008b6737`），用来把 (a) 和 (b)(c) 用实测分开：
   - fork 与 `official_2611` 都不同 → 一定是 (b) 或 (c)；
   - fork 与 26.10 不同、与 `official_2611` 相同 → 一定是 (a)，不需要"修"。
3. 由此可以判定：RZ `rho` 的两个残余缺陷（见 `RECOMPARE_79.md`）属于 (a)，
   **不是 fork 的缺陷**，而且其中一个是上游的物理修正（方位模在 r=0 的宇称）。
