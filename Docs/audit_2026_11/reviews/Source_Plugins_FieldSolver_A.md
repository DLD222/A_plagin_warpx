# Source_Plugins_FieldSolver_A 审查

**结论速览**：覆盖 50/50 文件（16489 行）；缺陷 7 条 = critical 0 / major 3（F1,F2,F3）/ minor 4（F4–F7）。
最该先看：F1 `MacroscopicEvolveE.cpp:267`（`macroscopic_solver=laxpw` 被静默降级为后向 Euler）、
F2 `EvolveE.cpp:98`（ECT 共形路径丢掉 `grad(F)` 散度清理）、F3 `ElectrostaticSolver.cpp:356`（丢了 `-1`，径向几何下 `E_r`→0）。
另有 5 处与上游不同但经推导确认是**修上游 bug**，见文末"反向核对"。

分区范围：`Source/Plugins/FieldSolver/{Electrostatic,FiniteDifference,Implicit}`，50 个文件、16489 行。
参照：`_upstream/warpX-26.10/Source/FieldSolver/…` 与 `_upstream/warpX-26.10/Source/NonlinearSolvers/…`（按文件名匹配，
`Implicit/NonlinearSolvers/*` 的上游对应在 `Source/NonlinearSolvers/`，不在 `ImplicitSolvers/` 下——这一点很容易漏掉）。

方法说明（可核验）：对 34 个在上游找到同名文件的目标，全部做 `diff -u` 并**读完整个 diff**；diff 只有 include
路径 / `WarpX::` → `config()/data()` 机械改名的文件，等价于全文无其它改动。有实质语义改动的文件另外用
read 工具逐段读完本地实现（EvolveE.cpp、EvolveB.cpp、EvolveECTRho.cpp、EvolveF.cpp、MacroscopicEvolveE.cpp、
ElectrostaticSolver.cpp、PoissonBoundaryHandler.*、LabFrameExplicitES.cpp、DarwinLinearFieldOperator.cpp、
ImplicitSolver.cpp 相关函数、AlgMGPC.H）。16 个无上游对应的新文件按"本地新写"处理，通读或抽样通读。

## 覆盖

