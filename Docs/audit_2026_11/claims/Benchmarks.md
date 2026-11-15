# Benchmarks 工作日志

claim: Benchmarks F6,F8–F20 @lead 2026-11-15T19:50

本轮处理（按 BACKLOG 顺序）：

F8 fixed: `inputs_ect_2d_degen` / `inputs_yee_2d_degen` 的注释把 PEC 侧写反（物理区是 implicit function
  的**负**区 ⇒ `eb_implicit_function="z"` 时导体在 z>0，注释却写 z<0），且 `fp2` 探针在 z=+1.5 即导体
  内部 ⇒ 与参考比较是 0 vs 0、**永远不可能失败**。注释改正；`fp2` 移到 z=-0.5（真空侧、靠近界面，
  与 z=-1.5 的 fp1 形成两个不同的真空取样点）。
F9 fixed: `inputs_ect_2d_45deg_tm` 注释写 "TE pulse, Bx=-Ey/c"，而文件本身与下方 `Ey=0`、`By∝Ex` 都是 TM；
  改正为 TM（+z 传播，By=+Ex/c），并删掉"BCK-only 用于 TE 分量"那句与 2D TM 不符的描述。
F10 fixed: 5 个脚本/12 个 deck 周转着用 `8.8541878128e-12`(CODATA) 与 `8.8541878188e-12`(WarpX
  `ablastr::constant::epsilon_0_v`)；README 还把 CODATA 值标成"WarpX value; do not mix CODATA"——恰好写反。
  全部统一到 WarpX 值，README 写明出处。（相对差 ~7e-10，数值上无害，但"约定"本身是错的。）
F15 invalid（已不成立）: `benchmarks/**/__pycache__/*.pyc` 已**未被跟踪**（`git ls-files | grep -c pyc` = 0），
  `.gitignore:26` 已含 `__pycache__`；磁盘上的几个残留已清掉。
F18 fixed: `verify_fresnel_rt.py` 的 q1/q2 注释是旧几何残留（说"用 q2 only"，但 q2 在透射侧、不可能测反射；
  引用的 9.0/9.3 ns 与本 deck 无关）。代码本身正确，注释已改写为当前几何的解释。
F9（Python 那条）与此处无关，勿混。

附带：`dependencies.json` 的 `_note_picmi` 说明**已撤回**——仓库 `AGENTS.md` 明确规定该文件由上游自动更新、
  不得手动修改，所以把"`version_picmi` 才是 install_requires 的 pin、`commit_picmi` 只是来源提交"这一结论
  记录在这里，而不是写进文件本身。

F12 fixed: `p1_tensor_reference.py` 的 "tanrot" 两行是**逐字节相同**的调用（case 36 = case 35，什么都没测），
  而 `--compare` 在打印 `failures = N` 后 `return`（exit 0），不能当门禁。
  现在：case 36 把材料绕 **n 旋转 0.9 rad**（新增 `rot_about()`，Rodrigues 公式），真正检验"结果只依赖
  法向、与切向基无关"这一对称性；新增 `selfcheck_tangent_rotation()` 在 `--write` 时断言
  `chi(ReRᵀ, n) = R chi(e, n) Rᵀ`（实测 5.3e-16，阈值 1e-12）；`--compare` 末尾 `sys.exit(1 if bad else 0)`。
  重新生成的 `p1_cases.csv`/`p1_expected.csv` 只差 case 36 一行（`git diff --stat` = 2 files, 2 changed）。
  反证: 把 Case 5 的期望值乘 1.001 → `--compare` 报 `failures = 1` 且 **exit code 1**。
F17 fixed（三个脚本）: `analyze_snell.py`、`verify_oblique_rt.py`、`verify_medium_speed.py` 原本只 print、
  恒返回 0。现在各自带判据并 `sys.exit(main())`：
  Snell |Δθ₂| ≤ 0.5°；斜入射 R/T 相对 2%、|R+T−1| ≤ 3%；介质相速相对 2%、材料加权能量漂移 ≤ 1%。
  （这是"verify 脚本不 verify"那一类，和你反复强调的"别让检查器自己失效"是同一个问题。）

