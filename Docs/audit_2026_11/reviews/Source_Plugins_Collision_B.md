# Source_Plugins_Collision_B 审查

审查对象：`Source/Plugins/Collision/` 后半（二元碰撞后半、核反应、CollisionHandler、混合电阻拖曳、逆轫致辐射、脉冲衰变）。
上游对照：`_upstream/warpX-26.10/Source/Particles/Collision/`（逐文件 `diff -u` + 逐函数阅读）。
本地构建状态：`audit_build/fork` 里 `WarpX_PLUGIN_COLLISION=ON` 且 `warpx_collision_{1d,2d,rz}` 目标文件齐全 ⇒ 本分区**无"幻觉 API/编不过"类缺陷**，问题都在语义层。

## 覆盖

| 文件 | 行数 | 类型 | 结论 |
|---|---|---|---|
| BinaryCollision/LinearCompton/LinearComptonCollisionFunc.H | 254 | modified（仅类型改名 + 多一个受委托构造函数） | OK（死代码，见 F8） |
| BinaryCollision/LinearCompton/LinearComptonInitializeMomentum.H | 178 | moved | OK（与上游逐字相同） |
| BinaryCollision/LinearCompton/SingleLinearComptonCollisionEvent.H | 136 | moved | OK（仅 include 路径） |
| BinaryCollision/NuclearFusion/BoschHaleFusionCrossSection.H | 118 | moved | OK（仅 include 路径） |
| BinaryCollision/NuclearFusion/NuclearFusionFunc.H | 316 | modified | 可疑（F5, F8） |
| BinaryCollision/NuclearFusion/ProtonBoronFusionInitializeMomentum.H | 197 | moved | OK（仅类型改名） |
| BinaryCollision/NuclearFusion/SingleNuclearFusionEvent.H | 147 | moved | OK（仅 include 路径） |
| BinaryCollision/NuclearFusion/TwoProductFusionInitializeMomentum.H | 111 | moved | OK（仅类型改名） |
| BinaryCollision/ParticleCreationFunc.H | 369 | modified | 可疑（F8：重复 include、guard 外的 include） |
| BinaryCollision/ParticleCreationFunc.cpp | 286 | modified | OK（`AmIA<T>()`→`isPhysicalSpecies()` 语义等价，见下） |
| BinaryCollision/TwoProductUtil.H | 268 | moved | OK（仅 include 路径） |
| CollisionBase.H | 58 | modified | OK（`doCollisions` 去掉 `mypc` 参数，调用点同步改） |
| CollisionFactory.cpp | 53 | new | 可疑（F4 的隐患来源；F3） |
| CollisionFuncBase.H | 31 | modified | OK（`AllocData(RuntimeData)`；`BinaryCollision` 正确转发） |
| CollisionHandler.H | 65 | modified | 可疑（F3, F8） |
| CollisionHandler.cpp | 223 | modified | 可疑（F3, F6, F7, F8） |
| CollisionSpeciesTable.H | 80 | new | 可疑（F3） |
| CollisionSpeciesTable.cpp | 59 | new | 可疑（F3） |
| HybridResistiveDrag/HybridResistiveDrag.H | 65 | modified | OK |
| HybridResistiveDrag/HybridResistiveDrag.cpp | 253 | modified | OK（内核体与上游逐字相同） |
| InverseBremsstrahlung/InverseBremsstrahlung.H | 85 | modified | 可疑（F8：重复 include） |
| InverseBremsstrahlung/InverseBremsstrahlung.cpp | 376 | modified | 可疑（F4） |
| PulsedDecay/PulsedDecay.H | 69 | **实质重写** | 缺陷（F1, F2） |
| PulsedDecay/PulsedDecay.cpp | 323 | **实质重写** | 缺陷（F1, F2） |

统计：24 个文件、4120 行全部用 read 工具读完（大文件分页）。逐文件 `diff -u` 见 `/tmp/dB/*.diff`（本次审查生成，可复现）。

### 等价性抽检（确认不是缺陷的搬移点）
- `first_species.AmIA<PhysicalSpecies::hydrogen2>()` → `first_species.isPhysicalSpecies(PhysicalSpecies::hydrogen2)`：
  上游 `WarpXParticleContainer.H:616-617` `return (physical_species == PhysSpec);`；本地 `WarpXParticleContainer.H:137-138` `return physical_species == species;`，枚举定义（`SpeciesPhysicalProperties.H:17`）与上游逐字相同 ⇒ 语义等价。
