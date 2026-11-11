# 提交后正确性检查报告（2026-11-11）

## 0. 审计范围与方法

- 仓库：`/workspace`（WarpX 非官方插件化 fork）。
- 仓库为 depth=1 的浅克隆，`git log` 只有 **1 个提交**（HEAD，2026-11-11 09:38 +0800）。因此“过去 24 小时内的提交”在本仓库中就是 HEAD；本报告审计 HEAD 的整棵树，重点覆盖本 fork 自己引入的介质求解器、插件运行时与最近被点名的 external-field 家族。
- 方法：从输入解析 → 初始化 → 每步演化 → 诊断/校验，逐条追踪完整执行路径；同时对照 `Docs/source/usage/parameters.rst` 中仍保留的上游接口语义，以及 `AGENTS.md` 的并行/可移植性约束。
- **本报告只读代码，未修改任何项目文件，也未生成/修改任何输入文件。**

结论：发现多处**静默的物理正确性缺陷**，其中第一项直接使上游文档中的导体/损耗介质功能整体失效，第二项使 3D 与任意非 z 法向界面下的等效介质系数错误。不是“未发现关键缺陷”的情况。

---

## 1. 严重（P0）

### F1. 默认 Yee 宏观介质路径完全不施加电导率，`macroscopic_sigma_method` 成为死参数

