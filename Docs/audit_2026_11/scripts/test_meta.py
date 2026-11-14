#!/usr/bin/env python3
"""Build a metadata table for every registered test of a WarpX build tree.

Reads `ctest --show-only=json-v1` and extracts, per test:
  name, dims (from the name/command), nprocs, binary, input file(s), workdir, env.
Optionally cross-checks whether the input file differs between two source trees.

Usage: test_meta.py --build DIR --json DIR [--other-source DIR] [--out FILE.csv]
"""
import argparse
import csv
import json
import os
import re
import subprocess


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--build", required=True)
    ap.add_argument("--out", default="")
    ap.add_argument("--repo", default=os.path.expanduser("~/WarpX_plugin"))
    ap.add_argument("--official", default=os.path.expanduser("~/WarpX_plugin/_upstream/warpX-26.10"))
    args = ap.parse_args()
    out = subprocess.run(["ctest", "--test-dir", args.build, "--show-only=json-v1"],
                         capture_output=True, text=True, check=True)
    j = json.loads(out.stdout)
    rows = []
    for t in j.get("tests", []):
        name = t["name"]
        if not name.endswith(".run"):
            continue
        name = name[:-4]
        cmd = t.get("command", [])
        nprocs = ""
        for i, a in enumerate(cmd):
            if a in ("-np", "-n") and i + 1 < len(cmd):
                nprocs = cmd[i + 1]
        inp = ""
        for a in cmd:
            if isinstance(a, str) and re.search(r"(inputs|\.py$)", os.path.basename(a)):
                inp = a
                break
        dims = ""
        m = re.match(r"test_(1d|2d|3d|rz|rcylinder|rsphere)_", name)
        if m:
            dims = m.group(1)
        props = t.get("properties", [])
        env = ""
        for p in props:
            if p.get("name") == "ENVIRONMENT":
                env = p.get("value", "")
        rel = ""
        if inp:
            for root in (args.repo, args.official):
                cand = os.path.relpath(inp, root)
                if not cand.startswith(".."):
                    rel = cand
                    break
        differ = ""
        if rel:
            pa = os.path.join(args.repo, rel)
            pb = os.path.join(args.official, rel)
            if os.path.isfile(pa) and os.path.isfile(pb):
                differ = "no" if open(pa, "rb").read() == open(pb, "rb").read() else "yes"
            elif os.path.isfile(pa) and not os.path.isfile(pb):
                differ = "fork-only"
            elif os.path.isfile(pb) and not os.path.isfile(pa):
                differ = "official-only"
        rows.append({"test": name, "dims": dims, "nprocs": nprocs,
                     "input": os.path.basename(inp) if inp else "",
                     "input_rel": rel, "input_differs": differ,
                     "workdir": t.get("backtrace_graph", "") and "" or ""})
    if args.out:
        with open(args.out, "w", newline="") as fh:
            w = csv.DictWriter(fh, fieldnames=list(rows[0].keys()))
            w.writeheader()
            w.writerows(rows)
    print(f"{len(rows)} tests -> {args.out}")


if __name__ == "__main__":
    main()
