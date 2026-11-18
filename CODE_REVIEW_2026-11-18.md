# 提交后正确性检查报告（2026-11-18）

**审查对象**：WarpX 插件化非官方 fork（核心 + 独立插件，插件经 DataBus/ABI 接入）。
**审查范围**：当前仓库快照。仓库历史为单次压缩提交（`2026-11-17 23:39`），因此“过去 24 小时内的提交”等价于该快照的全部内容；本报告重点关注插件物理内核、核心-插件总线契约与最近修复标注（`F-xx`）所在路径。
**审查方法**：沿每个可疑点追踪完整执行路径（插件注册 → 调度器排序 → DataBus 字段读写 → 物理内核），并用物理方程/量纲/离散一致性交叉验证；仅做只读审查，未修改任何代码或项目文件。
**判定标准**：物理正确性优先；其次为插件化/模块化的接口契约完整性；特别关注不报错、不崩溃、日志看似正常的“静默型”物理错误。

---

## 0. 结论摘要

| 编号 | 严重度 | 主题 | 静默性 |
|---|---|---|---|
| F-01 | **Critical** | 流体插件读取 `star_g{x,y,z}` 后直接丢弃，运行期恒星模拟实际没有自引力 | 静默（日志反而打印 `g: yes`） |
| F-02 | **High** | 引力源 `star_rho` 只在初始化时由静态太阳模型填充，从不随演化密度更新 | 静默 |
| F-03 | **High** | 全周期几何 + `open` 引力边界：Poisson 系统奇异且未做零均值扣除；周期 ghost 被单极子值污染 | 静默（MLMG 仅可能打印收敛信息） |
| F-04 | **High** | 张量电导只有对角元参与“是否求解”的门控；纯 Hall/非对角电导被整体忽略，且张量指数按行部分应用导致耦合不一致 | 静默 |
| F-05 | Medium | `subpixel` 的 3D 界面法向只取 x-z 分量，材料采样只沿 z；`mu` 完全不做亚网格平均，与用户文档承诺不符 | 静默 |
| F-06 | Medium | Dirichlet ghost 用“ghost 单元中心”的势，而 AMReX 把它当作“面”值使用，与其自身头文件契约相反 | 静默 |
| F-07 | Medium | 默认 `macroscopic_subpixel=0` 且 `sigma>0` 时，`rho_medium` 永不推进，Gauss 诊断/输出冻结 | 静默（仅警告 BE 回退，不警告电荷未更新） |
| F-08 | Medium | 节点电导采用“真空优先”置零规则，与文档写的算术/调和平均不一致；单层导体完全透明 | 静默 |

> 说明：仓库内已有一份 `CODE_REVIEW_2026-11-16.md` 与 `Docs/audit_2026_11/` 审计。本报告只列**尚未被该审计覆盖、或在修复后仍然残留**的问题；文末单列一节更正该审计中的一处误判，避免后续按错误建议“修复”而引入真实越界。

---

## F-01（Critical）恒星流体插件读取引力后丢弃，运行期没有自引力

**位置**
- `Source/Plugins/FluidMoments/StarFluidPlugin.cpp:213-231`：从总线读取 `star_gx/star_gy/star_gz` 到 `gx/gy/gz`，随后 `amrex::ignore_unused(gx, gy, gz);`。
- 同文件 `:349-351`：`kReads = {"star_gx","star_gy","star_gz",...}`，`kWrites = {"star_state"}`。
- 同文件 `:336-340`：每 50 步打印 `g: %s`，`have_g ? "yes" : "no"`。
- `Source/Plugins/FluidMoments/FluidMoments.H:428` 声明、`FluidMoments.cpp:124` 实现的 `applyGravitySource(...)`；全仓库仅 `test_jeans.cpp`、`test_hydrostatic.cpp`、`test_solar_column.cpp`、`test_stellar_model.cpp`、`test_angular_momentum.cpp` 调用它，生产路径 `StarFluidPlugin.cpp` **没有任何调用**。
- 引力侧确实在产生非零加速度：`Source/Plugins/NewtonGravity/StarGravityPlugin.cpp:150-158` 求解并 `publish_acceleration(...)`，`:376` 声明 `kWrites={"star_phi","star_gx","star_gy","star_gz"}`。

