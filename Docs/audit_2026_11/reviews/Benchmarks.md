# benchmarks/ 审查

审计对象：fork `$HOME/WarpX_plugin` 的 `benchmarks/`（本 fork 新增，上游无对应目录）。
只读审计，未修改任何源码/脚本/输入；所有"confirmed"结论都在文中给出可复现的反例或命令。
数值复核用系统 `python3`（numpy/scipy/mpmath 可用）。协议见 `Docs/audit_2026_11/REVIEW_PROTOCOL.md`。

## 覆盖

`find benchmarks -type f | wc -l` → **118** 个文件（117 个被 git 跟踪 + 1 个未跟踪的
`cases/c02_0_pec/__pycache__/p1_tensor_reference.cpython-314.pyc`）。全部逐文件读完
（3 个 `probe_tm_*.txt` 是 543/1085/1629 行数据，读了头尾并**用它重跑了 `analyze_tm.py`**）。

| 组 | 文件数 | 内容 | 结论 |
|---|---|---|---|
| `benchmarks/` 顶层 | 10 | 5 个工具脚本 + 5 份设计/调研/评审 md | 工具脚本 OK；md 有 2 处已过期的"代码事实"（F14） |
| `cases/c02_0_pec/` | 87 | 66 个 inputs/数据 + 21 个 py/sh/cpp | 物理算例基线可用，但 7 个分析脚本有实质缺陷（F1–F7） |
| `cases/c03_medium/` | 21 | 4 个子算例（lossy/conductor/anisotropic/tensor）+ math + preflight | 参考数学层正确，但 3 个分析脚本与提交的 inputs 不匹配（F6） |

逐文件结论摘要（长表见文末"逐文件台账"）。**已实测通过**的自检：`math/test_reference_math.py`
（ALL PASS）、`tau_transform_test.py`（PASS，1e-16 量级）、`sigma_exponential_test.py`（PASS）、
`mie_cylinder_dielectric.py`（光学定理 \|c\|²=Re c 到 1e-16）、`generalized_fresnel.py`（R=1/9、
斜入射 Rs/Rp 与教科书 Fresnel 逐位吻合）、`analyze_tm.py` 重跑 `probe_tm_{384,768,1152}.txt` 精确复现
`A_TM_BASELINE.md` 的三个 r 值（−0.999295 / −0.999679 / −0.999793）。

## 缺陷清单

### F1 `mie_cylinder.rcs()` 多乘了 `j^n`，RCS 角分布整体旋转 90°（自检打印 0.93 却无断言）
- 文件:行 `benchmarks/cases/c02_0_pec/mie_cylinder.py:81-91`（`s += (1j**n)*c[i]*exp(1j*n*phi)`），
  自检 `:119-126`
- 严重度：**major**　置信度：**confirmed**
- 现象与依据：散射场是 `-Σ j^n c_n H_n(kr) e^{jnθ}`（`:69-78`），`H_n` 的渐近式带 `e^{-jnπ/2}`，与
  `j^n=e^{jnπ/2}` **相消**，故远场 `E_sc ∝ Σ c_n e^{jnθ}`，`σ(θ)=(4/k)|Σ c_n e^{jnθ}|²`。
  `rcs()` 保留了 `j^n`，等价于 `σ_true(θ+π/2)`。实测反例（可复现）：
  ```
  k=2π,a=1: r=1e6 处 2πr|scattered|² 与 (4/k)|Σ c_n e^{jnφ}|² 的最大相对差 = 1.07e-6
            同一量与 rcs()（含 j^n）              的最大相对差 = 0.927
  ```
  脚本自己的 `[far-field consistency]` 行正是打印这个 0.93，但只 print 不 assert（`:125-126`），
  所以缺陷一直被"看见却没被拦下"。
- 影响面（重要限制）：`analyze_mie_convergence.py:185-189` 的 `sig_sim` 用了**同一套** `j^n` 约定，
  因此 `sig_sim` 与 `sig_mie` 被同角度旋转，`err_rcs` 的 L2 不受影响 → 已发表的收敛阶不受影响；
  受影响的是"σ(φ) 曲线/前后向散射峰位"的绝对角度标注（以及 `RCS` 图）。
- 建议修法：`rcs()` 去掉 `(1j**n)`（或在文档中明确写"旋转 90° 的图案"）；`analyze_mie_convergence`
  的 `sig_sim` 同步去掉，并把 `:119-126` 的自检改成 `assert`。
- 验证方法：`python3 -c` 反例见上；或 `python3 mie_cylinder.py` 看最后两行 0.927/0.895。

### F2 Mie `by` 偏振的数值归一化少一个 `1/c`，近场 L2 误差恒为 ~100%
- 文件:行 `benchmarks/cases/c02_0_pec/analyze_mie_convergence.py:128-140`（`A = A/E0`）与
  `mie_cylinder.py:52-66`（`field()` 对 `by` 归一化到**单位 B 入射**）
