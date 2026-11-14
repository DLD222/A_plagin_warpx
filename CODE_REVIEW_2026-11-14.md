# WarpX 插件化 Fork 提交后正确性审查报告

- 审查日期：2026-11-14
- 审查对象：`/workspace`（WarpX 的非官方插件化 fork）
- 提交范围：`e04fead`（`docs(s7): the full 573-test run finally got a clean window: 571/573`，2026-11-14 10:33:49 +0800）
- 仓库历史：`git rev-list --count HEAD = 1`，历史被压缩为单一提交，因此**整棵源码树即“过去 24 小时变更集”**
- 工作区状态：干净（审查开始前 `git status --short` 无输出）
- 审查方式：静态、系统性、跨文件追踪执行路径；未编译、未运行测试、未修改任何代码或项目文件
- 本报告是本轮审查唯一新增的文件

---

## 0. 结论摘要

本轮在介质 Maxwell 求解器（macroscopic）、材质插件接口、插件注册总线、以及两个独立物理插件中发现 **11 项确认缺陷**，其中 **5 项会直接产生静默的物理错误或未定义行为**，**2 项会破坏插件化根本目标**，**1 项会导致 GPU 构建失败**。

| 编号 | 严重度 | 位置 | 一句话结论 |
|---|---|---|---|
| F-01 | S0 静默物理 | `MacroscopicEvolveE.cpp:186-282`、`MacroscopicProperties.cpp:447-456` | 默认 `algo.macroscopic_subpixel=0` 时电导率 σ **完全不起作用**，导体退化为无损介质，且警告文本谎称“回退到一阶 BE” |
| F-02 | S0 静默物理 | `MacroscopicProperties.cpp:485-560, 842-849`；`WarpXPushFieldsEM.cpp:1342-1366` | 3D 介质路径把 **y 索引当成 z** 使用，节点材料、σ 矩阵、`rho_medium` 散度全部错误；3D 宏观介质功能实质失效 |
| F-03 | S1 静默物理 | `MacroscopicProperties.cpp:497-560`；`MacroscopicEvolveE.cpp:417` | 2D XZ 法向分量 Ez 的节点材料存在**半格错位**，且导体界面首层 σ=0，导致法向 E 有一层永不阻尼 |
| F-04 | S1 未定义行为 | `MacroscopicEvolveE.cpp:382-431` | `eold3` 在三分量不同交错布局之间 `ParallelCopy`，边界留下**未初始化内存**，指数步读取脏值（张量介质会污染结果，NaN 可传播） |
| F-05 | S2 物理精度 | `MacroscopicProperties.cpp:528-616` | 节点平均把界面法向**硬编码为 z**；x 法向/倾斜界面的标量 eps 分支忽略 `nrmv`，张量分支 f 恒为 0.5，亚网格平均退化 |
| F-06 | S2 诊断错误 | `MacroscopicProperties.cpp:458-460, 668-696`；`WarpXPushFieldsEM.cpp:1308-1309, 1362-1363` | 节点 eps/σ 数组未做 `FillBoundary`，周期缝上 `rho_medium` 与 Gauss 残差读 0 ghost（主求解不受影响，但诊断会假报守恒破坏） |
| F-07 | S1 构建 | `SubpixelTensor.H:90-115` | `frame()` 缺少 `AMREX_GPU_HOST_DEVICE`，却被 host-device 函数与 device lambda 调用；CUDA/HIP/SYCL 构建失败 |
| F-08 | S0 架构 | `MacroscopicEvolveE.cpp:51-57`；`IMacroscopicProperties.H:27-76` | 求解器对具体 `MacroscopicProperties` 做 `dynamic_cast`，且接口缺少指数积分所需方法；第三方材质插件无法接入 |
| F-09 | S1 架构 | `ServiceFactory.cpp:28-40, 49-67, 163-167` | 服务注册表是**单槽赋值**，无重复检测；同类插件不能共存，链接/静态初始化顺序决定哪个实现生效 |
| F-10 | S2 物理/内存 | `NewtonGravity.cpp:129-132` | 1D（及 RCYLINDER/RSPHERE）构建中 `computeAcceleration` 使用 `j±1` 越界/别名索引；RZ 无柱几何项 |
| F-11 | S2 可复现性 | `MacroscopicEvolveE.cpp:217`；`MacroscopicProperties.cpp:494-496, 930-931` | 生产代码含非物理 A/B 环境变量开关与输入参数开关，相同输入可因环境不同得到不同物理；调试打印无条件输出 |

补充说明：`MacroscopicEvolveE.cpp:61-64` 对 RZ 直接 `WARPX_ABORT`（“currently macro E-push does not work for RZ”）属于**上游继承限制**，不作为本 fork 新增缺陷计入；但 3D 的功能缺陷（F-02）属于本 fork 让其“看起来支持、实际算错”的静默问题。

---

## 1. F-01（S0，静默物理）：默认 `macroscopic_subpixel=0` 时电导率完全失效

### 1.1 位置

- `Source/Plugins/PicEm/WarpX.H:287`：`static inline int macroscopic_subpixel = 0;`
- `Source/Plugins/FieldSolver/FiniteDifference/MacroscopicProperties/MacroscopicProperties.H:124`：`int m_macroscopic_subpixel = 0;`
- `Source/Plugins/PicEm/Modules/FieldSolver/WarpXPushFieldsEM.cpp:1240-1247`
- `Source/Plugins/FieldSolver/FiniteDifference/MacroscopicEvolveE.cpp:186-208, 253-282, 293-319, 330-356, 369`
- `Source/Plugins/FieldSolver/FiniteDifference/MacroscopicProperties/MacroscopicProperties.cpp:447-456`