**产生原因**
调度器按字段依赖把 `star_gravity`（写 `star_g*`）排在 `star_fluid`（读 `star_g*`）之前，总线契约完全成立；流体插件也确实把数据拷进了本地 `gx/gy/gz`。但随后从未把它们组装成 `g_ptrs` 并调用 `applyGravitySource`，于是整条引力链在此处被截断。`have_g` 只表示“总线给了字段”，却被日志当成“重力已生效”的证据。

**对仿真的影响**
- 运行期恒星/太阳模型只受压力梯度驱动，**静水平衡被破坏**：初始太阳模型会向外膨胀/塌缩，而不是在自引力下振荡或平衡。
- 所有以“自引力恒星”为前提的结论（脉动、Mach 数、能量守恒检查）都不成立；这些量仍会打印出看似合理的数值。
- 单元测试全部通过（它们直接调用 `applyGravitySource`），形成“测试绿、集成错”的系统性盲区。

**为何静默**
无异常、无警告；`[fluid] ... g: yes` 反而给出错误的安全感。该问题与审计 `Source_Plugins_Star.md` 的 F2 同源，但当时的修复只补上了“发布非零 g”，没有补上“消费 g”，故仍属未修复。

---

## F-02（High）引力源 `star_rho` 冻结，流体密度永不反馈

**位置**
- `Source/Plugins/NewtonGravity/StarGravityPlugin.cpp:136`：每步只读 `star_rho`。
- 同文件 `:263-322`、`:345-370`：`fill_sphere` / `on_init` 用静态太阳模型剖面写 `star_rho`；`:129-132` 的 `fill_all_levels` 也只执行一次。
- `Source/Plugins/FluidMoments/StarFluidPlugin.cpp:351`：流体只写 `star_state`，不写 `star_rho`。
- `Examples/Tests/boundaries/inputs_3d_solar_amr:24-26`：`amr.dense.field_name = star_rho`，即 AMR 加密也基于该静态场。

**产生原因**
两个插件按“四个字段名”解耦（这一架构本身正确），但 `star_state` 的密度分量与 `star_rho` 之间没有桥接：既没有流体侧发布，也没有核心侧的 alias。`StarGravityPlugin.cpp:352` 的注释暗示可能存在“从文件加载 star_rho 的插件”，但本仓库没有任何插件把演化后的密度写回 `star_rho`。

**对仿真的影响**
即使在 F-01 修复后，Poisson 方程仍用初始密度求解：恒星一旦变形/脉动，引力场不响应，属于未经声明的 Cowling 近似；AMR 会持续加密初始轮廓而不是当前物质分布。对非稳态恒星，这会系统性低估/错估恢复力与增长率。

**为何静默**
`star_rho` 始终是“合理”的太阳模型数值，`star_phi`/`star_phi_min` 也正常输出；没有任何一致性检查比较 `star_state(0)` 与 `star_rho`。若确实只支持 Cowling 近似，应在参数文档与日志中显式声明，并提供/要求一个 `star_rho` 发布者；当前两者都没有。

**附加风险（需运行时验证，未计入上表）**
`StarGravityPlugin.cpp:129-132` 的 `fill_all_levels` 只执行一次，且该插件未注册 `on_regrid` 回调（`register_handlers` 只注册了一个 `apply` handler）。对启用 AMR 的算例，regrid 后新细网格 patch 上的 `star_rho` 是否被核心从旧布局迁移/插值决定了对 `rho` 的读取是否为 0 或旧值；若未被填充，细层 Poisson 解将使用错误源项。建议在 regrid 后重新检查/填充 `star_rho`，或明确依赖平台的字段迁移语义。

