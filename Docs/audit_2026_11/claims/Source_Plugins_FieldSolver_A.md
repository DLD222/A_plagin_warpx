# Source_Plugins_FieldSolver_A 工作日志

claim: Source_Plugins_FieldSolver_A F4,F5,F6,F7 @lead-mid 2026-11-15T18:30

F4 复核结论：**不成立（invalid）**。审查对照的是 26.10；26.11 的
`_upstream/warpX-26.11/Source/FieldSolver/ElectrostaticSolvers/EffectivePotentialES.cpp:160-171`
正是 `GetChargeDensityOnLevel(lev, true, /*finalize*/false)` → `ApplyFilterandSumBoundaryRho`
→ `FinalizeRhoOnLevel`，与 fork 逐行同序（注释也一致："Defer physical boundary folding and
volume scaling until after filtering."）。⇒ 上游 26.10→26.11 自己改的，不需要改。

F5 复核结论：**不成立（invalid）**。26.11 的 `ImplicitSolver.cpp:789-793` 同样是
`m_mass_matrices_pc_width = 1; pp.query(...)`，`:917-930` 也同样是 3D+Villasenor 时 abort
并提示 `Set implicit_evolve.mass_matrices_pc_width = 0.`，与 fork 一致。

F6（新张量材料只按二维取样）确认为**真缺陷**（fork 新增特性，26.11 没有该文件）：
`MacroscopicProperties.cpp` 的 `fill_one` 在 3D 下把 `j` 当 z、y 恒取 0，并且 1D 构建下
还读了 `dxa[1]`（越界）。改为按维度取样（3D: (x,y,z) 分别来自 i/j/k；
XZ/RZ: (x,0,z) 来自 i/j；1D: z 来自 i），与同文件 F-02/F-05 的写法一致。

F7（亚像素张量正定性检查用 `AMREX_ASSERT`，release 构建被编译掉）确认成立：
`SubpixelTensor.H` 的 4 处 `AMREX_ASSERT_WITH_MESSAGE` 改成
`AMREX_ALWAYS_ASSERT_WITH_MESSAGE`（这些函数是 `AMREX_GPU_HOST_DEVICE`，不能用
`WARPX_ALWAYS_ASSERT_WITH_MESSAGE` —— 后者经 `ablastr::utils::TextMsg` 走 `std::string`，非 device 安全；
`AMREX_ALWAYS_ASSERT_WITH_MESSAGE` 展开到 `amrex::Assert(const char*...)`，是 device 安全的）。

验证：见"验证记录"。
