#!/usr/bin/env python3
"""Check that each case's run directory holds output newer than the binary.

A fork-vs-official sweep is only evidence about the *current* build if every compared
case really ran with it.  The obvious check (mtime of the run directory) is wrong:
re-running a case overwrites files in place, which does not touch the directory entry,
so a directory can look days old while all its contents are fresh.  This script looks at
the newest file inside each case directory instead.

Usage:
  check_output_freshness.py --build audit_build/fork [--binary lib|bin/warpx.3d] [--json OUT]

Exit code 1 if any case has no output, or has output older than the binary.
"""
import argparse
import json
import os
import sys


def newest_file_mtime(root):
    newest = 0.0
    for dirpath, _dirs, files in os.walk(root):
        for f in files:
            try:
                newest = max(newest, os.path.getmtime(os.path.join(dirpath, f)))
            except OSError:
                pass
    return newest


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--build", required=True)
    ap.add_argument("--binary", default="bin/warpx.3d.MPI.OMP.DP.PDP.OPMD.FFT.EB.QED",
                    help="reference executable; its mtime is the build time")
    ap.add_argument("--json", default="")
    ap.add_argument("--tolerance", type=float, default=0.0,
                    help="seconds of slack between binary and outputs")
    args = ap.parse_args()

    binary = os.path.join(args.build, args.binary)
    if not os.path.isfile(binary):
        # Fall back to the newest executable in bin/ (the exact name depends on the
        # build options, e.g. .MPI.OMP... suffixes).
        cands = [os.path.join(args.build, "bin", f) for f in os.listdir(os.path.join(args.build, "bin"))
                 if f.startswith("warpx.") and os.access(os.path.join(args.build, "bin", f), os.X_OK)]
        if not cands:
            raise SystemExit(f"no warpx executable under {args.build}/bin")
        binary = max(cands, key=os.path.getmtime)
    build_time = os.path.getmtime(binary)

    cases = []
    base = os.path.join(args.build, "bin")
    for name in sorted(os.listdir(base)):
        d = os.path.join(base, name)
        if not os.path.isdir(d) or not name.startswith("test_"):
            continue
        t = newest_file_mtime(d)
        cases.append({"case": name, "newest": t,
                      "stale": t == 0.0 or t + args.tolerance < build_time,
                      "empty": t == 0.0})

    stale = [c["case"] for c in cases if c["stale"]]
    empty = [c["case"] for c in cases if c["empty"]]
    out = {"build": args.build, "binary": os.path.relpath(binary, args.build),
           "build_time": build_time, "cases": len(cases),
           "stale": len(stale), "empty": len(empty),
           "stale_cases": stale[:50], "empty_cases": empty[:50]}
    print(json.dumps({k: v for k, v in out.items() if not isinstance(v, list)}, indent=1))
    if args.json:
        json.dump({"summary": out, "cases": cases}, open(args.json, "w"), indent=1)
    return 0 if not stale else 1


if __name__ == "__main__":
    sys.exit(main())
