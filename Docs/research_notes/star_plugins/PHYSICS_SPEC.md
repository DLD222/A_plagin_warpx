# 恒星双插件：物理公式规格表

> **规矩**：本文件先于代码存在。代码里出现的每一个公式、常数、拟合式，都必须能在这里
> 找到对应条目和出处；反过来，这里没有写下的东西不许进代码。数值型参数（反应率系数、
> 不透明度表）在**转录自原始文献时**逐条标注页码/表号，不凭记忆填写。

状态图例：`[spec]` 已写下公式与出处 · `[todo-src]` 待从文献转录数值 · `[test]` 已有验收用例设计

---

## 0. 常数（来源：IAU 2015 Resolution B3 + CODATA 2018）

| 量 | 符号 | 值 | 来源 |
|---|---|---|---|
| 万有引力常数 | G | 6.67430e-11 m³ kg⁻¹ s⁻² | CODATA 2018 |
| 太阳质量 | M☉ | 1.98892e30 kg | IAU 2015 名义值派生 |
| 太阳半径 | R☉ | 6.957e8 m | IAU 2015 Resolution B3 |
| 太阳光度 | L☉ | 3.828e26 W | IAU 2015 Resolution B3 |
| 质子质量 | m_p | 1.67262192e-27 kg | CODATA 2018 |
| 玻尔兹曼常数 | k_B | 1.380649e-23 J/K | SI 2019 定义值 |
| 天文单位 | AU | 1.495978707e11 m | IAU 2012 |

`[spec]`

---

## 1. 牛顿引力求解器（插件一）

### 1.1 控制方程

∇²φ = 4πGρ                                        (1.1)

g = −∇φ                                            (1.2)

流体/粒子受到的加速度源项：S_mom = ρ g            (1.3)

**出处**：Binney & Tremaine, *Galactic Dynamics*, 2nd ed., §2.1（泊松方程与引力加速度）；
Kippenhahn, Weigert & Weiss, *Stellar Structure and Evolution*, 2nd ed., Ch. 2（恒星内部的
引力场）。

### 1.2 边界条件

- 开放（Dirichlet）：边界值取解析多极展开，最低阶为 φ = −GM_enc/r
- 周期：周期域上泊松方程无解，必须减去平均密度
  ∇²φ = 4πG(ρ − ρ̄)                                (1.4)
  即 Jeans swindle。**出处**：Binney & Tremaine §2.1 脚注；Kiessling (2003) 对 swindle 的讨论。

### 1.3 离散与求解

- 采用 AMReX MLMG 的 `computePhi`（七点/九点 Laplacian，与网格同阶）
- 收敛判据：残差 2-范数相对下降 1e-11，或绝对 1e-14

### 1.4 验收（解析解）

| 用例 | 解析式 | 容差 | 状态 |
|---|---|---|---|
| 均匀球内部 | g(r) = (4/3)πGρr | 相对误差 < 1e-3 | `[test]` |
| 均匀球外部 | g(r) = GM/r² | 相对误差 < 1e-3 | `[test]` |
| 点质量势 | φ = −GM/r | 相对误差 < 1e-3 | `[test]` |
| 双同心球壳 | 分段解析 | 相对误差 < 1e-3 | `[test]` |
| 网格加密 | 误差随 dx 收敛 | 阶 ≥ 1.8 | `[test]` |

---

## 2. 五阶矩流体（插件二）

### 2.1 守恒形式

∂U/∂t + ∇·F = S                                   (2.1)

U = (ρ_s, ρu, E)ᵀ                                  (2.2)
F = (ρ_s u, ρuu + p I, (E + p)u)ᵀ                  (2.3)
E = ½ρ|u|² + (3/2)p（单原子理想气体）               (2.4)
S = (反应源项, ρg, ρg·u + ε_react)                 (2.5)

多组分：每个组分 s 有自己的 ρ_s 方程，共享 u 与 p。

**出处**：Toro, *Riemann Solvers and Numerical Methods for Fluid Dynamics*, 3rd ed., Ch. 1–3
（守恒律与 Euler 方程）；LeVeque, *Finite Volume Methods for Hyperbolic Problems*, Ch. 2–6。

### 2.2 Riemann 求解器与重构

- HLLC 近似 Riemann 求解器（Toro 3rd ed., §10.4，波速估计 §10.5）
- 二阶精度：MUSCL-Hancock 预测-校正（Toro §6.4），限制器 minmod（Toro §13.3）
- CFL：0.3（可配置）

### 2.3 物态方程（契约注入）

p = p(ρ, e, X_i)                                   (2.6)