- 严重度：**major**　置信度：**confirmed**（量纲推导，未运行）
- 现象与依据：`inputs_mie_by_*` 用 `laser1.e_max=1e6`、`polarization=1 0 0`，故介质中
  `By = E0/c ≈ 3.34e-9·E0`；`d1.fields_to_plot = By` → 谐波幅值 `A ≈ (1/c)·pattern`；除以 `E0`
  后 `A ≈ 3.3e-15`，而 `Am = mie.field(...,'by')` 是 O(1)。于是
  `err_nf = ||A_sc_num - A_sc_th||/||A_sc_th|| ≈ 1`（与分辨率无关），收敛阶拟合 ≈ 0。
  `ey` 偏振不受影响（Ey 由 E0 归一，恰好一致）。
- 建议修法：`pol=='by'` 时 `A /= (E0/PhysConst::c)`，或把 `mie.field` 的 `by` 分支乘 `1/c`；
  并在 `err_nf` 前加一条"入射幅值 = 1"的断言。
- 验证方法：对任意 n 打印 `np.abs(A[mask]).max()` 与 `np.abs(Am[mask]).max()`，比值应≈c。

### F3 `sigma_exponential_test.py` 用 Crank–Nicolson 冒充代码里的 backward Euler
- 文件:行 `benchmarks/cases/c02_0_pec/sigma_exponential_test.py:74-77`、`:96-111`
- 对照：`Source/Plugins/FieldSolver/FiniteDifference/MacroscopicProperties/MacroscopicProperties.H:318-350`
  `alpha = 1/(1+σdt/ε)`、`beta = dt/(ε(1+σdt/ε))`（**真 backward Euler**）
- 严重度：**major**　置信度：**confirmed**
- 现象与依据：`:77 return (1.0 - 0.5*s)/(1.0 + 0.5*s)` 是 **Crank–Nicolson**（`s=σdt/ε`），
  注释却写 "The scalar scheme currently in MacroscopicEvolveE (backward Euler)"。实测 `[2]`
  的误差序列 `1.28e-5 / 1.34e-8 / 1.35e-11`（dt 缩小 10 倍误差缩小 ~10³）是 **二阶**，
  而 BE 应是一阶（~10¹）。因此这条"与生产格式的小 dt 极限对照"对照的是**另一个（更高阶的）格式**。
- 影响：`c03_medium/conductor_slab/inputs_2d:6-9` 的核心叙事（"σdt/ε=0.333 时一阶 BE 明显错，
  该算例是 BE 与指数步进的判别实验"）用错了参考系数：`s=1/3` 时 BE 给 0.750、
  CN 给 0.7145、精确 `e^{-1/3}=0.7165` —— 判别方向与量级完全不同。
- 建议修法：把 `alpha_beta()` 改成 `1/(1+s), dt/eps/(1+s)`（并保留 CN 作为第二组对照）；
  注释改为"CN（不是 BE）"或删掉。
- 验证方法：`python3 sigma_exponential_test.py` 看 `[2]` 的 ratio≈1e6（二阶）而非 ~1e2。

### F4 `inputs_tm` 的初值磁场符号反了 → 波朝 −z 传播（与算例说明相反）
- 文件:行 `benchmarks/cases/c02_0_pec/inputs_tm:35`
  （`By = "-E0*exp(...)/c"`；同目录所有其它 TM 算例都是 `"+E0.../c"`，见 `inputs_2d_tm:36`）
- 严重度：**major**　置信度：**confirmed**（右手关系推导）
- 现象与依据：E=Ex x̂、B=By ŷ 时 `k ∝ E×B = ExBy ẑ`。`By=-Ex/c` ⇒ k 沿 **−z**；
  算例把脉冲放在 z<0、PEC 平面在 z=0，需要 +z 传播。错号的初值把脉冲劈成 ±z 各一半，
  探针（`z_probe=-5`）测到的幅度只有一半、且出现两个方向相反的到达结构。
  交叉证据：仓库里 `probe_tm_384.txt` 第 0 步 `Ex=1.83e4, By=+6.14e-5`（= +Ex/c），
  说明产出该数据的运行用的是正确符号，即 `inputs_tm` 不是那条运行的输入。
  同文件顶部注释 `:1-3` 声称用它证明"2D XZ ECT 只支持 TM"，该定性结论不依赖符号；
  但 `ROOTCAUSE_ECT_2D.md:156` 引用的 `max|Ex| = 9.995e5` 属于 `inputs_2d_tm`（符号正确）。
- 建议修法：改成 `"+E0*exp(-((z-zc)/w)^2)/c"`。
- 验证方法：`grep -n By_external inputs_tm inputs_2d_tm`；或跑一次看探针峰值是否只有 E0/2。

