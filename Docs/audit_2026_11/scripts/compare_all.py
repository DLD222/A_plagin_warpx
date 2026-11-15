#!/usr/bin/env python3
"""Compare every registered test run of two WarpX build trees, case by case.

Usage:
  compare_all.py --a DIR --b DIR --out DIR [--label-a fork --label-b official] [--only NAME...]

For each test name registered in either build (ctest --show-only=json-v1) the per-test
working directories `<build>/bin/<name>` are compared with compare_case.compare_dirs().

Writes:
  <out>/cases/<name>.json   full comparison record for every case that is not identical
  <out>/cases.csv           one row per test
  <out>/summary.json        aggregate counts
"""
import argparse
import csv
import json
import os
import subprocess
import sys
import xml.etree.ElementTree as ET

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import compare_case as cc  # noqa: E402


def test_names(build):
    out = subprocess.run(["ctest", "--test-dir", build, "--show-only=json-v1"],
                         capture_output=True, text=True)
    if out.returncode != 0:
        raise SystemExit(f"ctest --show-only failed for {build}: {out.stderr[:400]}")
    j = json.loads(out.stdout)
    names = {}
    for t in j.get("tests", []):
        n = t["name"]
        if n.endswith(".run"):
            names[n[:-4]] = t
    return names


def junit_status(path):
    res = {}
    if not os.path.isfile(path):
        return res
    try:
        root = ET.parse(path).getroot()
    except ET.ParseError:
        return res
    for tc in root.iter("testcase"):
        name = tc.get("name", "")
        if name.endswith(".run"):
            name = name[:-4]
        status = "passed"
        for ch in tc:
            if ch.tag in ("failure", "error"):
                status = "failed"
            elif ch.tag == "skipped":
                status = "skipped"
        res[name] = {"status": status, "time": tc.get("time")}
    return res


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--a", required=True)
    ap.add_argument("--b", required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument("--label-a", default="A")
    ap.add_argument("--label-b", default="B")
    ap.add_argument("--only", nargs="*", default=None)
    args = ap.parse_args()

    os.makedirs(os.path.join(args.out, "cases"), exist_ok=True)
    na, nb = test_names(args.a), test_names(args.b)
    shared = sorted(set(na) & set(nb))
    only_a = sorted(set(na) - set(nb))
    only_b = sorted(set(nb) - set(na))
    if args.only:
        shared = [n for n in shared if n in set(args.only)]
    ja = junit_status(os.path.join(args.out, f"{args.label_a}_junit.xml"))
    jb = junit_status(os.path.join(args.out, f"{args.label_b}_junit.xml"))

    rows = []
    for name in shared:
        da, db = os.path.join(args.a, "bin", name), os.path.join(args.b, "bin", name)
        sa = "missing" if not os.path.isdir(da) else ""
        sb = "missing" if not os.path.isdir(db) else ""
        rec = {"name": name}
        if sa or sb:
            rec.update({"identical": None, "files_total": 0, "files_differing": 0,
                        "max_abs_diff": 0.0, "max_rel_diff": 0.0,
                        "note": f"run dir {sa}{sb}".strip()})
            rows.append(rec)
            continue
        res = cc.compare_dirs(da, db, name)
        rec.update({k: res.get(k) for k in ("identical", "files_total", "files_differing",
                                            "max_abs_diff", "max_rel_diff",
                                            "max_rel_diff_field")})
        rec["note"] = ",".join((res.get("files_only_ref") or [])[:2]
                              + (res.get("files_only_new") or [])[:2])
        rec["kinds"] = sorted({e.get("kind", "?") for e in res.get("details", [])})
        if not res.get("identical"):
            with open(os.path.join(args.out, "cases", name + ".json"), "w") as fh:
                json.dump(res, fh, indent=1)
        rows.append(rec)

    with open(os.path.join(args.out, "cases.csv"), "w", newline="") as fh:
        w = csv.writer(fh)
        w.writerow(["test", "identical", "files_total", "files_differing",
                    "max_abs_diff", "max_rel_diff", "max_rel_field", "kinds",
                    f"status_{args.label_a}", f"status_{args.label_b}", "note"])
        for rec in rows:
            w.writerow([rec["name"], rec.get("identical"), rec.get("files_total"),
                        rec.get("files_differing"), rec.get("max_abs_diff"),
                        rec.get("max_rel_diff"), rec.get("max_rel_diff_field"),
                        ";".join(rec.get("kinds", [])),
                        (ja.get(rec["name"], {}) or {}).get("status", ""),
                        (jb.get(rec["name"], {}) or {}).get("status", ""),
                        rec.get("note", "")])
    stats = {
        "label_a": args.label_a, "label_b": args.label_b,
        "tests_a": len(na), "tests_b": len(nb), "shared": len(shared),
        "only_a": only_a, "only_b": only_b,
        "identical": sum(1 for r in rows if r.get("identical") is True),
        "differing": sum(1 for r in rows if r.get("identical") is False),
        "missing": sum(1 for r in rows if r.get("identical") is None),
    }
    with open(os.path.join(args.out, "summary.json"), "w") as fh:
        json.dump(stats, fh, indent=1)
    print(json.dumps({k: v for k, v in stats.items() if not isinstance(v, list)}, indent=1))


if __name__ == "__main__":
    main()