| 文件 | 行数 | 类型 | 结论 |
|---|---|---|---|
| Electrostatic/EffectivePotentialES.H | 72 | modified | 可疑(F4 相关调用点) |
| Electrostatic/EffectivePotentialES.cpp | 300 | modified | **可疑(F4)** |
| Electrostatic/ElectrostaticSolver.H | 186 | modified | OK |
| Electrostatic/ElectrostaticSolver.cpp | 629 | modified | **缺陷(F3)** |
| Electrostatic/ElectrostaticSolverFactory.cpp | 46 | new | OK |
| Electrostatic/LabFrameExplicitES.H | 43 | modified | OK |
| Electrostatic/LabFrameExplicitES.cpp | 348 | modified | OK（含一处修上游 bug，见"反向核对"） |
| Electrostatic/PoissonBoundaryHandler.H | 147 | modified | OK |
| Electrostatic/PoissonBoundaryHandler.cpp | 206 | modified | OK（RZ `dim_start` 改动是修复） |
| Electrostatic/RelativisticExplicitES.H | 78 | modified | OK |
| Electrostatic/RelativisticExplicitES.cpp | 187 | modified | OK（deposit→finalize 等价性已核对） |
| FiniteDifference/ApplySilverMuellerBoundary.cpp | 394 | moved | OK |
| FiniteDifference/ComputeCurlA.cpp | 460 | modified | OK |
| FiniteDifference/ComputeCurlB.cpp | 231 | modified | OK |
| FiniteDifference/ComputeDivE.cpp | 257 | moved | OK |
| FiniteDifference/ComputeGradient.cpp | 182 | modified | OK |
| FiniteDifference/ComputeLaplacian.cpp | 531 | moved | OK |
| FiniteDifference/EvolveB.cpp | 615 | modified | OK（XZ ECT 是修复，见"反向核对"） |
| FiniteDifference/EvolveBPML.cpp | 192 | moved | OK |
| FiniteDifference/EvolveE.cpp | 757 | modified | **缺陷(F2)**、新增大段 ECT 共形内核 |
| FiniteDifference/EvolveECTRho.cpp | 168 | modified | OK（XZ 公式复核正确） |
| FiniteDifference/EvolveEPML.cpp | 260 | modified | OK |
| FiniteDifference/EvolveF.cpp | 299 | **modified** | OK（RZ 虚部指标改动是修复） |
| FiniteDifference/EvolveFPML.cpp | 130 | moved | OK |
| FiniteDifference/EvolveG.cpp | 113 | moved | OK |
| FiniteDifference/FiniteDifferenceSolver.H | 697 | modified | OK（ngrow 接口 + ECT 新增声明） |
| FiniteDifference/FiniteDifferenceSolver.cpp | 148 | modified | 可疑（`config()` 空上下文回退，见复核项） |
| FiniteDifference/FiniteDifferenceSolverFactory.cpp | 26 | new | OK |
| FiniteDifference/HybridPICSolveE.cpp | 1594 | modified | 可疑（Faraday 中加 grad Pe，物理改动） |
| FiniteDifference/MacroscopicEvolveE.cpp | 469 | **modified** | **缺陷(F1)** |
| FiniteDifference/MacroscopicProperties/MacroscopicProperties.H | 342 | modified | 可疑(F6) |
| FiniteDifference/MacroscopicProperties/MacroscopicProperties.cpp | 1214 | modified | 可疑(F6) |
| FiniteDifference/MacroscopicProperties/MacroscopicPropertiesFactory.cpp | 35 | new | OK |
| FiniteDifference/MacroscopicProperties/SubpixelTensor.H | 430 | new | **可疑(F7)**；Kottke tau/特征分解数学已逐式核对正确 |
| Implicit/DarwinLinearFieldOperator.H | 158 | modified | OK |
| Implicit/DarwinLinearFieldOperator.cpp | 216 | **modified** | OK（删掉一次 FillBoundaryAndSync，论证成立） |
| Implicit/ImplicitOptions.H | 4 | moved | OK（转发 include） |
| Implicit/ImplicitSolver.H | 479 | modified | 可疑(F5) |
| Implicit/ImplicitSolver.cpp | 1525 | **modified** | **缺陷(F5)** + 新增自适应子步 OneStep |
| Implicit/MassMatrixSolverOps.H | 415 | new | 未发现明显错（新特性，仅抽样） |
| Implicit/NonlinearSolvers/AMReXGMRES_Wrapper.H | 113 | modified | OK |
| Implicit/NonlinearSolvers/AlgMGPC.H | 162 | new | OK |
| Implicit/NonlinearSolvers/CurlCurlMLMGPC.H | 382 | modified | OK |
| Implicit/NonlinearSolvers/DarwinMLMGPC.H | 384 | modified | OK（注释/张量对角化说明改写） |
| Implicit/NonlinearSolvers/ExternLibPC.H | 158 | new | OK |
| Implicit/NonlinearSolvers/JacobiPC.H | 140 | **modified(重写)** | 可疑（委托 `WeightedJacobiImpl`，行为等价性未验证） |
| Implicit/NonlinearSolvers/JacobianFunctionMF.H | 242 | modified | OK（新 PC 类型分发） |
| Implicit/NonlinearSolvers/LinearSolver.H | 94 | modified | OK |
| Implicit/NonlinearSolvers/MatrixPCAlgMG.H | 67 | new | OK |
| Implicit/NonlinearSolvers/MatrixPCAlgMG.cpp | 164 | new | OK |

## 缺陷清单

### F1 宏观介质 E 推进：`algo.macroscopic_sigma_method`（Lax-Wendroff）被静默忽略，永远走一阶 Backward-Euler