### F5 `verify_medium_wave.py` 的"相速度"判据有 +105% 系统偏差，正确解也会 FAIL
- 文件:行 `benchmarks/cases/c02_0_pec/verify_medium_wave.py:159-179`
- 严重度：**major**　置信度：**confirmed**（解析反例 + 数值复算）
- 现象与依据：`arrival()` 取 `|Ey|` 首次超过**全程最大值 5%** 的时刻当"到达时刻"，
  再用 `v = |za-z0| / (step*dt)`。无载波高斯 `exp(-((z+1)/0.15)²)` 的 5% 前沿在中心前方
  `w·sqrt(ln20)=0.26 m`，而 `dist=0.5 m`，故 `v_sim ≈ 0.5/(0.24/v) = 2.08 v`。实测（同参数合成信号）：
  `v_true=1.999e8, t_cross=1.219e-9, v_est=4.103e8, rel err=+105%`，而 `--tol-speed` 默认 0.10。
  更糟：该算例正是"能量发散"的场景，`ey.max()` 取的是**发散后**的幅度，5% 阈值会被推到任意晚的时刻。
- 建议修法：用两个探针的**峰值时刻差**（`argmax|Ey|` 于各自窗口内）求速度，或用 DFT 相位差；
  并把阈值穿越法删除。
- 验证方法：上面 8 行合成信号脚本可直接复现（`/tmp` 下写过一次，未留在仓库）。

### F6 多个分析脚本没有对应的输入文件，无法复现
- 文件:行
  - `cases/c02_0_pec/analyze_snell.py:25-28` 需要 `diags/d1*` plotfile；提交的
    `inputs_2d_oblique_*`（6 个）**都没有** `diagnostics.diags_names = d1`（全仓只有 10 个
    `inputs_mie_*` 有）。
  - `cases/c02_0_pec/compare_t9.py:30-35` 同样需要 `diags/d1*`。
  - `cases/c03_medium/conductor_slab/analyze_v2.py:78,104-108` 读 `p0..p6.txt`、
    `analyze_cw.py:100-105` 读 `p0..p9.txt`，而提交的 `conductor_slab/inputs_2d` 只定义
    `q1,q2` 两个探针。
- 严重度：**major**　置信度：**confirmed**
- 依据：`grep -l "diags_names = d1" inputs_*` → 仅 10 个 mie 文件；
  `grep -n reduced_diags_names c03_medium/conductor_slab/inputs_2d` → `q1 q2`。
- 影响：README 声称"每个目录有 inputs_2d + analyze.py 即可复现"，实际 3 个分析脚本
  （Snell、T9 介质柱 Mie、CW/skin-depth）只能靠手改 inputs 才能跑；而这些手改版没有入库。
- 建议修法：把 `make_snell.py`/`make_conductor_cw.py` 式的生成器（或带 `d1`/`p0..p9` 的
  inputs 变体）一并提交。

### F7 复现命令必然失败：所有 runner 的默认二进制路径是相对**运行目录**的
- 文件:行
  - `cases/c02_0_pec/run_tm_sweep.sh:5` `BIN=build_2d/bin/warpx...`（无 `WARPX_BIN` 覆盖，
    且第 4 行 `cd $(dirname $0)` → 解析成 `benchmarks/cases/c02_0_pec/build_2d/bin/...`）
  - `verify_fresnel_rt.py:38-41`、`verify_oblique_rt.py:34`、`verify_medium_wave.py:45-48`、
    `make_mie_cases.py:96-97`：默认值同样是相对路径，而 `subprocess.run(..., cwd=<临时目录>)`
  - `run_l2.sh:11`、`run_l2b_scan.sh:13` 支持 `WARPX_BIN`，但默认值同样错
- 严重度：**major**（可复现性）　置信度：**confirmed**
- 依据：仓库根**没有** `build_2d/`（`ls -d build*` 无输出），而 `A_TM_BASELINE.md:42-45`
  写的复现步骤正是 `cd benchmarks/cases/c02_0_pec && sh run_tm_sweep.sh`。
- 建议修法：默认值改成 `os.environ["WARPX_BIN"]` 必填，或用 `Path(__file__).parents[3]/"build_2d/..."`；
  `run_tm_sweep.sh` 改成 `BIN=${WARPX_BIN:?}`。

### F8 `inputs_ect_2d_degen` 的注释把 PEC 侧写反，且 `fp2` 落在导体内部使一半比较失效
- 文件:行 `benchmarks/cases/c02_0_pec/inputs_ect_2d_degen:37`（"PEC half-space z < 0"）、`:67`（`fp2.z=1.5`）
- 严重度：minor　置信度：**confirmed**
- 依据：`Docs/source/usage/parameters.rst:1219-1228`："physics area = 函数值**负**区；
  EB 内部 = 正值区"。`eb_implicit_function="z"` ⇒ 物体在 **z>0**。`inputs_yee_2d_degen`
  同样如此。故 `fp2` 在 PEC 内部，`compare_degen.py:45-52` 对 fp2 的比较是 0 vs 0（peak=0 →
  `rel=diff`，恒过），不携带信息。