- `warpx.m_fields` → `const_cast<...>(m_data->fields())`；`WarpX::LowerCorner(box,lev,0._rt)` → `m_data->lowerCorner(box,lev)`（`RuntimeData.H:91` 默认 `delta=0.0`，`WarpXRuntimeData.cpp:83` 直接转调 `WarpX::LowerCorner`）⇒ 数值等价；`gett_new/nox/Geom/InvCellSize/costs/finestLevel` 均为 live view，无快照过期。
- `WarpXParIter` → `IParticleIterator`(= `ParticleTileIter : amrex::ParIterSoA<nattribs,0,...>`)：与上游 `WarpXParIter` 同一基类、同一 tile/attribs 视图 ⇒ HybridResistiveDrag 的 gather 循环等价。
- `mypc->TransformMomentumToCurvilinear` / `GenerateGlobalDebyeLength` / `GenerateVirtualPhotons` → `m_host->particleServices().*`：`WarpXHostServices.cpp:287-311` 逐项转调原函数 ⇒ 等价（但见 F4 的"host 为空则静默跳过"）。

## 缺陷清单

### F1 `pulsed_decay` 删除了上游必填参数 `fixed_product_weight`，无 BackwardCompatibility 保护，旧输入静默改物理
- 文件:行：`PulsedDecay.cpp:44-49`、`PulsedDecay.cpp:166`、`PulsedDecay.cpp:290-291`；`PulsedDecay.H:60`
- 严重度：**major**
- 置信度：confirmed
- 现象与依据：
  1. 上游该参数是**必填**（`_upstream/.../PulsedDecay.cpp:72` `pp_collision_name.get("fixed_product_weight", m_fixed_product_weight);`，`get` 而非 `query`），并决定每次衰变产生的宏粒子数（上游 `:254` `num_expected = total_product/fixed_product_weight`）和产物权重（上游 `:327` `wpAB = fixed_product_weight`）。
  2. 本地改为整数 `weight_ratio`（`pp_collision_name.query("weight_ratio", m_weight_ratio)`，默认 1），**从不读取** `fixed_product_weight`；全仓库 `grep -rn fixed_product_weight`（不含 `_upstream`）结果为 0 次命中，且 `CollisionBase::BackwardCompatibility()`（`CollisionBase.cpp:65`）未加任何迁移/告警。
  3. 后果：任何仍带 `fixed_product_weight = ...` 的旧 deck（含上游 `Examples/Tests/collision/inputs_test_3d_collision_pulsed_decay:111` 的写法）在本地跑会**静默**使用 `weight_ratio=1`，即"产物权重 = 母粒子权重、每个衰变母粒子只产生 1 对产物"，宏粒子数与统计量都与用户设定不同；无警告、无断言。
  4. 结果确实变了（可验证）：`Regression/Checksum/benchmarks_json/test_3d_collision_pulsed_decay.json` 相对上游被整份重写，例如 ions `particle_weight` 2.09194336e17 → 2.09275e17、neutrals 1.0805664e16 → 1.0725e16、`particle_position_x` 6415.65 → 7613.5。也就是说随机数流与逐字节可比性**已断**（本分区唯一改变 RNG 流的文件），benchmark 是"事后重生成"而不是"结果未变"。
- 建议修法（最小改动）：在 `PulsedDecay` 构造函数里保留旧键的迁移：
  `amrex::Real fw = -1; if (pp_collision_name.query("fixed_product_weight", fw)) { WARPX_ALWAYS_ASSERT_WITH_MESSAGE(!pp_collision_name.contains("weight_ratio"), "pulsed_decay: fixed_product_weight was replaced by weight_ratio; do not mix"); ablastr::warn_manager::WMRecordWarning(...); }`（或至少 `WARPX_ABORT_WITH_MESSAGE` 提示改键）；等价写法是在 `CollisionBase::BackwardCompatibility()` 里登记该改名。
- 验证方法：`grep -rn "fixed_product_weight" Source/ Docs/ Examples/`（应为空 ⇒ 无兼容处理）；把一个带 `fixed_product_weight=1.6328e13` 的上游 deck 喂给本地可执行文件，观察无任何 warning 且 `reduced_diags` 里 `ion_weight`/`ion_number` 与上游 benchmark 不同。