**位置**
- [MacroscopicEvolveE.cpp](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicEvolveE.cpp#L253-L282)（Ex；Ey/Ez 同构，见 L293-L319、L340-L356）
- [MacroscopicProperties.cpp](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicProperties/MacroscopicProperties.cpp#L447-L455)（“回退到 Backward-Euler”的注释）
- [MacroscopicProperties.H](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicProperties/MacroscopicProperties.H#L102-L108)（`has_sigma_matrices()` 直接返回 `m_sigma_mat_ready`）
- [MacroscopicProperties.cpp](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicProperties/MacroscopicProperties.cpp#L929)（`m_sigma_mat_ready = true` 只在这里）
- [WarpXPushFieldsEM.cpp](file:///workspace/Source/Plugins/PicEm/Modules/FieldSolver/WarpXPushFieldsEM.cpp#L1240-L1247)（`InitSubpixelRows` 只在 `WarpX::macroscopic_subpixel` 为真时调用）

**执行路径与原因**

1. 用户按上游文档配置：`algo.em_solver_medium = macroscopic`、`macroscopic.sigma > 0`、`algo.macroscopic_sigma_method = laxwendroff`（或 `backwardeuler`），不设置 `algo.macroscopic_subpixel`（默认 0）。
2. `MacroscopicEvolveE` 分派到 `MacroscopicEvolveECartesian<LaxWendroffAlgo>` 或 `<BackwardEulerAlgo>`；模板参数只用于计算 `alpha`/`beta`。
3. L253 把节点电导率强制为 0：`sigma_node = has_sig_mat ? sn0(...) : 0`。而 `has_sig_mat = has_sigma_matrices()`，只有 `fill_sigma_mats()` 在 L929 才会把 `m_sigma_mat_ready` 置真；`fill_sigma_mats()` 只由 `InitSubpixelRows()` 调用；`InitSubpixelRows()` 又只在 `algo.macroscopic_subpixel = 1` 时调用。默认情况下 `has_sig_mat == false`。
4. 于是 L261 `sigma_interp = 0`，L265-L266 得到 `alpha = 1`、`beta = dt/eps`。
5. **L280-L282 的实际更新根本没有使用 `alpha` 和 `beta`**：写的是 `E += dt*(eps^-1 curl) - dt*diag(eps^-1) J`。`sigma_mf` 在这个分支中完全没有被读取。
6. 因此 `sigma > 0` 的格子与真空/无损介质完全同构：没有阻尼、没有欧姆损耗。

更糟的是，即使走进“有矩阵”的分支，`alpha`/`beta` 仍是死变量；L267-L270 的注释写明 `beta0 = dt/(1+s) = beta*eps`，但 L280-L282 只用了 `dt`，缺少 `1/(1+s)` 因子。也就是说 `macroscopic_sigma_method` 在**任何配置下都没有效果**：`sigma>0` 且 `subpixel=1` 时被节点指数积分器覆盖；`subpixel=0` 时被直接丢弃。

**影响（物理正确性，静默）**

- 导体内部场不衰减、无趋肤深度，反射/透射系数按无损介质计算；损耗介质不吸收能量，能量守恒与加热率全错。
- 启动日志仍打印 “Lax-Wendroff algorithm / Backward Euler algorithm”，L447-L455 的注释还声称“回退到一阶 Backward-Euler”，与实际行为矛盾，用户几乎不可能从输出察觉。
- 这是对上游 WarpX 已支持功能的直接回归，且影响所有仍按 `parameters.rst` 使用 `macroscopic.sigma` 的用户。

**验证思路（不改代码）**

用均匀有损介质（无界面）跑一维/二维衰减测试：固定 `eps`、取较大 `sigma`，设 `em_solver_medium=macroscopic` 且**不设** `subpixel`。若场幅度不随 `sigma` 变化，即证实该缺陷。再设 `subpixel=1` 对比，幅度应出现明显差异——这正说明默认路径丢失了全部损耗。

---

### F2. 节点等效介质只沿“第二个坐标”平均，且把该方向硬编码为界面法向；3D、x 法向与斜界面全部错误，`subpixel` 开关语义被破坏

**位置**
- [MacroscopicProperties.cpp](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicProperties/MacroscopicProperties.cpp#L463-L467)（注释直接写明 “z here”）
- [MacroscopicProperties.cpp](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicProperties/MacroscopicProperties.cpp#L497-L560)（`zn` 由 `j` 与 `dz_n` 构造，`phi` 在 `(xn, 0, zn)` 求值）
- [MacroscopicProperties.cpp](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicProperties/MacroscopicProperties.cpp#L497-L560)（`normal = (c==2)`；`iL=iR=i, jL=j-1, jR=j`）
- [MacroscopicProperties.cpp](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicProperties/MacroscopicProperties.cpp#L584-L590)（各向同性时 `f` 硬编码 0.5）
- [MacroscopicProperties.cpp](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicProperties/MacroscopicProperties.cpp#L670-L687)（`m_eps_inv_node` 直接由节点 `eps` 求逆）
- [MacroscopicEvolveE.cpp](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicEvolveE.cpp#L212-L222)（默认 `use_node_eps = true`，只有设置环境变量 `WARPX_YEE_CELL_EPS` 才使用 Kottke cell rows）

**执行路径与原因**

`MacroscopicProperties::InitData` 在**所有**宏观介质运行中无条件填充 `m_eps_node`/`m_sigma_node`（与 `subpixel` 无关），`m_eps_inv_node` 再由其求逆。`MacroscopicEvolveE` 在默认 `use_node_eps = true` 时，无论 `has_sub` 与否都使用 `ein0/ein1/ein2`（L277-L279）。因此：

- **2D XZ**：节点材料的 L/R 取的是 `(i,j-1)` 与 `(i,j)`，即只沿 z 方向；`normal` 也只看 `c==2`（Ez）。这对“法向为 z、且界面正好落在节点上”的层状介质是正确的，但：
  - x 法向界面（例如竖直 slab）在节点处的 z 邻居通常属于同一材料，`eL == eR`，法向分量 Ex 得不到谐波平均，切向分量也得不到跨界面平均；等效于把界面挪了半格并丢失亚网格平滑。
  - 斜界面：材料取的是 z 偏移 ±0.25dz 处的两点，而不是沿真实法向 `nrm` 的两侧；`f` 在节点分支被硬编码为 0.5，与真实交点位置无关。`chi_inv_anisotropic` 虽然拿到了真实 `nrm`，但输入的两侧材料本身取错，结果不可信。
- **3D**：`j` 是 y，`k` 才是 z，但代码把 `j` 当 z 用（`zn = zlo_n + j*dz_n`），并把 `phi` 的 y 参数写死为 0，`k` 完全不参与材料构造。于是所有节点的 `eps/sigma` 在 z 方向被 extrude、在 y 方向被当作 z 求值——**任何在 y 或 z 方向非均匀的 3D 宏观介质都是错的**（包括 Ez 的法向平均）。
- **1D**：会读取 `dxa[1]`，而 `CellSizeArray()` 的长度是 `AMREX_SPACEDIM`，属于越界读取（仅在 1D 张量输入时可达）。
- **`algo.macroscopic_subpixel` 语义被破坏**：节点平均在 `subpixel=0` 时仍然执行，且默认路径用的正是节点 `eps`。所以 `subpixel=0` 并没有关闭界面亚网格平均，它只关闭了 `m_eps_inv_*` 行和 sigma 矩阵；而后者默认又被 `use_node_eps=true` 绕过。真正的通用 Kottke 行（`fill_rows`，支持任意 2D 法向与逐格填充率）被环境变量 `WARPX_YEE_CELL_EPS` 关在默认路径之外。
  - 代码在 L373-L380 对“设置了 `interface_function` 但 `subpixel=0`”只发警告，不 abort；警告文本声称 “epsilon2/tensor material 2 will be ignored”，但 L463 之后的节点填充是无条件的，并且确实读取 `m_eps2_tensor_mf`/`phi`。也就是说：**警告描述的行为与实际行为相反**——material 2 没有被忽略，而是继续通过节点路径进入 E 更新。这属于诊断信息误导，会让用户以为“关闭 subpixel 就回到无平滑基线”，实际上并没有。

**影响（物理正确性，静默）**

- 2D 竖直/斜界面、3D 任意非均匀介质的有效 `ε`、`σ` 全错：Fresnel 反射/透射、界面处法向/切向场匹配、导体穿透深度都会出现 O(1) 误差，而不是亚网格精度级别的修正。
- 由于现有测试只覆盖“z 法向层状”和“均匀 3D”（见 §4），这些错误不会在 CI 中暴露；代码也没有对 3D 或非 z 法向界面做任何 abort/警告。

---

### F3. 3D 张量解析函数的填充坐标错误：`(x, 0, z=f(y))`，且完全忽略 `k`

**位置**
- [MacroscopicProperties.cpp](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicProperties/MacroscopicProperties.cpp#L424-L438)

**执行路径与原因**

`InitData` 对 `epsilon_tensor_*`、`epsilon2_tensor_*`、`sigma_tensor_*` 的每个分量调用同一个 `fill_one`：`x=(i+0.5)*dx+lox`，`z=(j+0.5)*dxa[1]+loz`，然后 `pex(x, 0.0, z)`。在 2D XZ 中 `dxa[1]==dz`、`j` 是 z，正确；在 3D 中 `j` 是 y、`dxa[1]==dy`、`k` 是 z，于是：

- 解析函数的 y 实参恒为 0；
- 第三个实参拿到的是“用 y 索引和 dy 网格算出来的假 z”；
- 结果与真实 `k`（z）无关。

标量 `epsilon_function`/`sigma_function` 的 3D 分支是正确的（见同文件 L759 之后的 3D 分支），只有张量分支是 2D 专用，二者不一致，说明这不是有意的坐标约定。

**影响（物理正确性，静默）**

- 任何 3D 空间变化的张量介质（例如 `epsilon_tensor_xx = "2 + 0.1*sin(z)"` 或只依赖 y 的各向异性）都会被填充成错误的场；常数张量不受影响，因此当前 3D 测试（均匀介质）无法发现。
- 这是 fork 的“张量介质”核心新功能在 3D 下不可用的直接原因之一。

---

## 2. 高优先级（P1）

### F4. 指数积分器的 `eold3` 快照在边界面上读取未初始化内存

**位置**
- [MacroscopicEvolveE.cpp](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicEvolveE.cpp#L382-L388)（`define` + `ParallelCopy`）
- [MacroscopicEvolveE.cpp](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicEvolveE.cpp#L406)（`eo = eold3[c].const_array`）
- [MacroscopicEvolveE.cpp](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicEvolveE.cpp#L416-L430)（在 `Efield[c]` 的完整 valid box 上读取 `eo(...,d)`）

**执行路径与原因**

`eold3[c]` 定义在 `Efield[c]->boxArray()` 上（c 方向为节点），0 层 ghost；随后把 `Efield[d]` 的 **valid box** 逐分量拷入。Yee 各分量的 valid box 尺寸不同：以 2D XZ 的 Ex 为例，Ex 在 x 方向多一层 `i=nx`，而 Ey/Ez 的 valid box 只到 `nx-1`。因此 `eold3[0]` 在 `i=nx` 平面上、分量 d=1,2 不会被任何 `ParallelCopy` 覆盖（交集之外的区域保持未初始化；若 AMReX 在 debug 下做包含性断言则直接 abort）。而 kernel 恰好在 Ex 的完整 valid box（包含 `i=nx`）上读取 `eo(i,j,k,1)`、`eo(i,j,k,2)`。

**影响（物理正确性，静默/不确定）**

- 对**各向异性** `ε`/`σ`（M1 非对角非零），边界平面上的 E 会用垃圾值参与矩阵乘法，产生错误场，并沿波传播污染全域。
- 对各向同性介质，非对角项为 0；但若未初始化内存恰好是 NaN 位型，`0 * NaN = NaN`，仍会在边界产生 NaN。当前 lossy/conductor 测试是 2D s-pol，且 `sigma>0` 的那个分量恰好是完整覆盖的分量，所以测试通过属于侥幸。
- 该问题在 3D 中每个分量的高边界面都存在，是 tensor 功能的核心路径之一。

> 说明：L377-L381 的注释已经承认“快照在显式更新之后、M1 对角时该项不可见”，这属于已知近似；但**边界未初始化读取**并未被承认，也不是近似，而是未定义行为。

### F5. 各向异性电导/电流耦合被丢弃

**位置**
- [MacroscopicEvolveE.cpp](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicEvolveE.cpp#L277-L282)（Ex；Ey/Ez 见 L293-L319、L340-L356）
- [MacroscopicEvolveE.cpp](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicEvolveE.cpp#L253-L254)（对角 `sigma_node` 作为跳过条件）
- [MacroscopicEvolveE.cpp](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicEvolveE.cpp#L417)（`sn(i,j,k,c) <= 0` 返回）

**原因与影响**

1. **粒子电流只乘对角元**：L280-L282 对 Ex 只减 `dt*ix0*jx`，Ey 只减 `dt*iy1*jy`，Ez 只减 `dt*iz2*jz`。而 Ampere 定律的正确离散更新是 `E += dt*ε⁻¹(curlH - J)`，同一个 `ε⁻¹` 必须同时作用在 `curlH` 和 `J` 上。当前写法对非对角 `ε⁻¹` 不一致：curl 项用了完整张量，J 项只用了对角。L267-L270 的注释称“J 的非对角耦合是二阶的”，但 `ε⁻¹J` 与 `ε⁻¹curlH` 同阶，并非 `O(h²)`。后果是各向异性介质中束流/粒子电流的响应、离散 Gauss 定律与电荷守恒都不正确。
2. **纯非对角电导率被完全忽略**：节点是否走指数步只看 `sn(i,j,k,c)`（该分量的对角元）。一个合法的 Hall/旋磁电导率张量若对角元为 0、非对角元非 0，则 `sigma_node <= 0`，指数步被跳过；显式分支又因为 `has_sig_mat` 或 `sigma_node` 的门控不施加任何 `σ`。用户提供的各向异性电导率被静默丢弃。

---

### F6. Gauss 校验与 `rho_medium` 在 3D 下不是 3D 散度；诊断还可能因数据竞争而“假通过”

**位置**
- [WarpXPushFieldsEM.cpp](file:///workspace/Source/Plugins/PicEm/Modules/FieldSolver/WarpXPushFieldsEM.cpp#L1262-L1312)（种子：只算 x、z）
- [WarpXPushFieldsEM.cpp](file:///workspace/Source/Plugins/PicEm/Modules/FieldSolver/WarpXPushFieldsEM.cpp#L1335-L1363)（更新：只用 Jx、Jz）
- [WarpXPushFieldsEM.cpp](file:///workspace/Source/Plugins/PicEm/Modules/FieldSolver/WarpXPushFieldsEM.cpp#L1369-L1427)（校验：只算 Dx、Dz；`Ey2` 未 FillBoundary；`amrex::Gpu::Atomic::Max` 位于 `ParallelFor`）

**原因与影响**

- 3D 中 `div D` 需要 `∂Dx/∂x + ∂Dy/∂y + ∂Dz/∂z`，而种子、`rho_medium` 更新、残差校验都只使用 x 与 z。`rho_medium` 因此不是真实电荷密度，校验也不是 3D Gauss 定律；真正的 3D 电荷守恒误差无法被发现。
- 校验只对 `Ex2`、`Ez2` 调 `FillBoundary`，但 `Dx = en0(0)*ex + en0(1)*ey + ...`、`Dz = ... + en2(1)*ey + ...` 在非对角 `ε` 下会使用 `ey` 的邻居值；`Ey2` 的 ghost 未更新，周期/瓦片边界处的残差会用陈旧值（各向异性介质下静默错误）。
- [AGENTS.md](file:///workspace/AGENTS.md) 明确要求：跨迭代写同一内存（尤其 whole-loop sum/max）必须用 `amrex::Reduce`/`amrex::HostDevice::Atomic`，`amrex::Gpu::Atomic` 在 CPU 上并非原子，不能使 `ParallelFor` 安全。这里在 `ParallelFor` 内用 `amrex::Gpu::Atomic::Max` 归约 `pmax`，在 CPU/OpenMP 构建下是数据竞争，最大值可能被低估；单元测试中这意味着 Gauss 校验可能**假通过**，从而掩盖 F1/F2/F5 的真实误差。
- 校验间隔硬编码 `%20`，而研究笔记/文档声称由 `macroscopic.gauss_check_interval` 控制（该键并未被解析）。

> 该问题主要影响诊断，不直接改变 E/B 演化；但它是用来“证明新介质求解器正确”的唯一运行时校验，失效后会放大前述物理 bug 的不可见性。

---

## 3. 中优先级（P2）

### F7. 动态 regrid 不重建材料；moving window 下子网格矩阵与 `rho_medium` 未更新

**位置**
- [MacroscopicProperties.cpp](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicProperties/MacroscopicProperties.cpp#L318-L350)（`AllocateLevelMFs` 重新分配并清零所有 `ε`/`σ`/`μ` 与节点矩阵）
- [WarpX.cpp](file:///workspace/Source/Plugins/PicEm/WarpX.cpp#L2857)（`AllocLevelMFs` 每次 regrid 都会调用它）
- [WarpXRegrid.cpp](file:///workspace/Source/Plugins/PicEm/Modules/Parallelization/WarpXRegrid.cpp#L177-L230)（`RemakeLevel` 仅 `ClearLevel`→`AllocLevelData`→`InitLevelData`，**没有** `m_macroscopic_properties->InitData`）
- [WarpXMovingWindow.cpp](file:///workspace/Source/Plugins/PicEm/RuntimeBridge/Utils/WarpXMovingWindow.cpp#L705-L714)（moving window 会调用 `m_macroscopic_properties->InitData(...)` 重算材料，但**不**重建 `InitSubpixelRows`）
- [WarpXPushFieldsEM.cpp](file:///workspace/Source/Plugins/PicEm/Modules/FieldSolver/WarpXPushFieldsEM.cpp#L1235-L1247)（`InitSubpixelRows` 只在 `!has_subpixel_rows()` 时调用一次）
- [WarpX.cpp](file:///workspace/Source/Plugins/PicEm/WarpX.cpp#L2998-L3005)（`rho_medium` 仅在此处分配）
- 全仓库未发现 moving-window/regrid 对 `macroscopic` 的显式禁止，也未发现 `rho_medium` 的平移/复制。

**原因与影响（两条路径需分开看）**

- **动态 regrid（`RemakeLevel` 且 `ba != boxArray(lev)`）**：材料 `InitData` 不会被重新执行。`AllocLevelMFs` 把 `ε`/`σ`/`μ`、`m_eps_node`、张量矩阵全部 `setVal(0)` 后，新的 `AllocLevelData` 只重算场与 `rho_medium`（清零），旧材料被丢弃。随后 `inv3_sym`/`sigma_exponential_matrices` 在 Release 下（`AMREX_ASSERT_WITH_MESSAGE` 被编译掉）对零行列式求逆，产生 Inf/NaN，或 `ein0==0` 直接把 E 乘成 0。`rho_medium` 重新分配为 0，但 `m_rho_medium_seeded` 仍为真，种子步骤被跳过；即便重算也缺少 y 方向散度（见 F6）。这是静默失效。
- **moving window**：`WarpXMovingWindow.cpp` 在平移后会重新调用 `MacroscopicProperties::InitData`，因此 `ε`/`σ` 本身是**按新窗口重算**的，不存在“材料随窗口运动”的问题（这一点与早期草稿的猜测不同，已更正）。但存在两个残留问题：
  1. `InitSubpixelRows` 只在 `WarpXPushFieldsEM.cpp` 的 `!has_subpixel_rows()` 分支里调用；`m_subpixel` 一旦为真就再也不会重建。窗口移动后 `M1`/`M2`、`m_eps_inv_*`、`m_sigma_*` 仍是旧网格/旧 `dt` 的值；若 `dt` 随窗口自适应变化（`WarpX::dt` 可由 `dt_update` 触发），指数积分器会使用错误的 `dt`，导致 E 演化系统性偏差。
  2. `rho_medium` 既不平移也不重新种子化（`AllocateLevelMFs` 未重新调用），窗口滑过新区域后 `rho_medium` 与 `InitData` 重算出的新 `ε` 不再满足 `∇·D = rho_medium`，Gauss 校验（F6）会读到一个与当前材料不匹配的历史残差。
- 两类配置都不会 abort，属于静默失效；建议确认 `macroscopic` 是否应显式禁止（至少应禁止动态 regrid）或补齐重建逻辑。

### F8. 张量输入校验既会误杀合法输入，也会放过奇异 `epsilon2_tensor`

**位置**
- [MacroscopicProperties.cpp](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicProperties/MacroscopicProperties.cpp#L133-L150)（在 `(0,0,0)` 处判断“全零”）
- [SubpixelTensor.H](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicProperties/SubpixelTensor.H#L188-L190)（`AMREX_ASSERT_WITH_MESSAGE`）
- [SubpixelTensor.H](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicProperties/SubpixelTensor.H#L313-L346)（`AMREX_ASSERT_WITH_MESSAGE` 检查 `ε` 正定）

**影响**

- `epsilon_tensor_xy = "x"` 这类在原点恰为 0 的合法空间函数，会因为只检查 `(0,0,0)` 而被误判为“全零”并 abort（`sigma_tensor` 同理）。
- 只指定部分 `epsilon2_tensor_*`（例如仅 `xx`）时，其余对角元默认 0，`epsilon2` 矩阵奇异；`chi_inv_*`/`inv3_sym` 会除零。`AMREX_ASSERT_WITH_MESSAGE` 在 Release 下不生效，结果是静默 NaN，而不是 abort。
- 这是典型的“abort 路径”缺陷：该 abort 的没 abort，不该 abort 的却 abort。

### F9. 文档与代码漂移，进一步掩盖 F1/F2

**位置**
- [MACROSCOPIC_INPUTS.md](file:///workspace/Docs/research_notes/MACROSCOPIC_INPUTS.md)（宣传 `algo.macroscopic_solver = exponential`）
- [parameters.rst](file:///workspace/Docs/source/usage/parameters.rst#L4068-L4105)（仍把 `macroscopic_sigma_method` 作为有效选项）
- 源码中不存在 `macroscopic_solver` 的 ParmParse；`gauss_check_interval` 也不存在（硬编码 `%20`）。

**影响**

- 用户按研究笔记设置 `algo.macroscopic_solver=exponential` 并不会切换任何东西；真正的开关是未文档化的 `algo.macroscopic_subpixel`。若只设前者，则落入 F1 的“无损耗”路径，且用户以为已经启用指数积分器。

---

## 4. 项目根本目标（插件化）层面的系统性缺陷

> 这些不是“某一行算错”，而是与“核心/插件仅通过接口与总线解耦、用户可独立编写插件”的根本目标相冲突的设计事实。

### A1. 服务总线是单槽位、后注册覆盖，且无冲突检测

**位置**：[ServiceFactory.cpp](file:///workspace/Source/Core/Runtime/ServiceFactory.cpp#L28-L52)、[ServiceFactory.cpp](file:///workspace/Source/Core/Runtime/ServiceFactory.cpp#L149-L166)、[ServiceFactory.cpp](file:///workspace/Source/Core/Runtime/ServiceFactory.cpp#L217-L229)

- `Registry` 对每种服务只有一个函数指针；`register_*_factory()` 直接赋值。两个插件（例如两个诊断插件、两个介质模型插件）同时存在时，**最终生效者由静态初始化/链接顺序决定**，没有名字、优先级或冲突报错。
- 用户无法用现有总线在同一构建中“选择”多个实现；这与“用户按需求编写新功能插件”的扩展性目标不符。
- `clear_service_factories()` 只清空 `Registry`，但 `legacy_runtime_data()`/`legacy_host_services()` 的函数内 `static` holder 不会被重置；测试或插件热重载后仍指向旧提供者。

### A2. 介质模型不是真正的插件边界

**位置**：[MacroscopicEvolveE.cpp](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicEvolveE.cpp#L51-L57)（`dynamic_cast<MacroscopicProperties*>` 与 abort）、[MacroscopicEvolveE.cpp](file:///workspace/Source/Plugins/FieldSolver/FiniteDifference/MacroscopicEvolveE.cpp#L124-L130)

- 入口签名接收 `IMacroscopicProperties&`，但立即 `dynamic_cast` 到具体类；任何第三方 `IMacroscopicProperties` 实现都会触发 “expects the material model of this plugin” abort。
- 具体类 `MacroscopicProperties` 和其工厂都编译在 FieldSolver 插件内。因此“换一个材料模型插件”必须修改 FieldSolver 插件源码，无法仅通过接口/总线接入。
- 代码注释显示这是有意的临时设计（“material model can move into its own plugin”），但它确实违背了本次任务陈述的根本目标；应在架构上记录为已知限制，而不是当作已实现的插件化。

---

## 5. 已检查但未确认的问题

- **external-field 家族重构**（`ExternalField.cpp`、`ExternalParticleFields.cpp`、`GetExternalFields.H`）：逐条对照了 E/B 解析器类型、时间插值、粒子侧 `getExternalEB` 的取用与单位，未发现新的确定性缺陷；与提交信息中 “24/24 通过” 不矛盾。
- **张量矩阵指数本身**：`sigma_exponential_matrices` 的 `ε^{-1/2}…ε^{-1/2}` 变换、`J1 = ε(I-C)ε^{-1}` 的相似变换推导是正确的；问题在输入材料、坐标、边界快照和电流项，而不在 `SubpixelTensor.H` 的矩阵函数主体。
- **上游 macroscopic 测试**：`Examples/Tests/macroscopic_solver` 使用均匀 `ε`、`σ=0`，因此无法覆盖 F1–F5；fork 的 lossy/conductor 用例强制 `subpixel=1` 且为 2D s-pol，无法覆盖 F1、F4、F5。3D 用例使用均匀 `ε`，无法覆盖 F2/F3。
- **构建/运行**：本次为静态追踪，未编译（仓库无 build 目录，且本环境不适合拉取 AMReX 依赖）；因此未把“编译告警”当作结论。F1 中 `alpha`/`beta` 未使用在开启 `-Werror=unused-variable` 的构建上还会直接导致编译失败，这本身也说明该重构未完成。

---

## 6. 建议的验证顺序（只读，不修改代码）

1. **F1**：一维均匀有损介质，`sigma` 取大值，分别测 `subpixel=0` 与 `subpixel=1` 的场衰减；两者若在 `subpixel=0` 时完全不衰减，即确认。
2. **F2**：2D 竖向（x 法向）界面与 45° 斜界面，`subpixel=1`，对比解析 Fresnel 系数；重点看 Ex/Ez 的节点系数是否随法向变化。
3. **F3**：3D 中令 `epsilon_tensor_xx` 依赖 y 或 z，检查 `MacroscopicProperties` 初始化后的张量场是否在对应方向变化（可在调试器/诊断中观察）。
4. **F4**：3D 各向异性 `ε` + 边界处 `σ>0`，检查高边界面 E 是否出现异常/NaN；或开启 AMReX 内存初值填充以暴露未初始化读取。
5. **F6**：3D 下构造已知电荷分布，比较 `rho_medium` 与真实 3D 电荷；并在 CPU/OpenMP 构建下重复运行 Gauss 校验，观察最大值是否随线程数变化。

---

*本报告只陈述缺陷与证据，未对任何代码或项目文件做修改。*