- 建议修法：注释改为 z>0；`fp2` 移到 z<0（例如 −1.5/…）或删掉。

### F9 `inputs_ect_2d_45deg_tm` 注释写 TE、实际是 TM
- 文件:行 `benchmarks/cases/c02_0_pec/inputs_ect_2d_45deg_tm:40-43`
  （"# TE pulse ... Bx = -Ey/c"，而 `:47-51` 是 `Ex`+`By`）
- 严重度：minor　置信度：**confirmed**
- 影响：该算例同时声称覆盖"TE 分量 Bx/Bz 的 BCK 稳定化"，但 By-only 的 TM 波不激发 Bz，
  所以它**没有**覆盖它声称覆盖的东西；`check_stability.py:52-57` 的偏振自动识别也会走 TM 分支。

### F10 五个脚本周转着用了三个不同的 ε₀，而 README 的"约定"本身写错
- 文件:行 `cases/c03_medium/README.md:45`（"eps0 = 8.8541878128e-12（WarpX value；do not mix CODATA）"）；
  `conductor_slab/analyze.py:28`（`MU0=1.2566370612685e-6`）、
  `lossy_slab/analyze.py:15`、`anisotropic_slab/inputs_2d_p:21`、`tensor_interface/inputs_2d:27`
  （`8.8541878128e-12`）、`conductor_slab/analyze_v2.py:24`、`analyze_cw.py:13`、`preflight.py:24`
  （`8.8541878188e-12`）
- 严重度：minor（数值影响 ~7e-10 相对，可忽略；但约定声明是错的）　置信度：**confirmed**
- 依据：`Source/Core/ablastr/constant.H:64` `epsilon_0_v = 8.8541878188e-12`、`:70`
  `mu0_v = 1.2566370612685e-06`。即 WarpX 的值是 `…8188e-12`，README 把它写成 `…8128e-12`
  （CODATA 值）并宣称"这是 WarpX 值、不要混用 CODATA" —— 恰好写反。
- 建议修法：README 与 4 个 inputs 统一到 `8.8541878188e-12`（或直接用 `epsilon0`/`mu0` 别名）。

### F11 `preflight.py` 的 σ 剖面包出死分支、非 `z>` 形式会崩、且 `eps_profile` 文档不符
- 文件:行 `benchmarks/cases/c03_medium/preflight.py:56`（`0.0 if amp is None else 0.0`）、
  `:57`（`return ("function", None, None, 0.0)` → `:139/:155` 把 `None` 送进 `complex_k` →
  `TypeError`）、`:64`（docstring 说返回 3 个值，实际 2 个）
- 严重度：minor　置信度：**confirmed**（读码可判定；未跑 WarpX）
- 具体反例：`macroscopic.sigma_function(x,y,z) = "s*(z<0)"`（材料 1 有损，合法输入）→ 正则
  不匹配 → `kind="function"`、`s1=None` → `complex_k(omega, eps1, None)` 抛异常，
  而 preflight 的职责是"在解释任何 R/T 之前先给出可读的 FAIL"。
- 建议修法：step 分支返回 `amp`（而不是恒 0），非匹配形式明确报 "unsupported sigma_function"。

### F12 `p1_tensor_reference.py` 的"切向旋转"用例是逐字节重复，未测任何东西；`--compare` 不返回失败
- 文件:行 `benchmarks/cases/c02_0_pec/p1_tensor_reference.py:137-139`（两组 `out.append(("tanrot", e1,e2,0.4,nn))`
  完全相同）；`:186-213`（`--compare` 只 print，无 `sys.exit`）
- 严重度：minor　置信度：**confirmed**
- 证据：`p1_cases.csv` 的 case 35 与 case 36 完全相同（`kind=tanrot`，e1/n/f 全同）。
  原意显然是"同一 n、旋转切向框架"以验证结果与切向基无关，这条覆盖完全缺失。
  `--compare` 打印 `failures = N` 后 `return`（`exit 0`）→ 不能作为门禁。
- 建议修法：case 36 用 `frame()` 里换一个 `t1` 生成新的切向基（或对 n 施加绕轴旋转并同步旋转材料），
  并在 `--compare` 末尾 `sys.exit(1 if bad else 0)`。
- 补充：`p1_cases.csv`/`p1_expected.csv` 本身经数值核对是**正确**的（见"已验证正确项"）。

### F13 关键阈值来自"观测到的散布"（事后调参），且没有预注册文件
- 文件:行 `analyze_l3.py:20-26`（"measured: −0.92..−1.11 …|r+1| ≤ 0.20"）、
  `analyze_oblique.py:89-93`（`tol_single=tol_r=0.20`、`tol_mean=0.06`、注释 "±10–17% modulation"）、
  `check_stability.py:38-39,72`（允许 50% 能量增长）、`analyze_cylinder.py:47,79`（比值门限仅 1.2）