### F2 `pulsed_decay` 新采样器：整片母粒子被销毁 + 产物权重按 `weight_ratio` 逐代缩小；统计方差与文档表述都变了
- 文件:行：`PulsedDecay.cpp:203-221`（每宏粒子 Bernoulli）、`PulsedDecay.cpp:290-295`（`wA=w1/wr`、`w1=0`、`idcpu=Invalid`）；`PulsedDecay.H:25`（docstring）
- 严重度：minor
- 置信度：likely（推导清楚；量级需跑测试确认）
- 现象与依据：
  - 上游按**单元**总权重抽样（`wtot1*(1-exp(-nu dt))/fixed_product_weight` 个产物，再从同单元多个母粒子 `remove_weight_from_colliding_particle` 扣重，上游 `:243-341`），本地改为对**每个宏粒子**做一次 `probability = -expm1(-nu*dt)` 的 Bernoulli 并整片删除（`w1[ip]=0; idcpu1[ip]=Invalid`）。
  - 期望值正确（每步移除权重期望 `w*(1-e^{-nu dt})`，产物总权重恰等于被移除权重，我核算过 charge/weight 均守恒），但**方差结构**完全不同：产物宏粒子数 = `weight_ratio × 衰变母粒子数`，而单粒子抽样的相对涨落 ~ `1/sqrt(N_decayed)`。`Examples/Tests/collision/analysis_collision_3d_pulsed_decay.py:49,57` 的 0.5% 容差是配合上游"单元总量抽样"定的，未随之复核 ⇒ 有 flaky 风险。
  - 产物权重 = 母权重/`weight_ratio`；若存在**链式衰变**（A→B+C 且 B→D+E，各自 `weight_ratio>1`），产物权重会按 r·r′… 几何下降，无下界、无与 `particles.<species>.weight` 的一致性检查。
  - `PulsedDecay.H:25` 的 `nA(t+dt)-nA(t) = n1(t)*(1-exp(-nu*t))` 现在只在**期望**意义成立，注释未更新（注释与代码不符）。
- 建议修法：在 `doCollisions` 里对 `weight_ratio>1` 时的产物权重加一次 `WARPX_ALWAYS_ASSERT`/警告（例如要求 `w_parent/weight_ratio >= particles.<species>.weight * 1e-3`），并把 docstring 改成"期望意义"；或恢复"单元总权重抽样 + 扣重"以保持方差量级。
- 验证方法：`ctest --test-dir audit_build/fork -R test_3d_collision_pulsed_decay --repeat-until-fail 20`，看 0.5% 断言是否偶发失败。

### F3 `CollisionSpeciesTable` 的 `SpeciesHandle` 全套机制是死代码，且构造时付一次"全粒子数全局归约"
- 文件:行：`CollisionSpeciesTable.cpp:20-29`、`CollisionSpeciesTable.cpp:32-47`、`CollisionSpeciesTable.cpp:49-58`；`CollisionSpeciesTable.H:36-54,64-67`；`CollisionHandler.H:43`
- 严重度：minor
- 置信度：confirmed
- 现象与依据：
  - `m_handles`（`std::map<std::string, SpeciesHandle>`）在构造函数里为每个 species 建一个 `SpeciesSpec`，其中 `spec.count = pc.TotalNumberOfParticles(false)`（`CollisionSpeciesTable.cpp:27`）。但 `CollisionSpeciesView::handle()` 在整个仓库**没有任何调用点**（`grep -rn "\.handle()\|->handle()\|\.names()\|speciesTable()"` 只命中定义处），`CollisionSpeciesView::operator bool`、`operator->` 同样无人使用；`CollisionSpeciesTable::names()`（`:49-58`）、`CollisionHandler::speciesTable()`（`CollisionHandler.H:43`）也无人使用。顺带：`CollisionSpeciesTable.H:46` 的注释声称 `get(name)->` 是"为现有模型代码保留的兼容"，但全仓库没有一处 `get(name)->`（只有 `*get(name)`）⇒ 注释与代码不符。
  - 代价与风险：`TotalNumberOfParticles` 是 amrex 的**全局归约**（`AMReX_ParticleContainer.H:626`），构造点发生在 `WarpXKernel` 构造（`WarpXKernel.cpp:72`）即 `InitData` 之前，此时计数恒为 0 —— 一个"永远是 0 且永不被读"的 MPI 归约；且实参 `false` 表示 `only_valid=false`（连 invalid 粒子一起数），与本仓库其他位置 `totalNumberOfParticles()`（`WarpXParticleContainer.H:135`）的意图相反，属于典型的"快照型死数据"。