---

## F-03（High）全周期几何下的 “open” 引力：奇异线性系统 + ghost 污染

**位置**
- `Source/Plugins/NewtonGravity/StarGravityPlugin.cpp:150-152`、`:184-186`：始终传 `PotentialBoundary::open`。
- `Source/Plugins/NewtonGravity/NewtonGravity.cpp:50-53`：零均值（Jeans swindle）只在 `boundary == PotentialBoundary::periodic` 时执行。
- 同文件 `:82-87`：每个方向的 BC 由 `geom.isPeriodic(d)` 决定；全周期几何下三个方向都会变成 `Periodic`。
- 同文件 `:60-63`：`phi.FillBoundary` 也只在 `boundary == periodic` 时执行。
- `StarGravityPlugin.cpp:77-103`：`fill_dirichlet_ghosts` 把 **所有** ghost（包括周期方向的 ghost）写成 `-G M / r`。
- 触发例：`Examples/Tests/boundaries/inputs_3d_solar_amr:14-15` 六个边界全为 `periodic`。

**产生原因**
1. 调用者请求 `open`，但几何全周期 ⇒ 实际算子是全周期 Laplacian。该算子的零空间是常数，相容性要求源项零均值；零均值扣除却被 `boundary == periodic` 这个**请求值**（而不是**有效 BC**）门控，于是带非零均值的源被直接送入奇异系统。
2. `fill_dirichlet_ghosts` 无条件覆盖周期方向的 ghost，把周期镜像值换成了域外单极子势；而 `open` 分支解完后又不重填 `phi` 的周期 ghost，`computeAcceleration` 在最外层单元的中心差分就用到了这个错误 ghost。

**对仿真的影响**
- 奇异系统没有精确解；MLMG 通常无法把残差降到 `1e-13` 容差（且代码没有任何强制保护），势只能确定到一个任意常数，迭代行为不可保证，`star_phi_min` 失去物理意义。
- 周期边界最外层单元上的 `g` 使用的是非周期 ghost 值，边界层加速度错误；若恒星靠近盒边界，误差会传入内部。
- 全周期盒中孤立星体的正确做法是 Jeans swindle（`periodic` 模式），当前“请求 open 却被静默改判为 periodic 且不做零均值”是自相矛盾的组合。

**为何静默**
除非把 MLMG 的 verbose 调到足够高，否则不 abort、不 warn；势场仍然“有值”，加速度在星体附近甚至可能近似正确，问题只出现在边界层和诊断量上。

---

## F-04（High）张量电导的门控只看对角元，非对角（Hall）电导被静默忽略

**位置**
- `Source/Plugins/FieldSolver/FiniteDifference/MacroscopicEvolveE.cpp:184`：`has_sig_mat = has_sigma_matrices()`（只要存在非零 sigma 就为真，与是否为对角无关）。
- 同文件 `:249-251`（Ex）、`:304-306`（Ey）、`:348-350`（Ez）：Backward-Euler 分支的“让位”判据是 **对角** 节点电导 `sn0(i,j,k,0)` / `sn1(i,j,k,1)` / `sn2(i,j,k,2)`；为 0 时走 BE，并以 `sigma_interp = 0` 计算（`:261-263`、`:316-318`、`:360-362`）。
- 同文件 `:451`：指数分支对每个分量使用 `if (sn(i,j,k,c) <= 0) return;`，同样只看对角元。
- `Source/Plugins/FieldSolver/FiniteDifference/MacroscopicProperties/MacroscopicProperties.cpp:980-990`：只要 `m_has_sig_tensor` 就为所有单元构造完整 3×3 的 `S`；`:1041-1076` 又为所有节点构造完整 `M1/M2/J0/J1`。也就是说，非对角项被认真地算出来了，却在更新时被跳过。