> **已修复**（commit `ddfddf4`）：参数名是 `algo.macroscopic_sigma_method`（不是 `warpx.macroscopic_solver`）；
> 现在两种格式各自通过 `coefficients_from_inv_eps` 生效，`Ex` 在损耗算例上相对差 44%。
- 文件:行 `Source/Plugins/FieldSolver/FiniteDifference/MacroscopicEvolveE.cpp:267-268`、`:314-315`、`:356-357`（计算）与 `:285-286`、`:326-328`、`:368-370`（实际使用处）
- 严重度: **major**
- 置信度: **confirmed**
- 现象与依据: 三个分量内核都写成
  `alpha = T_MacroAlgo::alpha(sigma,eps,dt); beta = T_MacroAlgo::beta(...)`，随后**从未使用**这两个变量，更新式硬编码为
  `Ex = (Ex + dt*(eps^-1 curlH) - dt*eps^-1*jx) / (1 + dt*sigma*eps^-1)`。
  上游同一处是 `Ex = alpha*Ex + beta*(curlH) - beta*jx`（`_upstream/.../MacroscopicEvolveE.cpp:186-191`）。
  两种 `T_MacroAlgo` 的系数不同：`LaxWendroffAlgo::alpha=(1-σdt/2ε)/(1+σdt/2ε)`（二阶） vs
  `BackwardEulerAlgo::alpha=1/(1+σdt/ε)`（`MacroscopicProperties.H:286-336`）。
  内联式与 `BackwardEulerAlgo` 展开完全一致 ⇒ 用户设 `warpx.macroscopic_solver = laxpw` 时结果与隐式 Euler 逐位相同，
  二阶阻尼精度消失（σdt/ε ~ O(1) 时差异显著）。`alpha/beta` 成为未使用变量，是该重构的痕迹。
  影响面：默认值就是 `BackwardEuler`（`Source/Core/Utils/WarpXAlgorithmSelection.H:46-49`），所以默认算例不受影响；
  但 `Examples/Tests/macroscopic_solver/` 的 deck 都没写 `macroscopic_solver`，即**没有任何回归测试覆盖 Lax-Wendroff**，
  该退化不会被 CI 抓到。
- 建议修法: 在求 `D` 之前用 `alpha/beta` 恢复上游写法
  `E = alpha*E + beta*(Dx computed at same staggering) - beta*jx`；或在不支持时于 `ReadParameters` 里对
  Lax-Wendroff 直接 abort 并给出现有 PC 限制提示。最小改动是恢复 `alpha*E + beta*(D) - beta*j`。
- 验证方法: 在 `Examples/Tests/macroscopic_solver/` 的某个 deck 里加 `warpx.macroscopic_solver = laxpw`（该参数当前无人使用），
  对比改动前后 `Ex` 的 checksum——现在会与后向 Euler 逐位相同；或读 `MacroscopicEvolveE.cpp` 中 `alpha` 的所有引用
  （3 处定义、0 处使用）。

### F2 ECT 共形 Ampere 路径丢掉 div(E) 清理项 `grad(F)`，F 场只演化不生效
- 文件:行 `Source/Plugins/FieldSolver/FiniteDifference/EvolveE.cpp:96-98`（分发）与 `:245-256`（新函数签名无 Ffield）
- 严重度: **major**（仅 `algo.ect_conformal_E > 0` 时）
- 置信度: **confirmed**（代码路径可读；影响面由该开关决定）
- 现象与依据: `EvolveE` 在 `m_fdtd_algo==ECT && config().ect_conformal_E()>0` 时调用
  `EvolveECartesianECT(fields, Efield, Bfield, Jfield, eb_update_E, lev, dt, nullptr)`，函数**没有** `Ffield` 参数；
  而 Yee/CKC/nodal 分支都会在后面追加 `E += c2*dt*grad(F)`（同文件 `:213-233`）。
  当 `warpx.do_dive_cleaning = 1` 时 `EvolveF` 仍被调用、`F` 仍在演化，却永远不会反馈到 E ⇒ 电荷守恒误差不再被修正，
  而且没有任何警告（比 abort 更危险）。另外 `Lev` 上的 `cost` 计时也被跳过。
- 建议修法: 给 `EvolveECartesianECT` 增加 `amrex::MultiFab const* Ffield` 并在三个分量内核后追加
  `E += c2*dt*UpwardD*(F)`（注意 `Ex/Ey/Ez` 的 `Ld` 归一化与 `F` 的节点交错）；或在 `ReadParameters` 中
  abort："ect_conformal_E 与 do_dive_cleaning 不能同时使用"。
- 验证方法: 跑一个 2D XZ、`algo.em_solver=ect`、`algo.ect_conformal_E=1`、`do_dive_cleaning=1` 的算例，检查
  `max|div E - rho/eps0|` 是否随时间线性增长；或直接对比 Yee 路径。

