# WarpX 插件化 Fork 提交后正确性检查报告

- 审计日期：2026-11-10
- 审计对象：`/workspace` 仓库，HEAD = `80606cbc1afe48d130ad344c272a3712d55bbb7a`
- 提交窗口：过去 24 小时内只有 1 个提交。该提交是根提交（`git rev-list --count HEAD` = 1），包含 2973 个文件、390916 行新增，提交说明为 `docs(s7): state exactly which binary the 573-test run measured`。因此本报告不是对一个小 diff 的审查，而是对该根提交所含 Fork 新增代码及其与原版代码交互的系统性审查。
- 审计方式：静态阅读全部新增/改写文件，沿调用链追踪执行路径，核对接口契约、数据布局、维数分支、边界 ghost 和插件注册路径；未编译、未运行测试，未修改任何项目代码或项目文件。
- 说明：本报告只描述缺陷，不提供代码修改。

---

## 0. 项目目标与判定准则

该 Fork 的目标是把 WARPX 改造成“核心 + 插件”的非官方插件化版本：核心提供运行时、数据总线和接口；各插件彼此独立，只通过接口与总线接入；用户应能自行编写新插件（新场求解器、新材料模型、新诊断等）而不改核心。

据此，本报告按三条准则判定缺陷：

1. 是否破坏“插件只通过接口/总线交互、彼此可替换”的架构目标；
2. 是否在物理上给出错误但又不报错的结果（静默物理错误）；
3. 是否使原版 WARPX 已支持的功能在 Fork 中不完整、退化或不可达。

---

## 1. 关键缺陷总览

| 编号 | 严重度 | 类别 | 位置 | 一句话结论 |
|---|---|---|---|---|
| P0-1 | 严重 | 架构/ABI | `PluginCAdapter.cpp`、`DataBus.cpp`、`PluginCApi.H` | C ABI 的 `warpx_stage_read_field` 实际按“写”权限校验，并让异常穿越 C 边界 |
| P0-2 | 严重 | 架构/系统性 | `HandlerSpec.H`、`KeyMatch.H` | 重复写者检测用字符串全等，而总线/排序用 `key_matches`，聚合名与分量名可绕过冲突检测 |
| P0-3 | 严重 | 架构/项目目标 | `MacroscopicEvolveE.cpp`、`EvolveE.cpp`、`ServiceFactory.cpp` | 场求解器对材料接口做 `dynamic_cast` 到本插件具体类，用户材料插件无法替换 |
| P0-4 | 严重 | 物理/静默 | `MacroscopicProperties.cpp`、`MacroscopicEvolveE.cpp` | 界面法向虽由 ∇φ 计算，但材料采样只在 z 方向进行；x 法向界面完全不生效，倾斜界面取错材料 |
| P1-5 | 高 | 内存/物理/系统性 | `MacroscopicProperties.cpp`、`WarpXPushFieldsEM.cpp` | `Jcond` 分配 0 层 ghost，却被 `i-1/j-1` 散度使用；且 3D 只算 x、z，`rho_medium` 与高斯残差错误 |
| P1-6 | 高 | 物理/功能完整性 | `MacroscopicProperties.cpp`、`WarpX.cpp` | 3D 节点材料构造把 j 当 z、完全忽略 y/k；1D 张量解析器读越界 `dxa[1]`；无维数保护 |
| P1-7 | 高 | 物理/静默 | `SubpixelTensor.H`、`MacroscopicProperties.cpp` | 各向异性 ε 下 `J1 = ε(I-C)ε⁻¹` 与推导 `J1 = I-C` 不符，导致 `Jcond`/电荷守恒错误 |
| P1-8 | 高 | 架构/静默 | `PluginLoader.cpp`、`PluginCApi.H`、示例插件 | 插件加载器不检查 handler 注册是否成功，注册失败的插件仍被当作已加载 |
| P2-9 | 中 | 物理 | `MacroscopicEvolveE.cpp` | 张量 ε 的 D 项用整行，J 项只减对角项，破坏 `ε⁻¹J` 的完整矩阵作用 |
| P2-10 | 中 | 架构 | `DataBus.cpp` | `alias` 在目标名已存在时直接返回，完全绕过访问校验和别名一致性检查 |
| P2-11 | 中 | 内存/边界 | `MacroscopicEvolveE.cpp` | `eold3` 在 Yee 交错不同的组件间 `ParallelCopy`，目标多出的节点平面未初始化 |
| P2-12 | 中 | 功能完整性 | `Python/pywarpx/picmi.py` | Python/PICMI 接口完全没有新增的 ε₂/张量/界面/subpixel/exponential 选项 |