**产生原因**
“是否进入指数步/是否让位”的判据是逐分量的对角元，而指数的正确性依赖整个 3×3 矩阵。对纯非对角张量（例如磁化等离子体的 Hall 电导 `σ_xy ≠ 0, σ_xx = σ_yy = 0`），三个分量的判据全为 0：BE 用 `sigma = 0`，指数步直接 `return` ⇒ 电导完全无效。对一般张量（部分对角元为 0），部分行用完整 `M1/M2` 更新、部分行保留/仅 BE，等于对同一个耦合 ODE 只应用了矩阵的一部分行，结果既不是指数解也不是 BE 解。

**对仿真的影响**
- 磁化/回旋介质、Hall 效应、磁光材料、以及界面处由 `InitData` 特意填充的 normal–tangential 耦合项都会被静默丢弃。
- 由于界面节点的法向调和平均在“导体/真空”时可以为 0，而耦合项是算术平均、通常非零（见 `MacroscopicProperties.cpp:658-661`），该问题在**普通导体界面**上也可能触发：某些 E 分量的阻尼/耦合被错误地关闭。

**为何静默**
`fill_sigma_mats` 打印 `[SIGMA-EXP] ... M1/M2 filled where sigma>0`，看起来一切正常；没有任何检查确认“非对角元是否参与更新”。

**建议方向（不修改代码，仅记录）**
门控应基于整个张量（如 `max|σ_ij| > 0` 或矩阵范数），且指数步要么三分量整体执行、要么整体回退，不能逐行跳过。

---

## F-05（Medium）`subpixel` 在 3D 下的界面法向/材料采样维度不全；`mu` 完全未做亚网格平均

**位置**
- `Source/Plugins/FieldSolver/FiniteDifference/MacroscopicProperties/MacroscopicProperties.cpp:681-688`：法向梯度只计算 `gx0` 与 `gz0`，`nrmv_t[1]`（y 分量）恒为 0。
- 同文件 `:690-691`：两侧材料判定 `m1L/m1R` 只沿 `zn ± 0.25 dz` 采样。
- 同文件 `:731` 起：Kottke 平均的偏移量同样只沿 z。
- `MacroscopicEvolveE.cpp:431`（指数步）：`mun = macroscopic_properties.getmu_mf().array(mfi)`，`mu` 只有单元中心值，没有节点/亚网格版本。
- 用户文档：`Docs/source/usage/parameters.rst:4090-4097` 明确承诺“permittivity, conductivity and permeability are averaged … keeps the second-order accuracy … for curved or oblique interfaces”。

**产生原因**
亚网格实现实质上是 2D（x-z）的：法向只取 x-z 分量，材料采样只沿 z。对 3D 中法向含 y 的曲面（球面两极、竖直壁、任意斜界面），`gx0≈gz0≈0` 时法向回退为 `(0,0,1)`，两侧材料采样落在同一侧，界面被当成“无界面”或沿 z 的界面。

**对仿真的影响**
- 3D 曲面/斜界面的等效 ε、σ 在部分区域退化为单侧材料的阶梯值，承诺的二阶精度不成立；反射/透射系数出现方向相关的伪误差。
- 磁性材料的 `mu` 永远按单元阶梯取值，磁界面失去亚网格平滑。
- 仅在 `algo.macroscopic_subpixel=1` 且设置界面/张量时触发，因此现有以 z 法向平面界面为主的测试无法暴露。

**为何静默**
没有维度/方向限制的警告或 abort，参数文档也没有声明“仅支持 z 法向”。

---

## F-06（Medium）Dirichlet ghost 用单元中心势，而 AMReX 将其当作面值

**位置**
- `Source/Plugins/NewtonGravity/StarGravityPlugin.cpp:94-99`：
  `x = plo[0] + (i + 0.5) * dx[0]`，`a(i,j,k) = -G M / r`，即 ghost **单元中心**处的势。