### F3 静电场 1D 分支（`computeE` 的 `#else`）丢失 `-1` 项：共位网格 + 径向几何时 `E_r` 退化为 `β_xβ_z·∂φ`
- 文件:行 `Source/Plugins/FieldSolver/Electrostatic/ElectrostaticSolver.cpp:356`
- 严重度: **major（若该分支可在径向几何下到达）/ minor（否则）**
- 置信度: **needs-verification**（静态推导可靠，可达性取决于 grid_type；我未构建）
- 现象与依据: 该行由上游
  `+(beta_x*beta_z-1._rt)*0.5_rt*inv_dz*(phi_arr(i+1)-phi_arr(i-1))`
  改为 `+beta_x*beta_z*0.5_rt*inv_dz*(...)`（`_upstream/.../ElectrostaticSolver.cpp:324`）。
  同行 `Ey` 用 `β_yβ_z`、`Ez` 用 `β_z²−1` 未改动；改后 `Ex` 与 `Ey` 同构、失去了产生 `E=−∇φ` 的那个 `−1`
  （上游 1D 分支里 `Ez` 的 `−1` 就承担这个角色，且 1D 下 `Ex` 与 `Ey` 本就是横向死分量）。
  该 `#else` 分支同时覆盖 `WARPX_DIM_1D_Z`、`RSPHERE`、`RCYLINDER`（同文件 `:314/:334/:352`）；
  在 `RSPHERE`/`RCYLINDER` 中 `Efield_fp[0]` 正是径向场 `Er`（`:209-212`、`ablastr/fields/PoissonSolver.H:245`），
  β=0（实验室系）时上游给 `Er=−∂φ/∂r`（正确），本改动给 `Er=0`。
  可达性：仅当三个 E 分量全为 node 型才进"nodal solver"分支，即 `warpx.grid_type = collocated`
  （`Source/Plugins/PicEm/WarpX.cpp:2811-2816` 把 E 全设为 nodal）。`1D_Z` 下 `Ex` 是死分量，所以常见 1D ES 测试不会暴露。
- 建议修法: 恢复 `(beta_x*beta_z - 1._rt)`；若要区分几何（径向 E 在分量 0），应显式 `#if` 分支而不是靠丢对角项。
- 验证方法: 1D RSPHERE（或 1D_Z 加 `grid_type=collocated`）静电解，取 `phi = x^2`，解析 `Er = -2x`；
  打印 `Efield_fp_x` 是否为 0。`grep -rn "RSPHERE" Regression/` 选一个球面 ES 用例加 `grid_type=collocated` 复现。

### F4 有效势 ES：rho 的边界折叠/RZ 体权标被挪到 sum 之后
- 文件:行 `Source/Plugins/FieldSolver/Electrostatic/EffectivePotentialES.cpp:168-173`
- 严重度: minor
- 置信度: likely
- 现象与依据: 上游 `pc->GetChargeDensity(lev, true)`（finalize=true：PEC/PMC 镜像电荷折叠 + RZ `1/(2πr)` 体权标）
  之后才 `ApplyFilterandSumBoundaryRho`（guard→valid 求和）。本改动改为
  `GetChargeDensityOnLevel(lev, true, /*finalize=*/false)` → `applyFilterandSumBoundaryRho` → `finalizeRhoOnLevel`，
  即镜像电荷折叠发生在 guard 已并入 valid 之后：`PEC::ApplyReflectiveBoundarytoRhofield`
  （`Source/Plugins/PicEm/Modules/BoundaryConditions/WarpX_PEC.cpp:730+`，"Step 1: Reflect Rho deposited to guard cells
  back into the domain"）读 guard、写 valid，此时不再被 filter 看到；RZ 体权标也作用在"已求和"的值上而非各 guard 自身。
  上游注释明确"boundary condition and inverse volume scaling is done within GetChargeDensity"（该注释在本地保留但已与代码不符）。
- 建议修法: 把 `finalizeRhoOnLevel` 提到 `applyFilterandSumBoundaryRho` 之前（恢复上游次序）。
- 验证方法: `ctest -R effective_potential`（含 PEC 边界用例）+ RZ 用例；比较 rho 在 PEC 边界相邻单元的镜像贡献。

### F5 隐式求解器：`mass_matrices_pc_width` 默认 0→1，叠加新 abort，3D Villasenor 旧算例会直接中止
- 文件:行 `Source/Plugins/FieldSolver/Implicit/ImplicitSolver.cpp:745`（默认值）与 `:877-881`（新 abort）
- 严重度: minor（工作区可通过显式参数绕开，但会打断既有 deck）
- 置信度: likely
- 现象与依据: 上游 `m_mass_matrices_pc_width = 0;` 改为 `1;`；同时新增分支在
  `WARPX_DIM_3D` + Villasenor 沉积 + `use_mass_matrices_pc` 且 `mass_matrices_pc_width > 0` 时
  `WARPX_ABORT_WITH_MESSAGE("... pc width > 0 with the Villasenor deposition in 3D is not yet supported ...")`。
  两个改动叠加 ⇒ 既有 3D 隐式算例（`implicit_evolve.use_mass_matrices_pc = 1` + `algo.current_deposition = villasenor`）
  从"能跑"变成"启动即 abort"，且需用户显式设 `mass_matrices_pc_width = 0`。
