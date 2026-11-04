# A0 第二遍：核验记录与新增发现

<!-- SPDX-License-Identifier: BSD-3-Clause -->

> 状态：2026-11-04。第一遍自动目录 + 抽检已完成；本文记录第二遍的逐项核验、以及第一遍没暴露的 `RemakeLevel` 缺口。
> 结论：A0 可以收口，但 **A2（Level 资源）的范围要扩大**——不只是 `ClearLevel` 漏项，`RemakeLevel` 也漏了材料场和 PML。

---

## 1. 目录/接口核验

| 项 | 结果 |
|---|---|
| 头文件 | 371 个全部登记（`COVERAGE.md`） |
| 解析出类 | 235 个文件、348 个类/结构体 |
| 无类头文件 | 136 个：30 个 `_fwd.H` 前向声明 + 106 个 free function/kernel/枚举（已逐个 grep 确认无 `class/struct` 定义） |
| 公共方法 | 2,408 个（去重、去掉内联体碎片后） |
| 重点类抽检 | `WarpX`（217）、`MultiParticleContainer`（67）、`FiniteDifferenceSolver`（64）、`MacroscopicProperties`（38）、`Diagnostics`（34）——签名与头文件一致；模板/默认参数有简化 |

**解析限制**（已写进 `A0/README.md`）：`#if` 分支合并、嵌套类可能遗漏、宏不展开。用于“找类和接口”足够，不能替代逐行阅读。

---

## 2. 五个待验证项

### 2.1 PSATD restart

**结论：可重建，但依赖 `ComputeDt` 先于 `AllocLevelData`。**

- `InitData`：`ComputeDt()`（`WarpXInitData.cpp:833`）→ `InitFromCheckpoint()`（`:840`）。
- `InitFromCheckpoint` 对每个 level 调 `AllocLevelData(lev, ba, dm)`（`WarpXIO.cpp:213`）。
- `AllocLevelData` 调 `AllocLevelSpectralSolver`（`WarpX.cpp:2890-2920`），后者用 `dt[lev]` 构造 `SpectralSolver`（`WarpX.cpp:3296-3314`）。
- 谱求解器没有持久化状态；重启后按新 `ba/dm/dt` 重建，`t_new` 从 `WarpXHeader` 恢复。

**风险**：`m_JRhom`/`Strang_Implicit_Spectral_EM` 的 `solver_dt` 减半逻辑在 `AllocLevelSpectralSolver` 内，重启后仍按输入生效；未逐行验证 `m_spectral_index` 的 `rho_mid` 等场在重启后的初值。

### 2.2 `RemakeLevel` 覆盖

**结论：覆盖 8 类，漏 4 类；`ClearLevel` 另漏 5 类。**

`RemakeLevel`（`WarpXRegrid.cpp:176-350`）处理：

1. `m_fields.remake_level(lev, dm)`
2. EB 的 `m_eb_reduce_particle_shape` / `m_eb_update_E/B_fp/cp`
3. `m_borrowing`（仅 ECT）
4. `m_field_factory`（EB 重建 / FArrayBoxFactory）
5. `InitializeEBGridData`（edge lengths/face areas/dual geometry/flags）
6. PSATD `spectral_solver_fp/cp`
7. `m_accelerator_lattice` element finder
8. `costs`、`SetDistributionMap`、buffer masks、`multi_diags->InitializeFieldFunctors(lev)`

**未处理**：

| 对象 | 为什么是缺口 | 证据 |
|---|---|---|
| `pml[lev]` | PML 拥有自己的 split fields/SigmaBox，DM 变了但 PML 没重建 | `WarpX.H:1449`；`WarpXRegrid.cpp` 无 `pml` 引用 |
| `m_macroscopic_properties` 的系数场 | 它们不是 `m_fields` 里的场，而是 `unique_ptr<MultiFab>`；`m_fields.remake_level` 不会动它们 | `MacroscopicProperties.cpp:303-329` |
| `Efield_dotMask/Bfield_dotMask/Afield_dotMask/phi_dotMask` | 未在 `RemakeLevel` 重建 | `WarpXRegrid.cpp` 无 dotMask 引用 |
| `m_flag_info_face` | 只在 `InitializeEBGridData` 重算，但 `m_borrowing` 重建时未同步 | `WarpXRegrid.cpp` 只重建 `m_borrowing` |

`ClearLevel`（`WarpX.cpp:2368`）只清 `m_fields`、dotMask、PSATD solver、costs；**不清** `m_field_factory`、`m_eb_update_*`、`m_flag_info_face`、`m_borrowing`、`m_accelerator_lattice`。

**含义**：A2 的 `LevelResources` 必须把“场/材料/PML/EB/诊断/成本”作为一个整体登记，`RemakeLevel` 与 `ClearLevel` 使用同一张表。否则负载均衡 + 介质/PML 会产生悬空或旧 DM 的 MultiFab。

