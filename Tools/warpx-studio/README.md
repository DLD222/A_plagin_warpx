# WarpX Studio

本 fork（`A_plagin_warpx`）的图形化仿真工作室：填参数 → 校验 → 跑 → 边算边看。
**新目录，不改动仓库里任何既有文件。**

当前进度：**阶段 1（参数目录）已完成**，运行/可视化模块按阶段 2 起逐步接入。

## 为什么参数目录要读源码

用户要求"只在某功能开启时才生效的参数要折叠起来"，而这类关系只写在 C++ 里：

```cpp
if (collision_type == CollisionType::PairwiseCoulomb) {
    pp_coll.query("correct_energy_momentum", correct_energy_momentum);   // 只有成对库仑才读
}
```

所以目录由三份数据合成，全部可在 `warpx_studio/catalog/data/` 里审查：

| 文件 | 内容 |
|---|---|
| `params_docs.json` | 从 `Docs/source/usage/parameters.rst` 解析出的 386 条（去重后）文档参数：类型、默认值、说明、章节 |
| `param_sites.json` | 扫 `Source/**`（839 个文件、687 个读取点）得到的：完整键名、读入变量、外层 `if`/`#if` 条件 |
| `catalog.json` | 合并结果：732 条参数、分区、可重复组、`gating`（运行期条件）、`build_guards`（`#if` 特性） |

统计（2026-11-15）：

```
732 条参数 = 386 文档 + 346 仅源码（fork 自有：hybrid_pic_model.*、psatd.*、<solver_name>.* …）
136 条带运行期 gating      70 条带 #if 特性守卫
可重复组：<species_name> 99、<collision_name> 48、<diag_name> 47、
          <reduced_diags_name> 21、<solver_name> 20、<laser_name> 18
```

`build_guards`（如 `#ifdef WARPX_QED`）只作为**提示**展示，不隐藏字段——按你的要求不区分构建特性。

## 键名归一化

- 分节对象：`ParmParse pp_psatd("psatd"); pp_psatd.query("nx")` → `psatd.nx`
- 运行期分节：`ParmParse pp_species_name(species_name)` → `<species_name>.charge`，
  自动归一到文档用的占位符（`<species_name>` / `<collision_name>` / `<diag_name>` /
  `<reduced_diags_name>` / `<laser_name>` / `<solver_name>`），与 `parameters.rst` 一致。
- 拼接键：`pp.contains(section + ".field")` → 前缀不确定，归到 `<dynamic>.*`，不伪装成顶层参数。

## 重新生成

```bash
cd ~/WarpX_plugin
~/miniconda3/bin/python3 Tools/warpx-studio/warpx_studio/catalog/parse_docs.py      # 文档 → params_docs.json
~/miniconda3/bin/python3 Tools/warpx-studio/warpx_studio/catalog/scan_source.py     # 源码 → param_sites.json
~/miniconda3/bin/python3 Tools/warpx-studio/warpx_studio/catalog/build_catalog.py   # 合并 → catalog.json
```

## 已知限制（不猜，标出来）

1. 本机没有 clang/libclang、也没有 `compile_commands.json`，所以 gating 用"括号栈 + if 上下文"的纯 Python
   扫描器，不是编译器的 AST：宏展开、跨函数数据流（值在别处算出）、`switch` 分支都**不追**，
   这类读取点 `conditions` 为空，会落到"全部"分区而不是被错误折叠。
2. 只覆盖 `Source/**` 里的 `ParmParse` 读；AMReX 自身的输入（`amr.*`、`geometry.*`）以此类推也能扫到，
   但它们的语义不在本仓库文档内。

## 目录规划（后续阶段）

```
warpx_studio/
├── catalog/     # 阶段 1：已完成（本文件所述）
├── deck/        # 阶段 2：表单 ⇄ .inputs 双向映射、类型校验、ParmParse 报错翻译
├── runner/      # 阶段 2：二进制探测、mpirun/线程、日志→进度、停止/暂停/续算、CMake 构建
├── live/        # 阶段 3：pvpython 渲染 + 帧推送，一键转 ParaView 深挖
└── web/         # 阶段 2：FastAPI + 前端（分区折叠、+号组、向导）
```

