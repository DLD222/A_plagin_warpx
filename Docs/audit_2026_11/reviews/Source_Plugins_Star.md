# Source_Plugins_Star 审查

分区：`Source/Plugins/FluidMoments/*`（38 文件）+ `Source/Plugins/NewtonGravity/*`（8 文件）。
上游 `_upstream/warpX-26.10/Source/` **无任何对应物**（上游没有 FluidMoments/NewtonGravity、没有
`Source/Plugins` 插件层），因此本分区全部 46 个文件都是 **new（本地新写）**，无法用 `diff -u` 对照；
判据只能是方程、量纲、AMReX/MLMG 契约与文档自述。仅 `NewtonGravity.cpp` 借用了 AMReX `MLABecLaplacian`
的公共写法（`_upstream` 的 `Source/ablastr/fields/PoissonSolver.H` 是同类用法的参照）。

> 修复状态：用户已用 `AMREX_D_DECL` 修好 1D/2D **编译**（`StarFluidPlugin.cpp:99-100`、
> `StarGravityPlugin.cpp:54-55`）；本报告审查修复后文件。编译问题不再重复，**运行期维度错误仍在**（F1）。

## 覆盖

| 文件 | 行数 | 类型 | 结论 |
|---|---|---|---|
| FluidMoments/AngularMomentum.H | 228 | new | 缺陷（F1、m8、m9） |
| FluidMoments/Cf88Rates.H | 190 | new | OK（系数逐项复核通过，见"复核通过项"） |
| FluidMoments/Composition.H | 169 | new | 缺陷（F1、F7） |
| FluidMoments/Conduction.H | 194 | new | 缺陷（F1、F7） |
| FluidMoments/DegenerateEOS.H | 128 | new | 可疑（m2、m3；常数复核正确） |
| FluidMoments/Electromagnetic.H | 152 | new | 缺陷（F1、F4、F5） |
| FluidMoments/FluidMoments.H | 418 | new | 缺陷（F6 无正定性保护）；HLLC/MUSCL 公式复核正确 |
| FluidMoments/FluidMoments.cpp | 124 | new | 缺陷（F1、F4） |
| FluidMoments/FluidTestReference.H | 228 | new | 缺陷（m1，Newton 导数指数错；Riemann 解其余正确） |
| FluidMoments/Ionization.H | 162 | new | 缺陷（F1）+ 可疑（m5） |
| FluidMoments/Neutrino.H | 102 | new | 缺陷（F1、m12） |
| FluidMoments/Radiation.H | 262 | new | 缺陷（F1、F7、F8、m6） |
| FluidMoments/SolarNetwork.H | 467 | new | 可疑（m4、m11；化学计量/单位内部自洽） |
| FluidMoments/Spherical.H | 159 | new | OK（球坐标源项推导正确；仅 1D 索引，无 F1） |
| FluidMoments/StarFluidPlugin.cpp | 383 | new | **缺陷（F1、F2、F3、F9、F10、m16、m17、m18）** |
| FluidMoments/StellarSources.H | 167 | new | 缺陷（F1、m10） |
| FluidMoments/test_angular_momentum.cpp | 172 | new | 缺陷（F11：1D 下 assert 必崩） |
| FluidMoments/test_cf88_rates.cpp | 156 | new | OK |
| FluidMoments/test_composition.cpp | 194 | new | OK |
| FluidMoments/test_conduction.cpp | 200 | new | OK（但只测 1D 变场，掩盖 F7） |
| FluidMoments/test_conservation.cpp | 153 | new | OK |
| FluidMoments/test_degenerate_eos.cpp | 118 | new | OK（按插值的渐近精度设定 1e-2 容差，理由成立） |
| FluidMoments/test_electromagnetic.cpp | 182 | new | OK（只测代数恒等式，抓不到 F5） |
| FluidMoments/test_energy_budget.cpp | 169 | new | 缺陷（m15 恒真断言） |
| FluidMoments/test_extra_scalars.cpp | 155 | new | OK |
| FluidMoments/test_hydrostatic.cpp | 279 | new | OK |
| FluidMoments/test_ionization.cpp | 225 | new | OK |
| FluidMoments/test_jeans.cpp | 246 | new | OK（色散关系 λ_J 推导正确） |
| FluidMoments/test_network.cpp | 339 | new | OK（强） |
| FluidMoments/test_network_coupling.cpp | 226 | new | OK（并把 SI 契约钉死，见 F4） |
| FluidMoments/test_neutrino.cpp | 141 | new | OK（显式给 max_sub=2e5，不触发 m12） |
| FluidMoments/test_radiation.cpp | 209 | new | OK（同上） |
| FluidMoments/test_smooth_wave.cpp | 134 | new | OK |
| FluidMoments/test_sod_shock.cpp | 193 | new | OK |
| FluidMoments/test_solar_column.cpp | 315 | new | **缺陷（F12 断言自我比较 + 源项全关）** |
| FluidMoments/test_solar_core.cpp | 256 | new | 缺陷（m14 声称的 dr 收敛未被测） |
| FluidMoments/test_species_advection.cpp | 152 | new | OK（1e-12 可达到，因为 u≡1 时重建精确） |
| FluidMoments/test_stellar_model.cpp | 298 | new | **缺陷（F13 栈数组越界）** |
| NewtonGravity/GravityTestReference.H | 345 | new | OK（`phiShell` 三支我独立重推一致；含 F1 索引） |
| NewtonGravity/NewtonGravity.H | 94 | new | 可疑（F3 单位声明、F9 契约） |
| NewtonGravity/NewtonGravity.cpp | 170 | new | 缺陷（F9；算符/符号/setScalars 复核正确） |
| NewtonGravity/StarGravityPlugin.cpp | 317 | new | **缺陷（F1、F2、F3、F10）** |
| NewtonGravity/test_periodic_sine.cpp | 140 | new | OK |
| NewtonGravity/test_point_mass.cpp | 185 | new | OK |
| NewtonGravity/test_two_shells.cpp | 244 | new | OK（边界数据与参考同一函数，但外部支正确，成立） |
| NewtonGravity/test_uniform_sphere.cpp | 214 | new | OK |

