#!/usr/bin/env python3
"""Classify compare_all.py case records into numeric / text-only / metadata-only,
and summarise the largest deviations.

Usage: analyze_results.py --dir DIR --out FILE.md [--label "fork vs official"]
"""
import argparse
import glob
import json
import os


def case_summary(rec):
    numeric = []
    for p in rec.get("plotfiles", []):
        for f in p.get("fields", []):
            if f.get("n_differ") or f.get("shape_mismatch"):
                numeric.append({
                    "path": p.get("path", ""),
                    "field": f.get("field"),
                    "n": f.get("n"),
                    "n_differ": f.get("n_differ"),
                    "max_abs": f.get("max_abs_diff", 0.0),
                    "max_rel": f.get("max_rel_diff", 0.0),
                    "shape_mismatch": f.get("shape_mismatch"),
                })
    for e in rec.get("details", []):
        if e.get("kind") == "hdf5_diff":
            for ds in e.get("datasets", []):
                if ds.get("n_differ") or ds.get("shape_mismatch"):
                    numeric.append({
                        "path": e.get("path", ""),
                        "field": ds.get("dataset"),
                        "n": None,
                        "n_differ": ds.get("n_differ"),
                        "max_abs": ds.get("max_abs_diff", 0.0),
                        "max_rel": ds.get("max_rel_diff", 0.0),
                        "shape_mismatch": ds.get("shape_mismatch"),
                    })
    logs = []
    for e in rec.get("details", []):
        if e.get("kind") == "text_diff":
            logs.append({"path": e.get("path"), "lines": e.get("n_diff_lines")})
    used = next((e for e in rec.get("details", []) if e.get("kind") == "used_inputs_diff"), None)
    return numeric, logs, used


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--dir", required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument("--label", default="")
    args = ap.parse_args()
    rows = []
    for f in sorted(glob.glob(os.path.join(args.dir, "cases", "*.json"))):
        rec = json.load(open(f))
        numeric, logs, used = case_summary(rec)
        worst = max(numeric, key=lambda x: (x["max_abs"] or 0.0), default=None)
        rows.append({"name": rec["name"], "numeric": numeric, "logs": logs,
                     "used": used, "worst": worst,
                     "shape": sum(1 for x in numeric if x.get("shape_mismatch"))})
    num_cases = [r for r in rows if r["numeric"]]
    log_only = [r for r in rows if not r["numeric"] and r["logs"]]
    meta_only = [r for r in rows if not r["numeric"] and not r["logs"]]
    out = [f"# Numeric-difference analysis {args.label}", "",
           f"- cases with numeric differences: {len(num_cases)}",
           f"- cases with only log/text differences: {len(log_only)}",
           f"- cases with only metadata/used-inputs differences: {len(meta_only)}", "",
           "## Cases with numeric differences (worst field per case)", "",
           "| case | worst field | n differ | max abs | max rel | shape-mismatch |",
           "|---|---|---|---|---|---|"]
    for r in sorted(num_cases, key=lambda r: -(r["worst"]["max_abs"] or 0.0)):
        w = r["worst"]
        out.append("| {name} | {field} | {nd} | {abs:.3e} | {rel:.3e} | {sh} |".format(
            name=r["name"], field=w["field"], nd=w["n_differ"],
            abs=w["max_abs"] or 0.0, rel=w["max_rel"] or 0.0, sh=r["shape"]))
    out += ["", "## Cases with only log/text differences", "", "| case | file | differing lines |", "|---|---|---|"]
    for r in log_only:
        for l in r["logs"]:
            out.append(f"| {r['name']} | {l['path']} | {l['lines']} |")
    out += ["", "## Cases where only the composed input deck differs", "", "| case | lines |", "|---|---|"]
    for r in meta_only:
        if r["used"]:
            out.append(f"| {r['name']} | {r['used'].get('n_diff_lines')} |")
    open(args.out, "w").write("\n".join(out) + "\n")
    print(f"numeric={len(num_cases)} log-only={len(log_only)} meta-only={len(meta_only)} -> {args.out}")


if __name__ == "__main__":
    main()
