#!/usr/bin/env python3
"""Extract a self-contained OPAL opacity subset from the rm-tables distribution.

The full OPAL Type-1 archive ships inside the ``rm-tables`` Python package (``opal_type1.npz``),
which in turn came from Arnold Boothroyd's public mirror at CITA.  The tables are *not*
transcribed here: they are copied out of that archive, and this script records exactly which
entries were taken so that the copy can be reproduced and audited.

Source of the numbers
---------------------
Iglesias & Rogers 1996, ApJ 464, 943 (the OPAL opacity tables themselves).
Archive: ``rm-tables`` 0.1.3, ``rm_tables/data/opal_type1.npz``, set ``GN93hz``
(Grevesse & Noels 1993 metal ratios; the package notes it differs from GS98 by a median
0.002 dex).
Axes: 70 points in log10(T/K) from 3.75 to 8.7, 19 points in log10 R from -8 to 1, where
R = rho/(T/1e6)^3 with rho in g/cm^3 and T in K; opacity in cm^2/g, stored as log10.

What is kept
------------
Only the compositions a solar model needs: hydrogen mass fractions 0.2 to 0.8 (which covers the
solar profile from the centre's 0.34 to the envelope's 0.74) and metal fractions 0.01 to 0.04
(Z_sun ~ 0.02), i.e. a fraction of the 126 tabulated compositions.  The result is a few hundred
kilobytes instead of the 23 MB archive.

Usage
-----
    python3 Tools/star/extract_opal_subset.py --archive /path/to/opal_type1.npz

The output lands in Docs/research_notes/star_plugins/data/opal_gn93_subset.npz.
"""
import argparse
from pathlib import Path

import numpy as np

DATA_DIR = (Path(__file__).resolve().parent.parent.parent
            / "Docs/research_notes/star_plugins/data")

# The subset kept: enough X to cover the solar profile, and Z around the solar value.
KEEP_X = (0.2, 0.35, 0.5, 0.7, 0.8)
KEEP_Z = (0.01, 0.02, 0.03, 0.04)

LOG_T_MIN, LOG_T_MAX, N_LOG_T = 3.75, 8.7, 70
LOG_R_MIN, LOG_R_MAX, N_LOG_R = -8.0, 1.0, 19


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--archive", required=True,
                    help="path to rm_tables/data/opal_type1.npz")
    ap.add_argument("--set", default="GN93hz")
    args = ap.parse_args()

    archive = np.load(args.archive)
    xs = archive[f"{args.set}::X"]
    zs = archive[f"{args.set}::Z"]
    kappa = archive[f"{args.set}::kappa"]

    # Collect the (X, Z) pairs that exist and are wanted.
    pairs = []
    for i, (x, z) in enumerate(zip(xs, zs)):
        for keep_x in KEEP_X:
            for keep_z in KEEP_Z:
                if abs(x - keep_x) < 1e-6 and abs(z - keep_z) < 1e-6:
                    pairs.append((i, keep_x, keep_z))

    if not pairs:
        raise SystemExit("no requested composition is present in the archive")

    # The archive stores the density axis in *descending* order (rm-tables sorts it itself in
    # axes()); reorder it here so that the file in this repository is self-consistent, with both
    # axes ascending and the opacity array matching.
    log_r_archive = np.asarray(archive["log_R"], dtype=np.float64)
    log_t_archive = np.asarray(archive["log_T"], dtype=np.float64)
    order = np.argsort(log_r_archive)

    kappas = np.stack([kappa[i] for i, _, _ in pairs]).astype(np.float32)[..., order]
    xs_out = np.array([p[1] for p in pairs], dtype=np.float64)
    zs_out = np.array([p[2] for p in pairs], dtype=np.float64)

    log_t = log_t_archive
    log_r = log_r_archive[order]

    assert log_t.shape == (N_LOG_T,), log_t.shape
    assert log_r.shape == (N_LOG_R,), log_r.shape
    assert np.all(np.diff(log_t) > 0.0) and np.all(np.diff(log_r) > 0.0)
    assert abs(log_t[0] - LOG_T_MIN) < 1e-6 and abs(log_t[-1] - LOG_T_MAX) < 1e-5
    assert abs(log_r[0] - LOG_R_MIN) < 1e-6 and abs(log_r[-1] - LOG_R_MAX) < 1e-5

    out = DATA_DIR / "opal_gn93_subset.npz"
    np.savez_compressed(out,
                        x=xs_out, z=zs_out, log_t=log_t, log_r=log_r, kappa=kappas)
    print(f"wrote {out}  ({out.stat().st_size / 1024:.0f} KiB)")
    print(f"  compositions: {len(pairs)} from set {args.set}")
    print(f"  X values: {sorted(set(xs_out))}")
    print(f"  Z values: {sorted(set(zs_out))}")
    print(f"  grid: {N_LOG_T} x {N_LOG_R}, kappa in log10(cm^2/g)")


if __name__ == "__main__":
    raise SystemExit(main())