- `Source/Plugins/NewtonGravity/NewtonGravity.H:73-80`：明确写“AMReX places physical boundary data on the domain **face** … the caller must fill a boundary ghost cell with the potential evaluated on that face – the normal coordinate clamped to the domain … Filling ghost cells with cell-centre values instead is a half-cell shift … first-order error”。
- 使用方：`NewtonGravity.cpp:92` `mlabec.setLevelBC(0, &phi)`，AMReX 按面值解释 ghost。

**产生原因**
实现与自身接口契约相反：法向坐标没有 clamp 到域面，而是用了 ghost 单元中心。

**对仿真的影响**
边界势存在 `O(dx)` 的半格偏移（相对误差约 `0.5 dx / r_boundary`）；对太阳算例约 0.3%，对紧致计算域或精细层可显著增大，并通过 Poisson 传播到内部。属于“看起来收敛、实际降为一阶”的典型问题。

**为何静默**
无警告；金标准测试若用解析势在体心采样，未必能分辨边界半格偏差。

---

## F-07（Medium）默认路径下 `rho_medium` 永不推进，Gauss 诊断冻结

**位置**
- `Source/Plugins/PicEm/Modules/FieldSolver/WarpXPushFieldsEM.cpp:1275-1282`：仅当 `WarpX::macroscopic_subpixel` 为真才调用 `InitSubpixelRows`。
- `Source/Plugins/FieldSolver/FiniteDifference/MacroscopicProperties/MacroscopicProperties.cpp:923-1091`：`m_sigma_mat_ready` 与节点 `M1/M2/J0/J1` 只在 `InitSubpixelRows` 内建立；默认路径下 `has_sigma_matrices()==false`。
- `MacroscopicEvolveE.cpp:395`：指数步（唯一写 `Jcond` 的地方）被 `has_sigma_matrices()` 门控。
- `WarpXPushFieldsEM.cpp:1437-1467`：`rho_medium` 的推进只由 `MediumForEM::Macroscopic` 门控，使用 `get_jcond_mf(...)`；默认路径下这些数组保持 0。
- `MacroscopicProperties.cpp:485-490`：只警告“回退到 BE”，没有警告“`rho_medium` 不会更新”。

**产生原因**
电荷推进与电流来源的开关不一致：E 的阻尼在默认路径通过单元 sigma 的 BE 分支正常工作，但 `Jcond` 的唯一生产者是指数步。

**对仿真的影响**
场的演化本身仍近似正确；但 `rho_medium`（以及任何基于它的 Gauss 残差/输出）停留在初始 seed 值，导致本来用于验证守恒的诊断变成假阴性/假阳性。若下游用 `rho_medium` 做自洽性判断，会得到错误结论。

**为何静默**
日志只提示积分器回退，不提示电荷未推进；残差通常只在 `verbose` 较高时打印。

---

## F-08（Medium）节点电导“真空优先”规则与文档不符，单层导体完全透明

**位置**
- `Source/Plugins/FieldSolver/FiniteDifference/MacroscopicProperties/MacroscopicProperties.cpp:655-661`：
  法向用调和平均；切向在 `sL<=0 || sR<=0` 时直接置 0，否则算术平均。
- 用户文档 `Docs/source/usage/parameters.rst:4094-4096`：承诺 harmonic (normal) / arithmetic (tangential) 平均，没有“任一侧为真空则置零”的例外。
- `MacroscopicEvolveE.cpp:249-251` 等：节点 `sigma=0` 的单元走 BE 且 `sigma_interp=0`；指数步 `:451` 也跳过。

**产生原因**
为避免“真空侧界面节点被过阻尼”（c03-E6）引入了真空优先的置零规则；但该规则对薄层导体是致命的：厚度为 1 个单元的导体，其所有节点都同时邻接导体与真空，切向恒为 0、法向调和平均也为 0，于是所有节点的 `sigma` 都是 0。

