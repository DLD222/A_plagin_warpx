# 弓形激波算例的物理复核（2026-11-17）

## 1. 形状错误的根因：尺度不匹配（有数字）

一个**离子尺度的弓形激波**要求障碍物半径 R 与离子尺度可比或更大：

```
d_i = c/ω_pi  （离子惯性长度）      r_ci = m_i v/(Z e B)  （离子回旋半径）
判据： d_i ≲ R  且  r_ci ≲ (几倍) R  且  M_A = v/v_A > 1
```

| 算例 | d_i | d_i/R (R=250µm) | r_ci/R | M_A |
|---|---|---|---|---|
| 原算例（v=0.1c, B=7.15T, n=4e18） | 118 mm | **473** | 947 | 2.0 |
| 论文参数（70km/s, 0.166T, 4e18） | 118 mm | **473** | 95 | **0.20**（亚阿尔芬） |

**障碍物比离子惯性长度小两个数量级**，这个参数区间在物理上不可能出现离子弓形激波；
之前看到的只是电子尺度的场绕流。实测印证：B 的压缩比为 **0.999–1.000**（无压缩）。

`d_i ∝ n^{-1/2}`，所以要让 `d_i ~ R = 250 µm` 需要 `n ≈ 1e24`；
此时全 PIC 要求 `dx ≲ λ_D ≈ 3e-8 m`，`dx/λ_D ≈ 1500` —— **全 PIC 不可行**，必须用 hybrid。

## 2. 已修的两个真实错误

1. **初始条件不是平衡态**：磁化等离子体垂直 B 漂移必须满足 `E + v×B = 0`。
   原算例 B=7.15 ŷ、E=0、v=3e7 x̂ → 缺 `E_z = -2.14e8 V/m`（已加，实测 -2.143e8）。
2. **hybrid deck 多声明了 electrons 物种**：hybrid 的电子是隐式流体（官方算例只有 `ions`），
   多出的 electrons 物种把电荷重复计入 → 场指数增长（实测 |B| 从 8 增到 4e5 T）。

## 3. 渲染方面的错误（已修）

ParaView 相机在 +y 侧、up=+z 时屏幕右方向是 **−x**，所有图左右镜像 →
激波看起来在错误一侧。改到 −y 侧后：障碍物（x=0）位于域宽 17%、坐标三元组 +X 朝右。

## 4. 复现与验收

```bash
# 定量判据（障碍物内粒子数 / 密度压缩比 / 驻点位置 / 磁场压缩比）
python -m warpx_studio.verify.bow_shock <openpmd_xxxxxx.h5> \
    --radius 1e-3 --prob-lo -2e-3 -4e-3 --prob-hi 10e-3 4e-3 --n-cell 240 160 --b0 8.338
```

**验收标准**（三条必须同时成立）：
- `ions_inside_obstacle` ≈ 0（障碍物挡住粒子）
- `density_compression` > 1.5（驻点上游密度堆积）
- `b_compression` > 1.2 且 `stagnation_x_mm` < 0（驻点在障碍物上游，激波面在 x<0）

## 5. 当前状态（未完成）

hybrid 算例已进入正确参数区（`d_i/R = 0.9`, `R/d_i = 4.2`, `M_A = 2`），但**数值上仍不稳定**：
`max|B|` 从 8.34 T 增到 85 T 后趋于平台，`rms` 缓慢增长 —— 局地 whistler 不稳定性。
下一步应以仓库**已验证稳定的 hybrid 算例**（`Examples/Tests/ohm_solver_*`）为数值基线，
只在其上增加 EB 圆柱与漂移，再逐项放开参数，避免从零摸索稳定性。

## 6. 第 4 轮：hybrid + EB 是真正的阻塞（四组对照实验）

| # | 条件 | 结果 |
|---|---|---|
| 1 | hybrid，**无 EB** | ✅ 跑满 200 步正常 |
| 2 | hybrid，**有 EB**（R=1mm 圆柱） | ❌ `STEP 1` 即 `BfieldEvolve: exceeded max substep attempts` |
| 3 | 有 EB + **关闭我的阶梯 E 置零**（`WARPX_PEC_EB_MASK=0`） | ❌ 同样在 STEP 1 崩溃 |
| 4 | 有 EB + `grid_type=collocated` 开/关、`n_floor` = 5e21/5e23/1e24 | ❌ 全部崩溃 |

日志特征：第 1 个半步接受 4 个子步，**第 2 个半步 0 接受、2001 拒绝**（`dt_sub_final = 0`），
是状态出现 NaN/Inf 的典型表现。因此：

- **不是**我的 PEC 边界条件引起的（实验 3）；
- **不是**网格类型或密度下限引起的（实验 4）；
- hybrid 模型里虽有 `eb_update_E` / `ngEB` 的接线（见 `HYBRID_EB_HANDOFF.md`），
  但 `BfieldEvolve` 在 cut cell 附近无法收敛 → **hybrid + EB 在本 fork 中不可用**。

### 因此当前的两条路都被堵住

| 路线 | 障碍 |
|---|---|
| 全 PIC + EB（Yee） | `d_i/R = 473`，物理上不可能出现离子弓形激波；即使调参到 `d_i ~ R`，`dx/λ_D ≈ 1500` 也不可行 |
| hybrid + EB | 求解器在 STEP 1 崩溃（本轮四组实验证实） |