**46/46 读完。缺陷合计 32：critical 4（F1-F4）/ major 9（F5-F13）/ minor 19（m1-m19）。**

## 先说清楚"什么真的接上了"

`StarFluidPlugin` 只 include 了 `FluidMoments.H` 与 `Electromagnetic.H`（`StarFluidPlugin.cpp:17-18`）。
即运行期真正跑起来的只有：SSM profile 初始化 → 截断 → 电磁源 → **笛卡尔 `advanceSweep`（`IdealGasEOS`，
γ=1.4）** → Mach/能量标量。以下整层**只在单测里被实例化**，插件从不调用：`applyGravitySource`、
`applySolarNetwork`、`spherical::advanceSphericalRadial`、`DegenerateElectronEOS`、`Radiation.H`、
`Conduction.H`、`Ionization.H`、`Composition.H`、`AngularMomentum.H`、`Neutrino.H`。
也就是说"欧拉流体矩 + 反应网络 + 简并 EOS + 中微子 + 球坐标"这一整套在 **runtime 层没有接进时间推进**，
接进去的只有半个重力读取（还不用，见 F2）。这一点比单个公式错误更值得先看。

## 缺陷清单

### F1 【critical】1D/2D 下 `bx.smallEnd(1)/(2)` 越界读 → 循环上界是垃圾，写穿 FAB
- 文件:行 — `StarFluidPlugin.cpp:118-120, 168-170, 196-198, 283-285`；`FluidMoments.cpp:40-42`；
  `Electromagnetic.H:119-121`；`Radiation.H:91-93, 149-151, 173-175, 225-227`；`Conduction.H:122-124,
  152-154`；`Ionization.H:123-125`；`Composition.H:74-76, 141-143`；`AngularMomentum.H:95-97, 157-159,
  200-202`；`StellarSources.H:78-80, 95-97`；`Neutrino.H:90-92`；`StarGravityPlugin.cpp:221-226`。
- 严重度：critical；置信度：confirmed（头文件+布局可验证）
- 现象与依据：
  - `amrex::Box::smallEnd(int dir)` 就是 `smallend[dir]`（`AMReX_Box.H:132`），`IntVectND<dim>` 内部是
    `int vect[dim]`，`operator[]` 只有 `BL_ASSERT(i < dim)`（`AMReX_IntVect.H:339-343`），而 `BL_ASSERT`
    在非 DEBUG 构建是空语句（`AMReX_BLassert.H` 无定义）。
  - `BoxND` 的成员是 `IntVectND smallend; IntVectND bigend;`（`AMReX_Box.H:53-80`）。所以 1D 下
    `bx.smallEnd(1)` 读到的就是 `bigend.vect[0]` = **x_hi**，`bx.bigEnd(1)` 读到 Box 对象之外的 4 字节。
    循环 `for (j = x_hi; j <= 垃圾; ++j)` 若垃圾 ≥ x_hi，就会对 `s(i,j,k,c)` 以超界 j 写内存。
  - 用户用 `AMREX_D_DECL` 修的是 `IntVect{lo[0],lo[1],lo[2]}` 这类**构造**；这些三重 `for` 是把
    `smallEnd(1)` 当运行期取值用，`AMREX_D_DECL` 管不到，所以仍然编译通过、运行期错。
  - 变异后果举例：`StarGravityPlugin.cpp:221-226` 除越界外还把半径改成
    `r = sqrt(x² + y² + z²)`，其中 `y = plo[1] + (j+0.5)*d[1]` 而
    `warpx_stage_geometry` 对未用维度返回 `dx = 1.0`（`Source/Core/Runtime/PluginCAdapter.cpp:300`），
    于是 1D 下剖面被平移了半个"米"；`FluidMoments.cpp:40-42`（`applySolarNetwork`）和
    `Electromagnetic.H:119-121`（**被插件实际调用**）是同一模式。
- 建议修法：所有三重循环改为 `amrex::ParallelFor(mfi.growntilebox(...))` 或
  `#if AMREX_SPACEDIM > 1` 包裹；最低成本是集中一个 `for_each_cell(bx, f)` 工具函数，替换全部 47 处。
- 验证方法：`cmake -DWarpX_DIMS=1 -DWarpX_COMPUTE=NOACC ...` 后跑
  `test_fluid_*`（现只在 3D 注册，见 `Source/Plugins/FluidMoments/CMakeLists.txt:59`），或在 1D 构建下给
  `SmallEnd` 处加 `AMREX_ALWAYS_ASSERT(dir < AMREX_SPACEDIM)` 立刻暴露。

### F2 【critical】自引力在 runtime 中完全不存在：不算 `g`、也不用 `g`
- 文件:行 — `NewtonGravity/StarGravityPlugin.cpp:85-92`（`star_g*` 只被 `setVal(0.0)`）；
  `FluidMoments/StarFluidPlugin.cpp:210-226`（读 `gx/gy/gz` 后 `ignore_unused(gx,gy,gz)`，从不调
  `applyGravitySource`）。
- 严重度：critical；置信度：confirmed（`computeAcceleration` 全仓库调用点仅 4 个测试：
  `test_hydrostatic.cpp:206, test_jeans.cpp:169, test_uniform_sphere.cpp:118, test_two_shells.cpp:158,
  test_point_mass.cpp:121`；插件里 0 处）