### 1.2 触发条件

任意输入：

```text
algo.em_solver_medium = macroscopic
macroscopic.sigma > 0        # 或 sigma_function > 0
# 未设置 algo.macroscopic_subpixel，或显式设为 0（默认值）
```

### 1.3 完整执行路径

1. `algo.macroscopic_subpixel` 默认 0（`WarpX.H:287`、`MacroscopicProperties.H:124`）。
2. `WarpX::MacroscopicEvolveE` 在 `WarpXPushFieldsEM.cpp:1240` 判断
   `if (WarpX::macroscopic_subpixel && !has_subpixel_rows())`；条件为假，**`InitSubpixelRows` 永远不会被调用**。
3. `InitSubpixelRows` 内部的 `fill_sigma_mats()`（`MacroscopicProperties.cpp:792-933`）是唯一构建 M1/M2、J0/J1 并设置 `m_sigma_mat_ready = true`（第 929 行）的地方。
4. 因此 `m_sigma_mat_ready` 保持 `false`，`has_sigma_matrices()` 返回 `false`（`MacroscopicProperties.H:108`）。
5. `MacroscopicEvolveECartesian` 中 `has_sig_mat = false`（`MacroscopicEvolveE.cpp:186`）：
   - `sn0/sn1/sn2` 不绑定（第 203-208 行）；
   - 三个分量的 `sigma_node` 都被赋为 `0.0`（第 253、293、330 行）；
   - “sigma > 0 跳过 BE、交给指数步”的分支不触发。
6. 更新公式被改为（以 Ex 为例，第 280-282 行）：

   ```cpp
   Ex(i,j,k) = Ex(i,j,k)
             + dt * ( ix0*Dx + ix1*Dy + ix2*Dz )
             - dt * ix0 * jx(i,j,k);
   ```

   这是**无损介质的 Yee 更新**。第 265-266 行计算出的 `alpha`、`beta`、第 263-264 行的 `epsilon_interp` 全部**未被使用**（死代码）。
7. 指数步由 `if (macroscopic_properties->has_sigma_matrices())`（第 369 行）门控，同样不执行。
8. 最终：σ 对 E 的阻尼**一次都没有发生**。

### 1.4 根因

P1 重构把原来的 `alpha*E + beta*(curlH - J)` 形式替换为“`E + dt*eps^-1(curlH - J)`”的矩阵形式，以支持各向异性 eps；但**没有把与 σ 相关的 `alpha/beta` 阻尼项并入新公式**。随后又用 `has_sigma_matrices()` 把唯一的阻尼实现（指数步）锁在 `subpixel=1` 之后，形成“默认路径既无 BE 阻尼、也无指数阻尼”的空洞。

### 1.5 对仿真的影响（静默性）

- 导体（`sigma` 很大）在默认输入下表现为**无损介质**：波不会衰减，只会以 `c/sqrt(eps_r)` 传播/反射；趋肤深度、焦耳热、导体屏蔽全部消失。
- 能量不守恒于真实物理（但数值上“守恒”，所以不会崩溃、不会 NaN），因此极易被误判为“结果正常”。
- 更危险的是 `MacroscopicProperties.cpp:447-456` 的警告：

  ```text
  "macroscopic.sigma > 0 but algo.macroscopic_subpixel=0: the E update
   falls back to the first-order backward-Euler integrator instead of
   the exact exponential step."
  ```

  该文本**与实现不符**：并不存在“一阶 BE 阻尼回退”，BE 分支本身也没有阻尼。用户看到警告后若认为“只是精度降为一阶”，会得到完全错误的物理结论。
- 按项目 `AGENTS.md`，`warpx.abort_on_warning_threshold` 若设为高于该警告等级，警告不会中止运行，缺陷完全静默。

### 1.6 为什么 571/573 测试没有发现

- 所有带 `sigma > 0` 的基准（`benchmarks/cases/c03_medium/lossy_slab`、`conductor_slab`）都显式写了 `algo.macroscopic_subpixel = 1`。
- `Examples/Tests/macroscopic_solver` 的两个算例是 `sigma = 0` 的均匀介质/真空腔，只覆盖无损 BE 路径。
- 没有任何测试使用 `sigma > 0` 且 `subpixel` 默认 0。

### 1.7 建议的复现实验（不改代码）

复制 `conductor_slab/inputs_2d`，删除 `algo.macroscopic_subpixel = 1` 一行，其余不变；对比导体后方透射场与解析衰减。预期：透射场不随 σ 衰减，且与 σ=0 的介质结果几乎一致。

---

## 2. F-02（S0，静默物理）：3D 介质路径把 y 索引当作 z

### 2.1 位置

- `Source/Plugins/FieldSolver/FiniteDifference/MacroscopicProperties/MacroscopicProperties.cpp:485-488, 510-532, 546-560, 584-616, 842-849`
- `Source/Plugins/PicEm/Modules/FieldSolver/WarpXPushFieldsEM.cpp:1342-1366`

### 2.2 触发条件

任意 3D 构建（`WARPX_DIM_3D`）+ 宏观介质 + 界面（`interface_function`）或非均匀材料。

