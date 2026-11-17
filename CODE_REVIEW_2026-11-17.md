# 提交后正确性审查报告（2026-11-17）

## 0. 审查声明与范围

- **只读审查**：本次没有修改任何项目代码、构建文件、测试或数据文件；本文件是唯一新增内容。
- **审查对象**：仓库为**单根 squash 提交**（`git rev-list --count HEAD == 1`），HEAD =
  `09b73a1c296682675da85fa4a7e31312ba0e7f69`，提交时间 `2026-11-17T09:11:52+08:00`，
  提交主题 “studio: 修项目名 slug 冲突（∥/⊥ 都塌成 '-' 导致项目互相覆盖）+ selftest 检查”。
  由于历史被压平，**“过去 24 小时的提交”在事实上等于当前整棵树**；本报告按这个范围审查。
- **方法**：静态阅读 + 跨文件执行路径追踪（核心总线 / 阶段调度 / 宏观介质求解器 / 插件构建 /
  Studio 存储），并复核了 `Docs/audit_2026_11`、`Docs/research_notes` 中已有结论，
  只把**尚未被现有审查记录覆盖、或在当前 HEAD 仍然成立**的问题列为“新增/关键”。
- **限制**：沙箱内没有 `build/`，未执行 `cmake`/`ctest`。因此“确认”指代码路径与数学推导可静态确定；
  涉及数值表现的条目给出了触发条件与置信度。

## 1. 项目根本目标与判定标准

项目目标是：把 DOE 的 WarpX（PIC）改造成**非官方插件化 fork**——核心（Core）不依赖具体物理算法，
各插件通过核心总线/接口接入且彼此独立，用户可以只写自己的插件就扩展功能，同时**不削弱 WarpX 原版的
物理正确性与功能覆盖**。

因此本报告按以下顺序判定：

1. **物理正确性优先**：离散格式是否与 Maxwell/介质本构方程一致；是否存在“静默地算错”的路径。
2. **算法是否可被理论证明**：矩阵指数、界面平均、阶梯近似等是否有未声明的前提被打破。
3. **插件化/模块化契约**：插件是否能独立构建、是否只能通过总线通信、阶段上下文是否完整。
4. **功能覆盖**：上游 WarpX 支持的维度/配置是否被 fork 静默丢掉。

## 2. 结论摘要

| ID | 严重度 | 主题 | 是否静默 | 置信度 |
|---|---|---|---|---|
| N-01 | **高** | 1D `interface_function` 坐标轴错位；唯一界面算例/验证脚本掩盖该缺陷 | 是 | 高 |
| N-02 | **高** | 无电导的各向异性 ε 永不进入精确张量步；BE 路径把 `ε⁻¹J` 与阻尼截断为对角（一阶误差） | 是 | 高 |
| N-03 | 中高 | 界面 σ 张量的“分块拼接”不保正定，指数步可能出现非物理增长 | 是 | 中（需特定材料组合） |
| N-04 | 中 | 阶段上下文/ C ABI 的 `dt/step/time` 在多处为空（A-04 的扩展面） | 是 | 高 |
| N-05 | 中高 | 构建期插件层仍要求改核心 CMake；BoundaryConditions 插件被 park，接口从未接线 | 否（但违背目标） | 高 |
| N-06 | 中 | Studio slug 修复只覆盖 ∥/⊥；同类项目静默覆盖仍存在（本次提交主题） | 是 | 高 |
| N-07 | 低/潜在 | 3D `fill_rows` 把 j 当 z、丢弃 k；3D 胞元子采样冻结 y（当前为死代码） | 是 | 高（代码）/ 低（当前影响） |
| N-08 | 低 | `m_eps_inv_node` 的 FillBoundary 在填充之前调用，ghost 未更新 | 是 | 高（但仅 ghost 读取者受影响） |
| A-01…A-05 | 高…低 | 2026-11-16 报告中的旧问题：当前 HEAD **仍未修复**，且与 N-01/N-02/N-03 互相放大 | — | 已复核 |

> 说明：A-01…A-05 不是本次“新发现”，但它们在当前 HEAD 仍可复现，且与新增问题构成系统性耦合，
> 因此集中在第 4 节列出，不重复细节。

## 3. 新增关键缺陷（详细）

### N-01【高】1D 的 `interface_function` 用错了坐标轴；唯一界面算例与验证脚本把缺陷“测成了正确”

**位置（节点级宏观介质构建路径）**