- 现象与依据：`solvePotential` 算出 `star_phi`，但 `computeAcceleration(phi,gx,gy,gz,geom)`
  （`NewtonGravity.H:80`）从未被 `StarGravityPlugin` 调用，`star_gx/gy/gz` 被清零后原样发布；流体侧把
  `gx/gy/gz` 拷进本地 MultiFab 后直接丢弃（`amrex::ignore_unused`）。净效果：两个插件都在日志里报告
  "g: yes"（`StarFluidPlugin.cpp:334`），而流体感受到的重力恒为 0 —— 太阳模型没有任何支撑力，静水平衡
  与 Jeans 判据在 runtime 都不成立。这与 `StarGravityPlugin.cpp:10-13` / `StarFluidPlugin.cpp:10-13`
  的自述（"read star_g through the bus"）直接矛盾。
- 建议修法：`gravity_step` 里在 `solvePotential` 后调用 `computeAcceleration(phi, gx, gy, gz, geom)`
  并写入 `star_g*`；`fluid_step` 里在 sweep 前后各调用一次 `applyGravitySource`（Strang）。
- 验证方法：`Tools/verify/check_star_runtime_plugins.sh` 已有 deck 上，grep `star_gx` 的 `norminf`
  （应 > 0）；或在 `fluid_step` 里断言 `gx.norminf(0) + ... > 0`。

### F3 【critical】单位系统三处互斥（cgs 状态 vs SI 几何 vs SI 泊松），可观测
- 文件:行 — `StarGravityPlugin.cpp:171-172`（`SsmRow{v[0]*6.957e10, v[3], ...}`，`v[3]` = `rho_g_cm3`，
  写进 `star_rho`）↔ `NewtonGravity.H:44-46`（"Units are SI throughout: rho in kg m^-3"）+
  `NewtonGravity.cpp:26`（`G_newton = 6.67430e-11`）→ φ、g 系统性小 10³。
  `StarFluidPlugin.cpp:177-181`（`s(density)=row.rho` [g/cm³]、`s(energy)=row.P/0.4` [erg/cm³]）↔
  `StarFluidPlugin.cpp:112,140`（`dx = geom.CellSize(0)` 来自 deck，WarpX 是 SI 米）→
  `acoustic_dt = cfl*dx[m]/(c_s[cm/s])` 差 100 倍，`mass += rho[g/cm³]*cell_volume[m³]` 差 10⁶；
  `FluidMoments.cpp:24-72`（`applySolarNetwork` 的 SI 契约由 `test_network_coupling.cpp:93-101,159-161`
  钉死：kg/m³、Pa、J/m³）↔ 插件状态是 cgs。
- 严重度：critical；置信度：confirmed（可观测反例）：在
  `Tools/verify/check_star_runtime_plugins.sh` 用的 deck 上（`inputs_test_3d_particle_boundaries`，网格是
  米级），`xc` 落在 ~1e-6 m < `g_rows.front().r`(=6.75e9 cm)，于是**整个计算域取到同一行**（中心密度
  152 g/cm³），`mass = 152 × (1e-6)³ ≈ 1.5e-16`，日志打印 `%.4f M_sun` → `0.0000`。
- 建议修法：在 bus 边界强制 SI（profile 读入即 `rho*1e3`、`P*0.1`），流体状态全部 SI（`FluidMoments.H`
  注释里 `maxWaveSpeed`、`Spherical.H` 的 cgs 注释同步改），或反过来全部 cgs 并把 `warpx_stage_geometry`
  的 dx 也换算——但两者只能选一个。`applySolarNetwork` 的 SI 契约是正确的那个，应让插件服从它。
- 验证方法：在 deck 里 `amrex::Print` 一次 `state.sum(0)` 与解析 1 M_sun 比较（错误 10³/10⁶ 会立刻显现）。

### F4 【critical/major】`applyElectromagneticSources` 取错场分量：E=(Ex,Ex,Ex)
- 文件:行 — `Electromagnetic.H:127-128`（`Ev[3] = {ea[0](i,j,k), ea[1](i,j,k), ea[2](i,j,k)}`，
  `Array4::operator()` 省略分量时取 comp 0）；调用方 `StarFluidPlugin.cpp:245-254` 把
  `warpx_stage_read_field_dir(ctx,"Efield_fp",d,...)` 的指针按"每方向一个 MultiFab"存进 `ev[d]`。
- 严重度：critical（物理量直接错），置信度：confirmed。上游
  `_upstream/warpX-26.10/Source/WarpX.cpp:2549-2552`：
  `alloc_init(Efield_fp, Direction{0..2}, lev, convert(ba, Ex/Ey/Ez_nodal_flag), dm, ncomps=3, ...)`
  —— 每个方向都是**3 分量** MultiFab，第 idim 分量在 `Direction{idim}` 的 MultiFab 里。而
  `DataBus::field(name,dir,lev)` 只是转发到 `MultiFabRegister`（`Source/Core/Runtime/DataBus.H:61-64`），
  不做分量拆分，也没有单分量别名。
- 现象与依据：`ea[d](i,j,k)` = `Efield_fp[lev][d]` 的 **comp 0** = 第 d 个交错位置上的 **E_x**。
  于是 `Ev = (E_x@x面, E_x@y面, E_x@z面)`；对验证 deck 的均匀 `Ex=1e3, Ey=Ez=0`，实际喂给 Lorentz 力的是
  `E=(1e3,1e3,1e3)`：焦耳热最多 3 倍、且凭空出现 J_y、J_z。`Tools/verify/check_star_runtime_plugins.sh:44`
  只 grep `max|E| = 1000`（三个分量都一样，仍然命中），抓不到。
- 建议修法：`ea[d](i,j,k,d)` / `ba[d](i,j,k,d)`（并注明用的是 Yee 交错值、未做插值到格心）。
- 验证方法：deck 里只开 `Ex=1e3` 且 `u=0`、`B=0`，则力必须为 0、`joule = sigma*1e6`；当前会得到
  `3*sigma*1e6`。

