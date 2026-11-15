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
