# RZ `rho` 残余差异的根因（2026-11 审计，已结案）

对象：`test_rz_laser_acceleration`（RZ，64×512，2 个方位模，`dump_rz_modes=1`）
在 t=0 的 `rho` 三个分量。此前多轮把它记成"两个未定位缺陷 D-A / D-B"，本文件给出
完整证据链，结论是：

> **两个"缺陷"都不是 fork 的缺陷。fork 的代码与官方 WarpX 26.11 逐字节一致；
> 与官方 26.10 的差异来自上游 26.10→26.11 期间的两个提交。**
> 修复方向相应更正：见文末"处置"。

## 0. 结论摘要

| 项 | 证据 |
|---|---|
| D-A（z 高边界处 3 行） | 上游 `ec99a479a`（#7341，2026-10-16）引入 `WarpX::FinalizeRhoOnLevel`，并让 `RhoFunctor` 在"过滤+guard 求和"之后调用它；26.10 走的是"先标定再做过滤、且诊断路径不做 `ApplyRhofieldBoundary`" |
| D-B（r=0 折叠处 mode-1 实部） | 上游 `a180bc361`（#7331，2026-10-05）"RZ: fix azimuthal-mode parity when folding `rho` at `r=0`"；26.10 的 `imode` 公式在分量数位奇数时给 mode-1 实部算了 +1 宇称，正确值是 −1 |
| fork 与 26.11 是否一致 | **逐字节一致**：t=0 的全部 plotfile 文件（Header、8 个 FAB、`electrons`/`beam` 粒子数据）`cmp` 相同，`rho_0_real/rho_1_real/rho_1_imag` 最大差 0.0 |
| fork 与 26.10 是否一致 | 否，`rho_0_real` 最大差 1.3578e4（占场量级 3.1%），`rho_1_real` 2.0967e3 |

复现（`np=1`、`OMP_NUM_THREADS=1`、`max_step=0`）：

```
audit_runs/round11/rz_laser/{ref,ref2611,new}   # 官方 26.10 / 官方 26.11 / fork
Docs/audit_2026_11/scripts/rz_row_diff.py  <ref>/diags/diag1000000 <new>/diags/diag1000000
```

## 1. 先把"粒子"和"沉积"排除掉

1. **初始粒子逐字节相同**：`diags/diag1000000/{electrons,beam}/Level_0/{DATA_00000,Particle_H}`
   在官方与 fork 之间 `cmp` 全部相同。所以差异不可能来自粒子初始化、随机数、注入。
2. **原始沉积（未标定）相同**：在两边同一位置插桩，把 `ApplyInverseVolumeScalingToChargeDensity`
   入口处的 `rho` MultiFab（含 guard cell）导出为裸二进制后逐格比较：
   唯一不同的是 `j ≥ 516`（z 高边界之外的远端 guard cell），有效区与近边界 cell 全部相同。

也就是说：粒子一样、沉积核一样、原始 `rho` 一样 → 差异只可能出在
**finalization（反射边界 + 径向体积标定）与过滤的次序**上。

## 2. D-B：r=0 处的模式宇称（算式级证据）

在 `j=204`（束团区）取原始沉积 `raw(i)`，`dr = 4.6875e-7`：

| 量 | 值 |
|---|---|
| `raw(i=−1)` | `1.064425e-02` |
| `raw(i=+1)` | `2.253543e-02` |
| fork 的 `rho_1_real(i=1)` | `4.037418e+03` = `(raw(1) − raw(−1)) / (2π dr)` |
| 官方 26.10 的 `rho_1_real(i=1)` | `1.126551e+04` = `(raw(1) + raw(−1)) / (2π dr)` |

两者只差折叠符号：fork 用 `(−1)^m = −1`（m=1），26.10 用 `+1`。

代码层面对照（两侧除重构外一致）：

```cpp
// 26.10  Source/FieldSolver/WarpXPushFieldsEM.cpp
if (icomp == 0 || icomp == ncomp/2) { imode = 0; }
else if (icomp < ncomp/2)           { imode = (icomp+1)/2; }
else                                { imode = (icomp - ncomp/2 + 1)/2; }
Rho_arr(i,j,0,icomp) -= std::pow(-1, imode+1) * Rho_arr(-ishift-i,j,0,icomp);

// fork（= 26.11，Source/Plugins/PicEm/Modules/FieldSolver/WarpXPushFieldsEM.cpp:2136）
int const imode = (comp_in_copy + 1) / 2;
Rho_arr(i,j,0,icomp) += std::pow(-1, imode) * Rho_arr(-ishift-i,j,0,icomp);
```