### F5 【major】高斯 cgs 下洛伦兹力缺 1/c（同时又是 SI 公式，两套都说不通）
- 文件:行 — `Electromagnetic.H:77-86`（`f = J × B`，无 `1/c`；文件头 `:36` 声明 "Units are CGS"）；
  `StarFluidPlugin.cpp:144`（`sigma_em = 1.0e4`，注释 `[1/s]` 即 cgs）。
- 严重度：major；置信度：confirmed（量纲）：Gaussian-cgs 里 `J = σE'` 给出 `[J]=statA/cm²`，
  `f = J×B/c` 才是 dyn/cm³，缺 `1/c = 3.3e-11`，力大 3e10 倍；而若按 SI（V/m、T）则 `f = J×B` 正确，
  但状态是 erg/cm³、动量是 g/(cm²·s)，两种约定不可能同时成立。焦耳热 `Q = σ|E'|²` 在两种约定下都自洽，
  所以只有力错——这正是单测（`test_electromagnetic.cpp` 只查 `f·B=0`、`Q≥0`、内部恒等式）抓不到的方向。
- 建议修法：若坚持 cgs，`f = cross(J,B)/c_light`；并把 σ、E、B 的来源单位在头注释里写死。
- 验证方法：`test_electromagnetic.cpp` 增加与解析 `f = σ(E+u×B)×B/c` 的数值比较。

### F6 【major】HLLC/MUSCL 全链路无正定性/下限保护，强膨胀必产生 NaN
- 文件:行 — `FluidMoments.H:118-127`（`primitive`：`vel = mom/rho`、`e_int = E-½mom²/rho`，rho=0 直接除零）；
  `:165-166`（`soundSpeed(rho,p)=sqrt(γp/ρ)`，p<0 时 `sqrt` 得 NaN）；`:307-310`（minmod 重建后
  `ULm` 的密度/内能可负）；`:376-379`（`maxWaveSpeed` 用 `norminf` → 一个 NaN 污染整场 → `dt=NaN`）。
- 严重度：major；置信度：confirmed（构造反例）：Sod 型初值把左右密度比取到 1e4（或真空区 rho=ε、
  p=ε）时，接触/稀疏扇内边缘的 `ULm` 密度可小于 0 → `p=(γ-1)(E-½ρu²)<0` → `sqrt(负)` → NaN →
  `norminf` 返回 NaN → `dt=NaN` → 下一步全网格 NaN。作者自己在
  `StarFluidPlugin.cpp:190-192, 201-202` 承认"第一次太阳初始化正是这样产生 NaN"，但修法是在**外面**加
  floor（且只 floor `density(0)`/`energy`，不 floor `density(1)`、不 floor 压力），`advanceSweep` 本身
  仍无保护，`maxWaveSpeed` 用的是同一套 `primitive`。
- 建议修法：`primitive` 中对 `rho<=rho_floor` 返回 `vel=0`、`p=p_floor`；重建后做一次
  `if (rho<=0 || p<=0) 回退到一阶` （fallback to first order）；`maxWaveSpeed` 用 `ReduceMax` 并
  `if(!isfinite)` 报警。
- 验证方法：`test_sod_shock` 把左右密度比从 8 加到 1e4，当前会 NaN 而 `L1` 变成 NaN 从而"通过"
  （`std::abs(NaN)` 与比较为 false，`order` 也是 NaN，`order_rho < 0.7` 为 false → 假通过！见 m19）。

### F7 【major】显式扩散子步 0.2·dx²/D 超过 3D 稳定极限 dx²/(6D)，与"绝不静默失稳"的自述相反
- 文件:行 — `Conduction.H:64-68`（`0.4*dx*dx/(2*alpha)`），`Radiation.H:161`
  （`cfl*dx*dx/(2*dmax)`，cfl=0.4），`Composition.H:61`（同）。
- 严重度：major；置信度：confirmed（von Neumann）：`e^{n+1} = e^n + hα Σ_d (e_{i+1}-2e_i+e_{i-1})/dx²`，
  放大因子 `1 - 4dhα/dx²`，`d=3` 要求 `h ≤ dx²/(6α) = 0.1667 dx²/α`；代码给的是 `0.2 dx²/α`，最大模
  增长率 `|1-4·3·0.2| = 1.4 > 1` → **无条件增长**。`d=1` 极限 0.5（安全）、`d=2` 0.25（安全）。
- 为什么没被发现：`test_conduction.cpp`、`test_radiation.cpp`、`test_neutrino.cpp`、`test_composition.cpp`
  都在 3D 盒子里放**只沿 x 变化**的波（`fill_wave`/`fill_mode`），Laplacian 的有效特征值只有 4/dx²，
  于是 0.2 落在 1D 安全区。真实 3D 场（太阳对流区、辐射场）会失稳。
- 建议修法：`dt_stable = 0.4*dx*dx/(2*AMREX_SPACEDIM*D)`，或按 `geom.CellSizeArray()` 取
  `1/(2 Σ_d 1/dx_d²)`。
- 验证方法：把 `test_radiation` 的初值改成 `cos(kx)cos(ky)cos(kz)` 全部三个方向，当前会看到幅度增长。

### F8 【major】辐射扩散把未初始化内存 `ParallelCopy` 进辐射场
- 文件:行 — `Radiation.H:177`（`continue` 跳过 `rho<=0 || kappa<=0` 的格子），`:193`（只写有效格子），
  `:198`（`er.ParallelCopy(er_new)`）。
- 严重度：major；置信度：confirmed：`MultiFab er_new(ba,dm,1,0)` 分配后 AMReX **不清零**，被 `continue`
  跳过的格子保持未初始化值，随后整块拷回 `er` → 真空格子的辐射能量变随机数（可能是 NaN/1e300），
  下一步 `applyRadiationCoupling` 用它算 T⁴/交换 → 污染整个场。同类问题在 `Conduction.H:146-155` **不**
  存在（那里先整体 `s_new = s` 再改能量）。太阳外层 `rho` 被 floor 成 1e-10，所以现状不炸；一旦某处
  真的是 0（或 `kappa=0` 关掉不透明度），立刻出垃圾。
