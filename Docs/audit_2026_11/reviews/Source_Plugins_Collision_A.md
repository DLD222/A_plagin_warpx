# Source_Plugins_Collision_A 审查

分区：`Source/Plugins/Collision/{BackgroundMCC,BackgroundStopping,BinaryCollision::{BinaryCollision.*,BinaryCollisionUtils.*,Bremsstrahlung,Coulomb,DSMC,LinearBreitWheeler}}`
上游对照：`_upstream/warpX-26.10/Source/Particles/Collision/<same relative path>`（24 个文件在上游全部有一一对应实现，无新增/缺失文件）。

## 覆盖

| 文件 | 行数 | 类型 | 结论 |
|---|---|---|---|
| BackgroundMCC/BackgroundMCCCollision.H | 110 | moved + 机械重构 | OK |
| BackgroundMCC/BackgroundMCCCollision.cpp | 487 | moved + 机械重构 | OK |
| BackgroundMCC/ImpactIonization.H | 234 | moved | OK |
| BackgroundStopping/BackgroundStopping.H | 96 | moved + 机械重构 | OK |
| BackgroundStopping/BackgroundStopping.cpp | 301 | moved + 机械重构 | OK |
| BinaryCollision/BinaryCollision.H | 1554 | moved + 机械重构 | OK |
| BinaryCollision/BinaryCollisionUtils.H | 242 | moved + 机械重构 | OK（新增 2 个重载） |
| BinaryCollision/BinaryCollisionUtils.cpp | 334 | moved + 机械重构 | 可疑（F4 死代码） |
| BinaryCollision/Bremsstrahlung/BremsstrahlungFunc.H | 520 | **modified（语义变化）** | 缺陷（F1） |
| BinaryCollision/Bremsstrahlung/BremsstrahlungFunc.cpp | 125 | moved + 机械重构 | OK |
| BinaryCollision/Bremsstrahlung/PhotonCreationFunc.H | 300 | moved + 机械重构 | 可疑（F3） |
| BinaryCollision/Bremsstrahlung/PhotonCreationFunc.cpp | 40 | moved + 机械重构 | OK |
| BinaryCollision/Coulomb/ElasticCollisionPerez.H | 136 | moved | OK |
| BinaryCollision/Coulomb/PairWiseCoulombCollisionFunc.H | 172 | moved + 新增委托构造 | OK |
| BinaryCollision/Coulomb/UpdateMomentumPerezElastic.H | 276 | moved | OK |
| BinaryCollision/DSMC/CollisionFilterFunc.H | 121 | moved | OK |
| BinaryCollision/DSMC/DSMCFunc.H | 208 | moved + 机械重构 | 可疑（F2） |
| BinaryCollision/DSMC/DSMCFunc.cpp | 109 | moved + 机械重构 | OK |
| BinaryCollision/DSMC/SplitAndScatterFunc.H | 606 | moved + 机械重构 | 可疑（F2/F3） |
| BinaryCollision/DSMC/SplitAndScatterFunc.cpp | 85 | moved + 机械重构 | OK |
| BinaryCollision/LinearBreitWheeler/LinearBreitWheelerCollisionFunc.H | 258 | moved + 新增委托构造 | OK |
| BinaryCollision/LinearBreitWheeler/LinearBreitWheelerInitializeMomentum.H | 91 | moved | OK |
| BinaryCollision/LinearBreitWheeler/LinearBreitWheelerUtil.H | 272 | moved | OK |
| BinaryCollision/LinearBreitWheeler/SingleLinearBreitWheelerCollisionEvent.H | 129 | moved | OK |

方法：对 24 个文件逐个 `diff -u` 上游同名实现 + 全文阅读；把 diff 过滤掉 include/类型改名（`WarpXParIter`→`IParticleIterator`、`WarpXParticleContainer`→`IParticleContainer`、`mypc->GetParticleContainerFromName`→`*table.get(...)`、`WarpX::getCosts`→`m_data.costs` 等）后，**本分区唯一一处非机械的语义改动是 F1**（见下）。

