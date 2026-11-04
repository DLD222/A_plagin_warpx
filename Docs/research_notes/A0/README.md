# A0 源码审计：类目录 / 公共接口 / 生命周期

<!-- SPDX-License-Identifier: BSD-3-Clause -->

> 状态：**第一遍 + 第二遍核验完成**（2026-11-04）。371 头文件 / 348 类 / 2,408 公共方法；12 子系统矩阵；5 个待验证项已处理，并新增 4 个 `RemakeLevel` 缺口。详见 `VERIFICATION.md`。

本目录是 [`../SOURCE_AUDIT.md`](../SOURCE_AUDIT.md) 的 A0 执行产物，也是 [`../最终的技术路线.md`](../最终的技术路线.md) 的前置输入。

| 文件 | 内容 |
|---|---|
| [`HEADER_CATALOG.md`](HEADER_CATALOG.md) | 371 个头文件的类目录：348 个类/结构体、2,408 个公共方法、按子系统索引 |
| [`api/`](api/) | 逐子系统完整公共签名（16 个文件） |
| [`classes.csv`](classes.csv) | 机器可读：`subsystem,file,kind,class,bases,n_public_methods,...` |
| [`COVERAGE.md`](COVERAGE.md) | 371 个头文件的覆盖表；136 个无 class 的头文件清单 |
| [`LIFECYCLE_MATRIX.md`](LIFECYCLE_MATRIX.md) | 12 个子系统的“所有者/创建/初始化/每步/regrid/restart/归属”矩阵 |
| [`VERIFICATION.md`](VERIFICATION.md) | 第二遍核验：PSATD restart、RemakeLevel 缺口、诊断时序、Python callback、EB per-level；构建/冒烟基线 |

## 解析方法与限制

- 解析器：`Tools/A0/parse_headers.py`（可复现；`python3 Tools/A0/parse_headers.py` 输出 `/tmp/header_catalog.json`）。
- 启发式：按 `class/struct` + 花括号匹配提取公共段；跳过 `template<...>` 前缀、内联函数体、预处理器行。
- 已知限制：模板参数/默认值/宏展开可能被简化；嵌套类可能遗漏；`#if` 分支被合并。精确签名以头文件为准。
- 覆盖判据：235/371 头文件解析出类；其余 136 个需人工确认是否纯 free-function/kernel（`COVERAGE.md` 已列全）。

## 下一步（A0 第二遍）

1. 人工确认 136 个“无 class”头文件（区分 kernel/free function/解析遗漏）。
2. 对 `WarpX`、`MultiParticleContainer`、`FiniteDifferenceSolver`、`MacroscopicProperties`、`Diagnostics` 逐文件核对签名。
3. 补 `LIFECYCLE_MATRIX.md` 的“待验证项”。