## 阶段 2：2D 端到端闭环（已跑通）

```bash
# 启动（独立 conda 环境；渲染仍用 paraview 环境的 pvpython）
cd ~/WarpX_plugin
nohup env PYTHONPATH=$PWD/Tools/warpx-studio \
  ~/miniconda3/envs/warpx-studio/bin/python -m uvicorn warpx_studio.web.app:app \
  --host 127.0.0.1 --port 8770 > /tmp/studio_server.log 2>&1 &
# Windows 浏览器打开 http://127.0.0.1:8770
```

链路：表单（分区折叠 + `+` 号组）→ `deck/model.py` 生成 `.inputs` → `runner/job.py`
用 `warpx.2d` 跑（mpirun/OMP 可调，STEP 行解析成进度，可停止/暂停/继续）→
`live/service.py` 盯着 `diag1/openpmd_%06T.h5`，出现新迭代就调
`live/render_openpmd.py`（pvpython + ParaView 的 openPMD 读取器）渲染一帧，
网页轮询 `/api/frame` 显示。

实测（2026-11-15）：`warpx.2d` 200 步 / 128² 网格，2 s 跑完；
实时图按迭代刷新（每帧约 2.0 s，几乎全是 pvpython 冷启动），最后一次渲染
`iteration=200 ... ok=True`。

已知限制：
- 每帧起一个 pvpython 进程（约 2 s）。下一步改成常驻渲染进程，把单帧压到 ~0.1 s。
- 2D 数据是 (x, z) 平面、y 只有一层，渲染器按"退化轴"直接显示而不是 Slice（否则会切成一条线）。
- `algo.particle_shape`、`<species>.profile`、`<species>.momentum_distribution_type` 在本 fork 里是
  **必填**（源码里是 `get` 而不是 `query`），向导起步模板已带上。
- 中文说明目前覆盖向导与常用参数（`catalog/data/zh.json`），其余回退英文原文。

## 阶段 3：常驻渲染进程 + 校验/报错翻译（已跑通）

- **实时帧 70 ms**（原来每帧 2 s）：`live/render_server.py` 常驻 pvpython，
  一次加载 ParaView，之后按 JSON 请求出图；`live/render_core.py` 复用 openPMD 读取器，
  只在磁盘上新出现文件时才重建它（否则序列会停在启动时的那一帧）。
  实测：首帧 1135 ms（进程启动），之后 67–75 ms。
- **校验**（`deck/validate.py` + `POST /api/validate`）：
  - 类型检查（`amr.n_cell = 64 abc` → 报错；`warpx.cfl = abc` → 报错）；
  - 必填项：只对经验证会立即 abort 的 6 个键（`algo.particle_shape`、`<species>.charge/mass/profile/
    momentum_distribution_type`、`<diag>.diag_type`）硬性拦截，其余"源码里是 get"的仅作提示——
    因为 `get` 可能藏在某个分支里，误报会挡住本来能跑的算例；
  - 资源预估（网格点数 → 内存量级）与 openPMD 未设 `intervals` 的警告。
- **报错翻译**：`### ERROR : …` 与 `ParmParse::getval(): electrons..profile not found`
  都能定位到具体字段，并按运行时的 deck 把 `electrons.profile` 归到 `<species_name>.profile`
  （而不是激光的同名参数），附带中文/英文说明与可选值。
- **枚举参数变下拉框**：源码里的 `var == "value"` 比较被提取成选项（如 `<collision>.type` → 
  bremsstrahlung / dsmc / pairwisecoulomb …），27 个参数因此有了下拉。

实测（2026-11-15 23:40）：`/api/validate` 对起步 deck 返回 0 条问题；删掉 `profile` 后
`/api/run` 直接被拒（400）；强制运行后 `diagnosis = {param: "<species_name>.profile", hint: "Density profile for this species…"}`。

## 阶段 4：ParaView 深挖 + 续算（已跑通）

