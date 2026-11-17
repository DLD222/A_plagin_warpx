"""跨 EB 面法向磁场的连续性测量。

**为什么不是 ∇·B**：Yee 格式离散守恒，∇·B≈0 无论如何都成立，证明不了边界条件。
真正要测的是：在 EB 表面（导体面）处，磁场的法向分量 `B·n` 是否连续、是否满足
理想导体应有的行为（磁通冻结：B_n 内侧 = B_n 外侧；若导体内 B=0，则外侧 B_n 必须为 0）。

做法：在导体面 `r = r0` 的两侧（`r0 + 1.5Δ` 与 `r0 + 2.5Δ`）做双线性插值，沿边界法向
`n = (x, z)/r` 取 `B_n = B·n`，同时取切向 `B_t` 与电场 `E_t` 作对照。

    python -m warpx_studio.verify.bn <openpmd_*.h5> --radius 0.25e-3 \
        --prob-lo -1e-3 -2e-3 --prob-hi 5e-3 2e-3 --n-cell 120 80

输出 JSON：`{max_bn_over_b0, mean_bn_over_b0, max_et_over_e, ...}`，供 selftest / 报告引用。
"""
from __future__ import annotations

import argparse
import json
import math


def _read_fields(path: str) -> dict:
    import h5py
    import numpy as np

    with h5py.File(path) as handle:
        iterations = list(handle.keys())
        data_group = handle[iterations[0]]
        iteration = sorted(int(k) for k in data_group.keys())[-1]
        fields = data_group[str(iteration)]["fields"]

        def pick(name: str):
            group, _, comp = name.partition("/")
            node = fields[group]
            if isinstance(node, h5py.Dataset):
                return node[:]
            return node[comp][:]

        # 2D runs still carry the out-of-plane component (B_y / E_y), and for the
        # B-parallel-to-axis case that is the *tangential* one — it must be included.
        result = {name: np.asarray(pick(name))
                  for name in ("B/x", "B/y", "B/z", "E/x", "E/y", "E/z")}
    result["iteration"] = iteration
    return result


def measure(path: str, radius: float, prob_lo, prob_hi, n_cell,
            offsets=(1.5, 2.5), angles=range(0, 360, 10), b0: float | None = None) -> dict:
    import numpy as np

    fields = _read_fields(path)
    bx, by, bz = fields["B/x"], fields["B/y"], fields["B/z"]
    ex, ey, ez = fields["E/x"], fields["E/y"], fields["E/z"]
    nz, nx = bx.shape
    dx = (prob_hi[0] - prob_lo[0]) / n_cell[0]
    dz = (prob_hi[1] - prob_lo[1]) / n_cell[1]

    def interp(arr, x, z):
        fi = (x - prob_lo[0]) / dx - 0.5
        fj = (z - prob_lo[1]) / dz - 0.5
        i = int(min(max(math.floor(fi), 0), nx - 2))
        j = int(min(max(math.floor(fj), 0), nz - 2))
        a, b = fi - i, fj - j
        return (arr[j, i] * (1 - a) * (1 - b) + arr[j, i + 1] * a * (1 - b)
                + arr[j + 1, i] * (1 - a) * b + arr[j + 1, i + 1] * a * b)

    bn, bt, et, efull = [], [], [], []
    for offset in offsets:
        r = radius + offset * dx
        for deg in angles:
            theta = math.radians(deg)
            nxv, nzv = math.cos(theta), math.sin(theta)
            x, z = r * nxv, r * nzv
            bxv, byv, bzv = interp(bx, x, z), interp(by, x, z), interp(bz, x, z)
            exv, eyv, ezv = interp(ex, x, z), interp(ey, x, z), interp(ez, x, z)
            bn_normal = bxv * nxv + bzv * nzv          # n has no y component in 2D
            bn.append(bn_normal)
            bt.append(math.sqrt((bxv - bn_normal * nxv) ** 2 + byv * byv
                                + (bzv - bn_normal * nzv) ** 2))
            en = exv * nxv + ezv * nzv
            et.append(math.sqrt((exv - en * nxv) ** 2 + eyv * eyv
                                + (ezv - en * nzv) ** 2))
            efull.append(math.sqrt(exv * exv + eyv * eyv + ezv * ezv))

    scale = b0 if b0 else max(abs(v) for v in bn) or 1.0
    return {
        "iteration": fields["iteration"],
        "b0": scale,
        "max_bn_over_b0": max(abs(v) for v in bn) / scale,
        "mean_bn_over_b0": sum(abs(v) for v in bn) / len(bn) / scale,
        "max_bn": max(abs(v) for v in bn),
        "max_bt": max(bt),
        "max_et": max(et),
        "max_e": max(efull),
        "max_et_over_e": max(et) / (max(efull) or 1.0),
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("path")
    parser.add_argument("--radius", type=float, required=True)
    parser.add_argument("--prob-lo", type=float, nargs=2, required=True)
    parser.add_argument("--prob-hi", type=float, nargs=2, required=True)
    parser.add_argument("--n-cell", type=int, nargs=2, required=True)
    parser.add_argument("--b0", type=float, default=None)
    args = parser.parse_args()
    out = measure(args.path, args.radius, args.prob_lo, args.prob_hi, args.n_cell, b0=args.b0)
    print(json.dumps(out, ensure_ascii=False, indent=1))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