已逐条验证的重构等价性（无缺陷）：
- `isPhysicalSpecies(PhysicalSpecies::X)` ↔ 上游 `AmIA<PhysicalSpecies::X>()`：`WarpXParticleContainer.H:137-138` 实现为 `physical_species == species`，与上游 `WarpXParticleContainer.H:617` 完全一致，`BinaryCollisionUtils.cpp` 里的 D-T/D-D/D-He/p-B 判型逻辑未变。
- `m_data.geom/lowerCorner/costs`、`m_config->load_balance_costs_update_algo()`、`n_rz_azimuthal_modes()`：`WarpXRuntimeData.cpp:76,83,156`、`RuntimeConfig.cpp:228-235,298-301` 均直接转发到 `WarpX::Geom/LowerCorner/getCosts` 与 `WarpX::load_balance_costs_update_algo`，无行为差异。
- 核函数发射方式（竞态面）与上游逐一对齐：`BinaryCollision.H` 的 4 处 `amrex::For`（共享 per-cell 累加/失败计数器/weight-sort 分支）与 `amrex::ParallelFor`/`ParallelForRNG` 位置、数量与上游完全相同（`grep -o` 列表逐字比对为空 diff）；MCC/Stopping 的 `ParallelForRNG`/`ParallelFor` 只写自身 `ip` 的 `ux/uy/uz`，无 scatter。
- `PhotonCreationFunc`/`SplitAndScatterFunc`/`ParticleCreationFunc` 的 `ParallelForRNG` 均按 `p_offsets[i]` 唯一槽位写产品粒子，无冲突。
- `BinaryCollision.H:330-335` 把 `Array4<Real>` 改成 `Array4<Real const>` + `const_array(mfi)`：只收紧 const，行为不变。
- `PairWiseCoulombCollisionFunc` / `LinearBreitWheelerCollisionFunc` 的新 `(name, table, isSameSpecies)` 构造委托到 `(name, nullptr, isSameSpecies)`：被委托构造体的 `mypc` 参数在两者中都是 `[[maybe_unused]]`（`PairWiseCoulombCollisionFunc.H:59`、`LinearBreitWheelerCollisionFunc.H:68`），传 nullptr 安全。

## 缺陷清单

### F1 Bremsstrahlung 饱和分支把 `fmulti` 除以了“未夹紧的 arg”，光子权重与上游不同

- 文件:行：`Source/Plugins/Collision/BinaryCollision/Bremsstrahlung/BremsstrahlungFunc.H:361-362`
- 严重度：major（触发条件：`multiplier > 1` 且 `arg > 1`；默认 `multiplier = 1` 时与上游完全一致，故 uplift 上限为 `multiplier` 倍）
- 置信度：confirmed（代码差异可验证）/ likely（物理影响未跑用例）
- 现象与依据：

  fork：
  ```cpp
  if (arg > 1.0_prt) {
      ...
      fmulti = std::max(fmulti/arg, 1.0_prt);   // 用 arg>1 的旧值做除法
      arg = 1.0_prt;
  }
  ```
  上游 `_upstream/warpX-26.10/Source/Particles/Collision/BinaryCollision/Bremsstrahlung/BremsstrahlungFunc.H:357-358`：
  ```cpp
  arg = 1.0_prt;                              // 先夹紧
  fmulti = std::max(fmulti/arg, 1.0_prt);     // 再除 → 除以 1.0，实际是 no-op
  ```
  两条语句顺序被交换，语义随之改变。设用户 `multiplier = m ≥ 1`，且本对粒子 `arg = fmulti·v1·σ·n2·dt·γ_factor > 1`：
  - 上游：`fmulti` 保持 `m`，`q12 = arg = 1`（必碰撞），光子权重 `w_photon = w1/m`；
  - fork：`fmulti = max(m/arg, 1) ≤ m`，`q12 = 1`，光子权重 `w_photon = w1/fmulti`，最大放大 `m` 倍。

  `w_photon` 直接进入能量-动量记账：`PhotonCreationFunc.H:198-199` 用 `Ephoton*wp/(w1*m1*c2)` 计算电子/离子反冲，`PhotonCreationFunc.H:260,267-269` 用 `wp` 设置新光子权重并令其动量为 `Ephoton/(m_e c)`，所以两条路径各自守恒，差别只是“每次事件辐射的总能量/样品权重”不同。上游那一行实际是死代码（除以 1.0），fork 的写法更像“原本想要的修正”（在 `q12` 饱和到 1 时把未采样的 arg 倍率补进权重），但它未经说明、未经测试地改变了结果，且 `arg` 越大 `fmulti` 被压得越狠（夹到 1 为止）。
  注意 `KE_cm` 光子能量上限（`BremsstrahlungFunc.H:382-387`）只用 `weight1/weight2`、不含 `fmulti`，对应 `fmulti = 1` 的最坏情形，所以 fork 的改动不会突破该上限（改动方向是让 `fmulti` 更接近 1）。
