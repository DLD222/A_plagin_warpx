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
    far = dens[centres < xlo + 0.3 * (prob_hi[0] - xlo)]
    far_level = float(np.median(far)) if far.size else 0.0
    peak_i = int(np.argmax(dens))
    peak_x = float(centres[peak_i])
    ratio = float(dens[peak_i] / far_level) if far_level > 0 else float("inf")

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
        "density_peak": round(float(dens[peak_i]), 1),
        "density_compression": round(ratio, 2) if math.isfinite(ratio) else None,
        "stagnation_x_mm": round(peak_x * 1e3, 3),
        "standoff_mm": round((abs(peak_x) - radius) * 1e3, 3),
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