### 2.3 完整执行路径与证据

3D 的 Yee 交错（`Source/Plugins/PicEm/WarpX.cpp:2787-2796`）：

```cpp
Ex_nodal_flag = IntVect(0,1,1);   // cell in x, node in y, node in z
Ey_nodal_flag = IntVect(1,0,1);
Ez_nodal_flag = IntVect(1,1,0);
```

因此：

- 3D 中第二个索引 `j` 是 **y**，第三个索引 `k` 是 **z**；
- 沿 z 的相邻单元是 `k-1, k`，而不是 `j-1, j`。

但节点材料构造（`MacroscopicProperties.cpp`）沿用 2D XZ 代码：

```cpp
amrex::Real const zlo_n = geom.ProbDomain().lo(AMREX_SPACEDIM-1); // 3D 正确取到 z
amrex::Real const dz_n  = geom.CellSize(AMREX_SPACEDIM-1);        // 3D 正确取到 dz
...
int const jlo_dom = geom.Domain().smallEnd(1);  // 3D 这里是 y
int const jhi_dom = geom.Domain().bigEnd(1);
...
int const iL = ii, iR = ii;
int const jm = amrex::max(j - 1, jlo_dom);
int const jp = amrex::min(j, jhi_dom);
int const jL = jm, jR = jp;                     // 取的是 y 邻居
...
amrex::Real const xn = xlo_n + amrex::Real(i) * dx_n;
amrex::Real const zn = zlo_n + amrex::Real(j) * dz_n;  // y 索引 × dz
...
phi_n(xn + hh0, amrex::Real(0.0), zn)          // y 坐标恒为 0
```

问题链：

1. **邻居方向错**：所有分量都沿 y 取两侧材料，而不是沿各分量真正的法向/交错方向；3D 的 `k` 从未参与。
2. **坐标错**：`zn = zlo + j*dz` 把 y 索引乘上 z 间距，`phi_n(..., 0.0, ...)` 把 y 固定为 0；对 y 方向变化的材料完全采样不到。
3. **法向判定错**：`bool const normal = (c == 2)`（第 498 行）把 c=2（Ez）当作唯一法向；在 3D 中界面法向可以是 x/y/z 任意方向。
4. **σ 矩阵错**：`fill_sigma_mats` 的单元填充同样使用 `zc = plo_z + (j+0.5)*dzs`（第 843 行）与 `phi_s(x, 0, z)`（第 849 行），3D 下把 y 当 z。
5. **`rho_medium` 散度错**：`WarpXPushFieldsEM.cpp:1347-1363` 只有 x 与 z 两项：

   ```cpp
   rh(i,j,k) += -dtl * ((jx(i,j,k)-jx(i-1,j,k))/dx
                     + (jz(i,j,k)-jz(i,j-1,k))/dz);
   ```

   3D 中 `j` 是 y，却除以 `dz`；**缺少 y 方向散度项**。seed（第 1308-1309 行）同样如此。

对照证据：同文件的 `InitializeMacroMultiFabUsingParser` 有**正确的 3D 分支**（`AMREX_SPACEDIM == 3` 时分别用 `k` 索引和 `dz`），说明节点构造这段 3D 行为是遗漏，而不是设计选择。

### 2.4 对仿真的影响

- 3D 介质界面的 ε/σ 节点值、亚网格平均、M1/M2 矩阵全部建立在错误的邻居与坐标上；结果不是“精度差”，而是**物理上无意义**。
- 3D 各向异性/倾斜界面几乎必然错误；即使是轴对齐 z 界面，σ 的节点平均也沿 y 取，导致阻尼分布错误。
- `rho_medium` 在 3D 的守恒诊断错误（缺 y 项），会掩盖或伪造 Gauss 定律偏差。
- 非均匀材料的 3D 模拟会静默给出错误反射/透射系数，且不会崩溃。

### 2.5 为什么测试没有发现

- `benchmarks/cases/c03_medium/*` 全部是 2D（`inputs_2d`）。
- 唯一的 3D 宏观算例 `Examples/Tests/embedded_boundary_cube/inputs_test_3d_embedded_boundary_cube_macroscopic` 是**均匀 ε、σ=0**，节点材料处处相同，任何邻居/坐标错误都被掩盖。
- 因此“3D 构建通过测试”不能证明 3D 介质功能正确。

### 2.6 建议的复现实验

构造 3D 两介质平板（界面法向沿 y）：`interface_function(x,y,z)=y-0.5`，s/p 两种极化各一个算例，对比 2D 同参数结果。预期 3D 结果对 y 界面不敏感或出现非物理反射。

---

## 3. F-03（S1，静默物理）：法向分量 Ez 的半格错位与首层不阻尼

### 3.1 位置

- `Source/Plugins/PicEm/WarpX.cpp:2776-2786`（2D XZ 交错）
- `Source/Plugins/FieldSolver/FiniteDifference/MacroscopicProperties/MacroscopicProperties.cpp:497-560`
- `Source/Plugins/FieldSolver/FiniteDifference/MacroscopicEvolveE.cpp:417`

### 3.2 机理

2D XZ 中：

```cpp
Ex_nodal_flag = IntVect(0,1);   // x cell-centered, z nodal
Ey_nodal_flag = IntVect(1,1);   // x nodal, z nodal
Ez_nodal_flag = IntVect(1,0);   // x nodal, z cell-centered
```