**对仿真的影响**
- 单层金属膜/薄导电层在 E 更新中**完全不导电**；2 层时也只有中间一层节点有效，阻尼被显著低估。
- 这与文档承诺的界面平均模型不一致，且没有任何“层厚不足”警告。

**为何静默**
节点 sigma 为 0 被解释为“真空”，所有分支都合法地跳过；场看起来只是“没有损耗”。

---

## 3. 交叉验证：确认无问题的部分（避免误伤）

- **插件独立性/模块化**：生产代码中未发现插件之间的头文件包含或链接依赖；`StarFluidPlugin.cpp:15-16` 明确“includes nothing from another plugin”，跨插件仅通过字段名。`Source/Plugins/*/CMakeLists.txt` 也只链接 `ablastr`，模块化目标基本达成。
- **调度顺序**：`star_gravity` 写 `star_g*`、`star_fluid` 读 `star_g*`，调度器建立写→读依赖，顺序正确（问题不在顺序，而在消费）。
- **张量指数数学**：`SubpixelTensor.H` 中 `M1=exp(-A dt)`、`M2=(1-exp(-A dt))A^{-1}ε^{-1}` 与 `A=ε^{-1}σ` 的相似变换推导一致；`C=ε^{-1/2}[(1-e^{-Bdt})/(Bdt)]ε^{1/2}` 的实现与推导一致；小量展开系数正确。
- **4πG 量纲**：`StarGravityPlugin.cpp:145-152` 已改为 cgs `G`，与 cgs 密度/几何自洽（修复了旧问题）。
- **量纲/系数**：`Electromagnetic.H` 的 `J=σE'`、`f=J×B/c`、`w=f·u` 在 Gaussian cgs 下量纲一致。

---

## 4. 对既有审查意见的更正（重要，防止“按错误建议修复”）

`CODE_REVIEW_2026-11-16.md` 的 **F4** 声称 `Efield_fp` 每个方向是 3 分量、`Electromagnetic.H:149` 的 `ea[d](i,j,k)` 应改为 `ea[d](i,j,k,d)`。该判断在当前代码中**不成立**：

- `Source/Plugins/PicEm/WarpX.cpp:146`：`int WarpX::ncomps = 1;`
- 同文件 `:2911-2913`：`AllocLevelField("Efield_fp_x/y/z", ..., ncomps, ...)`，3D 下每个方向的 MultiFab 只有 **1** 个分量，分量 0 就是该方向的 E 分量（RZ 下为 `2*nmodes-1` 个方位模，分量 0 仍是模 0）。
- 因此 `Electromagnetic.H:149` 的 `Ev[d] = ea[d](i,j,k)` 是**正确**的；若按 F4 改成读取第 `d` 个分量，在 3D 会越界读取（分量 1/2 不存在）。

建议后续审查明确区分“WarpX 原版按分量命名/存储”的直觉与当前 fork 的实际分配，避免引入真实越界。

---

## 5. 优先级建议（仅记录，不修改代码）

1. **F-01**：在任何恒星/自引力算例投入生产前必须修复；否则整个物理结果无效，且日志会误导。
2. **F-02**：与 F-01 一起处理；明确选择“动态自引力”还是“Cowling 近似”，并在文档/日志中声明。
3. **F-03**：`open` 与全周期几何的组合应显式 abort 或自动改用零均值周期解；周期 ghost 不得被单极子覆盖。
4. **F-04**：张量电导的门控与指数积分必须作为整体矩阵处理。
5. **F-05/F-08**：要么补齐 3D 法向与 `mu` 的亚网格、并把 sigma 规则改回文档承诺的平均，要么在参数文档中明确限制并对不支持的配置 abort/warn。
6. **F-06/F-07**：边界半格与 `rho_medium` 推进的开关一致性。

---

**声明**：本报告仅做只读审查与文档撰写，未修改任何项目代码或项目文件。
