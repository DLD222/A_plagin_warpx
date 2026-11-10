#!/usr/bin/env python3
"""Compare the particle output of two openPMD series, order-independently.

The field comparison in Tools/verify cannot read openPMD, and several QED cases write
particles only (no meshes at all).  This script fills that gap: it reads the last iteration
of both series with openPMD's own dataframe view and, for every species, compares the
particle count and - component by component - the *sorted* positions, momenta and weights.
Sorting makes the comparison independent of the order the particles happen to be stored in,
which is all that can be asked of a particle dump.

It answers "are these the same particles, up to ordering", not "are they bit-identical".

Usage:
    compare_particles_openpmd.py <dir_a> <dir_b> [--series-pattern openpmd_%T.h5]
                                 [--rtol 1e-10]

Both arguments are the openPMD output directories (e.g. <run>/diags/diag1).
Exit code 0 when every species agrees within the tolerance.
"""
import argparse
import glob
import sys

import numpy as np
import openpmd_api as io

# Columns worth comparing; a species may carry a subset.
COLUMNS = (
    "position_x", "position_y", "position_z",
    "momentum_x", "momentum_y", "momentum_z",
    "weighting",
)


def load_last(directory, pattern):
    files = sorted(glob.glob(f"{directory}/{pattern.replace('%T', '*')}"))
    if not files:
        raise SystemExit(f"no openPMD files under {directory} matching {pattern}")
    series = io.Series(f"{directory}/{pattern}", io.Access.read_only)
    last = list(series.iterations)[-1]
    iteration = series.iterations[last]
    species = {}
    for name in iteration.particles:
        df = iteration.particles[name].to_df()
        species[name] = df
    return last, species


def compare_species(name, left, right, rtol):
    lines = []
    ok = True
    if len(left) != len(right):
        return False, [f"  {name}: particle count {len(left)} vs {len(right)}"]
    for column in COLUMNS:
        if column not in left or column not in right:
            continue
        a = np.sort(np.asarray(left[column], dtype=np.float64))
        b = np.sort(np.asarray(right[column], dtype=np.float64))
        if a.size == 0 or b.size == 0:
            # a species can legitimately be empty in one dump (it was created later)
            if a.size != b.size:
                return False, [f"  {column}: {a.size} vs {b.size} entries"]
            continue
        # normalise by the spread of the values themselves, not by their magnitude:
        # momenta around 1e-22 in SI would make a magnitude-relative test meaningless
        scale = max(np.max(np.abs(a)) - np.min(np.abs(a)), np.max(np.abs(a)), 1e-300)
        diff = float(np.max(np.abs(a - b)))
        rel = diff / scale
        if rel > rtol:
            ok = False
            lines.append(f"  {column}: max|diff|={diff:.3e} rel={rel:.2e}")
    return ok, lines


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("dir_a")
    parser.add_argument("dir_b")
    parser.add_argument("--series-pattern", default="openpmd_%T.h5")
    parser.add_argument("--rtol", type=float, default=1e-10,
                        help="relative tolerance on the sorted values (default 1e-10)")
    args = parser.parse_args()

    iter_a, data_a = load_last(args.dir_a, args.series_pattern)
    iter_b, data_b = load_last(args.dir_b, args.series_pattern)
    if iter_a != iter_b:
        print(f"comparing different iterations: {iter_a} vs {iter_b}", file=sys.stderr)

    print(f"iteration {iter_a}: species {sorted(data_a)}")
    all_ok = True
    for name in sorted(set(data_a) | set(data_b)):
        if name not in data_a or name not in data_b:
            print(f"{name}: present in only one of the two runs")
            all_ok = False
            continue
        ok, lines = compare_species(name, data_a[name], data_b[name], args.rtol)
        count = len(data_a[name])
        print(f"{name} (N={count}): {'OK' if ok else 'DIFFERS'}")
        for line in lines:
            print(line)
        all_ok = all_ok and ok
    return 0 if all_ok else 1


if __name__ == "__main__":
    sys.exit(main())