AMReX 约定：nodal 索引 `i` 对应物理坐标 `i*dx`；cell-centered 索引 `j` 对应 `(j+0.5)*dz`。

因此 Ez 的真实位置是 `(i*dx, (j+0.5)*dz)`，即**位于 z 方向的单元中心**，而不是单元面。但节点材料代码对所有分量统一使用：

```cpp
amrex::Real const zn = zlo_n + amrex::Real(j) * dz_n;   // 把 j 当节点坐标
...
int const jm = amrex::max(j - 1, jlo_dom);
int const jp = amrex::min(j, jhi_dom);
int const jL = jm, jR = jp;                             // 取 j-1, j 两个单元
```

这等于把 Ez 样本**平移了半格**，认为它位于 `z = j*dz` 的单元面上；两侧单元也随之取成 `j-1, j`，而真实位置所属/相邻的单元关系并非如此。`phi_n(xn, 0, zn∓0.25dz)`（第 559-560 行）同样以错位后的 `zn` 为中心判断材料。

### 3.3 与 σ 规则的叠加

σ 的法向规则是谐波平均（第 539-545 行）：

```cpp
sv_norm = (sL+sR>0) ? 2*sL*sR/(sL+sR) : 0;
```

当界面一侧为真空（σ=0）时，`sv_norm = 0`。于是界面节点 `sn2 = 0`：

- BE 循环中 `sigma_node = 0`，不跳过、也不阻尼（因为 `alpha/beta` 未使用）；
- 指数步第 417 行 `if (sn(i,j,k,c) <= 0.0) return;` 直接返回。

结果：**紧贴导体表面的那一层法向 E（Ez）永不衰减**，只能靠相邻内部节点的强衰减间接影响。

### 3.4 对仿真的影响

- p 极化（Ez≠0）在导体/介质界面上的反射系数、相位、趋肤层结构错误；会看到非物理的法向场穿透与表面电荷振荡。
- 由于该层 E 不阻尼，可能出现长时间不衰减的伪表面模，污染远场诊断。
- 该问题在 `MacroscopicProperties.cpp:534-538` 的注释中已被作者自己描述为 “interface node has sigma = 0 … biases the p-pol reflection”，但没有修复，且**没有任何 p 极化界面测试**。

### 3.5 为什么测试没有发现

`benchmarks/cases/c03_medium` 的所有界面算例都是：

- `lossy_slab/inputs_2d`：Ey（s-pol）
- `conductor_slab/inputs_2d`：Ey（s-pol）
- `tensor_interface/inputs_2d`：Ey（s-pol）
- `anisotropic_slab/inputs_2d_p`：Ex，但为**均匀 bulk 正入射**，没有界面
- `anisotropic_slab/inputs_2d_s`：Ey

即：**没有 Ez≠0 的界面测试**，也没有 p 极化导体测试。

---

## 4. F-04（S1，未定义行为）：`eold3` 跨交错布局复制留下未初始化边界

### 4.1 位置

`Source/Plugins/FieldSolver/FiniteDifference/MacroscopicEvolveE.cpp:376-388, 395-431`

### 4.2 代码

```cpp
amrex::Vector<amrex::MultiFab> eold3(3);
for (int c = 0; c < 3; ++c) {
    eold3[c].define(Efield[c]->boxArray(), Efield[c]->DistributionMap(), 3, 0);
    for (int d = 0; d < 3; ++d) {
        eold3[c].ParallelCopy(*Efield[d], 0, d, 1);
    }
}
```

注释声称：

```text
ParallelCopy is a pure index copy: all three E components live on the
same index space, only their box TYPES differ.
```

### 4.3 根因

该断言是**错的**。三个 E 分量的 nodal flag 不同（2D：`Ex=(0,1)`、`Ey=(1,1)`、`Ez=(1,0)`；3D 见 F-02），因此：

- 它们的 `BoxArray` 索引空间**范围不同**（例如 2D 中 Ex 的 valid box 在 j 方向到 `nz`，Ez 只到 `nz-1`；Ez 在 i 方向到 `nx`，Ex 只到 `nx-1`）。
- AMReX 的 `ParallelCopy` 按索引交集复制，不会为“源没有该索引”的位置写入任何值。
- `eold3[c].define(..., 3, 0)` 只分配、不初始化（无 `setVal`），这些空洞保持**未初始化内存**。

随后指数步遍历 `tb = mfi_n.validbox()`（第 396 行，即 `Efield[c]` 自己的完整交错 box），并在第 425 行读取**全部三个分量**：

```cpp
amrex::Real const eold[3] = { eo(i, j, k, 0), eo(i, j, k, 1), eo(i, j, k, 2) };
```

在 `eold3[c]` 的边界行/列上，至少有一个 `eo(..., d)` 来自从未被写入的内存。例如 2D：

- `eold3[0]`（Ex 布局）的 comp 2 在 `j = nz` 处是空洞（Ez 无此索引）；
- `eold3[1]`（Ey 布局）的 comp 0 在 `i = nx`、comp 2 在 `j = nz` 处是空洞；
- `eold3[2]`（Ez 布局）的 comp 0 在 `i = nx` 处是空洞。

### 4.4 对仿真的影响