F11 fixed: `preflight.py` 的 `sigma_profile()` 有两个真问题：
  (a) step 分支 `return ("step", 0.0 if amp is None else 0.0, amp, ...)` 把算出来的 `amp` 丢掉、
      第一段恒 0——顺序反了，等于把阶跃解释成"z>0 无损耗"；
  (b) 不匹配 `s*(z>z0)` 的形状（例如合法的 `s*(z<0)`）返回 `("function", None, None, ...)`，
      随后 `complex_k(omega, eps1, None)` 直接 `TypeError` —— 而 preflight 的职责是"给出可读的 FAIL"。
  现在 step 分支返回真实 `amp`；不支持的形状返回 `("unsupported", ...)`，`main` 里转成一条
  FAIL 说明并继续（不再崩）。另外 `eps_profile` 的 docstring 承诺第三个返回值 `eps_yy2` 但从不计算，已改为
  真实的两值契约。
  验证: `sigma_function="s*(z<0)"` → `PRE-FLIGHT: FAIL` 且 **python exit=1**（此前是 TypeError 回溯）；
  `"s*(z>0)"` → 打印 `sigma1 = 0.0  sigma2 = 0.05`（此前 `sigma2` 恒 0）。
F19 fixed: `make_mie_cases.py` 的 docstring 说 "domain [-3,3]^2"、"dx = 2/10…"，与 TEMPLATE 和
  `NCELLS=(40..640)`（域宽 8 m）都对不上，已改正；"CW 源跨网格边界不稳定"原本只写在生成模板的注释里、
  无法复现，现在把 `amr.max_grid_size` 暴露成 `--max-grid-size N`（默认仍为单箱），并生成 decks 同步更新
  （diff 仅注释 + 该行）。单箱同时是 `analyze_mie_convergence.read_pf` 只读单 FAB 的要求，这点也写进注释。
  验证: `make_mie_cases.py --max-grid-size 32` → `amr.max_grid_size = 32`；默认重新生成 10 个 deck 的
  diff 只有注释与这一行。

F20 fixed（c03/c02 若干小问题，逐项）:
  - `conductor_slab/analyze.py`: `print("PASS" if ok else "FAIL" + f"…")` 的优先级错（容差只在 FAIL 时打印）
    ——加括号，PASS 时也打印。
  - `lossy_slab/analyze.py` docstring 的用法写 `--t0 20e-9`，实际默认 `0.0`/自动；改为真实默认。
  - `tensor_interface/analyze.py`: `--eps2-yy` 被接受但从未使用 ⇒ 用 `help=argparse.SUPPRESS` 隐藏
    （保留兼容，不再假装是个可调参数）。
  - `anisotropic_slab/inputs_2d_p`: `laser1.e_max = 1.0` 与 `E0=1e6` 不一致；`parse_field_function` 不读
    e_max（幅度来自 E0），加注释说明它只是为了满足 laser Bucket 的必填键。
  - `mie_cylinder.py`: 对 `by` 打印 "should be ~0"，但 TE^z 的边界条件是 d(B_z)/dr = 0（打印里
    `max|dH/dr|` 已是 7.6e-10），标签错 ⇒ 按极化分开说明。
  - `sigma_exponential_test.py`: `Edec @ (eps_h @ E) * 0 + …` 的无效项删掉。
  - `lossy_slab/inputs_2d` 的 `max_step=5000`：**复核为不成立**。2D Yee 的
    `dt = cfl / (c*sqrt(2/dx²)) = 0.4*0.03125/(c*sqrt2) = 2.95e-11 s`（`CartesianYeeAlgorithm::ComputeMaxDt`），
    5000 步 = **147 ns > stop_time 95 ns**，时间窗取得到；review 里 dt=1.47e-11 的算式不对。
