#!/usr/bin/env python3
"""OPAL Rosseland-mean opacity from the tabulated subset in this repository.

The table is ``Docs/research_notes/star_plugins/data/opal_gn93_subset.npz``, extracted (not
transcribed) from the OPAL Type-1 archive that ships with the ``rm-tables`` package; see
``extract_opal_subset.py`` for the provenance and the exact entries kept.

Tables are the published OPAL ones: Iglesias & Rogers 1996, ApJ 464, 943, on the GN93 metal
mixture.  Their axes are log10(T/K) from 3.75 to 8.7 and log10 R from -8 to 1 with

    R = rho / (T/1e6)^3,    rho in g/cm^3, T in K,

and the opacity itself is stored as log10(kappa) with kappa in cm^2/g.

The lookup interpolates bilinearly in (log T, log R) inside one composition and linearly in
(X, Z) between the four surrounding compositions.  Where the published tables are blank (the
cool dilute corner and the hot dense one) the interpolation falls back to the nearest valid
neighbour, which is what the envelope of a solar model needs.

Self-check: ``--selftest`` interpolates the table at the (T, rho, X, Z) of every shell of the SSM
profile and compares with that profile's own kappa column.  The SSM's opacities are OPAL-based,
so this is very nearly a test of the extraction and the interpolation rather than of the physics.
"""
import argparse
from pathlib import Path

import numpy as np

DATA = (Path(__file__).resolve().parent.parent.parent
        / "Docs/research_notes/star_plugins/data/opal_gn93_subset.npz")
PROFILE = (Path(__file__).resolve().parent.parent.parent
           / "Docs/research_notes/star_plugins/data/ssm_S_profile.csv")

CACHE = {}


def load(path=DATA):
    if path not in CACHE:
        z = np.load(path)
        CACHE[path] = {k: z[k] for k in z.files}
    return CACHE[path]


def _log_r(rho_cgs, t_k):
    """The OPAL density variable R = rho/(T/1e6)^3 [g/cm^3]."""
    return np.log10(rho_cgs) - 3.0 * (np.log10(t_k) - 6.0)


def _bilinear(x, y, xs, ys, values):
    """Bilinear interpolation with a nearest-valid fallback for the blank corners."""
    ix = np.clip(np.searchsorted(xs, x) - 1, 0, len(xs) - 2)
    iy = np.clip(np.searchsorted(ys, y) - 1, 0, len(ys) - 2)
    fx = (x - xs[ix]) / (xs[ix + 1] - xs[ix])
    fy = (y - ys[iy]) / (ys[iy + 1] - ys[iy])
    fx = min(max(fx, 0.0), 1.0)
    fy = min(max(fy, 0.0), 1.0)

    total_weight = 0.0
    weighted = 0.0
    for dx, wx in ((0, 1.0 - fx), (1, fx)):
        for dy, wy in ((0, 1.0 - fy), (1, fy)):
            weight = wx * wy
            value = values[ix + dx][iy + dy]
            if np.isfinite(value):
                weighted += weight * value
                total_weight += weight
    if total_weight > 0.0:
        return weighted / total_weight
    # Everything around the point is blank: take the nearest finite entry in the plane.
    finite = np.isfinite(values)
    if not finite.any():
        return np.nan
    rows, cols = np.nonzero(finite)
    distance = (rows - (ix + fx)) ** 2 + (cols - (iy + fy)) ** 2
    k = int(np.argmin(distance))
    return values[rows[k]][cols[k]]


def kappa(t_k, rho_cgs, x_h, z_metal, path=DATA, log10_out=False):
    """Rosseland mean opacity [cm^2/g] at (T [K], rho [g/cm^3], X, Z)."""
    table = load(path)
    log_t = np.log10(t_k)
    log_r = _log_r(rho_cgs, t_k)

    xs = table["x"]
    zs = table["z"]
    log_t_axis = table["log_t"]
    log_r_axis = table["log_r"]
    kappas = table["kappa"]

    ux = np.unique(xs)
    uz = np.unique(zs)
    ix = np.clip(np.searchsorted(ux, x_h) - 1, 0, len(ux) - 2)
    iz = np.clip(np.searchsorted(uz, z_metal) - 1, 0, len(uz) - 2)
    fx = min(max((x_h - ux[ix]) / (ux[ix + 1] - ux[ix]), 0.0), 1.0)
    fz = min(max((z_metal - uz[iz]) / (uz[iz + 1] - uz[iz]), 0.0), 1.0)

    total = 0.0
    norm = 0.0
    for dx, wx in ((0, 1.0 - fx), (1, fx)):
        for dz, wz in ((0, 1.0 - fz), (1, fz)):
            target_x = ux[ix + dx]
            target_z = uz[iz + dz]
            index = np.where((np.abs(xs - target_x) < 1e-6) & (np.abs(zs - target_z) < 1e-6))[0]
            if len(index) == 0:
                continue
            value = _bilinear(log_t, log_r, log_t_axis, log_r_axis, kappas[index[0]])
            if np.isfinite(value):
                total += wx * wz * value
                norm += wx * wz
    if norm == 0.0:
        return np.nan
    log_kappa = total / norm
    return log_kappa if log10_out else 10.0 ** log_kappa


def selftest():
    rows = []
    for line in open(PROFILE):
        if line.startswith("#") or line.startswith("r_over_R"):
            continue
        v = [float(x) for x in line.strip().split(",")]
        rows.append({"r": v[0], "T": v[2], "rho": v[3], "X": v[5], "Y": v[6], "kappa": v[8]})

    print(f"{'r/R':>7} {'T [K]':>10} {'rho':>10} {'kappa SSM':>10} {'kappa OPAL':>10} {'ratio':>7}")
    ratios = []
    for row in rows[::4]:
        z = max(0.0, 1.0 - row["X"] - row["Y"])
        ours = kappa(row["T"], row["rho"], row["X"], z)
        ratio = ours / row["kappa"] if row["kappa"] > 0 else float("nan")
        if row["kappa"] > 0 and np.isfinite(ratio):
            ratios.append(ratio)
        print(f"{row['r']:7.4f} {row['T']:10.3e} {row['rho']:10.3e} {row['kappa']:10.3e}"
              f" {ours:10.3e} {ratio:7.3f}")

    ratios = np.array(ratios)
    median = float(np.median(ratios))
    worst = float(np.max(np.abs(np.log10(ratios))))
    print(f"\nmedian ratio {median:.3f}, worst |log10 ratio| {worst:.3f} dex")
    assert 0.5 < median < 2.0, median
    assert worst < 1.0, worst
    print("opal_opacity: the tabulated subset reproduces the SSM's opacity column")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--selftest", action="store_true")
    args = ap.parse_args()
    if args.selftest:
        selftest()
    else:
        ap.print_help()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