- 契约：`IEquationOfState`，由插件注册；求解器不写死任何 EOS
- 实现顺序：理想气体（解析验收）→ 外部表插值（OPAL 类）
- **出处**：Kippenhahn et al. Ch. 14（EOS 在恒星结构中的角色）；Rogers & Nayfonov 2002 (OPAL) `[todo-src]` 表格格式与适用范围待核对

### 2.4 与引力的耦合

- 动量源项 ρg（式 1.3）
- 能量源项：g·(ρu)
- 引力能：E_grav = ½∫ρφ dV                        (2.7)
  **出处**：Binney & Tremaine §2.2

### 2.5 验收

| 用例 | 判据 | 状态 |
|---|---|---|
| Sod 激波 | 与精确解 L1 误差随网格二阶收敛 | `[test]` |
| Jeans 不稳定性 | 临界波长 λ_J = c_s√(π/(Gρ)) 的 ±5% | `[test]` |
| 静水平衡（等温球） | 解析密度剖面，相对误差 < 1e-3 | `[test]` |
| 守恒（无源） | 质量/动量/能量 1e-12 相对误差 | `[test]` |

式 (2.5) 的 Jeans 临界波长出处：Jeans (1902)；教科书表述见 Binney & Tremaine §5.2.1。

---

## 3. 组分反应（太阳）

### 3.1 pp 链

p + p → D + e⁺ + ν_e                              (3.1a)
D + p → ³He + γ                                   (3.1b)
³He + ³He → ⁴He + 2p                              (3.1c)
³He + ⁴He → ⁷Be + γ                               (3.1d)  → ⁷Be + e⁻ → ⁷Li + ν_e → ⁷Li + p → 2α
³He + ⁴He → ⁷Be + γ → ⁷Be + p → ⁸B + γ → …        (3.1e, 高能支)

### 3.2 CNO 循环

¹²C + p → ¹³N + γ → ¹³C + e⁺ + ν_e                (3.2a)
¹³C + p → ¹⁴N + γ                                 (3.2b)
¹⁴N + p → ¹⁵O + γ                                 (3.2c)
¹⁵O → ¹⁵N + e⁺ + ν_e → ¹⁵N + p → ¹²C + ⁴He         (3.2d)

**出处**：Kippenhahn et al. Ch. 18（核反应章节，反应链示意）；Bethe (1939) 原始文献。

### 3.3 反应率

⟨σv⟩ 与能量产生率 ε 采用 NACRE II（Xu et al. 2013, Nucl. Phys. A 918, 61–169）提供的
解析拟合式；CF88（Caughlan & Fowler 1988, At. Data Nucl. Data Tables 40, 283）作为交叉校验。

**`[todo-src]`**：每条反应的拟合系数（T9 多项式的 (a0..a6)、有效温度范围、共振项）必须
逐条从上述文献转录，禁止凭记忆填写。转录时记录：表号 / 页码 / 适用 T9 区间。

### 3.4 能量与中微子分开记录

ε_nuc = ε_pp + ε_CNO（局部沉积）
ε_ν   = ε_pp,ν + ε_CNO,ν（逃逸；只从能量方程中扣除，不进流体）

这样光度积分可与中微子损失分别对账。`[spec]`

---

## 4. 太阳定量仿真验收

### 4.1 目标值（标准太阳模型，Christensen-Dalsgaard et al. 1996；Bahcall et al. 2001）

| 量 | 目标 | 容差 | 状态 |
|---|---|---|---|
| 中心温度 T_c | 15.7 MK | ±3% | `[todo-src]` 精确模型值待核 |
| 中心密度 ρ_c | 150 g/cm³ | ±10% | `[todo-src]` 同上 |
| 光度 L | 3.828e26 W | ±2% | `[spec]` IAU 名义值 |
| 表面 Z/X | AGSS09 组分自洽 | 一致 | `[todo-src]` 组分表待转录 |

### 4.2 对比方式

- 一维径向结构（球对称）：ρ(r)、T(r)、X(r)、L(r)
- 与 SSM 表格逐层比对，输出 Markdown 表 + 图
- 声速剖面 c_s(r) 作为附加对照（日震学）：`[todo-src]` 数据来源待定（Christensen-Dalsgaard 反演）

---

## 5. 实施顺序（与 60 轮排期对应）

1. 本文档定稿，反应率与 SSM 数值逐条转录（第 1–6 轮）
2. Core 契约：`IGravitySolver`、`IFluidSolver`、`IEquationOfState`、`IReactionNetwork`（7–12）
3. 插件一：泊松 + 解析验收（13–20）
4. 插件二：Euler/HLLC + Sod/Jeans/静水平衡（21–32）
5. 多组分 + EOS 注入 + 引力耦合 + 守恒（33–40）
6. 反应网络 + ε/ε_ν（41–48）
7. 太阳算例 + 对比（49–56）
8. 收尾：检查器、回归、文档（57–60）