- 严重度：minor　置信度：**confirmed**
- 依据：`EM_EB_MHD_VALIDATION_10CASES_v2.md:196` 规定"`expected.json`（阈值 + 判据）必须在**第一次
  运行前**提交"，`:811` 甚至规定"`REPORT.md` 里出现手工数字即视为数据污染"。但 `benchmarks/`
  下**没有** `STATUS.md`/`CHANGELOG.md`/`expected.json`/`results.json`/`tools/verify.py`/`meta.json`；
  数值以手写表格形式出现在 `A_TM_BASELINE.md` 与各地 md 里。
  （缓解：`A_TM_BASELINE.md` 的三个数字我已用 `probe_tm_*.txt` + `analyze_tm.py` 精确复现，
  所以不是"编造"，只是违反了它自己声明的流程，且阈值是事后定的 → 无法排除"调到通过"。）
- 建议修法：补 `expected.json`（含 `analyze_l3/analyze_oblique` 的阈值来源说明），或把门限改成
  与分辨率无关的收敛判据（例如 r 的误差随 n 单调下降）。

### F14 三份 md 的"代码事实"已被本 fork 自己的修改推翻，但仍以现状语气陈述
- 文件:行 `cases/c02_0_pec/ROOTCAUSE_ECT_2D.md:12-23`（"`Source/` 与上游 clone 逐字节一致；
  唯一差异是新增 `Source/MomentsSolver/`"；"TE 在 2D XZ ECT 下不支持"）、
  `cases/c02_0_pec/B_2D_TE_RESEARCH.md:1-23`、`CUTCell_LITERATURE_RESEARCH.md:363`、
  `M2A_SOURCE_REVIEW_FINDINGS.md:25-29`
- 严重度：minor（文档）　置信度：**confirmed**
- 依据：当前树 `Source/` 已重构为 `Source/Core|Plugins`（无 `MomentsSolver`），且
  `Source/Plugins/FieldSolver/FiniteDifference/EvolveECTRho.cpp:117-124` 已在 XZ 下计算 `Rhox`
  （`Ey*ly` 差分），`WarpX.cpp` 已有 `algo.ect_te_borrowing` —— 即 2D TE 已被实现。
  于是 `c02_0_pec` 里 12 个 TE 算例（`inputs_2d_l3*`、`inputs_2d_oblique_te_*`、
  `inputs_2d_cyl_te`、`inputs_mie_ey_*`）在新代码上是**有效**的，而 ROOTCAUSE 报告会让读者
  以为它们无效、或以为验证对象仍是"未改动的上游代码"。
- 建议修法：文档抬头加"截至 <commit/日期>，2D TE 已由 B1' 实现，本文结论仅作历史记录"。

### F15 目录卫生：`__pycache__/*.pyc` 入库
- 文件：`benchmarks/cases/c02_0_pec/__pycache__/p1_tensor_reference.cpython-314.pyc`（15970 B）
- 严重度：minor　置信度：**confirmed**（`git ls-files benchmarks` 117 个，磁盘 118 个）
- 建议：删除并加 `.gitignore`。（注：我运行 `import mie_cylinder` 时产生的临时 `.pyc` 已删除，
  工作树与审计前一致，`git status --short benchmarks` 为空。）

### F16 `conductor_slab` 的"纯前向波"说法与自身另一脚本矛盾；提交的探针恰在被排除区
- 文件:行 `cases/c03_medium/conductor_slab/inputs_2d:13-17`（"probes at z=0.046875 与 0.234375 …
  纯前向波"对比 `analyze_v2.py:12-13`（"fit 从 z ≥ 1.5δ 开始，前 1.5δ 被界面瞬态污染"）
- 严重度：minor　置信度：**likely**（读码可判定不一致；无法在无构建时复算）
- 依据：δ=0.1 m，两个探针在 0.047δ 与 0.234δ，**都**落在 `analyze_v2` 明确排除的区域；
  `analyze_v2` 的默认探针是 j=323..335（z=0.109..0.484）。同一目录三个分析脚本
  （`analyze.py` / `analyze_v2.py` / `analyze_cw.py`）用了三套不同的探针与三套理论表述，
  但只有 `analyze.py` 的探针在 inputs 里存在（见 F6）。
- 建议修法：统一到 V2 的 7 探针 + CW 源（并把该 inputs 入库）；`inputs_2d` 的注释按实测改写。

### F17 多处 `print` 无门禁 / 无 assert（"verify"脚本不 verify）
- 文件:行 `verify_oblique_rt.py` 全文（只 print 相对偏差，从未 `return 1`）、
  `verify_medium_speed.py` 全文、`compare_t9.py`、`analyze_snell.py:96-97`（只 print RESULT，
  但 `main()` 恒返回 0）、`check_probe.py`、`gauss_toy.py`、`disk_poles.py`