- 建议修法：二选一并写清理由：
  1. 恢复上游顺序（保持与官方 26.10 一致，避免静默行为差异）；
  2. 若要保留“饱和时把 arg 补进权重”的物理意图，写成显式两行（`arg_eff = ...; fmulti = max(fmulti/arg_eff,1); arg = 1;`）+ 注释 + 回归用例。
- 验证方法：构造 `multiplier = 10` 且单步 `arg > 1` 的 bremsstrahlung 算例（高 `n2`、大 `σ`、小 `dt` 使 `arg`≈3），比较 fork 与 upstream 输出的光子宏粒子 `w`、电子动能损失、`Ephoton` 分布；或直接 `git diff` 上游该函数确认只有这两行。

### F2 三个“接受 `MultiParticleContainer const*`”的构造函数只声明、无定义（悬空声明）

- 文件:行：
  - `Source/Plugins/Collision/BinaryCollision/DSMC/DSMCFunc.H:65-67`（`DSMCFunc(name, MultiParticleContainer const*, bool)`）
  - `Source/Plugins/Collision/BinaryCollision/DSMC/SplitAndScatterFunc.H:54`
  - `Source/Plugins/Collision/BinaryCollision/Bremsstrahlung/BremsstrahlungFunc.H:59-60`
- 严重度：minor
- 置信度：confirmed
- 现象与依据：重构后 `.cpp` 里的定义签名已改成 `(name, const CollisionSpeciesTable& [, bool])`（`DSMCFunc.cpp:21-24`、`SplitAndScatterFunc.cpp:13-15`、`BremsstrahlungFunc.cpp:15-17`），旧的 `mypc` 版本只剩声明。全树 grep 确认当前无调用点（因此能正常链接，`audit_build/fork` 里四个维度的 `DSMCFunc.cpp.o` 都存在），但任何后续/外部调用都会得到 undefined reference；同时这些声明上的 doxygen 注释仍写 `@param mypc pointer to the MultiParticleContainer`（如 `DSMCFunc.H:61`、`SplitAndScatterFunc.H:52`、`PhotonCreationFunc.H:55`），与实现不符。
- 建议修法：删除这三条无定义的声明（以及 `BremsstrahlungFunc()` 外的多余注释），把注释改成 `CollisionSpeciesTable`。
- 验证方法：`grep -rn "MultiParticleContainer const \*" Source/Plugins/Collision` 列出 6 条旧签名引用，其中只有 `DSMCFunc.H:66`、`SplitAndScatterFunc.H:54`、`BremsstrahlungFunc.H:59` 在 `.cpp` 里没有对应定义（另 3 条是委托构造或 `/*mypc*/` 形参，有定义）；或临时加一处旧签名调用看链接是否失败。

### F3 `#include` 被放在文件第 1 行、版权头与 include guard 之前

- 文件:行：`Source/Plugins/Collision/BinaryCollision/Bremsstrahlung/PhotonCreationFunc.H:1`、`Source/Plugins/Collision/BinaryCollision/DSMC/SplitAndScatterFunc.H:1`（均为 `#include "Modules/Particles/ParticleCreation/DefaultInitialization.H"`）
- 严重度：minor（可移植性/卫生；今天不报错）
- 置信度：confirmed
- 现象与依据：该 include 位于 `#ifndef WARPX_PHOTON_CREATION_FUNC_H_` / `#ifndef WARPX_SPLIT_AND_SCATTER_FUNC_H_` 之前，所以头文件自身的 guard 无法阻止它被重复展开（目前仅因 `DefaultInitialization.H:8-9` 自带 `WARPX_DEFAULTINITIALIZATION_H_` guard 才安全），并且把一个很重的头（含 `Runtime/RuntimeData.H`、`WARPX_QED` 下 `Particles/ElementaryProcess/...`）无条件变成这两个头的编译依赖；位置也破坏了仓库的 include 分组规则（AGENTS.md）。`SplitAndScatterFunc.cpp:11-12` 还残留连续两个空行。
- 建议修法：把该 include 移入正常 include 块（`Modules/Particles/ParticleCreation/SmartCopy.H` 附近），删掉多余空行；若确有循环依赖，应在头里前向声明而不是提前 include。
- 验证方法：`grep -n "DefaultInitialization" -A2 -B2` 查看位置；`pre-commit run -a` 会报格式问题。