---

## 2. P0 级缺陷

### P0-1 C ABI 的“只读字段”被当成“写字段”，并让异常穿越 C 边界

**位置**

- [PluginCAdapter.cpp](file:///workspace/Source/Core/Runtime/PluginCAdapter.cpp#L293-L301)：`warpx_stage_read_field`
- [DataBus.cpp](file:///workspace/Source/Core/Runtime/DataBus.cpp#L106-L119)：`DataBus::field` 的非 const 重载
- [PluginCApi.H](file:///workspace/Source/Core/Runtime/PluginCApi.H#L137-L150)：接口文档明确要求只读访问声明在 `reads_fields`

**原因与执行路径**

`warpx_stage_read_field` 的实现是：

```
const amrex::MultiFab* field = ctx->bus->field(std::string(name), lev);
```

`ctx->bus` 是非常量指针，因此这里选中的是 `DataBus::field(name, lev)` 的非 const 重载；该重载内部调用 `require_field_access(name, /*write=*/true)`，只检查 `writes_fields`。也就是说：

- 插件按文档只声明 `reads_fields = {"Efield_fp_x"}`，调用 `warpx_stage_read_field` 时会被判为“未声明写”，抛出 `std::runtime_error`；
- `warpx_stage_read_field` 没有 `try/catch`，异常直接穿过 `extern "C"` 边界进入插件代码；如果插件是 C 编译的，这是未定义行为，通常会 `std::terminate`；
- 反过来，如果插件为了让读取成功而把字段声明成 `writes_fields`，它在调度器眼里就成了写者，会与真正的写者在同一 `StagePoint` 产生冲突，或改变读写依赖顺序；
- 该函数还通过 `const_cast` 返回可写的 `MultiFab*`，所以“只读”接口实际上可以写字段，读写语义完全没有被强制。

**对仿真的影响**

这是总线访问控制层的系统性缺陷，不是单个插件的用法问题：

- 只读型插件（诊断、在线分析、与其它代码耦合的 I/O 插件）无法按契约工作；
- 插件作者被迫把所有想读的字段声明成写，导致 P0-2 的重复写者冲突或调度顺序改变；
- 异常穿越 C ABI 会让仿真进程直接终止，且错误信息很难定位到“只是读了一个字段”；
- 由于返回的是可变指针，插件可以绕过 `writes_fields` 直接修改核心字段，破坏“单写者/依赖可排序”的前提。

**为什么现有测试没发现**

`tests/plugins` 与 `tests/external_plugin` 中的示例插件全部只调用 `warpx_stage_field`（写）或只注册空 handler，没有“声明只读后用 `warpx_stage_read_field` 读取”的用例；`tests/runtime` 也没有覆盖该 C ABI 路径。

---

### P0-2 重复写者检测使用字符串全等，而总线使用 `key_matches`

**位置**

- [HandlerSpec.H](file:///workspace/Source/Core/Runtime/HandlerSpec.H#L80-L104)：`PluginHost::add` 中的 `w == ew`
- [KeyMatch.H](file:///workspace/Source/Core/Runtime/KeyMatch.H#L1-L32)：注释明确说明 `key_matches` 是“插件宿主（排序/冲突检测）与数据总线共用的唯一定义”
- [HandlerSpec.H](file:///workspace/Source/Core/Runtime/HandlerSpec.H#L255-L300)：`order_indices` 使用 `key_intersects` 建立写→读依赖

**原因**

`PluginHost::add` 在检测同一 `StagePoint` 的重复写者时，用的是：

```
if (w == ew) { throw ... }
```

而 `DataBus::require_field_access` 和 `order_indices` 用的是 `key_matches` / `key_intersects`。`KeyMatch.H` 的注释已经明确指出：如果冲突检测使用字符串全等，那么 `Efield_fp` 与 `Efield_fp_x` 这类“聚合名 vs 分量名”的冲突不会被发现。

实际后果是：

- 插件 A 声明写 `Efield_fp`，插件 B 声明写 `Efield_fp_x`，两者在同一 `StagePoint`；
- `DataBus` 认为两者都有写权限，均放行；
- `add` 的冲突检测因为字符串不相等而不报错；
- `order_indices` 只建立“写者→读者”的边，不建立“写者→写者”的边，因此两个写者的执行顺序完全由 `priority`、注册顺序和 `stable_sort` 决定；
- 两个写者都会写同一块 E 分量，最终结果取决于谁最后执行。换一个插件加载顺序或换一个构建，结果可能不同。

**对仿真的影响**

这是典型的系统性静默错误：不是崩溃，而是同一份输入在不同插件顺序下给出不同电磁场。对于“多插件协作”的核心使用场景，一个插件可能把另一个插件的物理更新覆盖掉，且没有任何警告。该缺陷与 P0-1 叠加时尤其危险：只读插件会被迫声明写，从而制造出本不存在的写者冲突/顺序依赖。

**为什么现有测试没发现**

没有测试在同一 `StagePoint` 注册“聚合名写者 + 分量名写者”。现有内置 handler 的命名风格基本一致，掩盖了该问题。

---

### P0-3 场求解器把材料接口 `dynamic_cast` 到本插件具体类，用户材料插件不可替换

**位置**

- [MacroscopicEvolveE.cpp](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicEvolveE.cpp#L50-L60)
- [MacroscopicEvolveE.cpp](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicEvolveE.cpp#L123-L132)
- [EvolveE.cpp](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/EvolveE.cpp#L270-L282)
- [ServiceFactory.cpp](file:///workspace/Source/Core/Runtime/ServiceFactory.cpp#L149-L153)：材料工厂是单一全局槽位，后注册者直接覆盖，无冲突检查

**原因**

核心定义了 `IMacroscopicProperties` 接口，但内置有限差分求解器在运行时执行：

```
auto* macro = dynamic_cast<MacroscopicProperties*>(&macroscopic_properties_contract);
```

如果转换失败，就直接报错退出；随后求解器只通过具体类 `MacroscopicProperties` 的 getter 访问 `m_eps_node`、`m_sigma_node`、`m_eps_inv_node` 等实现细节。这些 getter 并不在 `IMacroscopicProperties` 接口中。

这意味着：

- 按接口实现一个第三方材料插件，注册到 `register_macroscopic_properties_factory` 后，内置场求解器会拒绝它；
- 工厂是单槽位，后注册者静默覆盖先注册者；如果用户的材料插件覆盖了内置工厂，场求解器会在运行时 `dynamic_cast` 失败并终止；
- “场求解器插件”和“材料模型插件”实际上仍编译期/实现级耦合，没有做到只通过接口交互。

**对仿真的影响**

这直接违背项目的根本目标：用户无法在不修改场求解器插件的情况下替换材料模型。对于一个以“用户自写功能插件”为核心卖点的 Fork，这是架构级缺陷。即使第三方材料模型的物理完全正确，仿真也无法启动，或者内置求解器会绕过它使用自己的具体实现。

**为什么现有测试没发现**

测试只使用内置的 `MacroscopicProperties`，没有“注册一个替代 `IMacroscopicProperties` 实现并运行场求解”的用例。

---

### P0-4 界面法向由 ∇φ 计算，但材料采样只在 z 方向进行：x 法向界面完全不生效

**位置**

- [MacroscopicProperties.cpp](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicProperties/MacroscopicProperties.cpp#L495-L500)：`normal = (c == 2)`，把 z 硬编码为法向
- [MacroscopicProperties.cpp](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicProperties/MacroscopicProperties.cpp#L520-L560)：所有分量都取 `iL = iR = ii`，只变化 `j`（在 2D 中 j 是 z）；`m1L/m1R` 只探测 `z ± dz/4`
- [MacroscopicProperties.cpp](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicProperties/MacroscopicProperties.cpp#L599-L617)：`eL/eR` 取自 z 相邻单元，再送入 `chi_inv_anisotropic`
- [MacroscopicProperties.cpp](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicProperties/MacroscopicProperties.cpp#L1018-L1055)：`fill_rows` 的逐单元实现按 x-z 亚单元采样，本身能处理 x 分量；但它不是默认路径
- [MacroscopicEvolveE.cpp](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicEvolveE.cpp#L213-L221)：默认 `use_node_eps = true`，即默认使用上述节点材料，而不是 `fill_rows` 的逐单元 Kottke 行；因此 `fill_rows` 的正确性无法弥补默认路径

**原因**

代码确实用 φ 的 x/z 差分计算了界面法向 `nrm`，也用 `frame(nrm)` 做了旋转，看起来支持任意法向。但决定“界面两侧是什么材料”的采样仍然是：

- 侧别判定只看 `phi(xn, 0, zn ± dz/4)`；
- 两个材料样本只取 z 相邻的单元 `(ii, j-1, k)` 与 `(ii, j, k)`；
- `iL = iR = ii`，x 方向的两个相邻单元从未参与材料采样。

因此：

- 对于 `interface_function = -x`（x 法向界面），`zn ± dz/4` 两个探测点具有相同的 x 坐标，落在同一材料；两个材料样本也来自同一 x 单元。于是 `e1 == e2`，`chi_inv_anisotropic` 返回体材料张量，界面平滑完全失效。对 Ex（x 法向分量）而言，ε 还是单侧取值，而不是界面两侧的平均。
- 对于倾斜界面，虽然 `nrm` 有 x 分量，但两个材料样本仍沿 z 取，样本点不在法向两侧；倾斜角越大，取到的材料越不对。极端情况下（接近 x 法向）退化为 P0-4 的第一种情形。
- 对 y 方向变化（3D）则完全不可见，见 P1-6。

**对仿真的影响**

这是静默的物理错误：用户按文档提供 `interface_function`，期望任意法向界面都得到亚像素修正；实际只有 z 法向界面被正确模型化。x 法向界面会退化为“没有界面平滑”，倾斜界面会使用错误的两种材料，导致反射/透射系数错误、界面处出现非物理的场跳变或数值反射。由于代码不报错、也不打印警告，用户很难发现。

**为什么现有测试没发现**

`benchmarks/cases/c03_medium` 的界面用例全部是 z 法向（`interface_function = -z`）；文档中倾斜界面用例 ε1 = ε2，本身是平凡情形，无法暴露材料取样错误。没有“x 法向 + ε 对比”或“大倾角 + ε 对比”的基准。

---

## 3. P1 级缺陷

### P1-5 `Jcond` 0 层 ghost 却用 `i-1/j-1` 散度；3D 只算 x、z

**位置**

- [MacroscopicProperties.cpp](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicProperties/MacroscopicProperties.cpp#L315-L341)：`m_jcond[c]` 以 `nGrow = 0` 分配；同一段注释却写明“Need >= 1 ghost cell so that i-1 / j-1 reads at the domain boundary are in-bounds”
- [WarpXPushFieldsEM.cpp](file:///workspace/Source/Plugins/PicEm/Modules/FieldSolver/WarpXPushFieldsEM.cpp#L1296-L1311)：种子 `rho_medium` 初始化只计算 x、z 两个方向，且把 `j` 当作 z
- [WarpXPushFieldsEM.cpp](file:///workspace/Source/Plugins/PicEm/Modules/FieldSolver/WarpXPushFieldsEM.cpp#L1352-L1368)：`Jcx/Jcz` 的 `FillBoundary` 后，用 `jx(i-1,j,k)`、`jz(i,j-1,k)` 更新 `rho_medium`

**原因**

1. `m_jcond[0]`、`m_jcond[2]` 分别定义在 Ex、Ez 的 Yee 交错布局上，`nGrow = 0`。`FillBoundary` 对 0 层 ghost 是空操作，无法交换任何邻居数据。
2. 更新循环遍历的是 `rho_medium` 的 box，`rho_medium` 是全节点布局，而 `Jcx/Jcz` 是部分方向 cell-centered。`const_array(mfi)` 用同一个 `mfi.index()` 取到的是不同 BoxArray 的 Fab；在高节点面（cell-centered 方向）读取本身就超出 `Jcond` 的 valid box。
3. 周期边界时 `bnd_shrink = 0`，循环包含低/高边界节点：`i=lo` 时读 `jx(lo-1)`，`j=hi+1` 时读 `jx(hi+1)`，对 0 ghost 的 Fab 都是越界读。CPU tiling 或多 box 分解时，每个 tile/box 的边界也会越界。
4. 3D 下散度只写 `(Jx(i)-Jx(i-1))/dx + (Jz(i,j)-Jz(i,j-1))/dz`，完全没有 y 方向项，因为 `j` 被当成 z；`Jcy` 也没有被读取。

**对仿真的影响**

- 内存安全：周期/tile/多 box 边界存在越界读，GPU 上可能直接访问非法地址；CPU 上可能读到垃圾并静默污染结果。
- 物理：`rho_medium` 与高斯定律残差不正确，3D 下缺失 y 方向电流贡献，电荷守恒诊断失效。由于 `rho_medium` 当前不直接参与电磁场推进，场结果可能看起来正常，而电荷守恒已经错了，属于静默错误。
- 该问题跨两个文件：分配在 `MacroscopicProperties.cpp`，使用在 `WarpXPushFieldsEM.cpp`；单独看任何一个文件都不容易发现。

**为什么现有测试没发现**

现有 sigma 基准是 2D、非周期或使用 shrunken box，且没有检查 tile/周期边界上的 `Jcond`；没有 3D 宏观介质基准。

---

### P1-6 3D 节点材料构造把 j 当 z，1D 张量解析器读越界 `dxa[1]`

**位置**

- [MacroscopicProperties.cpp](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicProperties/MacroscopicProperties.cpp#L430-L437)：张量解析器按 `(x, 0, z)` 求值，`z = (j+0.5)*dxa[1] + loz`
- [MacroscopicProperties.cpp](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicProperties/MacroscopicProperties.cpp#L486-L531)：`zn = zlo_n + j*dz_n`，但 3D 中 j 是 y，k 是 z
- [MacroscopicProperties.cpp](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicProperties/MacroscopicProperties.cpp#L1018-L1055)：`fill_rows` 同样把 `j` 当 z，调用 `phi(xc, 0, zc)`
- [WarpX.cpp](file:///workspace/Source/Plugins/PicEm/WarpX.cpp#L621-L628)：只校验了 `macroscopic_subpixel` 需要 Yee，没有校验 3D
- [MacroscopicEvolveE.cpp](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicEvolveE.cpp#L61-L65)：只对 RZ 直接 abort，3D 不在保护范围

**原因**

新增的材料节点逻辑是严格按 2D-XZ 写的：第二个索引 `j` 被当作 z。到了 3D：

- `j` 是 y 索引，`k` 才是 z；`zn` 使用 `j*dz`，坐标完全错误；
- 所有 x/z 界面判定、`normal = (c == 2)`、z 邻居平均都作用在 y 上；
- `fill_one` 里 `pex(x, 0, z)` 直接丢掉 y，`z` 还用了 `dxa[1]`（3D 中是 dy）；
- 1D 构建中 `AMREX_SPACEDIM == 1`，`dxa[1]` 越界读（`GpuArray<Real,1>`），`loz` 也退化为 x。

更严重的是，这不是只影响新张量功能：默认 `use_node_eps = true`，且 `use_node_eps || !has_sub` 在未开启 subpixel 时也为真，所以 **3D 的原版标量宏观介质求解器也会走这套节点 ε 路径**。对均匀介质它碰巧正确；对 `epsilon_function(x,y,z)`、x/y 法向界面或任意 3D 材料分布，节点 ε/σ 会用错邻居和错法向，场更新静默失真。

**对仿真的影响**

- 3D 宏观介质：材料分布被错误地按 y 方向平均、按 z 方向复制，界面法向错位；原版 WARPX 支持的 3D 宏观介质功能在 Fork 中没有被完整实现。
- 1D 张量：`dxa[1]` 越界是内存未定义行为；即使不崩溃，z 坐标也是错的。
- 代码不 abort、不警告；文档里“3D 推广待做”只是说明，并没有在代码里阻止 3D 用户运行。

**为什么现有测试没发现**

新增的介质基准全部是 2D；没有 3D 宏观介质或 1D 张量基准。`p1_tensor_test` 只验证矩阵数学，不覆盖 `fill_one`/节点几何。

---

### P1-7 各向异性 ε 下 `J1` 与推导式不一致

**位置**

- [SubpixelTensor.H](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicProperties/SubpixelTensor.H#L298-L310)：推导写明 `⟨J_c⟩ = σ C Eⁿ + (I - C) f`，即 `J1 = I - C`
- [MacroscopicProperties.cpp](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicProperties/MacroscopicProperties.cpp#L910-L921)：代码实现为 `J1 = ε (I - C) ε⁻¹`
- [MacroscopicEvolveE.cpp](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicEvolveE.cpp#L359-L368)：`Jcond = J0·E_old + J1·rhs`

**原因**

从 ODE `dE/dt = -A E + ε⁻¹ f` 出发，步平均传导电流严格为：

```
⟨J_c⟩ = σ C Eⁿ + (I - C) f
```

其中 `C = ε⁻¹ᐟ² [(1 - e^{-BΔt})/(BΔt)] ε¹ᐟ²`。代码却把第二项算成 `ε (I - C) ε⁻¹ f`。对 ε 为标量或与 C 可交换的对角情形，两者相同；对旋转/非对角张量 ε，两者不同。

**对仿真的影响**

`Jcond` 被用于 `rho_medium` 的电荷更新。各向异性介质中，错误的 `J1` 会破坏 `∂ρ/∂t + ∇·J = 0` 的离散一致性，使高斯定律残差随步数漂移；由于 `rho_medium` 不直接反馈到 E/B，场看起来可能仍然“正常”，属于静默电荷守恒错误。

**为什么现有测试没发现**

没有“非对角 ε + 非零 σ”的基准；`p1_tensor_test` 只测矩阵函数本身，没有测步平均电流与电荷守恒。

---

### P1-8 插件加载器忽略 handler 注册失败，仍把插件当作已加载

**位置**

- [PluginLoader.cpp](file:///workspace/Source/Core/Runtime/PluginLoader.cpp#L56-L70)：调用 `desc->register_handlers(...)` 后没有检查任何返回值，随后无条件 `host.add_plugin(...)`
- [PluginCApi.H](file:///workspace/Source/Core/Runtime/PluginCApi.H#L98-L112)：`warpx_host_add_handler` 明确返回 0/非 0 表示成功/失败
- [example_plugin.c](file:///workspace/tests/plugins/example_plugin.c#L34)：官方示例直接 `(void)` 丢弃返回值；`dynamic_example_plugin.c`、`third_party_plugin.c` 同样如此

**原因**

C ABI 允许 `warpx_host_add_handler` 因重复写者、非法 stage、空 `apply` 等原因拒绝注册，失败码返回给插件。但：

- 加载器不知道插件内部注册了几个 handler、成功了几个；
- 官方示例教用户忽略返回值；
- 只要插件描述符本身合法，`host.add_plugin` 就会成功，后续 `validate_plugins` 只检查插件 manifest 的依赖/角色，不检查 handler 是否真的注册上。

**对仿真的影响**

一个插件可能“加载成功、manifest 校验通过、日志显示已加载”，但它的所有 handler 都被拒绝，仿真中完全没有该插件的物理/诊断。与 P0-2 结合时，重复写者冲突会被静默吞掉，而不是启动失败。这是插件化架构中最难排查的一类静默失效。

**为什么现有测试没发现**

没有“注册必然失败”的负向测试；示例插件全都不检查返回值，等于把错误路径固化成推荐用法。

---

## 4. P2 级缺陷与风险

### P2-9 张量 ε 的 D 项用整行，J 项只减对角项

**位置**

- [MacroscopicEvolveE.cpp](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicEvolveE.cpp#L277-L284)：`D = ε·E` 使用完整行 `ix0*Dx + ix1*Dy + ix2*Dz`
- [MacroscopicEvolveE.cpp](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicEvolveE.cpp#L282-L284)、[L314-L320](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicEvolveE.cpp#L314-L320)、[L351-L357](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicEvolveE.cpp#L351-L357)：J 项只减 `-dt*ix0*jx`、`-dt*iy1*jy`、`-dt*iz2*jz`

**原因**

各向异性介质的 Maxwell 方程为 `∂E/∂t = ε⁻¹(∇×H - J)`，`ε⁻¹J` 必须用完整矩阵行。代码对 D 用了完整行，对 J 只用对角项，前后不一致；指数路径的 `M2·(curlH - J)` 反而用了完整矩阵，因此真空分支与导电分支也不一致。

**对仿真的影响**

非对角 ε 介质中，沿 y/z 的电流不会通过 `ε⁻¹` 的 xy/xz 分量驱动 Ex，产生非物理的场响应；与指数路径相比结果不一致。代码注释把它称为“设计选择”，但它违反了本构关系，属于物理正确性缺陷。

---

### P2-10 `DataBus::alias` 在目标已存在时绕过访问校验

**位置**

- [DataBus.cpp](file:///workspace/Source/Core/Runtime/DataBus.cpp#L137-L147)

**原因**

`alias(name, source, ...)` 一进入就查 `m_fields`；如果目标名已存在，直接 `return it->second.get()`：

- 不调用 `require_field_access`；
- 不检查目标是否真的 alias 到给定 source；
- 不检查 alias 的布局/ghost 是否兼容。

**对仿真的影响**

任何 C++ 插件（内置插件都走 C++ 总线）可以在未声明 `writes_fields` 的情况下，通过一个已存在的目标名拿到可写 `MultiFab*`，绕过单写者/依赖排序。它可以覆盖核心或其他插件的字段，且调度器不知道这次写，导致数据竞争或结果依赖执行顺序。C ABI 目前没有暴露 alias，但核心 API 的契约已被破坏。

---

### P2-11 `eold3` 跨 Yee 交错 `ParallelCopy`，高节点平面未初始化

**位置**

- [MacroscopicEvolveE.cpp](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicEvolveE.cpp#L380-L406)

**原因**

`eold3[c]` 用 `Efield[c]` 的 BoxArray 定义，然后对每个 `d` 执行 `eold3[c].ParallelCopy(*Efield[d], ...)`。三个 E 分量的 Yee 交错不同：

- Ex 在 x 方向多一层节点；
- Ey 在 y 方向多一层（3D）；
- Ez 在 z 方向多一层。

源组件的 index space 不覆盖目标多出的节点平面，因此 `ParallelCopy` 只覆盖交集，目标多出的平面保持未初始化；在带断言的 AMReX 构建中，还可能因 BoxArray 不一致直接断言。指数更新循环遍历的是 `Efield[c]` 的 valid box，会读到这些未初始化值。对于各向异性 M1（非对角），Ex/Ez 会混入垃圾旧场。

**对仿真的影响**

边界受限：主要影响高节点面；周期情形可能被后续 `FillBoundaryAndSync` 覆盖，非周期 PEC 的法向分量（如 x 边界的 Ex）可能保留错误值。属于低概率但真实的内存/边界风险；在各向异性 σ 介质中更明显。

---

### P2-12 Python/PICMI 接口完全没有新增介质选项

**位置**

- [picmi.py](file:///workspace/Python/pywarpx/picmi.py#L3520-L3560)：`MacroscopicProperty` 只允许 `sigma/epsilon/mu`，且算法只列 `backwardeuler/laxwendroff`
- [picmi.py](file:///workspace/Python/pywarpx/picmi.py#L3609-L3630)：只写 `macroscopic_<name>_function` 或 `macroscopic_<name>`
- [picmi.py](file:///workspace/Python/pywarpx/picmi.py#L4100-L4102)：只设置 `algo.em_solver_medium = "macroscopic"`

**原因**

新增的 `epsilon2`、`epsilon_tensor_*`、`sigma_tensor_*`、`interface_function`、`algo.macroscopic_subpixel`、`algo.macroscopic_solver=exponential` 只存在于 C++ 的 ParmParse 解析中，没有进入 `Python/pywarpx` / PICMI 层，也没有对应的文档示例。

**对仿真的影响**

WARPX 原版以 Python/PICMI 作为一等输入接口；Fork 的新介质插件无法通过该接口使用，用户只能在 C++ 文本输入中手写参数。这与“用户可自行组合/扩展插件”的目标不一致，也属于原版功能完整性缺口。这个问题不会让已有输入出错，但会让新功能在官方 Python 工作流中不可达。

---

## 5. 对三个关注问题的直接回答

### 5.1 是否违背项目的根本目标

是。主要证据：

- P0-3：场求解器对材料接口 `dynamic_cast` 到本插件具体类，第三方材料插件不可替换；
- P0-1：C ABI 的只读访问实际要求写声明，只读插件无法按契约接入总线；
- P0-2：重复写者检测与总线使用不同的名称匹配规则，多插件写同一字段时顺序不确定；
- P1-8：handler 注册失败不被加载器发现，插件可以“显示已加载但完全不执行”；
- P2-10：`alias` 可以绕过访问声明，破坏总线的单写者和依赖模型。

这些都不是“某个插件写错了”，而是核心/总线/加载器层面的系统性问题。

### 5.2 是否违背物理理论

是，且多数是静默的：

- P0-4：界面法向与材料采样不一致，x 法向界面没有亚像素修正，倾斜界面取错材料；
- P1-5：`Jcond` 的 ghost/散度错误，3D `rho_medium` 缺 y 项，电荷守恒与高斯残差错误；
- P1-6：3D 材料构造把 y 当 z，原版 3D 宏观介质被静默退化；
- P1-7：各向异性 ε 下 `J1` 公式与推导不符，步平均传导电流错误；
- P2-9：张量 ε 的 J 项只取对角，违反 `ε⁻¹J`；
- P2-11：各向异性指数更新在高节点面可能混入未初始化旧场。

### 5.3 是否没有完全实现原版支持的功能

是：

- 原版 WARPX 的 3D 宏观介质在 Fork 中走的是 2D 节点逻辑，且没有 3D 保护；
- 原版通过 Python/PICMI 输入介质属性的工作流没有覆盖新增功能；
- 新张量/界面/subpixel 功能没有 3D 实现，也没有在 3D 输入时拒绝运行；
- 新增介质基准全部是 2D z 法向、对角张量，未覆盖原版 3D、倾斜界面、非对角张量和张量电导率。

---

## 6. 测试覆盖与“为什么代码审查遗漏”

对新增介质功能和插件运行时，现有测试的覆盖情况是：

- `benchmarks/cases/c03_medium` 的界面/电导率基准全部为 2D、z 法向、对角 ε/σ；
- 没有 3D 宏观介质基准；
- 没有 x 法向或大倾角界面 + ε 对比的基准；
- 没有非对角 ε + 非零 σ 的基准；
- 没有张量 σ 基准；
- `tests/plugins` 的示例插件只写字段，不读取字段，也不检查 `warpx_host_add_handler` 返回值；
- 没有“第三方材料模型替换内置模型”的测试；
- 没有周期/tile 边界上的 `rho_medium` 高斯残差检查。

因此，P0-1、P0-2、P0-3、P0-4、P1-5、P1-6、P1-7 都不会被现有 573 个测试触发。代码审查如果只看单个文件，也很难发现 P0-2（`HandlerSpec.H` 与 `KeyMatch.H` 跨文件契约）、P1-5（分配与使用分属两个文件）、P1-7（实现与同文件推导式不一致）这类系统性问题。

---

## 7. 结论

过去 24 小时的提交窗口内只有 1 个根提交（`80606cb`），其内容包含整个 Fork。对该提交所含代码的系统性审查确认了多项关键缺陷，其中：

- 4 项 P0：C ABI 读写契约错误、插件冲突检测规则不一致、材料接口不可替换、界面材料采样与法向不一致；
- 4 项 P1：`Jcond` ghost/3D 散度错误、3D/1D 材料几何错误、`J1` 公式错误、插件注册失败被静默吞掉；
- 4 项 P2：张量 J 项不完整、`alias` 绕过访问校验、`eold3` 边界未初始化、Python 接口缺失。

以上问题均只在本报告中描述，未对任何代码或项目文件进行修改。
