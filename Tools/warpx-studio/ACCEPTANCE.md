# 验收报告（自测，2026-11-16 00:20）

运行方式：

```bash
~/miniconda3/envs/warpx-studio/bin/python Tools/warpx-studio/selftest.py
# → 39/39 checks passed
```

`selftest.py` 会自己在随机端口起一个服务、跑完所有检查、再关掉；用到的算例都是几秒钟的小算例。

## 需求 → 证据

| 需求 | 实现 | 自测证据 |
|---|---|---|
| 覆盖全部输入参数 | 文档 386 条 ∪ 源码扫描 346 条 = **732 条** | `目录规模`：732 = 386 + 346 |
| 按物理功能分区 | 按 ParmParse 分节 + 文档章节分组 | 界面 5 步向导 + 分区折叠 |
| 功能未开启时折叠 | 从源码 `if`/`#if` 提取：**136 条 gating、31 条可执行规则** | `gating 提取` |
| 可重复组 + 号增删 | 从 `ParmParse pp_x(species_name)` 推导出 6 类模板组 | `可重复组` |
| 分步向导 | 几何→粒子→场→诊断→插件→构建 | `界面功能存在: renderPlugins/renderBuild/switchDims` |
| 中文说明 | 78 条参数 + 16 个分区标题（其余回退英文原文） | `中文说明` |
| 资源预估/危险值 | 网格→内存量级、openPMD 未设 intervals、无终止条件 | `校验：资源预估`、`校验：正确 deck 无问题` |
| 参数校验 | 类型检查 + 必填项（源码 `get` 且无条件）+ 边界 | `校验：类型错误被拦`、`校验：必填项被拦` |
| 报错翻译 | `### ERROR` 与 `ParmParse::getval` 两类，定位到具体字段 | `报错翻译 → algo.particle_shape / <species_name>.profile / <species_name>.injection_style` |
| 二进制探测 | 扫描 build 目录 + 界面构建产物，解析维度与特性 | `二进制探测`：17 个 |
| MPI/线程/维度 | 顶栏可选；切换维度重建模板并过滤二进制 | `起步模板 1d/2d/3d/rz` |
| 日志进度 | 解析 `STEP n starts/ends` → 步数/时间/dt/进度条 | `日志进度`：step 2000/2000 |
| 停止/暂停/继续 | 进程组 SIGTERM / SIGSTOP / SIGCONT | `暂停`、`继续`、`跑完` |
| 续算 | 列出 `chk*`，写 `amr.restart` 后运行 | `checkpoint 列表`、`续算启动`、`续算完成` |
| CMake 构建 | 复用 `audit_build/configure_common.sh` + 本地依赖，流式日志 | `构建命令`、`维度映射`；另一次真实构建 **214 s** 产出 `warpx.1d` 并成功运行 30 步 |
| 实时 ParaView 画面 | 常驻 pvpython 渲染进程，按新迭代重画 | `实时帧`：iteration 2000 · 68 ms；`帧可下载`：PNG 头正确 |
| 一键 ParaView 深挖 | 导出 `.vti` + `.pvd` 并拉起 ParaView | `导出 .pvd`：20 迭代 / 20 文件；`pvd 内容` 含 timestep |
| 新输入项自动进表单 | 目录驱动：`build_catalog.py` 重新扫源码即收录 | 346 条"仅源码"参数即为此路径 |
| 新 GUI 模块可插拔 | `plugins/*.py` 暴露 `NAME` + `panel(context)` | `插件发现`、`插件渲染`；插件抛错不影响服务 |

## 自测过程中发现并修掉的真实缺陷

1. **必填项漏检**：参数被删除（而不是留空）时，校验循环遍历不到该键，`<species>.profile` 缺失不会被拦。
   修法：单独遍历 `ALWAYS_REQUIRED`，对"不存在"的键也做检查。
2. **暂停后未恢复完成**：暂停用 `SIGSTOP` 打给整个进程组，恢复后需要等进程真正退出才算 `finished`；
   自测原来的 3 秒等待太短，改成 2000 步算例 + 轮询。
3. **界面检查拿到 bytes**：`/` 返回 HTML 时被当成 JSON 解析失败，测试里补了 `decode`。

## 已知限制

- **中文说明只覆盖 78/732 条**（向导、常用、碰撞、激光、PSATD）；其余显示英文原文，不做机器翻译。
- **gating 不是编译器 AST**：宏展开、跨函数数据流、`switch` 分支不追；这类参数落进"全部"分区而不是被错误折叠。
- **必填项只硬拦 6 个**（实测会 abort 的），其余 `get` 读取仅提示——因为 `get` 可能藏在某个分支里。
- **构建按钮**复用本仓库已验证的选项；若改 `WarpX_PLUGIN_*` 组合，可能需要人工确认依赖一致性。