- 建议修法: 默认值保持 0；或在检测到 3D+Villasenor 时把 `m_mass_matrices_pc_width` 降为 0 并记录 warning，
  而不是 abort。
- 验证方法: `grep -r use_mass_matrices_pc Examples/ Regression/`，取一个 3D 隐式+LT 用例运行；或 `ctest -R implicit`。

### F6 新张量材料填充只按二维取样（3D 下 y 方向冻结）
- 文件:行 `Source/Plugins/FieldSolver/FiniteDifference/MacroscopicProperties/MacroscopicProperties.cpp`（`epsilon_tensor_*` / `sigma_tensor_*` 的 `fill_one`，位于新增的 "per-component parsers" 段）
- 严重度: minor（仅新特性 `*_tensor_*`；标量路径不受影响）
- 置信度: likely
- 现象与依据: 取样写为 `x=(i+0.5)*dxa[0]+lox; z=(j+0.5)*dxa[1]+loz; pex(x, 0, z)`，在 `WARPX_DIM_3D` 下
  第二个索引是 y、第三个才是 z，因此 y 恒为 0、z 被当成 j。同文件的接口/节点材料构造明确修过同类问题
  （注释 "F-02: in 3D the second index is y and the third is z"），说明这是漏改的一处。
- 建议修法: 用与 `F-02` 相同的 `#if defined(WARPX_DIM_3D)` 分支（`y=(j+0.5)dy`、`z=(k+0.5)dz`）。
- 验证方法: 3D + 只随 y 变化的 `epsilon_tensor_xx(x,y,z)=1+y`，检查 `eps_mf` 是否随 y 变化。

### F7 亚像素张量：正定性/退化检查用 `AMREX_ASSERT`（release 构建被编译掉）
- 文件:行 `Source/Plugins/FieldSolver/FiniteDifference/MacroscopicProperties/SubpixelTensor.H:189-191`、`:345-347`、`:402-405`、`:410-411`
- 严重度: minor
- 置信度: confirmed（`audit_build/deps/amrex-26.10/Src/Base/AMReX_BLassert.H:35-45`：未定义
  `AMREX_DEBUG`/`AMREX_USE_ASSERTION` 时 `AMREX_ASSERT*(...)` 展开为 `((void)0)`）
- 现象与依据: `inv3_sym()`、`sigma_exponential_matrices()`、`chi_inv_anisotropic()` 用
  `AMREX_ASSERT_WITH_MESSAGE` 检查"ε/μ 张量正定 / det>0 / ε̄₁₁>0"，随后直接 `1/det`、`1/t11`、
  `sqrt(lam)`。生产构建里这些检查不存在，非正定或接近奇异的材料张量会得到 NaN 的 `eps^-1` 行或
  `1/(2πr)` 型放大，且没有任何消息。WarpX 同类检查惯例是 `WARPX_ALWAYS_ASSERT_WITH_MESSAGE`。
- 建议修法: 至少对 `inv3_sym` 与 `chi_inv_anisotropic` 的 `det>0` 用 `WARPX_ALWAYS_ASSERT`（或对
  `det` 加 `max(det, tiny)` 下限并 warn）。
- 验证方法: 给 `macroscopic.epsilon_tensor_xx/yy/zz` 一个非正定组合（如 xx=1, yy=-1, zz=1），
  release 构建下观察是否静默产生 NaN 场。

## 反向核对：以下与上游不同，但经推导/对照确认是**修上游 bug**，不应记为缺陷
- `LabFrameExplicitES.cpp:218`：Neumann 上边界的 Thomas 回代从 `-(−1)*φ` 改成 `-(−2)*φ`。
  该行上方 `diag = 2 − 2*zwork` 已是 Neumann 行的 `b−a c'`（a=−2），分子必须同样用 a=−2；
  低端 Neumann 行也用 `zwork(1)=2/diag` 对应 c=−2。改动自洽、上游原式自相矛盾。
- `EvolveF.cpp:206-210`（RZ）：模式 m 的**虚部**方程原来误用实部指标（rho/Drr 取 `2m-1`），改为 `2m`，
  与 `−m·Et(2m-1)/r`、`Dz(2m)` 一致；按 `e^{-imθ}` 约定推导正确。