- 建议修法：删掉 `m_handles` / `SpeciesSpec` 填充 / `CollisionSpeciesView::handle()` / `names()` / `speciesTable()`；若保留 handle，改为惰性按需生成。
- 验证方法：`grep -rn "handle()" Source/ | grep -v "CollisionSpeciesTable"`（空）；删掉这三处后应仍能编译。

### F4 `InverseBremsstrahlung`：`calculateNuei` 在缺少回调时**静默空转**，随后无检查地解引用 `nuei_*` / `N_*` 场
- 文件:行：`InverseBremsstrahlung.cpp:72`、`:136-144`；`CollisionFactory.cpp:37-43`；`CollisionSpeciesTable.H:69-72`
- 严重度：minor
- 置信度：confirmed
- 现象与依据：
  - `CollisionFactory.cpp:37-42` 只有在 `host != nullptr` 时才注入 `calculate_nuei`，否则留空；`CollisionSpeciesTable::calculateNuei()`（`.H:69-72`）用 `if (m_calculate_nuei)` 把空回调**静默跳过**。
  - 上游此处一定执行 `mypc->CalculateNuei(...)`（`_upstream/.../InverseBremsstrahlung.cpp:62`），它会计算出 `nuei_<species>` / `N_<species>` 两个场；本地若回调为空，`*...fields().get("nuei_"+electrons.getName(), lev)`（`:136-138`、`:142-144`）会拿到 `nullptr` 并被立即解引用 ⇒ 段错误而非可诊断错误。
  - 目前两条入口（`WarpXKernel.cpp:72`、`WarpXEvolve.cpp:755`）都传非空 host，所以是**潜伏**缺陷，但 `if (m_calculate_nuei)` 这种"缺依赖就静默不做"的写法正是本仓库其他位置（`CollisionHandler.cpp:176-220` 的 `if (m_host != nullptr)`）也在用的坏模式。
- 建议修法：`CollisionSpeciesTable::calculateNuei` 改成缺回调时 `WARPX_ABORT_WITH_MESSAGE`；`get()` 之后加 `WARPX_ALWAYS_ASSERT_WITH_MESSAGE(mf != nullptr, ...)`。
- 验证方法：临时把 `CollisionFactory.cpp` 的 host 传 `nullptr` 跑 `inputs_test_1d_collision_z_inverse_Bremsstrahlung`，观察崩溃点（**不要在审计阶段改源码**，可用 gdb 断点 `InverseBremsstrahlung::doCollisions` 观察 `m_species.m_calculate_nuei` 是否为空）。

### F5 `NuclearFusionFunc::executor()` 无检查解引用只在 `AllocData` 里赋值的 `m_data`（以及 `fields().get()` 的返回值）
- 文件:行：`NuclearFusionFunc.H:103-104`（赋值）、`:280-283`（使用）、`:118` 与 `:281`（两处 `const_cast`）
- 严重度：minor
- 置信度：confirmed
- 现象与依据：
  - `m_data` 默认 `nullptr`（`:291`），仅在 `AllocData(RuntimeData)`（`:103`）里设置；`executor()` 在 `m_save_particle_production == true` 时直接 `const_cast<RuntimeData*>(m_data)->fields()`。`CollisionFuncBase::AllocData` 是"可选钩子"（`CollisionFuncBase.H:26-27`，默认空实现），基类契约并未保证一定被调用；`CollisionHandler::AllocData()`（`CollisionHandler.cpp:148-153`）是唯一调用点（经 `BinaryCollision::AllocData`，`BinaryCollision.H:184-186`）。任何"构造 functor 后未走 `CollisionHandler::AllocData`"的路径（单测、将来新增的调用者）= 空指针解引用。
  - `fields().get(...)` 的返回值也未判空（`:282-283`），而在 legacy 路径下该字段是由 `AllocData` 自己 `alloc_init` 出来的（`fields.has()` 为假才分配，`:122`），判空成本为零。
  - 两处 `const_cast` 只是为了绕过 `const RuntimeData&` 接口：上游是 `WarpX& warpx = WarpX::GetInstance();`（非 const），说明接口真正需要的是可写 `RuntimeData`；`const_cast` 把"接口设计错误"藏了起来，`NuclearFusionFunc.H:116-121` 的注释（"Register access is mutable by design"）只是给 `const_cast` 找理由。
