# 边界条件 / EB / 求解器并列：阶段工作汇总（可交接）

本文汇总 WarpX fork 上"PEC 与电介质边界条件 + EB 几何接口 + 恒星/多阶矩求解器并列"这一阶段
的全部结论与证据。所有数字都来自实际运行，不是推断；标 ⚠️ 的地方是**我判断错过的**，一并记下以免重蹈。

## 一、五项需求的真实完成度

| 项 | 完成度 | 证据 | 缺什么 |
|---|---|---|---|
| (1) 修 B⊥ 段错误、两算例跑完 | **40%** | 已修 4 个真实 bug（见 §二）；hybrid+EB 能跑到 t≈0.8 ns | Ohm 律在导体停滞点发散（见 §三） |
| (2) 网页端运行 + 可视化 | **70%** | Studio `/api/run` 多次启动成功；`B∥/B⊥` 出图；`verify/bn.py` 定量测量 | 完整 20 ns 图未产出（依赖 (1)） |
| (3) IEBGeometry + 跨边界 B_n | **100%** | `IEBGeometry.H`/`EBGeometry.H` 编译通过 + core purity 检查；`verify/bn.py` + selftest 判据 | — |
| (4) 边界条件（PEC cut-cell / 电介质 subpixel） | **80%** | PEC 切向投影已落地并回归通过（§四）；subpixel 仓库已有且已文档化（§五） | PEC 二阶未验证；未插件化；未逐求解器推进 |
| (5) 恒星/多阶矩与 Yee 并列 | **35%**（见 §八） | 恒星/五阶矩已是独立目标；十阶矩无输入入口 | 十阶矩需新增选择键 + 驱动逻辑 |

## 二、已修的真实 bug（每个都有前后对比）

1. **EB 隐式函数符号反了**。文档规定"模拟区 = 函数为负、导体内为正"，原写法 `x²+z²−r²`
   把整个域外当导体、圆内当等离子体。症状：iteration 0 只有 311 个粒子且全在圆内、`E≡0`。
   修正为 `r²−(x²+z²)` 后：38086 个粒子、导体内仅 2 个。
2. **`momentum_distribution_type = maxwellian` 在本 fork 不存在**。类型名是 `gaussian`
   （`SpeciesUtils.cpp:154-180`），写错会静默退化成零动量（粒子完全不动）。
3. **RKF45 自适应子步在网格块边界产生台阶**。`use_rkf45=1` 时 `B_y` 呈 2×3 方块，
   边界与 `amr.max_grid_size` 对齐；`use_rkf45=0` 后连续。**hybrid 出图必须关掉 RKF45**。
4. **PIC 段错误的根因链**：场爆炸 → 粒子坐标非法 → `doEsirkepovDepositionShapeN` 越界。
   不是沉积算法本身的问题（三种算法都在爆炸后才崩）。

## 三、(1) 未解决部分：导体停滞点的 Ohm 律发散

实测（`hybrid + EB + use_rkf45=0 + dt=2e-12`，失败前）：

| step | max\|rho\| | max\|j\| | max\|E\| |
|---|---|---|---|
| 680 | 3.513 | 2.52e5 | 2.43e6 |
| 800 | 3.572 | **2.52e5（不变）** | **9.34e7（涨 14×）** |

密度和电流都稳、只有 E 在飞 → 不是输运问题，而是 Ohm 律里 `∇P_e/(e n)` 在 `n→0` 的切割格发散。

**已排除**（都实测过）：RKF45 自适应、`n_floor` 抬高、`substep_rtol/atol` 放宽、dt 缩小 10×、
密度堆积假设。失败时刻固定 t≈0.8 ns，与这些参数无关。

**已核实不是问题的**（⚠️ 我曾误判）：
- hybrid **有** EB 掩码通路（`eb_update_E` 贯穿 9 处，`HybridPICSolveE.cpp:190-192` 实际使用）；
- PEC **已作用到** hybrid 路径（`HybridPICSolveE` 末尾调 `warpx.ApplyEfieldBoundary`）；
- 电介质 subpixel **已实现**（`algo.macroscopic_subpixel`）；
- PEC 域边界**一直存在**（`Modules/BoundaryConditions/WarpX_PEC.cpp`，`WarpXFieldBoundaries.cpp:53` 调用）。

**下一步**：在 `HybridPICModel::CalculateElectronPressure`（`HybridPICModel.cpp:804`）与其
`rho_floor`（866/967/1101 行）处，把分母密度改成"cut/covered 格用邻格平均"，压力梯度用单侧差分。

## 四、PEC on EB：已落地的是"切向投影"