- **一键 ParaView 深挖**：`POST /api/paraview` 把当前 run 的 openPMD 导成
  `.vti` + `.pvd`（`live/export_vtk.py`），再拉起 ParaView 打开 `.pvd`。
  实测：`iterations: 4, files: 4`；用 `PVDReader` 复核 `TimestepValues` 有 4 个时间步、
  数组 `rho/B/E` 齐全（时间滑条可用）。
- **续算**：`GET /api/checkpoints` 列出历次 run 的 `chk*` 目录；界面上选一个 →
  写入 `amr.restart`（源码里是 `pp_amr.query("restart", ...)`）→ 运行。
  实测：第一段跑 40 步生成 `chk000000/020/040`；从 `chk000020` 续算后，
  新 run 的 openPMD 迭代从 **000030** 开始（不是 000000），checkpoint 继续写 040/060 —— 确实接上了。
- 起步模板自带一个 `format=checkpoint` 的诊断（`chk`, intervals=20），所以续算是开箱可用的。

## 阶段 5：维度切换 + 可插拔面板（已跑通）

- **维度选择**：界面左上角 1D/2D/3D/RZ。切换后按维度重建起步模板并只列出该维度的二进制：
  `amr.n_cell` 项数、`boundary.*` 项数、每方向宏粒子数、粒子注入区键名（1D 用 `zmin/zmax`，
  2D 用 `x..z`，3D 全用，RZ 用 `x..z` 作 r/z）全部随维度变化，避免"给 1D 二进制生成 2D 输入"。
  实测：`/api/starter?dims=1d` → `geometry.dims=1, n_cell='64'`；1D 端到端跑 60 步，
  实时帧 `iteration 60` 正常渲染。
- **可插拔面板**：`warpx_studio/plugins/*.py`（或 `~/.config/warpx-studio/plugins/`）里放一个模块即可：

  ```python
  NAME = "运行摘要"
  def panel(context):        # context: run_dir / job / live / params
      return "<table>…</table>"   # 任意 HTML 片段
  ```

  界面上"6 插件"页会自动列出并每 3 秒刷新；插件抛异常只会在自己的面板里显示，不影响服务与其它插件。
  自带示例 `plugins/run_summary.py`（运行目录/步数/时间/dt/最近一帧/色标范围）。
  实测：`/api/plugins` → `[{"id":"run_summary","name":"运行摘要"}]`，`/api/panel/run_summary` 返回 HTML。

## 阶段 6：CMake 构建按钮（已跑通）

界面第 7 页"构建"：选维度 → 开始构建 → 实时日志 → 构建出的二进制自动出现在"二进制"下拉里。

关键点是**不要自己编造编译选项**。开发过程中踩了三次坑，都记在这里：

| 现象 | 原因 | 修法 |
|---|---|---|
| `WarpX_DIMS=1d is not allowed` | 界面用 `1d`，WarpX 只认 `1/2/3/RZ` | `DIMS_MAP` 映射 |
| `invalid new-expression of abstract class type 'PhysicalParticleContainer'` | 少了 `WarpX_PLUGIN_*` 开关，粒子接口纯虚 | 用 `audit_build/fork/CMakeCache.txt` 的选项作基线 |
| 配置阶段去 GitHub 下载 AMReX / PICSAR / nlohmann_json / toml11 | 没有指向本地依赖 | 直接 `source audit_build/configure_common.sh` 复用它的 `COMMON` 数组，再补 `-DWarpX_amrex_src=` 与 `FETCHCONTENT_SOURCE_DIR_*` |
| `FindPkgConfig` 失败 | 清空环境时丢了 `PKG_CONFIG_PATH`（本地 FFTW 的 .pc） | `clean_env()` 里补上，与 `cleanenv.sh` 一致 |

实测：`POST /api/build {"dims":"1d"}` → `finished 214.4 s`，产出
`audit_runs/studio_builds/1d-smoke/bin/warpx.1d{,.MPI...}`；随后直接用它跑 30 步 → `finished`，无报错。