- 建议修法：`CollisionFuncBase::AllocData` 的形参改为 `RuntimeData&`（`BinaryCollision` 已有可写引用），或至少在 `executor()` 与 `AllocData` 里 `WARPX_ALWAYS_ASSERT_WITH_MESSAGE(m_data != nullptr, ...)`、对 `fields().get()` 判空。
- 验证方法：`grep -n "m_data" Source/Plugins/Collision/BinaryCollision/NuclearFusion/NuclearFusionFunc.H`；用 `-DWarpX_PYTHON=ON` 里直接构造 `NuclearFusionFunc` 并调 `executor()` 的单测可在 ASAN 下复现。

### F6 `declare_state` 与 `NuclearFusionFunc` 用**不同机制**解析同一个 `save_particle_production`，schema 路径可能被静默跳过
- 文件:行：`CollisionHandler.cpp:131-144`（尤其 `:134` `pp.query("save_particle_production", save_production)` 到 `int`）；`NuclearFusionFunc.H:88-89`（`utils::parser::queryWithParser(..., bool)`）
- 严重度：minor
- 置信度：likely
- 现象与依据：
  - `queryWithParser` 走的是 parser/`queryAsDouble` 再 cast 到 `bool`（`Source/Core/Utils/Parser/ParserUtils.H:100-104` → amrex `ParmParse.H:1563-1586`），而 `declare_state` 用 `ParmParse::query` 读 `int`。凡是非"纯整数"写法（`true`、`1.0`、`2*0.5` 之类）两者会给出不同结论：`NuclearFusionFunc` 认为要存产物，`declare_state` 认为不要 ⇒ `AllocPluginFields`（`WarpX.cpp:2723-2737`）不会在场 schema 里建这个场，注释 `CollisionHandler.cpp:129-130`"declared here so that they are part of the schema（S4）"在那种写法下不成立，场退回到首步由 `AllocData` 迟到创建。
  - 另外 `declare_state` 对**所有** `collision_names` 无差别检查该键（`:131-137`），非 `nuclearfusion` 类型若误设该键也会平白声明一个场；`FieldSpec` 也没填 `unit`（`FieldSpec.H:25-36`）。
- 建议修法：`declare_state` 只在 `collision_types[i] == "nuclearfusion"` 时处理，并统一用 `utils::parser::queryWithParser(pp, "save_particle_production", bool)` + `WARPX_ALWAYS_ASSERT_WITH_MESSAGE(declared == 实际, ...)`。
- 验证方法：把测试 deck 改成 `save_particle_production = true`，跑第一步后用 field 列表诊断（或 gdb 断在 `WarpX::AllocPluginFields`）看场是否在 schema 阶段创建。

### F7 `declare_state` 只在 kernel 路径生效；legacy/Python 路径的 handler 是"首步懒创建"，永不走 schema
- 文件:行：`CollisionHandler.cpp:126-145`；`WarpXKernel.cpp:72,116`；`Evolve/WarpXEvolve.cpp:746-761`
- 严重度：minor
- 置信度：confirmed
- 现象与依据：`CollisionStatePlugin`（`CollisionHandlerAdapter.H:58-78`）只在 `WarpXKernel::register_plugins`（`WarpXKernel.cpp:116`）里注册；legacy 路径下 handler 是 `RunCollisionStage` 第一次被调用时才 `create_collision_handler`（`WarpXEvolve.cpp:755`），那之后再也没有 schema 阶段 ⇒ `<collision>_particle_production` 在 `InitData`/诊断解析场名时**不存在**，只在首步中途由 `AllocData` 创建。同一份 collision 代码因此有两条能力不同的生命周期，`declare_state` 在 Python 路径上是死代码。
- 建议修法：把 handler 的创建提前到 `WarpX::InitData` 之前（或用 `SetCollisionHandler` 的同一处统一注册 `CollisionStatePlugin`），让两条路径一致。
- 验证方法：`grep -rn "CollisionStatePlugin\|create_collision_handler" Source/`；用 Python 驱动跑任意 `nuclearfusion` + `save_particle_production=1` 的 deck，检查 step 0 之前的 field 列表里没有该场。