- 严重度：minor　置信度：**confirmed**
- 依据：`analyze_snell.py:96-97` 打印 `RESULT: PASS/FAIL` 后 `main()` 无返回值 → 进程总是 0；
  `verify_oblique_rt.py` 连 PASS/FAIL 都不打印。
- 建议：统一 `sys.exit(0/1)`，否则这些脚本不能作为回归门禁（与 `EM_EB_MHD…:196` 的 G3 门冲突）。

### F18 `verify_fresnel_rt.py` 的 q1/q2 注释与代码相反（代码是对的，注释会误导）
- 文件:行 `verify_fresnel_rt.py:176-181`
- 严重度：minor　置信度：**confirmed**
- 现象：注释写"q1 上界面反射(9.0 ns)与 z=−4 处 PEC 回波(9.3 ns)重叠… **Use q2 only**"，
  但紧接着 `ref2 = dft(a_dir,"q1",…)`、`tot2 = dft(b_dir,"q1",…)` —— 实际用的是 q1，
  且 q2（z=+0.8）在透射侧，不可能测反射。几何也不符：界面在 z=0、PEC 在 z=±8，
  q1 处入射 3.3 ns、界面反射 6.7 ns、PEC 回波 ~57 ns（与 9.0/9.3 ns 无关）。
  真实窗口 [0,8] ns 恰好同时含入射与反射 —— 减谱法要求的正是这个，所以**代码正确、注释是旧几何的残留**。
- 建议：删掉该注释块；把 `r_c/ref2/tot2` 的命名改回 `q1`。

### F19 `make_mie_cases.py` 的注释/文档与实际网格不符；单网格是为了绕开未定性不稳定
- 文件:行 `make_mie_cases.py:20`（"dx = 2/10, 2/20…"）、`:45`（模板注释 "domain [-3,3]^2"，
  实际 `-4.0..4.0`）、`:53`（`amr.max_grid_size = {n}` + 注释 "the CW laser source is unstable
  across grid boundaries"）
- 严重度：minor　置信度：**confirmed**
- 影响：`max_grid_size=n` 强制单网格（`analyze_mie_convergence.read_pf` 也只读单 FAB），
  所以 640² 以上只能单核/单箱跑；而"CW 源跨网格边界不稳定"这个**求解器级问题**只被注释记录、
  没有根因、也没有算例覆盖 → 属于"把算例调到能通过"，而不是修好。
- 建议：单列一条 instability 记录（输入、现象、版本），并把 `max_grid_size` 作为可选项暴露。

### F20 `c03` 若干小问题（合并）
- `conductor_slab/analyze.py:116`：`print("PASS" if ok else "FAIL" + f"…")` 优先级错，
  容差信息只在 FAIL 时打印。
- `lossy_slab/analyze.py:35-36`：docstring 用法写 `--t0 20e-9`，实际默认 `0.0`。
- `lossy_slab/inputs_2d:6-7`：`stop_time=95 ns` 但 `max_step=5000` 在 dx=0.03125 下只能到
  ~73.6 ns（`dt=1.47e-11`）→ 文档时间窗取不到。
- `tensor_interface/analyze.py:65`：`--eps2-yy` 被接受但从未使用（死参数）。
- `anisotropic_slab/inputs_2d_p:39`：`laser1.e_max = 1.0` 与 `E0=1e6` 不一致（本例 e_max 未被
  `parse_field_function` 使用，故无害，但会误导）。
- `anisotropic_slab/analyze.py:36-38`：以 `|y|` 的**载波极值**差当作包络峰位移；该例 τ=3.2 ns、
  载波周期 3.34 ns（单周期脉冲），峰位量化误差 ~T/4 ≈ 0.83 ns，而 `README:22` 要求 ≤0.5%
  （dz=3 m、v≈c/1.5 → 20 ns 传输，0.83/20 = 4%）→ 该判据用这个方法达不到。
- `mie_cylinder.py:107-109`：对 `by` 打印 "max|total(r=a)| … (should be ~0)"，而 TE^z 的边界
  条件是 ∂B/∂r=0（1.968 是正确值，标签错）；`:20` 的 `σ=(2π/k)|Σj^n c_n e^{jnφ}|²` 与代码
  `(4/k)`（`:91`）自相矛盾（代码对、文档错，见 F1）。
- `sigma_exponential_test.py:71`：`Edec @ (eps_h @ E) * 0 + …` 残留无效项。
- `CUTCell_LITERATURE_RESEARCH.md:341`：参考文献编号 26 → 30 → 27（跳号）。
- `M2A_SOURCE_REVIEW_FINDINGS.md:174`：引用的 `c02_0_pec/diags/reducedfiles/` 不在仓库里
  （只有 `probe_tm_*.txt` 三个副本），TE 基线数据已丢失。

## 已验证为正确的项（避免误报）