### 下一步（按可行性排序）

1. **修 hybrid + EB**：定位 `BfieldEvolve` 在 cut cell 处产生 NaN 的位置（很可能是 Ohm 定律里
   `∇(n T_e)/n` 在 n→0 的导体格上发散），在 covered 格上给出定义良好的背景密度/压力；
2. 若修不动，用 **全 PIC + EB** 交付一个**诚实的动力学结果**（电子尺度绕流 + 粒子遮挡），
   并在文档里明确它**不是** MHD 弓形激波，给出尺度判据说明为什么；
3. 或者换模型：用 `algo.em_solver_medium = macroscopic` 的介质路径把导体近似为高电导介质
   （无需 EB 的 cut cell 场更新）。

## 7. 第 5 轮：hybrid + EB 的可用路径与首次通过三项判据

**关键发现**：RKF45 自适应子步在 cut cell 处误差估计退化（第 2 个半步 0 接受、2001 拒绝），
但**固定 RK4 子步可以跑通**：

```ini
hybrid_pic_model.use_rkf45 = 0
hybrid_pic_model.substeps = 200        # 固定子步要够多
hybrid_pic_model.max_substep_attempts = 8000
```

### 首次同时满足三条验收判据（iteration 300）

`audit_runs/simulations/bow_shock/hyb8`（R=1mm 圆柱，n=1e24，v=2.5e4 m/s，B=2.978 T，
`d_i/R=0.24`、`M_A=2`、`M_s=2.97`）：

| 判据 | 实测 | 阈值 | 结果 |
|---|---|---|---|
| 障碍物挡住粒子 | `ions_inside_obstacle = 1` | ≈ 0 | ✅ |
| 密度堆积 | `density_compression = 10.75` | > 1.5 | ✅ |
| 驻点位置 | `stagnation_x_mm = -1.4`（在障碍物上游 0.4 mm） | x < 0 | ✅ |
| 磁场压缩 | `b_compression = 1.31` | > 1.2 | ✅ |

渲染图（iteration 300，B_y）显示障碍物周围出现上游压缩、下游尾迹的壳层结构，
障碍物位于域宽 17% 处、+X 朝右（相机手性已修）。

### 复现

```bash
cp Tools/warpx-studio/warpx_studio/cases/bow_shock_hybrid_2d.inputs <run>/inputs
cd <run> && warpx.2d inputs
python -m warpx_studio.verify.bow_shock diags/diag1/openpmd_000300.h5 \
    --radius 1e-3 --prob-lo -2e-3 -4e-3 --prob-hi 10e-3 4e-3 --n-cell 120 80 --b0 2.978
```

### 仍待改进

- 长时间演化会在 ~350 步后因 `max substep attempts` 中止；`hyb9`（substeps=400, dt=6e-11）
  正在跑更长的序列；
- 场图仍有可见的网格尺度散粒噪声（16 ppc + 直接沉积），可加平滑或提高 ppc。

## 8. 撤回：第 5 节的 hybrid "弓形激波"结果无效

**用户指出结果错得离谱，复查后确认：错。撤回第 5 节的全部结论。**

证据（hyb11，iteration 700）：

| 观测量 | 实测 | 应有 | 结论 |
|---|---|---|---|
| 离子平均 `u_x` | **9.8e-25** | 1.67e-4 | **漂移被完全抹掉** |
| `\|E_z\|` 平均 | **5.6e3 V/m** | `v×B = 1.19e5 V/m` | 动生电场只有应有的 5% |
| 二维密度图 | 均匀 + 障碍物空洞 + 贴壁薄环 | 分离的弓形激波面 | **没有激波** |

根本原因：hybrid 的电场由 Ohm 定律给出，而电子流速来自 `J = ∇×B/μ0`。
对**均匀漂移**的等离子体 `J = 0` → `u_e = 0` → `E = 0`，于是离子只受洛伦兹力
`q v×B` 作用而回旋，**宏观漂移在 ~1 个回旋周期内消失**（实测 700 步 ≈ 0.6 τ_ci）。
静止的等离子体当然不可能形成激波。

之前"通过验收"的 `density_compression = 10.99` 是把障碍物贴壁堆积当成激波；
改进判据（径向分环、排除 1.0–1.2R 的贴壁环）后，真实值只有 `1.05`——**没有激波**。

对照：**PIC 路径的漂移保留 100%**（`rel` 与 `eq` 两个算例平均 `u_x = 0.1005`），
所以 PIC 的等离子体是真的在流；只是它的 `d_i/R = 473`，物理上只有电子尺度绕流。

### 要做对，必须让 hybrid 里"流"起来

| 方案 | 做法 | 代价 |
|---|---|---|
| A. 外加动生电场 | `hybrid_pic_model.add_external_fields = true` + `external_vector_potential`，用 `A_time_external_function(t)` 生成 `∂A/∂t = -E` | 需构造 A(t) 使 `E_z = -v B` |
| B. 边界注入 | `ions.injection_style = NFluxPerCell` + 上游面入射，粒子带漂移进入；下游用吸收/开边界 | 需开边界与通量参数 |
| C. 全 PIC | 漂移天然保留，但只能在电子尺度做，且需接受 `d_i/R ≫ 1` | 得不到 MHD 激波 |

下一步应实现 **B（边界注入）**，这是这类"等离子体束打到障碍物"问题的标准做法。