### F8 头文件/包含卫生类残留（guard 外的 include、重复 include、空 `#ifdef`、被遗留的重载）
- 文件:行：`NuclearFusionFunc.H:1`、`ParticleCreationFunc.H:1`（`#include` 在版权头和 `#ifndef` **之前**，即落在 include guard 之外）；`ParticleCreationFunc.H:19-22`、`InverseBremsstrahlung.H:11-14`、`CollisionHandler.H:14`+`:18`、`:16`+`:20`（完全重复的 include / forward 声明）；`CollisionHandler.cpp:32-33`（`#ifdef WARPX_QED` / `#endif` 空块，上游此处是 `#include "Particles/Collision/BinaryCollision/VirtualPhotonCreation.H"`）；`NuclearFusionFunc.H:290-295`（`private: public: private:`）；`LinearComptonCollisionFunc.H:66-70`（仍保留接收 `MultiParticleContainer const*` 的构造函数，新调用者只走 `:60-64` 的 table 版）
- 严重度：minor（不改变行为，但"guard 外的 include"是真的分层错误：被包含文件自身的 guard 仍生效，所以现在不炸；一旦该头被纳入循环包含就会出问题）
- 置信度：confirmed
- 建议修法：把 line 1 的 include 移到 guard 内并删掉无用者（`NuclearFusionFunc.H` 里 `DefaultInitialization.H` 实际没被用到）；去重；删空 `#ifdef` 块；删除 `LinearComptonCollisionFunc` 的 `MultiParticleContainer` 重载。
- 验证方法：`grep -c '#include' Source/Plugins/Collision/BinaryCollision/ParticleCreationFunc.H`；`clang-format`/`include-what-you-use` 不在 CI 里，靠人工。

## 不确定但值得复核的点
- **本轮基准是否通过**：`audit_build/fork/Testing/Temporary/LastTest.log` 为空（ctest 还没跑），`audit_runs/meta_fork.csv` 里 collision 用例齐全但无结果列。F1/F2 的"结果改变是否可接受"最终应由 `test_3d_collision_pulsed_decay`、`test_1d_collision_z_inverse_Bremsstrahlung` 的实际运行来定。
- **跨分区（不属于本清单，但会影响本分区行为）**：
  - `Source/Core/Runtime/CollisionHandlerAdapter.H:41-44` 有重复的 `if (m_handler == nullptr)`：第一个 `return` 让第二个 `throw`（`:42-44`）成为不可达代码 —— 这条 throw 引入的 `stdexcept` 依赖是纯死代码。
  - `Source/Plugins/Collision/BinaryCollision/BinaryCollisionUtils.cpp:321-333` 保留了接收 `IParticleContainerAccess*` 的旧重载，内部**每次调用都新建一张 `CollisionSpeciesTable`**（即每个 species 一次 `TotalNumberOfParticles` 全局归约）；本分区已全部改用 table 版，这层重载若无其他调用者应一并删除（属 Collision_A 分区）。
  - `SingleNuclearFusionEvent.H:126-130` 写 `particle_production(global_index)`：`global_index` 是全局 cell 索引，`executor()` 里 `particle_production_mf->array(mfi)` 取的是 **FAB**（grid，不是 tile）的 Array4，因此 tiling 下仍在界内 —— 我核过 `MultiFab::array(MFIter)` 的语义，**不构成越界**，仅记录以免重复排查。
- **`declare_state` 声明的场在被 `AllocPluginFields` 分配后，`AllocData` 因 `has()` 为真而跳过**（`NuclearFusionFunc.H:122-125`）：这样 `remake/redistribute_on_remake/checkpoint` 就由核心的 `FieldSpec`（`checkpoint=true`）决定，两边一致；但如果将来 `FieldSpec` 的 `checkpoint` 默认值被人改成 `false`，重启后的产物场会静默丢失 —— 建议保留这一行注释里的不变量。
