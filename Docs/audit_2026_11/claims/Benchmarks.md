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