### 2.3 诊断时序

**结论：构造早于场分配，`InitData` 晚于场分配；必须拆成两步。**

- `MultiDiagnostics` 在 `WarpXInitData.cpp:815` 构造（`InitData` 内，`InitFromScratch` 之前）。
- `multi_diags->InitData(*mypc)` 在 `WarpXInitData.cpp:1021`（`InitFromScratch` 之后）。
- `FullDiagnostics` 的 `InitializeFieldFunctors(lev)` 在 `InitData` 与 `RemakeLevel` 后调用。

**含义**：Runtime 的 `on_schema`（注册诊断类型）和 `on_init`（分配输出/绑定场）必须分开；诊断插件不能在构造期取场指针。

### 2.4 Python callback 在 implicit/hybrid 分支

**结论：不完整，且不是统一阶段。**

- 显式分支：`beforestep/afterstep`、`beforedeposition/afterdeposition`、`beforecollisions/aftercollisions`、`beforeEsolve/afterEsolve`、`particleinjection` 等（`WarpXEvolve.cpp`）。
- Hybrid：`Evolve` 的 hybrid 分支有 `beforeEsolve/afterEsolve`（`:338-343`）。
- 隐式：`OneStep` 直接调 `m_implicit_solver->OneStep`（`:422`），**没有** `beforestep/afterstep/beforeEsolve`；只有 `SemiImplicitDarwin.cpp:207` 的 `afterdeposition`。
- `allocdata` 在 `InitFromScratch`/`InitFromCheckpoint` 都有。

**含义**：Runtime 的 Stage 不能假设现有 callback 覆盖所有分支；A3 要给隐式/ Hybrid 补阶段，或明确“隐式分支暂不插件化”。

### 2.5 EB per-level 与 `RemakeLevel`

**结论：EB 几何是静态的；`m_eb_index_space` 不重建是合理的，但 flags/factory 必须重建。**

- `warpx.build_eb_data_per_level` 默认 true（`WarpXInitEB.cpp:88` 附近），`InitEB` 对每个 level 调 `EB2::Build`（`:105-130`）。
- `InitializeEBGridData(lev)` 计算 edge lengths/face areas/dual geometry/flags（`WarpXInitData.cpp:1603`）。
- `RemakeLevel` 重建 factory + `InitializeEBGridData`，但**不重建** `m_eb_index_space`（几何与 DM 无关，合理）。
- ECT `ect_conformal_E` 仍 assert `maxLevel()==0`（`WarpXInitData.cpp:1615-1621`），多层 ECT 不可用。

**含义**：EB 插件只能提供 cut-cell 算法；factory/index space/flags 生命周期归 `LevelResources`。

---

## 3. 构建与基线（A1 前置）

已在本机完成 2D 构建：

```
cmake -S . -B build -DWarpX_DIMS=2 -DWarpX_EB=ON -DWarpX_MPI=ON \
  -DWarpX_COMPUTE=OMP -DWarpX_PYTHON=OFF -DWarpX_OPENPMD=OFF -DWarpX_FFT=OFF \
  -DCMAKE_BUILD_TYPE=Release \
  -DFETCHCONTENT_SOURCE_DIR_FETCHEDAMREX=/path/to/amrex-src
cmake --build build -j 24
```

- 产物：`build/bin/warpx.2d.MPI.OMP.DP.PDP.EB.QED`
- 冒烟：`ctest -R "^test_2d_langmuir_multi\.(run|analysis)$"` → 2/2 PASS
- 说明：本机基线，不代表 CI 全矩阵；后续每个 A1 批次都要跑同一组。

---

## 4. A1 首批代码（Runtime facade）

已加入 `Source/Runtime/`：

- `Runtime.H/.cpp`：进程级生命周期持有者；当前转发到 `WarpX` 单例，行为不变。
- `Source/main.cpp`：改用 `warpx::runtime::Runtime`，`initialize/evolve/finalize`。
- CMake/Make 接入；`Runtime::finalize()` 幂等，析构时兜底。

**验证**：增量构建通过；langmuir run+analysis 通过。这是 A1“去单例”的容器，不是完成；下一步把 `RuntimeConfig`/`TimeState`/`Services` 填进去。

---

## 5. A0 收口结论

- 类目录/公共接口：第一遍完成，第二遍重点类抽检完成；剩余 106 个 free-function 头文件已确认无类。
- 生命周期：12 子系统矩阵 + 5 个待验证项已处理；新增 4 个 `RemakeLevel` 缺口。
- 可以进入 A1，但 **A2 范围要扩大**：`LevelResources` 必须覆盖材料场/PML/dotMask/flag_info，不只是 `ClearLevel`。

**未完成/不在 A0 范围**：逐行读完 100,539 行；GPU kernel 细节；全部 Python/PICMI 映射；CI 全矩阵。