- 当 M1/M2 为**对角**（各向同性 ε、σ）时，空洞值乘 0；但 IEEE `0 * NaN = NaN`，若未初始化内存恰好是 NaN 位模式，NaN 会沿指数步传播到整个场。
- 当 M1/M2 有**非对角耦合**（各向异性 ε 或 σ 张量）时，空洞值直接以非零权重进入 `en`，边界处场值被污染；这类组合当前**没有任何测试**。
- 结果依赖内存分配历史，可能表现为“同一输入、不同 MPI 分解/机器得到不同结果”，或偶发 NaN。
- 在不做 `setVal` 的前提下，该问题是结构性的；不应依赖“通常分配到的页是 0”这种偶然性。

### 4.5 补充

该路径只在 `has_sigma_matrices()` 为真（即 `subpixel=1` 且 σ>0 或 σ 张量）时执行。现有 c03 的 σ 算例恰好是各向同性对角材料，非对角权重为 0，因此未暴露。

---

## 5. F-05（S2，物理精度）：界面法向被硬编码为 z，标量分支忽略 `nrmv`，f 恒为 0.5

### 5.1 位置

`Source/Plugins/FieldSolver/FiniteDifference/MacroscopicProperties/MacroscopicProperties.cpp:528-560, 584-616, 599-609`

### 5.2 事实

- 第 528 行：`int const iL = ii, iR = ii;`——两侧都取同一个 x 单元，**永远不沿 x 取邻居**。
- 第 498 行：`bool const normal = (c == 2);`——只有 c=2 被视为法向，即**默认界面法向为 z**。
- 第 548-557 行计算了 `nrmv`，但标量 eps 分支（第 584-590 行）完全没有使用它：

  ```cpp
  if (!tensor_mode) {
      amrex::Real const ev = normal
          ? harmonic(eL(0,0), eR(0,0))
          : 0.5*(eL(0,0)+eR(0,0));
      for (int q = 0; q < 3; ++q) { en(i,j,k,q) = ev; sn(i,j,k,q) = sv; }
      ...
  }
  ```

  因此**标量材料的倾斜界面**仍然按 z 法向做谐波/算术平均。
- 张量分支虽然用 `nrm` 调用了 `chi_inv_anisotropic`（第 609 行），但：

  ```cpp
  warpx::subpixel::chi_inv_anisotropic(eL, eR, amrex::Real(0.5), nrm, chi);
  ```

  填充比 f **硬编码为 0.5**，没有像 `fill_sigma_mats` 那样做 4×4 子采样（第 840-852 行）。除界面恰好落在节点中点外，Kottke 平均的几何位置是错的。
- 材料侧选择 `m1L/m1R` 只沿 z 在 `zn∓0.25dz` 采样（第 559-560 行），对倾斜界面/ x 法向界面会选错材料。

### 5.3 影响

- x 法向界面：Ex 是法向分量，但代码把 Ex 当切向（`normal=(c==2)` 为假），且 `iL=iR` 导致两侧 ε 相同 → **完全没有亚网格平均**，退化为阶梯。
- 倾斜界面：标量材料退化为 z 法向的阶梯近似；张量材料的法向方向对，但填充比固定 0.5，等效界面被吸附到半格位置。
- 后果是反射/透射系数的一阶几何误差，以及高对比度材料下的非物理表面波；不是崩溃型 bug，但与“亚网格平均提高界面精度”的设计目标不符。
- 现有测试全部是 z 法向轴对齐界面，无法发现。

---

## 6. F-06（S2，诊断错误）：节点材料数组缺少周期 `FillBoundary`

### 6.1 位置

- `MacroscopicProperties.cpp:458-460`：只对 cell-centered 的 `m_eps_mf / m_sigma_mf / m_mu_mf` 做 `FillBoundary`。
- `MacroscopicProperties.cpp:668-696`：`m_eps_node / m_sigma_node / m_eps_inv_node` 在 `InitData` 中按解析式填充，但**没有**周期交换；第 693-696 行的注释声称“FillBoundary … so that i-1 / j-1 reads … are wrapped values”，其下并没有对应代码。
- 使用点：`WarpXPushFieldsEM.cpp:1308-1309`（seed）、`1362-1363`（update）、`1412+`（Gauss 残差）都用 `en0(i-1,j)` / `en2(i,j-1)`。

### 6.2 影响

- `m_eps_node[c]` 分配时 `setVal(0.0)`（`MacroscopicProperties.cpp:348`），ghost 从未被周期填充。
- 在周期边界缝上，`rho_medium` 的 seed/update 与 `div D - rho_medium` 诊断会在缝后第一个节点读到 ε=0，导致：
  - `rho_medium` 在缝处错误；
  - Gauss 残差在缝处出现虚假的非零值，可能被误读为“守恒被破坏”。
- **重要限定**：主 E 更新只在本地节点索引读取节点 ε（指数步）或通过已填充的 cell ε 插值（BE 路径），因此**主求解不直接受此影响**；这是诊断/守恒检查层面的缺陷。报告中不应把它夸大为“场求解错误”。

---

## 7. F-07（S1，构建）：`frame()` 缺少 GPU 注解

### 7.1 位置

`Source/Plugins/FieldSolver/FiniteDifference/MacroscopicProperties/SubpixelTensor.H:90-115`

```cpp
Mat3 frame (const amrex::Real n_in[3])
{
    ...
    amrex::Real const nn = std::sqrt(...);
    ...
}
```

### 7.2 证据链

