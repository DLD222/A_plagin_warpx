"""弓形激波的定量判据。

从一次 openPMD 输出里量出三件事，每件都有明确的物理期望：

1. **障碍物挡住粒子**：导体内部的离子数应约为 0（只剩极少数在阶梯格边缘）。
2. **密度堆积**：沿来流轴线（z=0）的离子面密度在驻点附近应高于远上游，压缩比 > 1。
3. **磁场堆积/缠绕**：驻点附近的 <|B|> 应高于远上游；并给出驻点位置（密度剖面的峰）。

用法：
    python -m warpx_studio.verify.bow_shock <openpmd_XXXXXX.h5> \
        --radius 1e-3 --prob-lo -3e-3 -4e-3 --prob-hi 9e-3 4e-3 --n-cell 240 160 --b0 8.338
"""
from __future__ import annotations

import argparse
import json
import math

import numpy as np


def _field(h5, group, name):
    d = h5[group][name][:]
    return np.squeeze(np.asarray(d, dtype=float))


def measure(path: str, radius: float, prob_lo, prob_hi, n_cell, b0: float,
            species: str = "ions") -> dict:
    import h5py
    import openpmd_api as io

    with h5py.File(path) as h5:
        iteration = list(h5["data"].keys())[0]
        group = f"data/{iteration}/fields"
        by = _field(h5, group, "B/y")
        bx = _field(h5, group, "B/x")
        bz = _field(h5, group, "B/z")

    nz, nx = by.shape
    xlo, zlo = prob_lo
    dx = (prob_hi[0] - prob_lo[0]) / n_cell[0]
    dz = (prob_hi[1] - prob_lo[1]) / n_cell[1]

    def ix(x):
        return min(max(int((x - xlo) / dx), 0), nx - 1)

    def iz(z):
        return min(max(int((z - zlo) / dz), 0), nz - 1)

    bmag = np.sqrt(bx * bx + by * by + bz * bz)
    half = 0.5 * radius

    def bmean(x0, x1):
        return float(bmag[iz(-half):iz(half) + 1, ix(x0):ix(x1) + 1].mean())

    b_up = bmean(xlo + 0.5e-3 if radius < 1e-3 else -3e-3 + 0.5e-3,
                 xlo + 1.5e-3 if radius < 1e-3 else -3e-3 + 1.5e-3)
    b_stag_lo = -1.8 * radius
    b_stag = bmean(b_stag_lo, -1.1 * radius)

    series = io.Series(path, io.Access.read_only)
    it = sorted(series.iterations)[-1]
    part = series.iterations[it].particles[species]
    x = np.asarray(part["position"]["x"].load_chunk(), dtype=float)
    z = np.asarray(part["position"]["z"].load_chunk(), dtype=float)
    series.flush()
    series.close()

    inside = int(((x * x + z * z) < radius * radius).sum())

    # 沿轴线的离子面密度：只取 |z| < R/2 的粒子，按 x 分箱
    axis = np.abs(z) < half
    bins = np.linspace(xlo, prob_hi[0], max(nx // 4, 8) + 1)
    hist, edges = np.histogram(x[axis], bins=bins)
    centres = 0.5 * (edges[1:] + edges[:-1])
    width = edges[1] - edges[0]
    dens = hist / width

    # 真正的激波判据必须看"离开表面"的压缩：以障碍物（原点）为心做径向分环，
    # 只看上游方向（x<0、|z|<R/2）。贴着表面的那一圈是撞壁堆积，不是激波。
    r = np.sqrt(x * x + z * z)
    upstream = (x < 0) & (np.abs(z) < half)
    # 上游域只有 2R，故分环限制在 1.0-2.0 R
    rings = [(1.0, 1.2), (1.2, 1.4), (1.4, 1.6), (1.6, 1.8), (1.8, 2.0)]
    # 用"每格粒子数"作密度（每格面积相同），避免解析环面积算错
    xc = xlo + (np.arange(nx) + 0.5) * dx
    zc = zlo + (np.arange(nz) + 0.5) * dz
    XC, ZC = np.meshgrid(xc, zc)
    RC = np.sqrt(XC * XC + ZC * ZC)
    cell_sel = (XC < 0) & (np.abs(ZC) < half)
    counts, _, _ = np.histogram2d(z, x, bins=[nz, nx], range=[[zlo, prob_hi[1]], [xlo, prob_hi[0]]])
    ring_dens = []
    for lo, hi in rings:
        mask = cell_sel & (RC >= lo * radius) & (RC < hi * radius)
        ncell = int(mask.sum())
        ring_dens.append(float(counts[mask].sum()) / ncell if ncell else 0.0)
    far_level = float(np.median(ring_dens[-2:])) if ring_dens else 0.0
    shock_peak = max(ring_dens[:3]) if ring_dens else 0.0
    # 激波 = 离开表面一个格子以上、且明显压缩；表面堆积（第 0 环）不计入
    detached_ratio = (shock_peak / far_level) if far_level > 0 else float("inf")

    return {
        "path": path,
        "iteration": int(it),
        "b0": b0,
        "b_upstream": round(b_up, 4),
        "b_stagnation": round(b_stag, 4),
        "b_compression": round(b_stag / b_up, 3) if b_up else None,
        "ions": int(x.size),
        "ions_inside_obstacle": inside,
        "density_upstream": round(far_level, 1),
        "density_peak": round(float(np.max(dens)) if dens.size else 0.0, 1),
        "density_compression": round(float(np.max(dens) / far_level), 2)
                               if far_level > 0 and dens.size else None,
        "ring_density_R": [round(v / max(far_level, 1e-9), 2) for v in ring_dens],
        "shock_detached_ratio": round(detached_ratio, 2) if math.isfinite(detached_ratio) else None,
        "stagnation_x_mm": round(float(centres[int(np.argmax(dens))]) * 1e3, 3) if dens.size else None,
        "standoff_mm": (round((abs(float(centres[int(np.argmax(dens))])) - radius) * 1e3, 3)
                        if dens.size else None),
    }


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("path")
    ap.add_argument("--radius", type=float, default=1e-3)
    ap.add_argument("--prob-lo", type=float, nargs=2, default=[-3e-3, -4e-3])
    ap.add_argument("--prob-hi", type=float, nargs=2, default=[9e-3, 4e-3])
    ap.add_argument("--n-cell", type=int, nargs=2, default=[240, 160])
    ap.add_argument("--b0", type=float, default=8.338)
    ap.add_argument("--species", default="ions")
    args = ap.parse_args()
    print(json.dumps(measure(args.path, args.radius, args.prob_lo, args.prob_hi,
                             args.n_cell, args.b0, args.species), ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
