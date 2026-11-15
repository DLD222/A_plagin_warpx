#!/usr/bin/env python3
"""Compare one RZ plotfile (written with dump_rz_modes=1) between two run
directories, and break the difference down by radial row (i) and axial
column (j).

Usage:
    rz_row_diff.py REF_PLOTFILE_DIR NEW_PLOTFILE_DIR [--vars a,b,c] [--top N]

The plotfile is read with yt, so this works for any grid/box decomposition.
yt exposes the RZ mode fields as boxlib fields with their plain names
(e.g. `rho_0_real`).
"""
import argparse
import sys

import numpy as np
import yt


def load_fields(path, wanted):
    ds = yt.load(path)
    grid = ds.covering_grid(
        level=ds.max_level,
        left_edge=ds.domain_left_edge,
        dims=ds.domain_dimensions,
    )
    out = {}
    available = {str(f[1]) for f in ds.field_list}
    for name in wanted:
        if name not in available:
            raise SystemExit(f"field {name} not in {sorted(available)}")
        out[name] = np.asarray(grid["boxlib", name])
    return ds, out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("ref")
    ap.add_argument("new")
    ap.add_argument("--vars", default="rho_0_real,rho_1_real,rho_1_imag")
    ap.add_argument("--top", type=int, default=8)
    ap.add_argument("--rows", default="")
    a = ap.parse_args()
    wanted = [v.strip() for v in a.vars.split(",") if v.strip()]

    ds_r, d_r = load_fields(a.ref, wanted)
    ds_n, d_n = load_fields(a.new, wanted)
    ncell = ds_r.domain_dimensions
    dx = ds_r.domain_width / ncell
    lo = ds_r.domain_left_edge

    print(f"# ref={a.ref} new={a.new}")
    print(f"# dimensions={list(ncell)} dx={[float(x) for x in dx]}")
    print(f"{'var':13s} {'ref_max':>11s} {'new_max':>11s} {'maxdiff':>11s} "
          f"{'ndiff':>7s}  rows")
    rows_all = set()
    for v in wanted:
        A = d_r[v]
        B = d_n[v]
        diff = np.abs(A - B)
        rows = np.where(diff.any(axis=1))[0]
        rows_all |= set(rows.tolist())
        rowstr = " ".join(str(r) for r in rows[:a.top])
        if len(rows) > a.top:
            rowstr += "..."
        print(f"{v:13s} {np.abs(A).max():11.4e} {np.abs(B).max():11.4e} "
              f"{diff.max():11.4e} {int((diff > 0).sum()):7d}  {rowstr}")

    for v in wanted:
        A = d_r[v]
        B = d_n[v]
        hits = np.where(np.abs(A - B).any(axis=1))[0]
        if hits.size == 0:
            continue
        print(f"\n### {v}")
        sel = hits
        if a.rows:
            keep = {int(x) for x in a.rows.split(",")}
            sel = np.array([r for r in hits if int(r) in keep], dtype=int)
        for r in sel:
            d = np.abs(A[r] - B[r])
            jmax = int(np.argmax(d))
            print(f"  row {r:3d} (r={float(lo[0] + (r + 0.5) * dx[0]):.4e}) "
                  f"diffmax={d.max():.4e} at j={jmax} "
                  f"(z={float(lo[1] + (jmax + 0.5) * dx[1]):.4e}) "
                  f"ref={float(A[r, jmax].squeeze()):.6e} new={float(B[r, jmax].squeeze()):.6e} "
                  f"cells={int((d > 0).sum())}")


if __name__ == "__main__":
    sys.exit(main())