`Source/Plugins/PicEm/Modules/BoundaryConditions/WarpXFieldBoundaries.cpp`，
在 `WarpX::ApplyEfieldBoundary` 里对 `FabType::singlevalued` 的切割格：

```cpp
amrex::Real const En = (e0*n0 + e1*n1 + e2*n2) / norm2;   // n 来自 getBndryNormal()
if (has_x) ex_a(i,j,k) = En * n0;                         // 只留法向分量 → E_t = 0
```

三个版本的首步 `max|E|`（同一算例）：无 PEC 5.85e5 → 整格置零 5.36e4 → **切向投影 4.93e4**。
回归：`Examples/Tests/embedded_boundary_cube` 40 步通过。

**未证明**：二阶精度。要做 cut-cell 面积加权 + 切面质心，并做 2× 加密收敛测试（误差降 4×）。

## 五、电介质 subpixel：仓库已有，只需验证与文档

- `MacroscopicProperties.cpp:41` 读 `algo.macroscopic_subpixel`；
- `MacroscopicEvolveE.cpp:173` 用 `has_subpixel_rows()`；
- 实测 `algo.macroscopic_subpixel = 1` + 圆柱 `epsilon_function` → exit 0；
- 本轮已把该参数的文档补进 `Docs/source/usage/parameters.rst`（原先是空的）；
- `Tools/warpx-studio/warpx_studio/verify/subpixel.py` 是对应的**独立解析校验**
  （法向 1.6 / 切向 2.5 / Fresnel 1/3 都对得上），可用来校验 C++ 实现。

## 六、可用工具与环境（避免重复搭建）

| 工具 | 用法 |
|---|---|
| `python -m warpx_studio.verify.bn <h5> --radius ... --b0 ...` | 跨 EB 面 `B_n/B0`、`E_t/E` 定量测量 |
| `python -m warpx_studio.verify.subpixel` | subpixel 面 ε 的解析校验 |
| Studio（`http://127.0.0.1:8770`） | 网页建模 + 运行 + 实时渲染（项目里已存 `弓形激波 · B∥/B⊥` 两个项目） |
| 构建 | `cmake --build audit_build/fork --target warpx_picem_2d -j 8`（约 100 秒） |
| 冒烟 | `node Tools/warpx-studio/tests/ui_smoke.js ...`；`selftest.py` 全量约 10 分钟 |

## 七、如果继续，建议的顺序

1. **Ohm 律密度处理**（唯一挡住 (1)(2) 的事，改动集中在一个函数，1–2 轮）；
2. **PEC 二阶化**（cut-cell 面积加权 + 收敛测试，1–2 轮）；
3. **插件化**（仓库要求 `Source/Plugins/<层>` 必须登记在 `WARPX_MODULE_LAYERS`，而登记后顶层会为它建
   无源 OBJECT 目标、子目录 `target_sources` 仍失败；需先读懂这一协议，见 `Source/ParkedPlugins/`）；
4. **(5) 求解器并列**（`MomentsSolver` 目前在 `PicEm/Modules` 内，需抽成独立目标并注册）。

**不要**再花轮次去调 PIC 的 dt/密度/容差：全 PIC 在论文参数区本质不可行（`ω_pe/ω_ci ~ 10³–10⁴`），
这是 20 轮实测的结论。


## 八、第 37 轮核实：(5) 的真实基线

| 求解器 | 现状 | 与 Yee 是否并列 |
|---|---|---|
| 场求解器（Yee/CKC/PSATD/ECT/隐式） | `Source/Plugins/FieldSolver`，`algo.maxwell_solver` 选择 | — （基准） |
| **五阶矩流体** | `warpx_fluidmoments_static_${SD}`（独立 STATIC 目标）+ `fluids.species_names` | ✅ **已是独立目标** |
| **恒星流体** | `star_fluid_plugin_${SD}`（独立 MODULE）+ `register_handlers(WarpXPluginHost*)` 自注册 | ✅ **已是独立插件模块** |
| 十阶矩 / 矩方程 | 在 `PicEm/Modules/MomentsSolver` 内，经 `Source/MomentsSolver` 头文件路径编进 PicEm；**全目录无任何 `ParmParse`** | ❌ 既无独立目标，**也没有输入入口** |

结论：所谓"与 Yee 并列"，对恒星与五阶矩**已经成立**（它们是独立的 CMake 目标/模块）；
真正缺的是十阶矩——而它缺的不只是 CMake 目标，**缺的是从输入文件启用它的通路**（没有任何参数读取），
这意味着要新增求解器选择键 + 驱动逻辑，属于功能开发而非结构搬移。

所以 (5) 的正确描述是：「恒星/五阶矩已并列（可验收）；十阶矩需先有输入入口才能谈并列」。