- 建议修法：`er_new.setVal(0)` 或把跳过分支写成 `e_new(i,j,k) = e(i,j,k)`（更正确）。
- 验证方法：`test_radiation` 里把 `rho` 的一块设 0，打印 `er.norminf()`；当前会出现非有限值。

### F9 【major】开放边界（Dirichlet）的边界数据从未填充 → MLMG 用未初始化 ghost
- 文件:行 — `NewtonGravity.H:53-60` 明确要求调用方把**面心**势写进 `phi` 的 ghost；
  `NewtonGravity.cpp:60`（`phi.setVal(0,0,1,0)` 只清有效格）→ `:90`（`setLevelBC(0,&phi)`）；
  `StarGravityPlugin.cpp:73,80-81`（拿到 `star_phi` 直接求解，从未写 ghost）；
  `on_schema` 只为 `star_phi` 申请 `ngrow=1`（`StarGravityPlugin.cpp:143`），而 ghost 内容无人赋值。
- 严重度：major；置信度：confirmed（契约层面）：AMReX `MLABecLaplacian::setLevelBC` 对 Dirichlet 是
  "从传入 MultiFab 的 ghost 取边界值"，未初始化 ghost 使势解依赖于分配内存内容 → 不可重复、且与
  `test_uniform_sphere.cpp:92-110`（那里老老实实填了 `phi` 的 ghost 面值）的做法相反。
  另外 `rhs` 也是 `1` ghost 而只 `Copy(...,0)`（`NewtonGravity.cpp:48-49`），open 分支下
  `rhs.FillBoundary` 只填周期方向（`:56`），非周期 ghost 同样未初始化。
- 建议修法：`StarGravityPlugin` 里先用解析 `-GM/r`（或上一层插值）填 `phi` 的 ghost；
  `setLevelBC` 之前 `phi.FillBoundary` 或 `phi.setVal(0)` 再填 ghost；`rhs` 也应 `setVal(0)`。
- 验证方法：同一 deck 连跑两次，比较 `star_phi_min` 是否逐位相同（当前可能不同）。

### F10 【major】"AMR 多层势"实为逐层独立求解 + 每层重新盖同一解析剖面（含 restart 语义错误）
- 文件:行 — `StarGravityPlugin.cpp:99-130`（`for lev 1..7` 各自 `solvePotential`，无 coarse-fine
  修正、无 `average_down`）；`:65-68` + `:240-259`（`fill_all_levels` 在**第一次** handler 调用时把
  `star_rho` 在所有层上重写成同一 SSM 球）；`:262-277`（`on_init` 又做一遍）。
- 严重度：major；置信度：confirmed：每层都满足 ∇²φ=4πGρ 但层间 φ 不连续（粗层解未作为细层边界条件），
  与 `NewtonGravity.cpp:63`（"one level: the plugin solves on the level the caller hands it"）自述一致，
  但头注释与用户任务描述的"AMR 多层势"不成立。更实际的问题是：
  - 每层的 `mass` 都打印 ≈1 M_sun（同一球被每层各自采样），若任何下游把各层相加就是重复计数；
  - `g_levels_filled`/`g_loaded` 是文件级 static（`:39-41, 152-153`），**从不复位**；
    `fill_all_levels` 在第一次调用时无条件覆盖密度 → 从 checkpoint restart 后，星体被解析模型重置，
    续算丢弃。多实例（同进程内多次仿真）也会串状态。
- 建议修法：用 `average_down`/`average_down_faces` + `MLMG` 多级（或 `setCoarseFineBC`）做 composite
  solve；把"已初始化"状态挂到 `WarpXStageContext` 而不是 static；增加 restart 用例。
- 验证方法：两级跑同一太阳模型，比较细层边界处 `star_phi` 与粗层插值的差（当前差 O(1) 相对）。

### F11 【major】1D 构建下 `applyCentrifugalSource` 的 assert 必崩（默认参数就是非法值）
- 文件:行 — `AngularMomentum.H:86`（`AMREX_ALWAYS_ASSERT(axis < AMREX_SPACEDIM && axis_dir <
  AMREX_SPACEDIM)`），默认 `axis_dir = 1`（`:81`）；`test_angular_momentum.cpp:131,134` 显式传
  `axis_dir=1`，`test_stellar_model.cpp:176` 也传 1。
- 严重度：major（对 1D 测试/用例）；置信度：confirmed：`AMREX_ALWAYS_ASSERT` 在 release 也生效，1D 下
  `axis_dir=1 < 1` 为假 → abort。目前靠"所有 FluidMoments 单测只注册 3D"
  （`Source/Plugins/FluidMoments/CMakeLists.txt:59`）躲过去了。附带物理问题：力只加在
  `momentum(axis)` 一个分量上（`:128`），绕 y 轴转的 3D 星体缺 z 向离心力分量 → 不是矢量力。
- 建议修法：`axis_dir` 的有效域应为 `AMREX_SPACEDIM`，1D 时默认 0 或 assert 只查 `axis`；
  力改成沿"垂直于轴的径向"的两个分量（或在 2D 里明确只算一个）。
- 验证方法：`cmake -DWarpX_DIMS=1 && ctest -R fluid_angular`（需先把该测试注册到 1D）。