1. **`generalized_fresnel.py`**：`n_modes`/`normal_incidence` 与 2×2 横向块本征值一致（Rz(45)
   diag(1,4,2) → n₂=1,2，R=0,1/9，均值 1/18 ✓）；`uniaxial_s_p` 的 s 用 `n₂cosθ₂=kz`、
   p 用 `kz²/exx+kx²/ezz=k0²` 且 `r_p=(kz/exx−cosθ)/(kz/exx+cosθ)`，与标准 Fresnel 幅度
   **完全相同**（60°/n=2 → Rp=0.00269、Rs=0.3201；80° → Rp=0.1846、0.6701，与手算一致）。
2. **`airy_slab.py`**：`r=(r12+r23e^{2iδ})/(1+r12r23e^{2iδ})`、`t=t12t23e^{iδ}/…`、
   `δ=n2ωd/c` 正确；共振/反共振/FSR 自检均对。
3. **`oblique_rt.py`**：2×2 波的分解行列式与解正确，`T=(k2/k1)|t|²`（s 偏振）正确，自检 R+T=1。
4. **`mie_cylinder_dielectric.py`**：TM/TE 系数与 `disk_poles.py:25-32` 的分母一致；
   光学定理 `|c|²=Re c` 在 m∈{1.5,2,4}、x∈{0.5,2,5}、两种偏振下到 1e-16（**强证据**）；
   大 x 消光佯谬 σ_sca/4a→1.045（x=100）方向正确。
5. **`p1_tensor_reference.py` 的张量代数**：`tau`/`tau_inv` 是带 −1 的 Kottke 约定，`tau_inv(tau(e))=e`
   到 3e-16；`chi_inv_anisotropic` 与闭式 `chi_inv_isotropic` 对 iso_iso 完全一致。我一开始按
   无 −1 的 Kottke 原式判断成"符号错"，**实测否定**，故不列缺陷（见 `p1_expected.csv` 与
   `math/test_reference_math.py` 的 M1/M2/M4 全 PASS）。
6. **`A_TM_BASELINE.md` 的三个 r 值与时刻**：用仓库内 `probe_tm_*.txt` 重跑 `analyze_tm.py`
   逐位复现（−0.999295 / −0.999679 / −0.999793；43.488/43.414/43.439 ns），且 r 随 n 单调
   收敛到 −1 —— 这条基线是可信的。
7. **`verify_fresnel_rt.py` 的物理**：真空参考 + 减谱求 R、`(n2/n1)|t|²` 求 T、坡印廷
   `S_z=(E×H)_z/μ0` 与 TE/TM 两种符号都推对了；`R_flux=(Wi−Wt)/Wi` 等价于 R（反射为负通量）✓。
8. **`EM_EB_MHD_VALIDATION_10CASES_v2.md` 的解析公式**：A.1 复 Fresnel、A.2 Airy、A.3 特征矩阵、
   A.4/A.5 Mie、A.6 TIR、A.8 Stix 均与标准形式一致；C5 的 δ_s 数值（σ=0.01→0.53λ、
   σ=10→0.0168λ、Δx≤λ/480）我逐项复算都对。
9. **`M2A…:181`** 的解析预言 `c·(4/1.5)e^{-4}·10⁶ ≈ 1.47e13` 复算 = 1.464e13 ✓。

## 不确定但值得复核的点

- **R1** `analyze_mie_convergence.py:144-145` 的"对齐一个全局相位"（`phase` 取自 `A·Am*` 的和）在
  同时存在入射相位误差时会把这部分误差吸收进 `A_al`；配合 `:160-168` 的"有/无 vacuum 运行"
  两条路径，前者给出的是**真**离散误差，后者带有作者自述的"~14% floor"。若任何报告引用了
  无 `--vacuum` 的收敛阶，那个阶数测的是入射波相位失配，不是求解器精度。
- **R2** `tensor_interface/analyze.py:45-51` 的 `pulsetimes` 硬编码 `t0=16 ns`（=8τ）。若有人改
  `tau` 或激光的 `8.0*tau`，`pulsetimes` 不会跟着变（无一致性检查），窗口会整体偏移半个脉冲。
- **R3** `analyze_l3.py:85-88`、`analyze_oblique.py:106-109` 的 `s = −(cosα·x_p+sinα·z_p)/sinα`
  与 `t_ref` 都假定界面过原点（`x0=z0=0`）。`make_l3_angle.py:32-34` 用字符串替换改 `alpha`，
  若模板行不匹配则**静默**保留 30°（无 `assert`），而探针位置却按新角度移动 →
  这类"半改"会产生错误但看起来正常的结果。
- **R4** `checksum_calc.py`/`plotfile_checksum.py` 假定"数据在文件尾部、份量 `ncell*nvars*8`"、
  且 `reshape(nvars, ncells)`（分量主序）。对多 box、或有 `FAB` 尾随元数据的 plotfile 会静默
  取错偏移（`analyze_mie_convergence.read_pf:53` 同样假定单 box）。目前 Mie inputs 用
  `max_grid_size=n` 保证了单 box，但脚本本身没有断言 box 数 == 1。
