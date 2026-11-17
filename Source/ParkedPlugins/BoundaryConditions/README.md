# 边界条件插件（PEC / 电介质）

**独立性**：只 include 核心头文件（`Runtime/IBoundaryConditions.H`、`Runtime/IEBGeometry.H`），
不引用 `FieldSolver`、`PicEm/Modules/EmbeddedBoundary` 或任何其它插件的头文件。

## 当前实现范围（第一片）

| 接口 | 状态 |
|---|---|
| `hasEB` / `factory(lev)` 几何通路 | ✅ 通过核心的 `IEBGeometry`，已编译验证 |
| `ApplyPecToEfield` | ✅ **已实现**：遍历 cut cell，把该格三个 E 分量置零（阶梯型 `E_t=0`），并统计 cut-cell 数 |
| `ApplyPecToBfield` | ⬜ 待实现（B_n 连续投影） |
| `FillFacePermittivity` / `ApplyDielectricJump` | ⬜ 待实现（subpixel 平滑：面心 ε 按体积分数方向性平均） |
| 流体壁面条件（法向速度/热流/应力张量） | ⬜ 待实现（供五阶矩/十阶矩/恒星求解器用） |


## 编译（**注意：目前没有生效的构建开关**）

顶层的 `CMakeLists.txt` **没有** `WarpX_PLUGIN_BC` 选项，也没有把这个目录加进
`WARPX_MODULE_LAYERS`。因此下面的命令**不会启用任何东西**，本目录的代码**不会**被编进
`warpx.2d`：

```bash
# 无效：选项不存在，configure 会忽略它（只是不会报错）
cmake -S . -B build -DWarpX_DIMS=2 -DWarpX_PLUGIN_BC=ON
```

单文件语法检查仍然有效（用来防止代码腐烂）：

```bash
g++ -std=c++20 -fopenmp -fsyntax-only \
    $(grep '^CXX_DEFINES' build/CMakeFiles/warpx_picem_2d.dir/flags.make | cut -d= -f2-) \
    $(grep '^CXX_INCLUDES' build/CMakeFiles/warpx_picem_2d.dir/flags.make | cut -d= -f2-) \
    -I Source/Core -I Source/ParkedPlugins/BoundaryConditions \
    Source/ParkedPlugins/BoundaryConditions/PlasmaBoundary.cpp
```

## 实际生效的 PEC 在哪里

`Source/Plugins/PicEm/Modules/BoundaryConditions/WarpXFieldBoundaries.cpp`（PicEm 插件内部）：

- `ApplyEfieldBoundary`：EB 覆盖格 `E = 0`（阶梯掩码，`E_t = 0`）；
- `ApplyBfieldBoundary`：EB 覆盖格复制相邻流体格的**法向** B，切向 B 冻结（`B_n` 连续）。

核心接口 `RuntimeData::boundaryConditions()` / `IBoundaryConditions` 目前**没有消费者**：
求解器直接调用了 PicEm 里的实现，尚未通过接口。

## 要真正插件化还缺什么（2026-11-17 审查 N-05）

1. 顶层 `CMakeLists.txt` 允许**扫描** `Source/Plugins/*` 自动登记层，而不是硬编码
   `WARPX_MODULE_LAYERS`；
2. 自动生成 `_WARPX_PLUGIN_ANCHORS` 的 `-u,<anchor>`，不再要求手工添加；
3. 在 `WarpX::ApplyEfieldBoundary` / `ApplyBfieldBoundary` 里引用核心接口指针
   （`RuntimeData::boundaryConditions()`），由插件注册表提供实现；
4. 处理静态库链接期注册被丢弃的问题（`--whole-archive` 或显式初始化函数）。
