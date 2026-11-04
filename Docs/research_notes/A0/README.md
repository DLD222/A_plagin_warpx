# A0 源码审计：类目录 / 公共接口 / 生命周期

<!-- SPDX-License-Identifier: BSD-3-Clause -->

> 状态：**第一遍 + 第二遍核验完成**（2026-11-04）。372 头文件 / 349 类 / 2,418 公共方法；12 子系统矩阵；5 个待验证项已处理，并新增 4 个 `RemakeLevel` 缺口。详见 `VERIFICATION.md`。

本目录是 [`../SOURCE_AUDIT.md`](../SOURCE_AUDIT.md) 的 A0 执行产物，也是 [`../最终的技术路线.md`](../最终的技术路线.md) 的前置输入。

| 文件 | 内容 |
|---|---|
| [`HEADER_CATALOG.md`](HEADER_CATALOG.md) | 372 个头文件的类目录：349 个类/结构体、2,418 个公共方法、按子系统索引 |
| [`api/`](api/) | 逐子系统完整公共签名（16 个文件） |
| [`classes.csv`](classes.csv) | 机器可读：`subsystem,file,kind,class,bases,n_public_methods,...` |
| [`COVERAGE.md`](COVERAGE.md) | 372 个头文件的覆盖表；136 个无 class 的头文件清单 |
| [`LIFECYCLE_MATRIX.md`](LIFECYCLE_MATRIX.md) | 12 个子系统的“所有者/创建/初始化/每步/regrid/restart/归属”矩阵 |
| [`VERIFICATION.md`](VERIFICATION.md) | 第二遍核验：PSATD restart、RemakeLevel 缺口、诊断时序、Python callback、EB per-level；构建/冒烟基线 |
| [`STATIC_INVENTORY.md`](STATIC_INVENTORY.md) | 66 个数据 static 的引用清单与 A1 迁移批次（Config/State/Cache） |

## 时点说明

`HEADER_CATALOG.md` / `classes.csv` 是 **A0 收口时点**（2026-11-04）的快照，尚未包含 A1 新增的 `Source/Runtime/*` 类；A1 结束后统一刷新，避免每个提交都改计数。

## 解析方法与限制

- 解析器：`Tools/A0/parse_headers.py` → `/tmp/header_catalog.json`；生成器：`Tools/A0/build_catalog.py`（重建 CSV/API/索引/覆盖表）。
- 启发式：按 `class/struct` + 花括号匹配提取公共段；跳过 `template<...>` 前缀、内联函数体、预处理器行。
- 已知限制：模板参数/默认值/宏展开可能被简化；嵌套类可能遗漏；`#if` 分支被合并。精确签名以头文件为准。
- 覆盖判据：236/372 头文件解析出类；其余 136 个需人工确认是否纯 free-function/kernel（`COVERAGE.md` 已列全）。

## 下一步（A0 第二遍）

1. 人工确认 136 个“无 class”头文件（区分 kernel/free function/解析遗漏）。
2. 对 `WarpX`、`MultiParticleContainer`、`FiniteDifferenceSolver`、`MacroscopicProperties`、`Diagnostics` 逐文件核对签名。
3. 补 `LIFECYCLE_MATRIX.md` 的“待验证项”。