- 同文件其余数值函数（`tau_of`、`tau_inv_of`、`chi_inv_isotropic`、`chi_inv_anisotropic`、`mat3_fun`、`jacobi_eigh`）均带 `AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE`（第 118、140、160、181、209、313、363、380 行）。
- `chi_inv_anisotropic`（第 380-384 行）是 `AMREX_GPU_HOST_DEVICE`，内部直接调用 `frame(n)`。
- `MacroscopicProperties.cpp:497-665` 的 `amrex::ParallelFor(b, [=] AMREX_GPU_DEVICE ...)` lambda 也调用到 `chi_inv_anisotropic` / `frame`。

在 CUDA/HIP/SYCL 编译中，`__host__ __device__` 函数调用未标注的 host 函数是**编译错误**（`calling a __host__ function from a __host__ __device__ function is not allowed`），而不是警告。CPU 构建不受影响，因此 571/573 的 CPU 测试全部通过。

### 7.3 影响

- 任何启用 GPU 后端的构建（`-DWarpX_COMPUTE=CUDA/HIP/SYCL`）在编译该插件时失败；`.github/workflows` 中的 GPU 编译任务会暴露该问题，但本地 CPU 测试不会。
- 这是“测试通过 ≠ 可移植”的典型案例：项目声称支持多后端，而新增的材质插件代码路径只验证了 CPU。

---

## 8. F-08（S0，架构）：`dynamic_cast` 收窄 + 接口缺方法，第三方材质插件无法接入

### 8.1 位置

- `Source/Core/Runtime/IMacroscopicProperties.H:27-34`（契约注释）
- `Source/Core/Runtime/IMacroscopicProperties.H:59-76`（接口方法列表）
- `Source/Plugins/FieldSolver/FiniteDifference/MacroscopicEvolveE.cpp:51-57`

### 8.2 矛盾

接口注释写明：

```text
The kernel and the field-solver plugin both need the medium coefficients ...
They talk to this contract, so neither side names the concrete implementation
and the material model can move into its own plugin.
```

但求解器第一行就是：

```cpp
auto* macroscopic_properties =
    dynamic_cast<MacroscopicProperties*>(&macroscopic_properties_contract);
WARPX_ALWAYS_ASSERT_WITH_MESSAGE(
    macroscopic_properties != nullptr,
    "MacroscopicEvolveE expects the material model of this plugin");
```

更关键的是，求解器后续使用的以下方法**根本不在 `IMacroscopicProperties` 中声明**：

- `has_sigma_matrices()`
- `get_sigma_m1_node_mf(c)`、`get_sigma_m2_node_mf(c)`
- `get_sigma_node_mf(c)`
- `get_j0_node_mf(c)`、`get_j1_node_mf(c)`
- `get_eps_inv_node_mf(c)`
- `get_sigma_dt()`
- `geteps_inv_x/y/z_mf()`

接口只暴露了 `get_jcond_mf`、`get_eps_node_mf`、`getsigma_mf`、`getepsilon_mf`、`getmu_mf`、`has_subpixel_rows`、`rho_medium_*`、`gauss_check_step`、`materials`。

### 8.3 影响

- 这正是 `dynamic_cast` 存在的原因：求解器需要接口之外的数据。于是任何**第三方材质插件**（按项目目标由用户自行编写）即使正确实现了公开接口，也会在运行时被断言拒绝，无法替换材料模型。
- “核心与插件通过接口解耦”的目标在最重要的介质求解路径上被违反；材质模型实际上是**编译期绑死**的。
- `dynamic_cast` + `WARPX_ALWAYS_ASSERT` 还把“接口不匹配”从编译期错误变成了运行时错误，且只有在真正进入宏观求解时才会触发。
- 从插件化/模块化评分看，这是 S0 级结构缺陷：它直接阻断了用户扩展的核心用例。

### 8.4 建议的验证

在树外写一个最小 `IMacroscopicProperties` 实现并注册工厂，使用 `algo.em_solver_medium=macroscopic` 运行：预期在 `MacroscopicEvolveE.cpp:55` 断言失败。

---

## 9. F-09（S1，架构）：服务工厂单槽静默覆盖

### 9.1 位置

`Source/Core/Runtime/ServiceFactory.cpp:28-40, 49-67, 163-167`

### 9.2 事实

```cpp
struct Registry {
    DiagnosticsServiceFactory diagnostics = nullptr;
    ReducedDiagsServiceFactory reduced_diags = nullptr;
    ...
    MacroscopicPropertiesFactory macroscopic = nullptr;
    ...
};
```

所有 `register_*_factory` 都是无条件赋值：

```cpp
void register_macroscopic_properties_factory (MacroscopicPropertiesFactory factory)
{
    registry().macroscopic = factory;
}
```

没有：

- 重复注册检测；
- 名称/键；
- 实现列表（`std::vector`）；
- 运行期选择机制。

### 9.3 影响

- 两个同类插件（例如两个碰撞处理器、两个材质模型、两个诊断后端）同时链接时，**后注册者静默覆盖先注册者**；实际生效者由静态初始化/链接顺序决定，输入文件无法选择。
- 这与“插件之间相互独立、通过核心总线接入”的目标直接冲突：总线只能承载**每类恰好一个**实现，无法组合。
- 更隐蔽的是没有诊断输出：用户不会知道自己的插件没有生效。
- `clear_service_factories()`（第 182-186 行）会把整个表清空，说明设计上存在“全局单例 + 重置”的耦合，不适合多实例/多插件共存。

### 9.4 与 F-08 的关系

