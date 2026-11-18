"""用 yt 渲染 openPMD 输出（替代 ParaView/pvpython）。

只依赖 h5py + yt：
  * 二维 XZ 数据（二维下 z 是第二维，y 是"纸面外"）→ 在 y 方向做 SlicePlot，得到正视平面图
  * 颜色连续：yt 默认对数据做插值渲染
  * 可选对数色标（密度类跨量级时必需）
"""
from __future__ import annotations

import glob
import os
from typing import Any

os.environ.setdefault("YT_DISABLE_STREAM_LOGGING", "1")
os.environ.setdefault("MPLBACKEND", "Agg")

import h5py          # noqa: E402
import numpy as np   # noqa: E402


def _field_path(group: h5py.Group, field: str) -> str | None:
    """openPMD 把矢量存成组 B/x,B/y..., 标量存成数据集 rho 等。"""
    if field in group:
        return field
    return None


def iterations_in(path: str) -> list[int]:
    if os.path.isdir(path):
        files = glob.glob(os.path.join(path, "openpmd_*.h5"))
    else:
        files = [path]
    out = []
    for f in files:
        base = os.path.basename(f)
        try:
            out.append(int(base.split("_")[-1].split(".")[0]))
        except ValueError:
            continue
    return sorted(out)


def render(iteration_file: str, out_png: str, field: str, component: str | None = None,
           log: bool = False, width: int = 1200, height: int = 800,
           title: str | None = None) -> dict[str, Any]:
    import yt

    with h5py.File(iteration_file) as h5:
        it = list(h5["data"].keys())[0]
        base = f"data/{it}"
        grp = h5[f"{base}/fields"]

        if field in grp and isinstance(grp[field], h5py.Dataset):
            node = grp[field]
            data = np.squeeze(np.array(node))
            attrs = node.attrs
            label = field
        else:
            g = grp[field]
            comp = component or ("y" if "y" in g else sorted(g.keys())[0])
            data = np.squeeze(np.array(g[comp]))
            attrs = g.attrs          # 矢量场的格点信息在分量组上，不在分量数据集上
            label = f"{field}_{comp}"

        unit = float(attrs.get("gridUnitSI", 1.0))
        spacing = np.array(attrs.get("gridSpacing", [1.0, 1.0]), dtype=float) * unit
        offset = np.array(attrs.get("gridGlobalOffset", [0.0, 0.0]), dtype=float) * unit

    data = np.asarray(data, dtype=float)
    if data.ndim != 2:
        raise RuntimeError(f"只支持二维输出，得到 {data.ndim} 维")
    # openPMD 是 [z, x] 的顺序
    nz, nx = data.shape                      # openPMD 顺序是 (z, x)
    dz, dx = float(spacing[0]), float(spacing[1])
    z0, x0 = float(offset[0]), float(offset[1])
    bbox = np.array([[x0, x0 + nx * dx],
                     [0.0, max(dx, 1e-9)],        # y 方向一层厚度，否则切片/投影为空
                     [z0, z0 + nz * dz]])
    arr = data.T[:, None, :]                      # yt 需要三维形状 (nx, 1, nz)
    dims = (arr.shape[0], 1, arr.shape[2])
    ds = yt.load_uniform_grid({"d": arr}, dims,
                              length_unit="m",
                              bbox=bbox,
                              periodicity=(False, False, False),
                              nprocs=1)
    # 网格在 y 方向只有一层，切片会切空；用"沿 y 的投影"得到正视平面图
    sl = yt.ProjectionPlot(ds, "y", ("stream", "d"), method="sum", window_size=(4, 3))
    sl.set_log(("stream", "d"), log)
    sl.set_cmap(("stream", "d"), "turbo")
    sl.annotate_title(title or f"{label}  iter {it}")
    # 不画坐标轴，保持纯图
    sl.set_axes_unit("mm")
    sl.save(out_png)
    finite = data[np.isfinite(data)]
    return {"ok": True, "out": out_png, "iteration": it, "field": label,
            "range": [float(finite.min()), float(finite.max())],
            "shape": [int(nx), int(nz)]}


if __name__ == "__main__":
    import sys
    f = sys.argv[1]
    print(render(f, sys.argv[2], sys.argv[3], sys.argv[4] if len(sys.argv) > 4 else None))
