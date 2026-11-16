# 边界条件插件设计：PEC（cut-cell，二阶）与电介质（subpixel，二阶）

目标架构：边界条件 = 独立插件，核心只认 `IBoundaryConditions`；场求解器（Yee/CKC/ECT/PSATD/
隐式/hybrid）与流体求解器（五阶矩/十阶矩/恒星）通过核心接口调用它，互不 include。
**先只做 Yee（CartesianYeeAlgorithm）这条线，作为参考实现与验证基线。**

## 1. PEC —— cut-cell，二阶

物理条件（EB 面）：`E_tangential = 0`，`B_normal` 连续。

现有接入点（已确认，非推断）：
- `IFiniteDifferenceSolver::EvolveE/D/...` 已带 `eb_update_E`（`iMultiFab[3]`），
  由 `WarpXInitData.cpp:1541` 传入、`WarpX.cpp:3019` 分配，语义是"该格点是否更新 E"。
  这是**阶梯（staircase）**版本，只有 0/1，没有切面几何。
- cut-cell 需要切面几何：`face_areas`、`edge_lengths`（`EmbeddedBoundaryInit.cpp` 里为 ECT 已构造），
  以及 AMReX `EB2::IndexSpace` 的切面质心。

二阶做法：
1. 对每个与 EB 相交的 Yee 棱（E 分量），用切面质心插值求棱上的 E，令其为零
   （不是把整格 E 置零——那只有一阶）；
2. B 的法向分量用切面面积加权（等价于在切面上做通量守恒），保证 `B_n` 连续；
3. 用 `edge_lengths` 修正 E 的环量，使 Faraday 定律在切面上仍是二阶。

实现落点：`IBoundaryConditions::ApplyPecToEfield/ApplyPecToBfield`，
在 `EvolveE.cpp` 的 `EvolveECartesian<CartesianYeeAlgorithm>` 之后调用（核心持接口指针，没插件则 no-op）。

## 2. 电介质 —— subpixel，二阶（注意：不是 cut-cell）

物理条件：`E_t` 连续、`D_n` 连续。

subpixel 平滑（Farjadpour et al., Opt. Lett. 31, 2972 (2006)；Kottke et al., PRE 77, 036611 (2008)）：
- 每个面中心的 ε 用相邻两格的材料体积分数做**方向性平均**：
  `ε_face^-1 = Σ f_i / ε_i`（法向方向用法向平均，切向用切向平均——各向异性等效 ε 张量）；
- 不做几何切割、不引入切面质心，保持二阶精度且不产生伪电荷；
- 表面电荷只在 `D_n` 不连续处出现，由 `ApplyDielectricJump` 计算并反馈到 J。

实现落点：`IBoundaryConditions::FillFacePermittivity`（每步前填充面心 ε），
Yee 的 E 更新已经用 `eps_r`（`EvolveE.cpp:270` 附近有 `MacroscopicEvolveE` 的宏观看路径），
所以 subpixel 只需替换/覆盖面心 ε 的来源，不动主循环结构。

## 3. 数据通道（阶段 0，必须先做）

EB 几何目前只在 `Modules/EmbeddedBoundary/EmbeddedBoundaryInit.cpp` 内部构造，
插件拿不到。核心需要一个只读访问接口 `IEBGeometry`：
- `flags(lev)`：`EBCellFlagFab`
- `faceAreas(lev, dir)`：切面面积
- `edgeLengths(lev, dir)`：切割棱长
- `volumeFraction(lev)`：体积分数
- `faceCentroid(lev, dir)`：切面质心（subpixel 不需要，cut-cell PEC 需要）

由核心在初始化时构造一次并持有；EB 插件、边界条件插件、ECT 求解器都通过它读取几何，
彼此不依赖。

## 4. 验证（每阶段都要有）

1. **PEC**：均匀 `E_z` + 接地导体圆柱，跑若干步后检查
   - 圆柱内 `E` 恒为 0（cut-cell 下是切面上 `E_t=0`，不是整格为 0）；
   - `B` 的法向分量在切面两侧连续；
   - 收敛阶：网格加密 2×，切面处误差按 4× 下降（二阶）。
2. **电介质**：平面介质界面 + 正入射波，检查反射/透射系数与解析 Fresnel 解一致，
   且网格加密时误差二阶下降；
3. **回归**：无 EB 时所有现有测试通过（插件未启用 = 零开销）。

## 5. 现状

- `Source/Core/Runtime/IBoundaryConditions.H`：已写（接口 + 能力查询 + 流体壁面条件），未编译。
- 本文件：设计定稿。
- 未做：阶段 0 的 `IEBGeometry`、PEC cut-cell kernel、subpixel ε、构建与验证。