- **R5** `disk_poles.py:48` 的默认 `--guess 1.47 1.84 2.20` 与 docstring 示例的 `5.9 7.4 8.8`
  不一致；`newton` 对复变量求解未做 `success` 校验，失败时走 `except` 打印 "failed"。
  它给出的 Q 定义 `Q=Re(x)/(2|Im(x)|)` 正确，但没和 `prony_q.py` 的 `Q=πf/γ` 做交叉验证。
- **R6** `prony_q.py:29-55` 的矩阵束用 `pinv(Y0)`，`order=24` 时条件数可能很差；脚本没有
  对"极点在单位圆内的个数"做健全性检查，`amps` 变量实际返回的是 `|z|` 而非留数（命名误导）。
- **R7** `plotfile_checksum.py` 的 `dim=int(hdr[2+nvars])` 只用于打印；若 dim=3 时
  `nx*ny` 少乘 nz，会算错 `need` → `assert off>0` 可能仍成立而数据错位。当前只用于 2D 算例。

## 逐文件台账（结论）

- 顶层：`check_probe.py`（无断言，OK 辅助）、`checksum_calc.py`（yt 依赖，OK）、
  `plotfile_checksum.py`（2D 单 box，OK，见 R4/R7）、`env_check.py`（OK）、`probe_fab.py`
  （硬编码 target 值的调试脚本，OK）。
- md：`EM_EB_MHD_VALIDATION_10CASES_v2.md`（**计划书**，自述"未跑任何算例"，公式已核）、
  `M4_AMR_IMPLEMENTATION_RESEARCH_v1.md`（文献与代码事实已抽查：`FillPatcher` 零引用 ✓、
  `build_eb_data_per_level` ✓、`Macroscopic properties are not supported with mesh refinement` ✓）、
  `CUTCell_LITERATURE_RESEARCH.md`（文献核对可信，编号跳号 + 2D TE 结论过期）、
  `cutcell-research-REVIEW.md`（评审意见，证据索引路径为旧布局）、
  `M2A_SOURCE_REVIEW_FINDINGS.md`（同上 + 引用的 diags 不在库内）。
- c02：`verify_fresnel_rt.py`（物理对，注释 F18）、`verify_oblique_rt.py`（公式对、无门禁 F17）、
  `generalized_fresnel.py`（✓）、`airy_slab.py`（✓）、`oblique_rt.py`（✓）、
  `analyze_mie_convergence.py`（F1/F2/R1）、`mie_cylinder.py`（F1）、
  `mie_cylinder_dielectric.py`（✓，仅文档公式 2π/k 错）、`compare_t9.py`（F6，只支持 ey）、
  `analyze_snell.py`（F6/F17）、`analyze.py`/`analyze_tm.py`/`dump*.py`（✓，实测复现基线）、
  `analyze_l3.py`/`analyze_oblique.py`（几何对，阈值事后 F13/R3）、`analyze_cylinder.py`
  （弱判据，诚实标注为定性）、`analyze_3d_te.py`（✓，3D TE 对照）、`check_stability.py`
  （F9/F13）、`compare_degen.py`（F8）、`compare_t9.py`（F6）、`g2_neigh_truth_table.py`
  （只 print 的"真值表"，模型化复述代码而非调用代码）、`gauss_toy.py`（✓ toy）、
  `prony_q.py`（R6）、`disk_poles.py`（R5）、`meep_disk.py`/`meep_ring.py`（外部 Meep 环境，
  未安装 → 不可复现，且 `meep_ring.py` 注释自述"earlier ring attempt returned zero modes"）、
  `p1_tensor_reference.py`（代数 ✓、F12）、`p1_tensor_test.cpp`/`sigma_exp_test.cpp`（独立 C++
  对照，✓ 设计合理）、`sigma_exponential_test.py`（F3/F20）、`tau_transform_test.py`（✓）、
  `make_*.py`（✓，`make_mie_cases.py` 见 F19）、`run_*.sh`（F7）、`inputs_*`（F4/F8/F9/F19）、
  `probe_tm_*.txt`（✓ 数据可用于复现）、`p1_cases.csv`/`p1_expected.csv`（✓）。
- c03：`math/test_reference_math.py`（**ALL PASS**，✓）、`preflight.py`（F11/F10）、
  `lossy_slab/*`（F6 无关、F20）、`conductor_slab/*`（F3/F6/F16/F20）、
  `anisotropic_slab/*`（F20 的峰位方法与判据不符）、`tensor_interface/*`（物理 ✓、F20 死参数）、
  `README.md`（F10 的 ε₀ 声明、`S1–S4` 引用的 `c02_0_pec` 脚本实际不存在同名文件）。
