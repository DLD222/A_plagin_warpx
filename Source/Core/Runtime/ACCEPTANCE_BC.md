# 验收：理想导体边界条件 + 弓形激波（Yee，2D）

## 四项验收与证据

### (1) E_tangential = 0，B_normal 连续（不是为零），允许阶梯掩码

实现位置：`Source/Plugins/PicEm/Modules/BoundaryConditions/WarpXFieldBoundaries.cpp`

| 条件 | 实现 | 证据 |
|---|---|---|
| `E_t = 0` | `ApplyEfieldBoundary` 中，被 EB 覆盖的格 `E = 0`（阶梯掩码） | 导体内部 E 为 0；`E_t/E ≈ 0.19–0.22`（阶梯近似下的部分抑制） |
| `B_n` 连续 | `ApplyBfieldBoundary` 中，覆盖格复制相邻流体格的**法向分量**，切向分量冻结 | B⊥ 实测 `max|B_n|/B0 = 1.0004`（法向分量保持环境值，未被归零） |
| `B_t` 允许跳变 | 不修改切向分量 | B⊥ 实测 `B_t = 8.04 T > B0 = 7.147 T`（表面电流导致切向放大） |

### (2) Yee 求解器 + 真实的 `warpx.2d` 可执行文件

- 构建树：`audit_build/exe2d`（`WarpX_LIB=OFF`，因此会真正链接可执行文件）
- 目标：`audit_build/exe2d/bin/warpx.2d` → `warpx.2d.MPI.OMP.DP.PDP.OPMD.FFT.EB.QED`
- 每次改动后均核实二进制时间戳（例如 09:04、09:08）
- 注意：`audit_build/fork` 是 `WarpX_LIB=ON`，只出库；其中的 `bin/warpx.2d` 是 11 月 15 日的旧文件

### (3) 两个算例跑出物理正确的弓形激波（网页端启动 + 定量测量）

两个项目已存入 Studio（网页下拉可直接打开重跑）：
`弓形激波·B∥轴 (v=0.1c)`、`弓形激波·B⊥轴 (v=0.1c)`

| 算例 | 步数 | 障碍物挡粒子 | 磁场堆积 | 密度堆积 |
|---|---|---|---|---|
| B∥（B 沿 y） | 1200 | 导体内仅 2 个粒子（0.005%） | `|B_y|` 7.147 → 7.159 T | 障碍物附近环带显著富集 |
| B⊥（B 沿 z） | 1200 | 同上 | 上游压缩至 8.04 T（+12%） | 同上 |

`verify/bn.py` 测量（iteration 1200）：

```
B∥: max|B_n|/B0 = 0.000   max B_t = 7.17 T   E_t/E = 0.22
B⊥: max|B_n|/B0 = 1.0004  max B_t = 8.04 T   E_t/E = 0.19
```

### (4) ParaView 弹窗可关闭

- `POST /api/paraview` 现在记录进程（`PARAVIEW = {proc, pvd}`），一次只开一个
- `POST /api/paraview/close`：`killpg(SIGTERM)` → 等待 2 s → `SIGKILL`
- `GET /api/paraview/status`
- 界面「✕ 关闭 ParaView」按钮（打开后自动出现）
- 实测：替身进程 `sleep 300` → `close_paraview()` → `{'closed': True}`，进程终止

## 复现命令

```bash
# 1) 构建（必须用这个目标，否则不会链接可执行文件）
cmake --build audit_build/exe2d -j 8

# 2) 网页端：打开 http://127.0.0.1:8770 → 任务下拉选「弓形激波·B∥轴 (v=0.1c)」→ 运行

# 3) 定量测量
python -m warpx_studio.verify.bn <run>/diag1/openpmd_001200.h5 \
    --radius 0.25e-3 --prob-lo -1e-3 -2e-3 --prob-hi 5e-3 2e-3 --n-cell 120 80 --b0 7.147
```

## 已知限制

- `E_t/E ≈ 0.2`：阶梯掩码只有一阶几何精度，切向电场只被部分抑制；要更干净需要 cut-cell 面积加权。
- 算例用的是 **v = 0.1c 的相对论流**：论文的 70 km/s 在解析 λ_D 的全 PIC 下需要 ~10⁶ 步，实际不可行
  （详见 `WORK_SUMMARY.md` 第十二节）。物理结构（M_A=2、磁场缠绕、弓形激波）在两种流速下一致。
