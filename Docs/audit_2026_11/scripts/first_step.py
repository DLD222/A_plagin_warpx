#!/usr/bin/env python3
"""For each mismatching case, compare the FIRST (earliest) diagnostic step of the
fork and official runs.  If the very first output already differs, the cause is in
initialisation / the diagnostic itself; otherwise the dynamics diverge later.

Usage: first_step.py --a DIR --b DIR --names FILE --out CSV
"""
import argparse
import csv
import glob
import json
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import compare_case as cc  # noqa: E402


def first_plotfile(d):
    best = None
    for dp, dn, fn in os.walk(d):
        if "Header" in fn and any(x.startswith("Level_") for x in dn):
            r = os.path.relpath(dp, d)
            name = os.path.basename(dp)
            import re
            m = re.search(r"(\d+)$", name) or re.search(r"(\d+)$", os.path.basename(os.path.dirname(dp)))
            step = int(m.group(1)) if m else 0
            if best is None or step < best[0]:
                best = (step, r)
    return best


def first_h5(d):
    files = sorted(glob.glob(os.path.join(d, "**", "*.h5"), recursive=True))
    if not files:
        return None
    return files[0]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--a", required=True)
    ap.add_argument("--b", required=True)
    ap.add_argument("--names", required=True)
    ap.add_argument("--out", required=True)
    args = ap.parse_args()
    names = [l.strip() for l in open(args.names) if l.strip()]
    rows = []
    for name in names:
        da = os.path.join(args.a, "bin", name)
        db = os.path.join(args.b, "bin", name)
        if not os.path.isdir(da) or not os.path.isdir(db):
            rows.append({"test": name, "stage": "missing"})
            continue
        pf = first_plotfile(da)
        rec = {"test": name}
        if pf and os.path.isdir(os.path.join(db, pf[1])):
            res = cc.cmp_plotfile(os.path.join(da, pf[1]), os.path.join(db, pf[1]))
            worst = max((f.get("max_abs_diff", 0.0) for f in res.get("fields", [])), default=0.0)
            rel = max((f.get("max_rel_diff", 0.0) for f in res.get("fields", [])), default=0.0)
            ndiff = sum(f.get("n_differ", 0) for f in res.get("fields", []))
            wf = max(res.get("fields", []), key=lambda f: f.get("max_abs_diff", 0.0), default={})
            rec.update({"stage": "first plotfile " + pf[1], "max_abs": worst, "max_rel": rel,
                        "n_differ": ndiff, "field": wf.get("field", "")})
        else:
            h5a = first_h5(da)
            h5b = first_h5(db)
            if h5a and h5b:
                res = cc.cmp_hdf5(h5a, h5b)
                worst = max((d.get("max_abs_diff", 0.0) for d in res.get("datasets", [])), default=0.0)
                rel = max((d.get("max_rel_diff", 0.0) for d in res.get("datasets", [])), default=0.0)
                ndiff = sum(d.get("n_differ", 0) for d in res.get("datasets", []))
                wf = max(res.get("datasets", []), key=lambda d: d.get("max_abs_diff", 0.0), default={})
                rec.update({"stage": "first h5 " + os.path.basename(h5a), "max_abs": worst,
                            "max_rel": rel, "n_differ": ndiff, "field": wf.get("dataset", "")})
            else:
                rec.update({"stage": "no output found"})
        rows.append(rec)
        print(json.dumps(rec))
    keys = ["test", "stage", "max_abs", "max_rel", "n_differ", "field"]
    with open(args.out, "w", newline="") as fh:
        w = csv.DictWriter(fh, fieldnames=keys)
        w.writeheader()
        for r in rows:
            w.writerow({k: r.get(k, "") for k in keys})


if __name__ == "__main__":
    main()