## v2 · 阶段 1：libclang AST 扫描（已跑通，42/42 自测）

`catalog/scan_ast.py` 用 libclang 解析 `Source/**` 里含 ParmParse 读取的 112 个 TU（不需要
`compile_commands.json`：参数取自 `audit_build/fork/.../flags.make` + GCC 系统头 + `-fopenmp`）。

| 指标 | v1 正则 | v2 AST | 合并后目录 |
|---|---|---|---|
| 读取点 | 687 | 314 | **780** |
| 键 | 567 | 276（仅 clang 新增 77） | **644** |
| 带运行期条件 | 199 | 128 | **194 参数** |
| 来自 switch/case | 0 | **4** | 4 |
| 无条件 `get`（真必填） | 0 | **45** | 47 |

两路是互补的：clang 给结构化条件（真实条件表达式、switch/case、"这个 get 是否无条件"），
正则补齐 clang 只解析了一部分的 TU（碰撞插件那批文件）。合并后参数 732 → **802**。

踩坑记录（都在 `scan_ast.py` 注释里）：
1. 少了 `-fopenmp` → AMReX 的 `#error` 让整个 TU 解析失败；
2. 少了 GCC `-isystem` → `stddef.h` 找不到；
3. `string_literals` 只看直接子节点 → 实参被 `UNEXPOSED_EXPR` 包装，**一个字面量都抓不到**；
4. helper 形式 `query(pp, section, "key", var)` 的 receiver 是第一个实参（`children[0]` 是 callee 包装）。

## v2 · 阶段 2：结构化控件（已跑通，51/51 自测）

三类"其实是小语言"的参数不再要求手写裸字符串：

| 参数族 | v1 | v2 |
|---|---|---|
| `boundary.field_lo/hi`、`boundary.particle_lo/hi` | `"periodic periodic"` 手写 | 按方向的下拉框（2D 给 2 个、3D 给 3 个，RZ 给 2 个），自动拼回字符串 |
| `*_function`、`*(x,y,z)` 表达式 | 自由文本 | 多行编辑框 + 5 个模板 + "检查表达式"（括号平衡、非法字符、未知符号） |
| `*_init_style` 外场/粒子初始化 | 自由文本 | 下拉框（grid 5 种 / particle 5 种，取自源码里的字符串比较） |

表达式校验的原则：**括号不平衡/非法字符 = 错误，未知名字 = 警告**。因为 WarpX 接受任意
`my_constants.*` 常量，把它们当错误会误拒本来合法的输入（`sin(k*x)` 里的 `k` 就是这种）。

自测新增 9 项，覆盖：控件清单接口、2D/3D 边界项数、边界写回、表达式三种判定、API、键识别、风格选项。

## v2 · 阶段 3：运行持久化与重新 attach（已跑通，54/54 自测）

**问题**：v1 的服务一重启就丢了 job 句柄；而且 WarpX 的 stdout 接的是管道，服务一死管道断裂，
WarpX 会被 SIGPIPE 杀掉 —— 根本没机会 attach。

**修法**（`runner/job.py`）：
1. WarpX 的 stdout **直接写 `run.log` 文件**，不再走管道 → 服务死活与仿真无关；
2. 启动时写 `studio_job.json`（pid/pgid/binary/mpi/omp/max_step/started_at），正常结束时删除；
3. 进度由**轮询日志文件**得到（同一个 `_handle_line` 供实时与回放共用）；
4. 服务启动时 `Job.attach(run_dir)`：读 `studio_job.json` → 检查 pid 是否存活 → 回放 `run.log`
   恢复 step/time/dt → 继续 tail。

**实测**（不是设想）：

```
before: state=running step=1237
（kill 掉服务）→ warpx alive after server death: 1
after : state=running step=4260   ← 回放出 4260，且继续 4260 → 5581
stop  -> finished
```

部署：`deploy/warpx-studio.service`（systemd user unit，WSL 默认没开 systemd 时用
`~/.local/bin/warpx-studio` 启动器）+ `deploy/README.md` 三种方式说明。
