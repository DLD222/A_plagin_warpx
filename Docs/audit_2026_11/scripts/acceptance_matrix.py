#!/usr/bin/env python3
"""Classify the acceptance set: which fork/official cases can be compared by output.

The acceptance criterion is "the ~400 bundled test cases produce the same output as
official WarpX 26.11".  Not every case can support that claim the same way, so this
script makes the classification explicit instead of hiding it in a percentage:

  comparable    : the case wrote output files in both builds -> compare_case can judge it
  exit-only     : neither side wrote any file (e.g. test_2d_pass_mpi_comm_picmi only
                  checks that a Python script can hand an MPI communicator over); the only
                  evidence is the exit status, and it must be reported as such
  one-sided     : output exists in one build only -> not comparable, a real red flag
  not-run       : the case has no run directory in one of the builds

Usage:
  acceptance_matrix.py --fork audit_build/fork --official audit_build/official_2611
                       [--progress audit_runs/final_incremental/cases.csv] [--json OUT]
"""
import argparse
import csv
import json
import os
import subprocess
import sys


def run_tests(build):
    out = subprocess.run(["ctest", "--test-dir", build, "--show-only=json-v1"],
                         capture_output=True, text=True)
    if out.returncode != 0:
        raise SystemExit(f"ctest --show-only failed for {build}: {out.stderr[:300]}")
    return {t["name"][:-4] for t in json.loads(out.stdout).get("tests", [])
            if t["name"].endswith(".run")}


def count_outputs(build, name):
    d = os.path.join(build, "bin", name)
    if not os.path.isdir(d):
        return None
    n = 0
    for _root, _dirs, files in os.walk(d):
        # warpx_used_inputs is written by every run, including one that dies immediately;
        # it is not evidence that the case produced a result.
        n += sum(1 for f in files if f != "warpx_used_inputs")
    return n


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--fork", required=True)
    ap.add_argument("--official", required=True)
    ap.add_argument("--progress", default="")
    ap.add_argument("--json", default="")
    args = ap.parse_args()

    fk, of = run_tests(args.fork), run_tests(args.official)
    shared = sorted(fk & of)
    rows = []
    for name in shared:
        nf = count_outputs(args.fork, name)
        no = count_outputs(args.official, name)
        if nf is None or no is None:
            kind = "not-run"
        elif nf == 0 and no == 0:
            kind = "exit-only"
        elif nf == 0 or no == 0:
            kind = "one-sided"
        else:
            kind = "comparable"
        rows.append({"test": name, "fork_files": nf, "official_files": no, "kind": kind})

    compared = set()
    if args.progress and os.path.isfile(args.progress):
        compared = {r["test"] for r in csv.DictReader(open(args.progress))}

    counts = {}
    for r in rows:
        counts[r["kind"]] = counts.get(r["kind"], 0) + 1
    summary = {
        "fork_tests": len(fk), "official_tests": len(of),
        "fork_only": sorted(fk - of), "official_only": sorted(of - fk),
        "shared": len(shared), "kinds": counts,
        "compared": len(compared & set(shared)),
        "remaining": len(set(shared) - compared),
        "not_comparable": [r["test"] for r in rows if r["kind"] != "comparable"],
    }
    print(json.dumps(summary, indent=1))
    if args.json:
        json.dump({"summary": summary, "rows": rows}, open(args.json, "w"), indent=1)
    return 0


if __name__ == "__main__":
    sys.exit(main())
