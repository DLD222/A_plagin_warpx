#!/usr/bin/env python3
"""Compare the final plotfile of two runs, field by field.

Companion to run_ab.sh: it walks every ``<case>/{up,mine}`` directory pair under the
current directory, loads the last plotfile of each side with yt and reports

    case  plotfile  fields  scale  max|diff|  max|diff|/scale

so a bit-identical run shows 0.000e+00 and a rounding-level one shows a value at the
double-precision floor.  For inputs that write particles only, use
Tools/verify/compare_particles_openpmd.py instead - this one needs meshes.

Usage:
    python3 Tools/verify/ab/compare_fields.py [--cases case1 case2 ...]
"""
import argparse
import glob
import os
import sys

import numpy as np
import yt

yt.funcs.mylog.setLevel(0)

# Relative tolerance for "the two builds agree".  Anything above this is a real difference;
# printing it and returning 0 (as this script used to) let a table of differences look like a
# successful comparison.
RTOL = 1e-12


def load(name, side):
    files = [f for f in sorted(glob.glob(f"{name}/{side}/diags/diag1*")) if not f.endswith(".old")]
    if not files:
        return None, None
    try:
        ds = yt.load(files[-1])
    except Exception:
        # an input can ask for openPMD output: it is a directory, not a plotfile, and yt
        # cannot read it.  Say so once and let the caller skip the case.
        return None, "openpmd"
    grid = ds.covering_grid(level=0, left_edge=ds.domain_left_edge, dims=ds.domain_dimensions)
    fields = {f: np.array(grid[f]) for f in ds.field_list if np.array(grid[f]).ndim > 0}
    return fields, os.path.basename(files[-1])


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--cases", nargs="*", default=None,
                        help="restrict to these case names (default: every *_/up directory)")
    args = parser.parse_args()

    names = args.cases or sorted(
        d for d in os.listdir(".") if os.path.isdir(f"{d}/up") and os.path.isdir(f"{d}/mine")
    )

    rows = []
    skipped_names = []
    for name in names:
        up, plotfile = load(name, "up")
        mine, _ = load(name, "mine")
        if plotfile == "openpmd" or mine == "openpmd":
            print(f"{name}: openPMD output, not comparable with yt - skipped")
            skipped_names.append(name)
            continue
        if not up or not mine:
            print(f"{name}: missing plotfile on one side, skipped")
            skipped_names.append(name)
            continue
        if not common:
            print(f"{name}: no common field to compare - skipped")
            skipped_names.append(name)
            continue
        common = sorted(common)
        scale = max(np.max(np.abs(v)) for v in up.values())
        dmax = max(np.max(np.abs(up[f] - mine[f])) for f in common)
        # scale == 0 means every reference field is identically zero: a relative error then says
        # nothing (and divided by zero), so fall back to an absolute tolerance.
        rel = dmax / scale if scale > 0 else dmax
        rows.append((name, plotfile, len(common), scale, dmax, rel))

    print(f"{'case':14} {'plotfile':14} {'fields':>6} {'scale':>10} {'max|diff|':>10} {'rel':>10}")
    failed = 0
    for name, plotfile, nfields, scale, dmax, rel in rows:
        verdict = "" if rel <= RTOL else "  FAIL"
        if rel > RTOL:
            failed += 1
        print(f"{name:14} {plotfile:14} {nfields:6d} {scale:10.3e} {dmax:10.3e} {rel:10.2e}{verdict}")
    print(f"compared {len(rows)} case(s), {len(skipped_names)} skipped, {failed} above rtol={RTOL:g}")
    if not rows:
        print("compare_fields.py: nothing was compared", file=sys.stderr)
        return 1
    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
