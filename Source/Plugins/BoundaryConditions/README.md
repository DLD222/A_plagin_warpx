# 边界条件插件（PEC / 电介质）

**独立性**：只 include 核心头文件（`Runtime/IBoundaryConditions.H`、`Runtime/IEBGeometry.H`），
不引用 `FieldSolver`、`PicEm/Modules/EmbeddedBoundary` 或任何其它插件的头文件。

## 当前实现范围（第一片）

| 接口 | 状态 |
|---|---|
| `hasEB` / `factory(lev)` 几何通路 | ✅ 通过核心的 `IEBGeometry`，已编译验证 |
| `ApplyPecToEfield` | 🟡 已算出去重后的 cut-cell 数量（几何通路验证）；**E 置零的 kernel 待接** |
| `ApplyPecToBfield` | ⬜ 待实现（B_n 连续投影） |
| `FillFacePermittivity` / `ApplyDielectricJump` | ⬜ 待实现（subpixel 平滑：面心 ε 按体积分数方向性平均） |
| 流体壁面条件（法向速度/热流/应力张量） | ⬜ 待实现（供五阶矩/十阶矩/恒星求解器用） |

## 编译

```bash
cmake -S . -B build -DWarpX_DIMS=2 -DWarpX_PLUGIN_BC=ON
```

单文件语法检查（不接主构建时也可验证）：

```bash
g++ -std=c++20 -fopenmp -fsyntax-only \
    $(DEFINES from build/CMakeFiles/warpx_picem_2d.dir/flags.make) \
    -I Source/Core -I Source/Plugins/BoundaryConditions \
    Source/Plugins/BoundaryConditions/PlasmaBoundary.cpp
```

## 待接入主构建

`CMakeLists.txt` 已写好目标，但**尚未**在主 `CMakeLists.txt` 里 `add_subdirectory`，
因为接入需要同时确定"哪个求解器在何时调用它"（核心侧要持有 `IBoundaryConditions*`，
由 Yee/CKC 的 `EvolveE` 在更新后调用）。这一步留到下一阶段，避免半接入导致构建/行为不一致。