F-08 让“材质模型插件”无法替换；F-09 让“即使能替换，也只能有一个”。两者叠加使“用户自行编写新功能插件”的核心目标在材质方向上不可实现。

---

## 10. F-10（S2，物理/内存）：NewtonGravity 1D 与 RZ 维度缺陷

### 10.1 位置

`Source/Plugins/NewtonGravity/NewtonGravity.cpp:117-133`

```cpp
#elif defined(WARPX_DIM_XZ) || defined(WARPX_DIM_RZ)
    gx = -(phi(i+1,j,k)-phi(i-1,j,k))/(2*dx[0]);
    gy = 0;
    gz = -(phi(i,j+1,k)-phi(i,j-1,k))/(2*dx[1]);
#else
    gx = 0; gy = 0;
    gz = -(phi(i,j+1,k)-phi(i,j-1,k))/(2*dx[0]);   // 1D: j 不存在
#endif
```

### 10.2 问题

- **1D / RCYLINDER / RSPHERE**（`AMREX_SPACEDIM == 1`）：`Array4` 的 `j` 维长度为 1，`j±1` 是越界索引。AMReX 的线性偏移实现会使 `j+1` 别名到相邻 `i` 的数据（或在最后一个单元越界读），得到沿错误轴的差分；同时除以 `dx[0]`，量纲看似正确、结果完全错误。
- **RZ**：代码把 RZ 当作 XZ 笛卡尔处理，没有 `1/r` 项；而 `solvePotential` 使用 `MLABecLaplacian`（`NewtonGravity.cpp:68`），在 RZ 几何下同样不是柱坐标 Poisson 算子。若在 RZ 中使用该插件，引力场在物理上是错的。
- 测试只在 3D 注册（`Source/Plugins/NewtonGravity/CMakeLists.txt:27-39`，且 `GravityTestReference.H:28-31` 明确写 “formulas are for three dimensions only … registered for WARPX_DIMS=3”），1D/RZ 分支从未被执行。

### 10.3 影响范围

该插件目前没有在核心总线注册（`Grep NewtonGravity` 只命中插件自身文件与 CMake），主 WarpX 求解流程不会自动调用它；因此当前影响限于直接使用该库/自行接线的用户。但它是仓库交付的插件，1D 构建会编译该分支，属于真实的正确性/内存缺陷。

---

## 11. F-11（S2，可复现性）：生产代码中的非物理开关与无条件调试输出

### 11.1 位置与事实

- `Source/Plugins/FieldSolver/FiniteDifference/MacroscopicEvolveE.cpp:212-217`：

  ```cpp
  // Temporary A/B switch (controlled experiment, one binary):
  //   WARPX_YEE_NODE_EPS set   -> node tau eps^-1 rows (option B)
  //   unset                    -> cell Kottke eps^-1 rows (option A, the old path)
  bool const use_node_eps = (std::getenv("WARPX_YEE_CELL_EPS") == nullptr);
  ```

  环境变量直接改变 E 更新使用哪一套 ε⁻¹ 行，但**不进入输入文件、不进入校验和、不进入日志**。相同输入在不同 shell 环境下可得到不同物理结果。

- `Source/Plugins/FieldSolver/FiniteDifference/MacroscopicProperties/MacroscopicProperties.cpp:490-496`：

  ```cpp
  // Temporary mechanism-probe switch (default 0 = physical harmonic normal mean):
  int sig_arith_opt = 0;
  amrex::ParmParse("macroscopic").query("sigma_normal_arith", sig_arith_opt);
  bool const sig_arith_norm = (sig_arith_opt != 0);
  ```

  这是可通过输入文件打开的**非物理**开关（把串联谐波平均改为算术平均）。注释自己标注 “NOT physics”，但它仍然存在于生产路径。

- 无条件调试打印：
  - `MacroscopicProperties.cpp:165-169`（`[MACRO] eps1 tensor`）
  - `MacroscopicProperties.cpp:930-931`（`[SIGMA-EXP] ...`）
  - `WarpXPushFieldsEM.cpp:1221-1233`（`[DIAG]` 材料数组）

### 11.2 影响

- 科学可复现性：输入卡不能完整描述一次运行；环境变量是“隐藏输入”。
- 物理正确性：`sigma_normal_arith` 会让导体界面节点的 σ 从 0 变为非零，直接改变 p 极化反射，且没有对应测试。
- 日志污染：`[SIGMA-EXP]` 等打印在每次初始化时输出，可能被误认为官方诊断；在批量运行中增加噪声。

---

## 12. 为什么 571/573 通过仍然漏掉这些缺陷

1. **介质测试的极化/方向覆盖不全**：c03 全部是 z 法向界面，且界面算例只用 Ey（s-pol）；没有 Ez（p-pol）、没有 x 法向、没有倾斜界面。
2. **维度覆盖不全**：所有界面/σ 测试是 2D；唯一 3D 宏观算例是均匀材料。3D 节点逻辑（F-02）完全未执行。
3. **开关组合覆盖不全**：σ>0 的算例都强制 `subpixel=1`；没有 `subpixel=0 + σ>0`（F-01），也没有“张量 ε + σ>0”（F-04 的非对角权重场景）。
4. **诊断量未作为断言**：`rho_medium` / Gauss 残差在周期缝的错误（F-06）没有被测试脚本断言。
5. **CPU-only 验证**：F-07 是设备编译错误，CPU 测试天然无法发现。
6. **插件接口未被替换测试**：没有任何测试实现第二个 `IMacroscopicProperties`，也没有注册第二个同类服务，因此 F-08/F-09 不会被触发。
7. **平台 checksum 失败被当作噪声**：提交信息中的 2 个失败被归因于平台差异，容易掩盖真实回归；本报告不依赖这 2 个失败来判断，而是基于代码路径本身。