### F4 `BinaryCollisionUtils` 新增的两个转发重载在树内无调用者（死代码 + 多余快照）

- 文件:行：`Source/Plugins/Collision/BinaryCollision/BinaryCollisionUtils.cpp:320-334`，声明见 `BinaryCollisionUtils.H:41-57`
- 严重度：minor
- 置信度：confirmed
- 现象与依据：`get_collision_type(name, IParticleContainerAccess const*)` / `get_nuclear_fusion_type(...)` 全树除自身外无调用（`grep -rn "get_collision_type\|get_nuclear_fusion_type" Source/ | grep -v BinaryCollisionUtils` 为空）；且每次调用都会 `CollisionSpeciesTable table(*mypc)`（`CollisionSpeciesTable.cpp:15-27` 会遍历所有 species 并快照 `getCharge/getMass/TotalNumberOfParticles`）再丢弃。属重构期的兼容垫片，留着会增加“两个入口谁在用”的歧义。
- 建议修法：删除这两个重载及对应声明，只保留 `CollisionSpeciesTable` 版本。
- 验证方法：删除后 `cmake --build build -j8`（当前无调用者，应无链接错误）。

## 不确定但值得复核的点

1. **GNU Make 构建路径已失效（跨分区）**：`Source/Plugins/Collision/Make.package:6-11` 与各子目录 `Make.package`（如 `BinaryCollision/Make.package:7-9`、`BackgroundMCC/Make.package:3`）仍 `include $(WARPX_HOME)/Source/Particles/Collision/...`，而 fork 里 `Source/Particles/` 已不存在（移到 `Source/Plugins/`）。CMake 构建正常（`audit_build/fork` 四个维度都有 collision 目标产物），但顶层 `GNUmakefile` 仍在，legacy make 会找不到这些包。这些文件不在我的清单内，建议归到构建系统分区。
2. **cost-timer 访问方式不一致（仅风格）**：`BackgroundMCCCollision.cpp:284,292` 用 `m_config->load_balance_costs_update_algo()`，同文件 `:455,480` 用 `species1.config().load_balance_costs_update_algo()`。今天两者都转发到同一个 `WarpX::load_balance_costs_update_algo`（`RuntimeConfig.cpp:298-301`），无功能差异，但建议统一。
3. **`m_data`/`m_config` 生命周期**：`BinaryCollision.H:1529` 持有 `const RuntimeData&`，`BackgroundMCCCollision.H:83-84`/`BackgroundStopping.H:83-84` 持有裸指针。我已确认它们指向 `WarpXKernel` 的 `m_data`(unique_ptr)/`m_config`(值成员)（`WarpXKernel.cpp:64,69`）或 `WarpX::WarpXRuntimeData` 单例（`WarpXEvolve.cpp:748`），寿命长于 collision handler，当前无悬垂；但把“引用成员 + 默认拷贝”作为长期约定值得复核（`CollisionBase` 侧若被拷贝到别处仍共享同一份 data 引用，属预期）。
4. **继承自上游、非本 fork 引入的两处注释/实现不符**（不计入缺陷，供参考）：`BackgroundStopping.cpp:264-265` 注释写 “slowing down of beam ions due to collisions with electrons”，实际是 ions 背景分支（`doBackgroundStoppingOnIonsWithinTile`）；`BackgroundMCCCollision.cpp:159-198`（最大碰撞频率扫描，上游同）用固定能量网格 `E_start…E_end`（默认 1e-4…5000 eV）扫描 `nu_max`，若用户截面表超出该上界会低估 `nu_max`，只靠 `coll_n > 0.1` 的 warning（`:228-233`）兜底，不会中止。
5. **`BinaryCollision.H:341` 的 `dV`**：`AMREX_D_TERM` 只取 cell size 乘积，RZ/RCYLINDER/RSPHERE 再乘 `volume_factor(i_cell)`（`:354-378`）——与上游逐字一致，但 `RSPHERE` 那支注释 “This needs double checking” 是上游遗留，若本项目要支持 RSphere 碰撞需单独验证 `4πr²` 权重与 `i_cell = ri` 的一维映射。