- [MacroscopicProperties.cpp](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicProperties/MacroscopicProperties.cpp#L574-L585)：
  非 3D 分支把网格第一个索引 `i` 当成 `x`：
  `xn0 = xlo_n + i*dx_n`；`zn0 = zlo_n + j*dz_n`。在 1D 中 `j == 0`，所以 `zn0 ≡ zlo`。
- 同文件 [L596-L604](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicProperties/MacroscopicProperties.cpp#L596-L604)：
  `iL = iR = ii`、`jL = jR = j`，只有 `k` 方向在 3D 中按 `nrmv[2]` 偏移。
- 同文件 [L654-L659](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicProperties/MacroscopicProperties.cpp#L654-L659)：
  材料判定只在 **z** 方向探测 `zn ∓ 0.25 dz_n`，与界面法向无关。
- 同文件 [L856-L866](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicProperties/MacroscopicProperties.cpp#L856-L866)：
  **官方 1D 坐标约定**清楚地是 `x=0, y=0, z=zlo + i*dz`（同一个 helper 也用于
  `epsilon_function`/`sigma_function`/`mu_function`）。

**原因**

1D 构建时，求解器实际使用的节点 ε/σ 来自上面那条“节点级”路径，而该路径把 `i` 当 `x`。
于是 `interface_function(x,y,z)` 在 1D 中收到的是 `(i*dx, 0, zlo)`，而不是 `(0, 0, i*dz)`。
一个按文档写 `interface_function = z - 0.5` 的正确算例，会得到 `phi ≡ zlo - 0.5`（常数），
**整个区域被判成同一种材料**；界面完全消失，而且没有任何报错/警告。

**影响（物理）**

- 1D 介质界面静默退化为均匀介质；反射/透射、介质加载腔频率、界面处 D 法向连续性全部错误。
- 即使界面“侥幸”出现（因为写错轴），法向判定也会落到 `nrmv[0]`（x），导致 1D 唯一的物理分量
  `Ez` 被当作**切向**分量做算术平均，而不是法向分量做调和平均：
  `2ε₁ε₂/(ε₁+ε₂)` 被替换为 `(ε₁+ε₂)/2`。对 ε₁=1.5、ε₂=1，节点 ε 从 1.2 变成 1.25（约 4% 偏差），
  且这是系统性的、不随网格收敛消失的界面误差（只随界面位置振荡）。
- 3D 中同一缺陷更明显：`nrmv[1]` 从未被计算（只算 `gx0`、`gz0`，
  [L583-L587](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicProperties/MacroscopicProperties.cpp#L583-L587)），
  且材料探测只沿 z，所以 **x / y 法向界面在节点 ε/σ 中被整体抹掉**（见 A-02 复核）。

**测试为什么没发现（系统性掩盖）**

- 唯一的界面算例 [inputs_1d_periodic_interface](file:///workspace/Examples/Tests/macroscopic_solver/inputs_1d_periodic_interface#L36-L38)
  写的是 `interface_function = x - 0.5`。按官方 1D 约定 `x ≡ 0`，它本应是常数；它之所以能“形成界面”，
  恰好是因为 N-01 把 `i` 当成了 `x`。**算例依赖 bug 才能工作。**
- 该算例自身还自相矛盾：注释说均匀 `ε_r=1.5`（第 6 行），却又设置 `epsilon2=1.0` 和界面（第 33、38 行）；
  注释说 PEC 壁（第 3 行），实际边界是 periodic（第 25-26 行）；注释说 `p=1`（第 5 行），实际 `p=2`（第 42 行）；
  `max_step=60` 小于诊断间隔（若 `diag1.intervals=200`，运行结束也不会输出任何场）。
- 它**没有注册进 CTest**：[CMakeLists.txt](file:///workspace/Examples/Tests/macroscopic_solver/CMakeLists.txt)
  只有 3 个测试，均不使用 `interface_function`/张量。
- 唯一的“验证”脚本 [check_macroscopic_periodic_interface.sh](file:///workspace/Tools/verify/check_macroscopic_periodic_interface.sh#L8-L15)
  只检查 `div D - rho_medium` 的高斯残差，并明确写着“stays zero either way”——
  种电荷与检查读的是同一份 ghost，**该残差对错误的界面平均不敏感**，因此不能证明界面物理正确。
- 现有审查记录 [AUDIT_RESPONSE_2026_11_12.md](file:///workspace/Docs/research_notes/AUDIT_RESPONSE_2026_11_12.md#L56-L71)
  已承认“节点级 interface_function 只在 XZ 下坐标正确”，并称“没有任何测试使用它”；
  但该算例文件确实存在（只是未注册），且验证脚本还把它描述成 “interface at x = 0.5”。
  **这是审查遗漏的关键点：算例不是独立证据，而是与 bug 同构。**

**修复方向（本报告不实施）**

- 1D 节点路径必须与 `InitializeMacroMultiFabUsingParser` 使用同一坐标约定（`x=y=0, z=i*dz`）；
- 材料探测与邻居选择必须沿真实法向（至少支持 x/y/z 三个轴），否则应在参数层显式拒绝并报错；
- 回归测试要用 `z - 0.5` 且断言界面两侧的 ε 节点值/反射系数，而不是只看自洽残差。

---

### N-02【高】纯各向异性介质（σ=0）永远走不到精确张量步；BE 路径的 `ε⁻¹J` 与阻尼被截断为对角，是一阶误差而非“二阶小量”

**位置**

- 精确矩阵指数步的开关：
  [MacroscopicEvolveE.cpp L184-L185](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicEvolveE.cpp#L184-L185)
  `bool has_sig_mat = macroscopic_properties.has_sigma_matrices();`
- `has_sigma_matrices()` 只有在 `fill_sigma_mats` 真正建立 M1/M2 后才为真：
  [MacroscopicProperties.cpp L897-L947](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicProperties/MacroscopicProperties.cpp#L897-L947)
  （`smax <= 0 && !m_has_sig_tensor` 直接 `return`，不设置 `m_sigma_mat_ready`）。
- 因此 **σ=0 时必然走 BE 分支**：
  [MacroscopicEvolveE.cpp L289-L291](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicEvolveE.cpp#L289-L291)、
  [L331-L333](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicEvolveE.cpp#L331-L333)、
  [L373-L375](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicEvolveE.cpp#L373-L375)。
- 同文件 L267-L291 的 BE 写法：
  `E_c = α E_c + β (ix0 Dx + ix1 Dy + ix2 Dz − ix_c * j_c)`，
  其中 `α = 1/(1 + dt σ (ε⁻¹)_{cc})`。

**原因（数学）**

介质中的方程是 `∂D/∂t = ∇×H − J`，`D = εE`。因此一步更新必须是

```
E^{n+1} = E^n + dt ε⁻¹ (∇×H − J)
```

其中 **`ε⁻¹` 同时作用于 `∇×H` 和 `J`**。代码对 `∇×H` 用了完整行 `(ix0, ix1, ix2)`，
但对 `J` 只保留了 `ix_c * j_c`。在 `ε` 有非对角分量时，
`(ε⁻¹J)_x = ix0 Jx + ix1 Jy + ix2 Jz`，丢失的 `ix1 Jy + ix2 Jz` 与保留项同阶，
并不是 `O(h)` 或“二阶小量”。

数值例子：`ε = [[3,1,0],[1,3,0],[0,0,1]]`，`ε⁻¹ = (1/8)[[3,-1,0],[-1,3,0],[0,0,8]]`，
取 `J=(0,1,0)`，正确源项 x 分量为 `-1/8`，代码给出 `0`。**相对误差 100%。**

同样，`α` 用标量 `1/(1+dt σ (ε⁻¹)_{cc})` 代替矩阵 `(I + dt σ ε⁻¹)^{-1}`，
在 `ε⁻¹` 有非对角项时也不是精确的 Backward-Euler。

**触发条件**

- `epsilon_tensor_xy/xz/yz ≠ 0`（任意主轴旋转的介质）；
- `sigma = 0`（纯介质，最常见的介质波导/透镜场景）；
- 有粒子电流沉积 `J ≠ 0`（PIC 的正常工作方式）。
- 注意：**即使把 `macroscopic_subpixel=1` 也救不了**——σ=0 时 `has_sig_mat` 仍为 false，
  精确指数步根本不会执行。所谓“exponential 是唯一支持张量的路径”，在纯介质下是**不可达**的。

**影响（物理）**

- 各向异性介质中的极化/欧姆响应算错，E 的横向分量少掉来自 J 的交叉耦合；
- 能量守恒、介质中的折射/双折射、粒子-场能量交换都会出现静默偏差；
- 现有测试只覆盖**对角**张量（`T10` 的 “diagonal tensor vs scalar”），非对角张量完全无回归测试，
  也没有任何示例使用 `epsilon_tensor_*`/`sigma_tensor_*`。

**备注**

`C_COMMIT_HISTORY` 中写明“J 保持对角（离对角耦合为二阶小量，P1 不做）”。该前提只在
**弱各向异性或 J 沿主轴**时近似成立；代码接受任意非对角 ε 且不限制、不警告，
因此对用户暴露的是一个未受控的一阶误差。至少应在参数层限制或显式警告。

---

### N-03【中高】界面 σ 张量的“逐块平均”不保正定；精确指数步可能出现非物理指数增长

**位置**

- 节点 σ 张量的拼接：
  [MacroscopicProperties.cpp L716-L757](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicProperties/MacroscopicProperties.cpp#L716-L757)：
  法向分量取调和平均，切向块在任一侧为真空时置零，**但法向-切向耦合项仍取算术平均**
  （`sh.m[1] = 0.5*(sLh.m[1] + sRh.m[1])` 等）。
- 指数步：
  [SubpixelTensor.H L341-L361](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicProperties/SubpixelTensor.H#L341-L361)
  构造 `B = ε^{-1/2} σ ε^{-1/2}`，对其做对称 Jacobi 特征分解；
  [L276-L284](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicProperties/SubpixelTensor.H#L276-L284)
  的 `mat3_fun` 对特征值**不做任何截断**。

**原因（数学）**

对两个正定矩阵做“按块分别平均”（法向块调和、切向块算术、耦合块算术）**不保证结果仍正定**。
一个单侧各向异性导体 + 真空的例子：在界面坐标系中

```
σ_L = 0,  σ_R = [[λ, c, 0],
                 [c, t, 0],
                 [0, 0, t]]   (正定)
```

拼接后得到 `[[0, c/2, 0],[c/2, 0, 0],[0,0,0]]`，特征值为 `±c/2`，**负特征值**。
两个各向异性导体、主轴不同时也可构造出对角为正但整体不定的矩阵。

`B` 与 `σ` 合同，因此也有负特征值。指数步使用 `exp(-B dt)`：
对负特征值 `λ<0`，`exp(-λdt) = exp(+|λ|dt)` 会**指数放大**，`M2` 同步放大。
代码对 `B` 的特征值没有正定性检查（只对 `ε` 有 `lam > 0` 断言）。
此外，若拼接后所有对角元恰好为 0（单侧真空情形），
`if (has_sig_mat && sigma_node > 0) return;` 会让这些节点**完全跳过电导**，
静默丢失表面/法向电流。

**触发条件**

- `macroscopic_subpixel=1`；
- 使用 `sigma_tensor_*`；
- 界面两侧是主轴不同的各向异性导体（或单侧各向异性导体 + 真空，此时表现为“电导被静默丢弃”）。

**影响（物理）**

- 最坏情况下 E/B 在界面处按 `exp(|λ|dt)` 增长，仿真发散，但不会给出任何“材料非法”的提示；
- 较轻的情况下，界面电导率张量非物理（负本征模），阻尼/趋肤深度错误。

**建议的验证（不实施）**

对拼接后的 `σ_node` 做最小特征值断言/投影到 PSD 锥；并为“各向异性导体 + 真空/不同主轴导体”
建立单元测试，直接检查 `eig(B) ≥ 0`。

---

### N-04【中】阶段上下文与 C ABI 的 `dt/step/time` 仍有缺口（A-04 的扩展面）

**位置（当前 HEAD）**

- [RunFinalizeStage L613-L628](file:///workspace/Source/Plugins/PicEm/Modules/Evolve/WarpXEvolve.cpp#L613-L628)：`ctx` 未设置任何 `step/time/dt`。
- [RunFieldSolveStage L636-L653](file:///workspace/Source/Plugins/PicEm/Modules/Evolve/WarpXEvolve.cpp#L636-L653)：只设 `ctx.time`，`dt/step` 为 0。
- [RunAfterFieldSolveStage L686-L700](file:///workspace/Source/Plugins/PicEm/Modules/Evolve/WarpXEvolve.cpp#L686-L700)：设了 `time/dt`，但**没有 `step`**。
- [RunRegridStage L661-L671](file:///workspace/Source/Plugins/PicEm/Modules/Evolve/WarpXEvolve.cpp#L661-L671)：只设 `step`。
- [RunInitStages L673-L683](file:///workspace/Source/Plugins/PicEm/Modules/Evolve/WarpXEvolve.cpp#L673-L683)：什么都没设。
- [RunDiagnosticsStage L702-L719](file:///workspace/Source/Plugins/PicEm/Modules/Evolve/WarpXEvolve.cpp#L702-L719)：只设 `step`，`time/dt` 为 0。
- 对照正确写法：[RunPushStage L590-L601](file:///workspace/Source/Plugins/PicEm/Modules/Evolve/WarpXEvolve.cpp#L590-L601) 同时设置 `step/time/dt`。

**契约面**

C ABI 对每个阶段都暴露了三个访问器：
[PluginCApi.H L129-L131](file:///workspace/Source/Core/Runtime/PluginCApi.H#L129-L131)
`warpx_stage_step/time/dt`，且阶段常量包含
[`WARPX_STAGE_FIELD_SOLVE` / `WARPX_STAGE_COLLISIONS`](file:///workspace/Source/Core/Runtime/PluginCApi.H#L53-L57)。
`StageContext` 的默认值全为 0（`Source/Core/Runtime/HandlerSpec.H` 的 `StageContext` 定义）。

**影响**

- 一个用户在 `WARPX_STAGE_FIELD_SOLVE` 写“替换场求解器”的插件，`warpx_stage_dt()` 得到 **0**：
  任何 `E += dt*(...)`、CFL 判据、`1/dt` 归一化都会静默失效（或除零）。
- 在 `WARPX_STAGE_DIAGNOSTICS` 写时间相关输出的插件拿到 `t=0`，文件名/时间戳错误；
  在 `AFTER_FIELD_SOLVE` 拿不到 `step`。
- 这类错误不会中断运行，属于“插件写得很对、结果全错”的静默缺陷，直接削弱本项目“用户可独立写插件”的核心价值。

---

### N-05【中高】构建期插件层仍要求修改核心 CMake；唯一的独立边界条件插件被 park，接口从未接线

**位置/证据**

- [CMakeLists.txt L332-L351](file:///workspace/CMakeLists.txt#L332-L351)：`WARPX_MODULE_LAYERS` 是**硬编码列表**
  （`core picem` + 若干 `if(WarpX_PLUGIN_*)`）。新增一个物理插件必须改这个文件。
- [CMakeLists.txt L440-L469](file:///workspace/CMakeLists.txt#L440-L469)：每个插件还必须手工加入
  `_WARPX_PLUGIN_ANCHORS` 的 `-u,<anchor>` 列表，否则静态注册会被链接器丢弃。
- [ParkedPlugins/BoundaryConditions/README.md L50-L68](file:///workspace/Source/ParkedPlugins/BoundaryConditions/README.md#L50-L68)：
  作者自己记录了失败原因——
  “本仓库要求 `Source/Plugins/<层>` 必须登记在 `WARPX_MODULE_LAYERS`，登记后顶层为它创建无源 OBJECT
  目标，插件子目录再 `target_sources` 仍导致 `CMake Error: No SOURCES given`”，因此把插件移到
  `ParkedPlugins/`。
- 同 README L16-L29 的编译命令使用 `-DWarpX_PLUGIN_BC=ON`，但顶层根本没有这个选项
  （只有 `WarpX_PLUGIN_DIAGNOSTICS/COLLISION/FIELDSOLVER/PARTICLECORE/NEWTONGRAVITY/FLUIDMOMENTS/IMPLICIT`），
  命令不会启用任何东西。
- 实际生效的 PEC 实现仍在 **PicEm 插件内部**：
  `Source/Plugins/PicEm/Modules/BoundaryConditions/WarpXFieldBoundaries.cpp`（README L65）。
- 核心接口 `RuntimeData::boundaryConditions()` / `IBoundaryConditions` **没有任何消费者**：
  在全仓库搜索 `IBoundaryConditions`、`boundaryConditions()`、`MakeBoundaryConditions`，
  除定义处外没有求解器调用。也就是说，边界条件既没有插件化，也没有接线。

**影响**

- 用户无法在不修改核心 `CMakeLists.txt` 的前提下，把一个**带物理功能**的插件加进 WarpX 可执行文件。
  运行期 C ABI 可以加载观察者/阶段替换器，但构建期物理插件（尤其边界条件、场求解器）仍被核心构建系统锁死。
  这与“插件彼此独立、只通过总线接入”的根本目标不符。
- 唯一的“独立边界条件插件”证明件已经腐烂：README 的选项不存在、路径写成 `Source/Plugins/...`
  （实际在 `Source/ParkedPlugins/...`）、接口无人调用。这会误导后续插件作者。
- 此外，`WarpX_PLUGIN_IMPLICIT` 显式依赖 `WarpX_PLUGIN_FIELDSOLVER`、
  `WarpX_PLUGIN_COLLISION` 依赖 `WarpX_PLUGIN_PARTICLECORE`（CMake L82-L91），
  是插件到插件的构建依赖；虽然不是源码耦合，但同样削弱“插件独立”的表述。

---

### N-06【中】Studio slug 修复只覆盖 ∥/⊥；同类“不同项目名映射到同一目录”的静默覆盖仍然存在（本次提交主题）

**位置**

- [store.py L27-L37](file:///workspace/Tools/warpx-studio/warpx_studio/deck/store.py#L27-L37)：新增
  `∥/‖ → par`、`⊥ → perp`、`→ → to`、`× → x` 的预映射，再做 ASCII slug 化。
- [store.py L78-L102](file:///workspace/Tools/warpx-studio/warpx_studio/deck/store.py#L78-L102)：`save()` 直接用
  `slug(name)` 作为目录名写入，**不做唯一性检查，也不检查已有同名 slug 是否对应不同原始名**。

**残留的碰撞类（均可静默覆盖项目）**

| 原始名 A | 原始名 B | 共同 slug |
|---|---|---|
| `B∥` | `Bpar` | `Bpar` |
| `a b` | `a-b` | `a-b` |
| `A/B` | `A-B` | `A-B` |
| `run#1` | `run-1` | `run-1` |
| `B×` | `Bx` | `Bx` |
| 任意两个前 60 字符相同、仅尾部不同的长名字 | — | 被截断为同一 slug |

selftest 只断言了 `B∥ != B⊥`（`B∥` 与 `Bpar` 的碰撞不在检查范围），因此**本次提交声称修复的
“项目互相覆盖”这一类问题并没有被消除**，只是消除了消息里点名的两个符号。

**影响**

- 两个不同项目被静默写入同一目录：后保存的覆盖前一个的 `project.json`/inputs，用户无感知的数据丢失。
- 这类缺陷与物理仿真无关，但会直接破坏“项目/算例管理”的可信度；对 Studio 的自动保存/重命名路径尤其危险。

---

### N-07【低/潜在】3D `fill_rows` 的坐标映射错误与 3D 胞元子采样冻结 y；当前是死代码，但公共 getter 与文档会受影响

**位置**

- [MacroscopicProperties.cpp L1167-L1191](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicProperties/MacroscopicProperties.cpp#L1167-L1191)：
  `fill_rows` 在 3D 下仍用 `j` 当 z：`zc = plo[AMREX_SPACEDIM-1] + (j+0.5)*dz`，
  同时把 `phi_exec(xc, 0.0, zc)` 的 y 固定为 0，并且 `k`（真正的 z 索引）完全没用到。
  `nsub` 子网格也只在 x-z 上采样。
- [同文件 L950-L1030](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicProperties/MacroscopicProperties.cpp#L950-L1030)：
  胞元 ε/σ 的子采样在 3D 下把 `yc` 固定，只循环 x-z 两个方向，y 依赖被冻结。
- 目前求解器读取的是 `m_eps_inv_node`（`use_node_eps = true`，
  [MacroscopicEvolveE.cpp L277-L279](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicEvolveE.cpp#L277-L279)），
  因此 `m_eps_inv_x/y/z` 与胞元 M1/M2 **当前没有被消费**；这是“潜在”而非“当前生效”的原因。

**影响**

- 若未来把 `use_node_eps` 打开为 false，或用外部插件调用 `IMacroscopicProperties::geteps_inv_*_mf`，
  3D 界面/张量结果会完全错误（z 方向被 j 取代、y 被冻结）；
- 当前仍会把错误的 `chi^{-1}` min/max 打印到日志，误导调试；
- `Docs/audit_2026_11` 的 `SOURCE_Plugins_FieldSolver_A.md` 已记录该问题，`BACKLOG` 也把
  “3D y 方向冻结”标为 fixed，但代码中的胞元路径**并未修复**，属于记录与实现不一致。

---

### N-08【低】`m_eps_inv_node` 的 FillBoundary 在填充之前调用，ghost 从未更新

**位置**

- [MacroscopicProperties.cpp L769-L773](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicProperties/MacroscopicProperties.cpp#L769-L773)：
  在 `m_eps_inv_node` 尚未计算时就调用了 `FillBoundary`；
- [L777-L794](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicProperties/MacroscopicProperties.cpp#L777-L794)：
  之后才真正填入 `inv3_sym(...)`，但**没有再调用 FillBoundary**。

**影响**

- 求解器只读 valid box（`tb`），所以当前数值结果不受影响；
- 但 `get_eps_inv_node_mf` 是公共 getter，任何读取 ghost 的插件/诊断（例如跨 periodic 边界的散度检查）
  会得到第一次分配时的未初始化/零值。属于接口正确性问题，建议把 FillBoundary 移到填充之后。

---

## 4. 旧问题复核（2026-11-16 报告中的 A-01…A-05，当前 HEAD 仍存在）

> 这一节只做“是否仍成立”的复核，细节见 `CODE_REVIEW_2026-11-16.md`。

| ID | 复核结论 | 当前证据 |
|---|---|---|
| **A-01** 碰撞阶段被静默禁用 | **仍存在**。`RunCollisionStage` 用 `scheduler.run(BeforeCollisions, AfterCollisions) > 0` 判断“是否被接管”，因此任何注册在 `BEFORE_COLLISIONS=6000` / `COLLISIONS=7000` 区间的**只读观察者**都会让 `m_collision_handler->doCollisions()` 被整段跳过。 | [WarpXEvolve.cpp L747-L751](file:///workspace/Source/Plugins/PicEm/Modules/Evolve/WarpXEvolve.cpp#L747-L751)；对照正确写法 [L597-L601](file:///workspace/Source/Plugins/PicEm/Modules/Evolve/WarpXEvolve.cpp#L597-L601)；C ABI 阶段常量 [PluginCApi.H L53-L54](file:///workspace/Source/Core/Runtime/PluginCApi.H#L53-L54) |
| **A-02** x/y 法向界面退化为阶梯/被抹掉 | **仍存在**。3D 只算 `gx0/gz0`，`nrmv[1]` 恒为 0；`iL=iR`、`jL=jR`，材料探测只沿 z。x 法向界面的两侧来自同一胞元，`material 2` 完全不进入节点 ε/σ。 | [MacroscopicProperties.cpp L583-L587](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicProperties/MacroscopicProperties.cpp#L583-L587)、[L596-L604](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicProperties/MacroscopicProperties.cpp#L596-L604)、[L654-L659](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicProperties/MacroscopicProperties.cpp#L654-L659) |
| **A-03** `sigma_tensor` 在 `macroscopic_subpixel=0` 下完全失效 | **仍存在，且警告更弱**。`InitSubpixelRows` 只在 `macroscopic_subpixel` 为真时调用；`has_sigma_matrices()` 随之为 false；BE 使用标量 `m_sigma_mf`（默认 0）。而“未指定电导率”的警告只检查标量 σ（[L485-L490](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicProperties/MacroscopicProperties.cpp#L485-L490)），只给张量时不会触发；启动 banner 仍然打印 “matrix exponential M1/M2 (tensor capable)”。 | 调用门 [WarpXPushFieldsEM.cpp L1275-L1276](file:///workspace/Source/Plugins/PicEm/Modules/FieldSolver/WarpXPushFieldsEM.cpp#L1275-L1276)；门控 [MacroscopicEvolveE.cpp L184-L185](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicEvolveE.cpp#L184-L185) |
| **A-04** 阶段上下文缺 `dt/step` | **仍存在，且范围更大**：见 N-04（新增了 `AFTER_FIELD_SOLVE` 缺 `step`、`INIT` 全空、`REGRID` 缺 `time/dt`、`DIAGNOSTICS` 缺 `time/dt`）。 | [WarpXEvolve.cpp L613-L719](file:///workspace/Source/Plugins/PicEm/Modules/Evolve/WarpXEvolve.cpp#L613-L719) |
| **A-05** `DataBus::alias` 写穿透 | **仍存在**。`alias()` 只要求 `dst` 的写权限和 `src` 的读权限，却返回可变 `MultiFab*`；通过 `dst` 写入会直接改 `src`，而插件从未声明对 `src` 的写权限。 | [DataBus.cpp L137-L152](file:///workspace/Source/Core/Runtime/DataBus.cpp#L137-L152) |

**A-01 的静默性尤其需要注意**：碰撞不会报错、不会警告，只会“消失”。对 1D/2D 的弛豫/电离/复合
算例，结果表现为温度/动量不收敛、分布函数不 Maxwell 化，但所有守恒检查（能量由场+粒子自洽）可能仍然通过。

## 5. 系统性关联（为什么不是孤立文件问题）

1. **N-01 + A-02 + 测试**：节点级界面构建在 1D 用错轴、在 3D 丢 y，唯一“界面测试”又依赖同一个坐标错误；
   三者构成闭环，导致**“界面路径没有任何独立验证”**。这解释了为什么 A-02/N-01 能长期存在。
2. **N-02 + A-03**：张量 ε 和 σ 在默认配置下都会被静默降级：
   - σ 张量在 `macroscopic_subpixel=0` 下完全无效（A-03）；
   - ε 张量在 σ=0 时永远走 BE，`ε⁻¹J` 只取对角（N-02）。
   两条路径都“能跑、不报错”，但都不是所声明的张量算法。
3. **N-03 + N-02**：唯一能走精确张量步的分支（σ>0 且 subpixel=1）又可能被非正定 σ 破坏，
   出现非物理增长；而纯介质（σ=0）则根本进不去这条分支——**“精确路径要么不可达，要么可能不稳定”**。
4. **N-04 + N-05 + A-01/A-05**：插件 ABI 一方面暴露 `dt/step/time` 与 `COLLISIONS/FIELD_SOLVE` 阶段，
   另一方面在实现里给出 0 值、把观察者当成接管者、允许 alias 写穿透；
   构建期又要求改核心 CMake。**接口看起来完整，契约实际不闭合**，这正是“用户插件静默算错”的温床。
5. **覆盖缺口**：仓库的 3 个 `macroscopic_solver` 测试都不使用 `interface_function` 或张量参数；
   `epsilon_tensor_*`/`sigma_tensor_*` 在用户手册 `Docs/source/usage/parameters.rst` 中**没有条目**，
   只在 research notes 里出现。因此上述物理缺陷没有回归测试兜底。

## 6. 未发现问题的关键公式（避免误报，供交叉核对）

以下实现经逐项推导**未发现错误**，可作为“不是这里”的排除项：

- 矩阵指数系数：
  `M1 = ε^{-1/2} e^{-B dt} ε^{1/2}`，`M2 = ε^{-1/2} [(1-e^{-B dt})/B] ε^{-1/2}`，
  `B = ε^{-1/2} σ ε^{-1/2}` —— 与 `E^{n+1}=M1 E^n + M2(curlH-J)` 自洽。
- 步平均电导：`C = ε^{-1/2} [(1-e^{-B dt})/(B dt)] ε^{1/2}`；
  `J0 = σC`、`J1 = ε(I-C)ε^{-1}` —— 因 `C` 是 `A` 的矩阵函数、与 `A` 对易，
  `σ(I-C)σ^{-1} = ε(I-C)ε^{-1}` 成立。
- Kottke 的 `τ(ε)` / `τ^{-1}` 分块公式与符号（法向 `-1/ε₁₁`、切向 Schur 补）正确；
  `inv3_sym` 的正定性断言与伴随矩阵求逆正确。
- `curl H` 的三个分量符号（`Dx = -∂z Hy + ∂y Hz` 等）与 Yee 约定一致。
- 3D 的 `rho_medium` 种子已改为对 `en0/en1/en2` 使用**完整行**乘积；
  之前审查提到的 `Dy` 分量错误（使用 `en1(...,4)`）在当前代码中已修正为 `en1(...,1)`。

## 7. 优先级建议（仅建议，不实施）

1. **P0**：先修 N-02（`ε⁻¹J`/阻尼）与 N-01（1D 坐标），因为它们都在“默认/最常见”配置下静默算错；
   同时补一个真正独立的 1D `z` 界面回归（断言界面前后 ε 节点值与反射系数）。
2. **P0**：修 A-01（用 `has_handlers` 判断接管）+ N-04（所有阶段填 `step/time/dt`），
   否则用户插件一接入就会静默改变物理。
3. **P1**：对 N-03 的 σ 拼接做 PSD 投影/断言；对非对角 ε 的 BE 路径加限制或警告。
4. **P1**：解决 N-05 的 `WARPX_MODULE_LAYERS`/anchor 硬编码，使新增插件不需要改核心 CMake；
   同步清理 `ParkedPlugins/BoundaryConditions/README.md` 中不存在的 `-DWarpX_PLUGIN_BC=ON`
   与错误路径。
5. **P2**：N-06 slug 唯一性检查（save 前检测同名不同源，拒绝或自动改名）。
6. **P2**：N-07/N-08 作为“潜在/接口正确性”问题，在重构 `use_node_eps` 之前必须先修。

## 8. 最终结论

本次审查在“过去 24 小时”对应的唯一提交树中，确认了 **N-01…N-08 共 8 项新增/新扩展缺陷**，
其中 **N-01、N-02 属于会静默改变物理结果的关键缺陷**，且与 2026-11-16 报告中仍未修复的
A-01/A-02/A-03 相互叠加。项目在“物理正确性”和“插件化契约”两条主线上都还存在系统性缺口：
界面/张量路径缺少独立验证，精确张量步的开关与物理场景不匹配，插件阶段上下文与构建期注册也不完整。

**本报告没有修改任何代码、构建文件、测试或数据；只新增了这份 Markdown 文档。**