- `EvolveECTRho.cpp:117-126,146-153` + `EvolveB.cpp:262-265`：XZ 下补齐 `Rhox/Rhoz` 并让 ECT 更新全部三个 B 分量。
  复核：XZ 中 `(curl E)_x=−∂Ey/∂z`、`(curl E)_z=∂Ey/∂x`，且 `Ey` 为 (1,1) 节点量、`Bx` 为 (1,0)、`Bz` 为 (0,1)
  （`Source/Plugins/PicEm/WarpX.cpp:2778-2783`），公式与交错一致。上游 XZ 下 `Rhox/Rhoz` 从未写入，属上游缺陷。
- `DarwinLinearFieldOperator.cpp`：删掉了输出 `rhs_vec` 上的最后一次 `FillBoundaryAndSync`，并改用自有
  `m_dA_x/m_chidA_x` 代替上游借用 `dA_fp`/`Efield_fp` 作 scratch。删 sync 的论证成立（全部由已 sync 的 Zscratch
  与质量矩阵算出、逐项同序 ⇒ 周期重复节点逐位相同）；不再覆写寄存器里的 `dA_fp` 也修掉了"GMRES 迭代中污染诊断场"的隐患。
- `PoissonBoundaryHandler.cpp:61-89`：RZ 下 `dim_start` 只在 `ProbLo(0)==0` 时才跳过 r，并补 `has_non_periodic=true`，
  使"r 非周期 + z 周期"的算例不再漏设 r_max Dirichlet 电位。

## 不确定但值得复核的点
1. `HybridPICSolveE.cpp`：新增 `include_grad_pe = !solve_for_Faraday || params.solve_electron_energy_equation`，
   即电子能量方程开启时把 `grad(Pe)` 放进 Faraday 步。物理上 `curl(grad Pe)≠0` 成立（Pe 不再是 ne 的单值函数），
   属于有意修正；但它是与上游相反的物理选择，建议与 hybrid 负责人确认并跑 `ctest -R hybrid` 的电能方程用例。
2. `FiniteDifferenceSolver::config()` / `ElectrostaticSolver::config()`（`FiniteDifferenceSolver.cpp:129-147`、
   `ElectrostaticSolver.cpp:590-598`）：上下文为空时返回**默认构造**的 `RuntimeConfig`，而 `data()` 有
   `legacy_runtime_data()` 可用。目前所有工厂都传入 context（`FiniteDifferenceSolverFactory.cpp:18`、
   `ElectrostaticSolverFactory.cpp:33-38`），但一旦有人用默认参数构造，`grid_type`、`n_rz_azimuthal_modes`、
   `field_boundary_*`、`poisson_solver_id` 都会是默认值而**不报错**。建议在空上下文时 abort。
3. `JacobiPC.H` 被重写为委托 `WeightedJacobiImpl<T>` + `MatrixSolverOps<T>`（两者不在本分区清单内）。
   `weighted jacobi` 的 omega 自适应、`adaptive_omega` 参数语义、`(I+M)x=b` → `Ax=b` 的归一化是否完全等价，需连同
   `WeightedJacobiImpl.H` 一起审。
4. `ImplicitSolver::OneStep`（新函数，`ImplicitSolver.cpp:147-219`）：退出时 `m_dt` 停留在"减半后"的值而不复位，
   若上层在 `OneStep` 之后读取 `m_dt`（或统计 `dt`）会不一致；子步失败重启的 `isubstep *= 2` / `m_dt /= 2` 组合
   在多次失败下我推导是对的（重做整步），但仍建议加断言 `m_dt == a_dt` 于返回前并跑
   `ctest -R implicit` 的 substepping 用例。
5. `MacroscopicEvolveE.cpp` 的"P2 指数步"（`has_sigma_matrices` 分支，`:383-466`）：节点 sigma/M1/M2 与
   `eold3` 快照的组合我未做完整推导；`get_sigma_dt() == dt` 有断言保护是好的，但 `Jc`（步平均传导电流）写入
   `jcond` 后是否被下游正确消费、以及 `m1/m2` 的张量对称性，需要材料模型负责人复核。
6. `EvolveECartesianECT`（`EvolveE.cpp:245-430`）的宏观分支永不执行（`EvolveE` 传 `nullptr`，
   只有 `MacroscopicEvolveE.cpp:66-68` 传实体），且该处 `lev` 硬编码为 0——这两条路径的行为差一整个材料模型，
   将来若从 `EvolveE` 打开宏材料会静默走真空。建议加断言或统一入口。