### F12 【major】`test_solar_column` 的能量账本断言是自我比较（恒真），且源项全部关闭
- 文件:行 — `test_solar_column.cpp:278-283`：
  `booked = ledger.gas_nuclear + ledger.gas_cooling + ledger.gas_radiation;`
  `actual  = ledger.gas_nuclear + ledger.gas_cooling + ledger.gas_radiation;   // measured inside the ledger`
  `worst_booking = |booked-actual|/max(|booked|,1e-30)` → 恒为 `0/1e-30 = 0`；而 `:253-256`
  `opts.nuclear/cooling/radiation = false`，`booked` 本来就是 0。判据 `:308-309` 里的
  `worst_booking < 1e-12` 因此永远成立。
- 严重度：major（测试完整性：这正是"断言被悄悄弱化"）；置信度：confirmed
- 反例：把 `applyStellarSources` 的符号写反、或让它加热 1e30 erg/cm³，这个测试仍然 PASSED。
- 建议修法：`actual` 改为 `e_after_sources - e_before_sources`（调用前后 `state.sum(energy)`），并把
  三个 `opts` 打开（或干脆删掉这条判据，别假装测了）。
- 验证方法：故意把 `StellarSources.H` 的 `ledger.gas_cooling` 符号取反，`ctest -R fluid_solar_column`
  应仍 PASSED。

### F13 【major】`test_stellar_model` 栈数组越界：`w[4]` 被索引到 8
- 文件:行 — `test_stellar_model.cpp:128-133`：`amrex::Real const w[4] = {0.70,0.28,0.02,0.0};` 之后
  `for (int a = 0; a < n_species /* = 9 */; ++a) { wsum += w[a]; }` 与
  `s(...,layout.density(a)) = rho*(w[a]/wsum);`
- 严重度：major；置信度：confirmed：读栈上越界 5 个 double，物种 `be7,c12,c13,n14,n15` 的密度是垃圾
  （可能为负/1e308），随后 `applyStellarSources` 把垃圾喂给 CF88 网络与冷却（`n_a n_b` 乘起来），
  测试声称"每个算符都有事可做"却不保证输入有限。
- 建议修法：`amrex::Real const w[SolarNetwork::n_species] = {0.70,0.28,0.02};`（其余自动补 0，
  `test_energy_budget.cpp:56` 就是这么写的）。
- 验证方法：`-fsanitize=address` 编译该测试即报 `stack-buffer-overflow`。

### m1 【minor】精确 Riemann 参考解的 Newton 导数指数写错
- 文件:行 — `FluidTestReference.H:82-83`：`std::pow(p/p_k, -expo - 1)/(rho*c_k)`。
- 正确值：`d/dp[2c/(γ-1)((p/p_k)^e - 1)] = (p/p_k)^{e-1}/(ρ_k c_k)`，`e=(γ-1)/(2γ)`；代码给的是
  `-(3γ-1)/(2γ)`，正确是 `-(γ+1)/(2γ)`（γ=1.4：-1.143 vs -0.857）。因为 `solveStarState` 外层有
  bisection 夹逼（`:100-104`），只影响 Newton 收敛速度，结果仍对。置信度 confirmed。

### m2 【minor】`mu_e = 2` 的注释写反（"2 for hydrogen"）
- `DegenerateEOS.H:99`：氢的 `mu_e = A/Z = 1`，2 对应氦；太阳混合物的真值约 1.17。用 2 会把
  `n_e` 低估 1.7 倍、简并压低估 5 倍（`P∝n_e^{5/3}`）。太阳中心 `n_e~6e25 ≪ n_c=1.14e30`，故当前影响
  可忽略，但注释会误导后续使用者。置信度 confirmed。

### m3 【minor】简并压进入了动量/能量通量，但简并内能没有进入 E
- `DegenerateEOS.H:108-113`（`p = p_ideal + p_deg`）与 `FluidMoments.H:146`（`F_E=(E+p)u`）：E 里只有
  热+动能，简并压做的功 `p_deg ∇·u` 无处存 → 能量账本不闭合（在强简并区，这正是 O(1) 误差）。
  头注释 `:28-33` 承认"ideal-gas 部分会重复计数"，但没提能量不闭合。置信度 likely。

### m4 【minor】网络 Jacobian 的代价与可移植性
- `SolarNetwork.H:359-450`：每个 Newton 迭代用 9 次（n+2 次）`rhs` 做数值 Jacobian、最多 60 次迭代、
  9×9 高斯消元，全部是 host 代码且在 `AMREX_GPU_HOST_DEVICE` 头里（`rightHandSide` 标了 GPU 可用，
  `advance` 没有）。当前只在单测/host 路径使用，接进 GPU 构建前必须改成解析 Jacobian 或搬到 host 侧。
  置信度 confirmed（性能/可移植性，不是数值错误）。

### m5 【minor】电离的"速率"是无量纲比值，不是 1/s
- `Ionization.H:143`（`a = saha_function(T)/n`，量纲 = cm⁻³/cm⁻³ = 1）→ `relax_fraction` 把 `a` 当
  1/s 与 dt 相乘，电离时间尺度 ~`1/a` 秒，与任何碰撞/复合速率无关（真实复合率 ~1e-13 cm³/s）。
  `test_ionization.cpp:187` 的注释"the ionisation time scale here is 1/a = n/S ~ 3e3 s"说明作者按这个
  约定在用；但 `Ionization.H:14` 的 `dn_e/dt = a(n-n_e) - b n_e²` 左右量纲不一致。平衡点仍精确等于
  Saha（`x²/(1-x)=S/n`，我验证过），所以只影响瞬态。置信度 confirmed。

### m6 【minor】`applyRadiationCoupling` 硬编码 `extra(0)` 当电离度，与 `applyOpticallyThinCooling(x_comp)` 不一致
- `Radiation.H:241-242` vs `:82-83,105`。若 extra 0 是别的量（辐射能量、角动量密度），温度算错。
  置信度 confirmed。

