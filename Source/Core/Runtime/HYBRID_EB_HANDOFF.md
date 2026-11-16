# hybrid + embedded boundary: 定位结果与后续改动点（交接说明）

## 结论一句话

hybrid 模型**有** EB 掩码通路（`eb_update_E` 贯穿 9 处签名），但**Ohm 定律求 E 的 kernel 没有使用它**。
导体内部没有粒子 → 密度≈0 → Ohm 求解用 `n_floor` 兜底 → 有效电阻率极大 → 方程变刚性 →
RKF45 子步误差判据无法满足 → 报 `BfieldEvolve: exceeded max substep attempts`。

## 调用链（全部行号来自 `Source/Plugins/PicEm/Modules/HybridPIC/HybridPICModel/`）

```
HybridPICModel.cpp:2287  BfieldEvolve              (多级入口)
HybridPICModel.cpp:2306  BfieldEvolve              (单级)
HybridPICModel.cpp:2349  BfieldEvolveRKF45         error = ... ; 判据在此
HybridPICModel.cpp:2472/2488/2544/2560  FieldPush  (4 个 RKF45 阶段)
        FieldPush 内:
HybridPICModel.cpp:~2495 CalculatePlasmaCurrent(Bfield, eb_update_E);
HybridPICModel.cpp:~2497 HybridPICSolveE(Efield, Jfield, Bfield, rhofield, eb_update_E, true);
HybridPICModel.cpp:731   HybridPICSolveE            (多级，继续下传)
HybridPICModel.cpp:~745  → 单级实现（约 756 行起），继续把 eb_update_E 传给更深一层的 kernel
```

**唯一真正读取并使用掩码的地方**：`ExternalVectorPotential.cpp:294-296`
（`update_Fx_arr = eb_update[0]->array(mfi)`，只用于外场矢量势 F 的更新判定）。

## 需要改的两处

1. Ohm 求 E 的最内层 kernel：对 `eb_update_E[i](i,j,k) == 0` 的格
   - `E = 0`（导体内无电场）；
   - 不要用 `n_floor` 兜底后的密度参与运算（否则产生假刚性）。
2. `BfieldEvolveRKF45` 的误差归约处（`HybridPICModel.cpp:2349` 附近返回的 `error`）：
   把 covered 格的误差排除，否则不管怎么缩子步都不会收敛。

## 已排除的其他原因（都实测过，不必重查）

| 怀疑 | 实测结论 |
|---|---|
| EB 隐式函数符号 | **真 bug，已修**：文档规定"模拟区为负、导体内为正"，原写成 `x²+z²−r²`（符号反了），导致等离子体被初始化进导体内部、域外全被当导体 |
| `momentum_distribution_type = maxwellian` | **不存在**：本 fork 的类型名是 `gaussian`（`SpeciesUtils.cpp:154-180`），写 `maxwellian` 会静默退化成零动量 |
| 冷束导致不稳定 | 反了：**热速度**才是爆炸触发条件；无热速度时稳定 |
| 沉积算法 | `direct`/`esirkepov`/`villasenor` 三种都在爆炸后崩；换算法不是解 |
| 粒子排序、线程数、ppc、边界类型 | 均实测无影响 |
| 全 PIC 参数能否稳定 | 不能：dt=2e-12 爆炸、2e-13 稳但需 1e5 步；这与 ω_pe/ω_ci 相差 3-4 个数量级一致 |

## 两个算例的当前文件

```
audit_runs/simulations/bow_shock/
├── yee_case1.inputs / yee_case2.inputs   全 PIC(Yee)+EB，参数已按论文反演（M_A≈1.9, M_S≈4.1）
├── fluid_case2.inputs                     冷流体 + 宏观介质 σ 导体（σ 太大时刚性，需慎重）
└── yee_case{1,2}.inputs 的 EB 函数已修正为 (0.25e-3)^2 - (x*x+z*z)
```

## 验证工具

```bash
python -m warpx_studio.verify.bn <diag1/openpmd_*.h5> --radius 0.25e-3 \
    --prob-lo -1e-3 -2e-3 --prob-hi 5e-3 2e-3 --n-cell 120 80 --b0 0.166
```
输出跨 EB 面 `B_n/B0`、`E_t/E`，用于判定 PEC 是否真正生效（**不要用 ∇·B**）。