26.10 的公式是为"两份拷贝（`rho_old`/`rho_new`，`ncomp = 2*ncomps = 6`）"写的；
诊断用的 `rho` 只有一份（`ncomp = 3`），于是 `icomp=1` 命中 `icomp == ncomp/2` 分支被判成
mode 0（宇称 +1）。物理上 `ρ_m(−r) = (−1)^m ρ_m(r)`，mode 1 必须是 −1。
上游 `a180bc361` 正是修这个，fork 已经带着修好的版本。

## 3. D-A：z 高边界的反射 + 过滤次序

26.10 的诊断路径：

```
GetChargeDensity()            // 逐物种：沉积 + ApplyRhofieldBoundary(仅笛卡尔) + 体积标定
ApplyFilterandSumBoundaryRho  // 过滤 + guard 求和
```

26.11（`ec99a479a`）改成：

```
GetChargeDensityOnLevel(lev, local=true, finalize=false)   // 只沉积
ApplyFilterandSumBoundaryRho                               // 过滤 + guard 求和
FinalizeRhoOnLevel                                         // ApplyRhofieldBoundary + 体积标定
```

实测后果（本算例 `boundary.field_hi = pec pec`，`use_filter=1`，`filter_npass_each_dir = 0 1`）：
`FinalizeRhoOnLevel` 里的 `ApplyRhofieldBoundary` 会把 z 高边界的 PEC 反射写进 guard cell，
紧接着的 z 向过滤把它折回 `j = 509..511` 三个 cell；26.10 根本不在这条路径上做反射，
所以这三个 cell 不同。`rho_1_real` 在同一带的差异只有 1e-13 量级（那里 m=1 本身是
1e-11 的噪声），属于连带舍入。

## 4. 由此产生的**处置**（已落地）

1. **撤销我此前的 `ca4a7cb`"修复 #9"**。该提交把 `computeRho` 里两步的顺序改成
   "先 `FinalizeRhoOnLevel` 再 `ApplyFilterandSumBoundaryRho`"，也就是把 fork 从它自己的
   基线（26.11）拉回 26.10 的行为。撤销后：

   ```cpp
   m_warpx.ApplyFilterandSumBoundaryRho(lev, lev, *rho, 0, rho->nComp());
   m_warpx.FinalizeRhoOnLevel(lev, rho.get(), PatchType::fine);
   ```

   与 26.11 的 `RhoFunctor` 完全一致。撤销后本算例 t=0 的 `rho` 与官方 26.11
   **逐字节相同**（这正是本文件开头的结论）。
2. **去掉 fork 独有的 `ApplyVolumeWeightedFilter(..., bool is_volume_scaled)` 扩展**。
   上游（26.10/26.11/development）都没有这个参数；fork 给 `rho` 传 `false` 走了一条
   "在密度空间做守恒通量"的分支。该分支在数学上更自洽（26.11 的次序把"未标定"的量
   喂给了为"已标定密度"写的公式），但它让 fork 偏离上游，因此按"先与上游对齐"的原则移除。
   若要向上游提这个不一致，属于另一件事（见 `Docs/audit_2026_11/ATTRIBUTION.md`）。
3. `GetExternalEBField` 缺少运行时上下文：与本节无关的独立缺陷，已在同一轮修掉
   （`Ionization` / `PairGeneration` / `PhotonEmission` 三个 functor 现在显式接收
   `RuntimeContext`），见 `FIXES.md`。

## 5. 教训（写进审计方法）

前几轮把"与 26.10 不一致"直接当成了"fork 有缺陷"，并在没有确认基线版本的情况下
做了两处"对齐 26.10"的改动（`ca4a7cb` 与其前身）。这类改动的风险是**把对的改错**。
本轮引入的 `upstream_provenance.py`（见 `UPSTREAM_BASE.md`）给出了判定方法：
先确定 fork 对应的上游提交，再判断差异属于"上游演进"还是"fork 重构/新功能"，
最后才决定改不改。