### m7 【minor】子步尺度只用 `CellSize(0)`
- `Conduction.H:88`、`Radiation.H:137`、`Composition.H:60,128`、`StarFluidPlugin.cpp:112`、
  `AngularMomentum.H:89`：立方网格下等价；一旦 deck 给各向异性 dx（WarpX 支持），稳定步与 r_floor
  都只按 x 算。置信度 confirmed。

### m8 【minor】`total_angular_momentum` 只乘一次 dx
- `AngularMomentum.H:218`：`total += rho u_phi * r * dx`，应是 `dx^AMREX_SPACEDIM`（或
  `dx*dy*dz`）。作为比值型诊断无损，作为"守恒量"在 dx≠1 时差 `dx^{D-1}`。置信度 confirmed。

### m9 【minor】离心力只作用在一个动量分量 + 参考点取域中心
- `AngularMomentum.H:128`（只加 `momentum(axis)`）；`:112-114`（半径以 `0.5*(ProbLo+ProbHi)` 为原点，
  域不居中于轴时半径错）。2D 轴对称可用，3D 不完整。置信度 confirmed。

### m10 【minor】`applyStellarSources` 用 `AMREX_ALWAYS_ASSERT` 做物理符号检查
- `StellarSources.H:150`：release 构建里断言仍然生效，round-off 超限会 abort 整个仿真（注释里
  自己说 1e19 量级求和的噪声约 1e3）。同类：`:62-67 exchange_closure` 只被测试用。置信度 confirmed。

### m11 【minor】`StepResult.iterations` 差一
- `SolarNetwork.H:359-361`：`iterations = iter;` 在收敛 break 之前赋值 → 报告的是最后一次尝试的序号。
  无功能影响。

### m12 【minor】三个显式算子达到 `max_sub` 后静默少走时间
- `Radiation.H:201-202`、`Conduction.H:187-188`、`Composition.H:114-115`：`break` 后 `remaining > 0`
  直接返回，不报警。`test_stellar_model.cpp:204` 用默认 `max_sub=1000` 调用中子扩散，稠密介质下
  就是"只输运了 1/1000 的步长"。稳定性没问题，但算符没走完。置信度 confirmed。

### m13 【minor】唯一的端到端验证脚本 grep 一条不存在的日志，且跑在无关 deck 上
- `Tools/verify/check_star_runtime_plugins.sh:48`：`grep -q "g from bus: yes" both.log`，而
  `StarFluidPlugin.cpp:332-335` 打印的是 `g: %s` → 永远不匹配，`set -e` 下脚本直接退出。
  整脚本用的是 `Examples/Tests/boundaries/inputs_test_3d_particle_boundaries`（几格的边界测试 deck，
  `max_step=3`），域名在米级，星体初值退化成"整域中心密度"（见 F3），所以**星体物理没有任何端到端验证**。
  置信度 confirmed。

### m14 【minor】`test_solar_core` 没有测它声称的"随 dr 收敛"
- `test_solar_core.cpp:16-18`（"the residual converges with dr"）vs `:244-250`（只在 `n_cell==256`
  判 `res0 < 0.1`）。10% 的静水平衡残差被接受；两个网格的 `res0` 只打印不比较。置信度 confirmed。

### m15 【minor】`|ledger.gas_cooling| >= 0.0` 恒真
- `test_energy_budget.cpp:163`：显然本意是 `gas_cooling <= 0`（"冷却不能加热"）。置信度 confirmed。

### m16 【minor】初始化忽略 `warpx_stage_geometry` 返回值
- `StarFluidPlugin.cpp:157-161`：`try { (void)warpx_stage_geometry(...); } catch (...) {}` 之后立刻用
  `d[0..2]`。该 ABI 在失败时**先返回 0 后不填**（`PluginCAdapter.cpp:291`）→ `d={0,0,0}`、
  `plo={0,0,0}` → `cell_volume=0`、`r=0` 全域取中心密度、质量 0，且不报错。置信度 confirmed。

### m17 【minor】真空 floor 的常数很随意，且只 floor 一个物种
- `StarFluidPlugin.cpp:199-203`：只给 `density(0)` 和 `energy` 加 `1e-10 / 1e-6` 下限，
  `density(1)` 保持 0；`1e-6 erg/cm³` 对应的压力 4e-7 dyn/cm² 无出处，且与 `IdealGasEOS` 的
  `p=(γ-1)E` 一起会把"真空"变成 1e-6 的内能。注释只说"第一次产生 NaN 就是这里"，没有给出
  floor 的物理来源。置信度 confirmed（工程性）。

### m18 【minor】日志标量/初始化标志是文件级 static，restart/多实例下语义不清
- `StarFluidPlugin.cpp:39-42, 47`（`g_steps/g_energy/g_mach/g_initialised`）、
  `StarGravityPlugin.cpp:39-41, 152-153`（`g_steps/g_phi_min/g_levels_filled/g_loaded`）：
  进程内不复位；`star_steps` 会从 0 重新计数（新进程）但在同进程多次仿真时累加；
  `g_initialised` 使 restart 后的第二次 init 被跳过（`StarFluidPlugin.cpp:155`）。
  置信度 confirmed。

### m19 【minor】`test_sod_shock` 失败模式是"静默通过"（NaN 传染）
- `test_sod_shock.cpp:177-188`：判据是 `order_rho < 0.7` 与 `l1_rho.back() > 1e-2`。若 `state` 出现
  NaN（F6 的真实场景），`l1_rho` 是 NaN，`NaN > 1e-2` 为 false、`NaN < 0.7` 为 false → **PASSED**。
  建议所有测试的判据改成 `if (!(x <= tol))` 形式或先 `isfinite`。置信度 confirmed。

## 复核通过项（给出推导，避免重复劳动）

- HLLC：`SM`（`FluidMoments.H:189-190`）、星态密度/动量/能量（`:202-221`）、Davis 波速（`:171-172`）
  与 Toro 10.37-10.42 一致；多组分共享速度时 `U*_s = ρ_s(S-u)/(S-S_M)` 正确。