---

## 13. 建议的验证实验清单（只描述，不修改代码）

| 目标 | 输入构造 | 预期暴露 |
|---|---|---|
| F-01 | 复制 `conductor_slab/inputs_2d`，删除 `algo.macroscopic_subpixel=1` | 导体不衰减，结果接近 σ=0 |
| F-02 | 3D 平板，`interface_function(x,y,z)=y-0.5`，分别 s/p 极化 | 结果与 2D 同参数不一致；σ 分布沿 y 错位 |
| F-03 | 2D，`interface_function(x,y,z)=z-0.5`，源设为 Ez（p-pol） | 导体表面法向 E 长时间不衰减；反射系数偏离解析 |
| F-04 | 2D，`epsilon_tensor_*` 非对角 + `sigma > 0` + `subpixel=1`，多 MPI 分解对比 | 边界场值随分解变化/出现 NaN |
| F-05 | 2D，`interface_function(x,y,z)=x-0.5`（x 法向） | Ex 法向分量无亚网格平均，反射退化为阶梯 |
| F-06 | 周期边界 + 非均匀 ε，启用 Gauss 残差诊断 | 缝后第一个节点残差虚假偏大 |
| F-07 | `-DWarpX_COMPUTE=CUDA`（或 HIP/SYCL）编译该插件 | 编译报 `frame` host/device 错误 |
| F-08 | 树外实现第二个 `IMacroscopicProperties` 并注册 | 运行时 `MacroscopicEvolveE expects the material model of this plugin` 断言 |
| F-09 | 同时链接两个同类工厂 | 后注册者静默生效，无警告 |
| F-10 | 1D 构建运行 `computeAcceleration` | 加速度沿错误轴、可能越界 |
| F-11 | 同一输入分别设置/不设置 `WARPX_YEE_CELL_EPS`、`macroscopic.sigma_normal_arith` | 物理结果不同 |

---

## 14. 已核对且未发现问题的部分（避免误报）

为保证报告可信度，以下高风险点已逐行核对，**未**发现缺陷：

1. `SubpixelTensor.H` 的 τ(ε)、τ⁻¹、各向异性 χ⁻¹ 旋转/求逆公式与 Kottke 2008 一致；`sigma_exponential_matrices` 的
   `M1 = ε⁻¹ e^{-B dt} ε`、`M2 = ε⁻¹ (1-e^{-B dt}) B⁻¹ ε⁻¹`、`J1 = ε(I-C)ε⁻¹` 推导正确。
2. `FluidMoments` 的 HLLC 通量、Hancock 预测器、minmod 限制器公式正确；未发现关键缺陷（但其未接入核心总线，见下）。
3. `MacroscopicEvolveE.cpp:190-192` 的 `dt` 一致性断言正确，能防止 M1/M2 使用过期 dt。
4. `MacroscopicProperties::InitData` 在 `subpixel=0` 时仍会填充节点 ε/ε⁻¹（第 497-687 行），因此 **BE 无损路径本身仍能正常演化场**；F-01 的问题是“σ 阻尼缺失”，不是“场完全不更新”。
5. `AllocateLevelMFs` 对 `lev==0` 的限制（`WarpX.cpp:2925-2935` 附近）与当前 AMR 不支持宏观求解的声明一致，不构成新缺陷。
6. RZ 下 `WARPX_ABORT` 的限制与上游一致，不计为本 fork 回归。

---

## 15. 结构性结论（对项目根本目标的判断）

本项目声称的目标是：

> 核心与插件独立；插件之间相互独立；插件仅通过接口接入核心总线；用户可以自行编写新功能插件，获得极强扩展性。

本轮审查显示，这一目标在**介质 Maxwell / 材质模型**这条主线上尚未成立：

1. **接口不可替换**（F-08）：求解器依赖接口之外的具体类型，第三方材质插件会在运行时被断言拒绝。
2. **总线不可组合**（F-09）：每类服务只有单槽，第二个实现静默覆盖第一个。
3. **物理内核与插件边界渗漏**（F-11）：环境变量和“非物理探针”输入直接改变内核数值路径，破坏“输入卡可复现”的基本契约。
4. **插件独立但未接入**（NewtonGravity / FluidMoments）：两者只由自身单元测试调用，没有核心侧注册/调用点，无法通过输入文件启用；从“插件总线”角度是未完成的集成，而非可交付的插件能力（此项为观察，严重度低于 F-08/F-09）。

同时，物理正确性方面存在 5 个静默缺陷（F-01～F-05），其中 F-01、F-02 会让用户在**不报错、不崩溃、测试通过**的情况下得到完全错误的电磁响应。这与“提交后正确性检查”的目标高度相关：它们不是格式/风格问题，而是会污染科学结论的系统性缺陷。

---

## 16. 声明

- 本轮审查**未修改任何项目代码或项目文件**。
- 未运行构建或测试；所有结论均可由报告给出的文件与行号直接复核。
- 本报告为唯一新增文件：`CODE_REVIEW_2026-11-14.md`。