- MUSCL-Hancock：minmod 中心斜率（`:303-306`）与边值 Hancock 预测步
  `U^{n+1/2}_± = U_± + (Δt/2Δx)(F_∓ - F_±)`（`:322-328`）是标准二阶写法（我按特征形式推导过）。
- 奇异点：`advanceSweep` 的 2 层 ghost 刚好够（最高面需要 `bigEnd+2`，`:302`）✓；flux MultiFab 与
  state 同 DistributionMap，`flux.array(mfi)` 合法 ✓；`ParallelFor` 写冲突：**未发现**（本分区所有
  `ParallelFor` 都是逐格写自身）✓。
- 球坐标源项 `p_c(A_h-A_l)`（`Spherical.H:139`）= ∫p dA 的一阶离散，正确；`V`、`A` 也正确。
- CF88：f2/f4/f18/f24/f41/f42/f61/f64/f71/f76/f78 系数与 CF88 表 2 逐项一致（含 f24 的
  `T9'=T9/(1+4.95e-2 T9)`、f41 的 `2.515e-3/T9` 项、f78 的 `rc78` 分支）。N_A、MeV→erg、`f/N_A`
  的量纲都对。
- 网络化学计量：9 个反应的质量数/电荷数我逐个配平过（含 pp-I 的 2p→d、3He+3He→4He+2p，CNO 的
  n15+p→c12+he4，pp-II 折入的 be7+p→2he4）；`event_rate` 的加减与 `he4_produced` 计数一致。
- 简并常数：`k_nr = (3π²)^{2/3}ħ²/(5m_e) = 2.3375e-27`（题给 2.335e-27，差 0.1%）；
  `k_rel = (3π²)^{1/3}ħc/4 = 2.4454e-17`（2.440e-17，差 0.2%）；`n_c=(k_rel/k_nr)³=1.141e30`
  （1.1406e30）✓。混插公式在两极限的渐近行为我也验证过（低密度误差 `(n/n_c)^{2/3}`，与注释一致）。
- 太阳模型数值（cgs，来自 `data/ssm_S_profile.csv`）：中心 ρ=152.7 g/cm³、T=1.5634e7 K、
  P=2.3346e17 dyn/cm²，最外 r=1.000704 R⊙=6.9619e10 cm，`r_sun=6.957e10`、`m_sun=1.989e33`。
  与 GS98/BP04 标准太阳模型（ρ_c≈150、T_c≈1.57e7、P_c≈2.3e17）一致 → **不存在**"太阳中心值编造"
  的问题；有问题的只是单位（F3）与初始化实现（m16、F2）。
- 泊松算符：`setScalars(0,-1)` + `setACoeffs(0)` + `setBCoeffs(1)` 确实给出 `∇²φ`；`rhs=4πG(ρ-<ρ>)`
  的 Jeans swindle、周期解零均值（`:101-106`）、`g=-∇φ` 中心差分与 Laplacian 同阶 —— 都对。
  `#if defined(WARPX_DIM_RZ)` 的 `amrex::Abort`（`:68-73`）也确实是必要的（Cartesian 多重网格没有
  1/r 测度）。
- `test_jeans` 的 `λ_J = c_s√(π/(Gρ))`、`m=1`(λ=2λ_J) 不稳/`m=4`(λ=λ_J/2) 稳定、5% 增长率判据 —— 推导正确。
- `Spherical.H` 与 `FluidMoments.H` 都只用 `s(i,0,0,c)` 或 `ParallelFor`，不踩 F1。

## 不确定但值得复核的点

1. **F4 的交错位置**：分量语义已由上游 `WarpX.cpp:2549-2552` 确认（每方向 3 分量）。但
   `Efield_fp[dir]` 是 **Yee 交错**（`convert(ba, Ex_nodal_flag)` 等），插件当格心用（`ea[d](i,j,k,d)`
   不做插值），因此即使修好分量，力/热仍带 O(dx) 的交错误差。是否接受需在 PHYSICS_SPEC 里写明。
2. **F3 的 deck 单位**：`warpx_stage_geometry` 返回 deck 的 ProbLo/dx。若这些星体 deck 全部写成 cgs
   （`warpx.prob_lo = ... 6.957e10`），则流体侧自洽而重力侧仍错 10³；需要看一下真实的星体输入文件
   （我在本分区只找到 `Tools/verify/check_star_runtime_plugins.sh` 用的边界测试 deck）。
3. **`advanceSweep` 的 2 ghost 与 `FillBoundary`**：`StarFluidPlugin.cpp:278-297` 的
   `fill_halo` 是"最近邻复制"零梯度外推，对超音速外流（太阳表面、Sod）会在边界反射；这会污染
   `test` 之外的运行。是否接受取决于 deck 的边界意图，需要 PHYSICS_SPEC 的说明。
4. **`applySolarNetwork` 的 `t9`**：用 `gamma` 参数（插件传 1.4）与 `mass_fractions` 算 T，但
   `SolarNetwork::temperature` 用的 μ 是"完全电离"的平均分子量；太阳外层（部分电离/中性）会高估 T。
   单测里 `x=1` 掩盖了这一点。
5. **`max_components=16`**：`n_species=9` + `SPACEDIM=3` + 1 + `n_extra`，`n_extra>3` 时
   `advanceSweep` 的 `AMREX_ALWAYS_ASSERT`（`FluidMoments.H:260-262`）会在运行期 abort；当前
   `test_stellar_model` 用 `n_extra=2`（15 分量）刚好够，接成太阳全网络（+辐射+角动量）就会撞上。
6. **`g_steps` 的 restart 语义**（m18/F10）：需要跑一次 `amr.restart` 才能确认星体密度被
   `fill_all_levels` 重置 —— 建议加一条 ctest。
