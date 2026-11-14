#!/usr/bin/env python3
"""Turn compare_all.py output into the case-level tables of the audit report.

Usage: make_report.py --dir audit_runs/compare_fork_vs_official --out Docs/audit_2026_11/CASE_RESULTS.md
"""
import argparse
import csv
import glob
import json
import os


def load_cases(d):
    out = {}
    for f in glob.glob(os.path.join(d, "cases", "*.json")):
        try:
            rec = json.load(open(f))
        except Exception:  # noqa: BLE001
            continue
        out[rec["name"]] = rec
    return out


def classify(row, rec):
    """Return a verdict string for one case."""
    kinds = set((row.get("kinds") or "").split(";")) - {""}
    data_kinds = kinds - {"build_metadata_diff", "used_inputs_diff"}
    if row.get("identical") == "True":
        return "identical"
    if row.get("identical") in ("None", ""):
        return "not-run"
    if not data_kinds:
        return "metadata-only"
    return "data-differs"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--dir", required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument("--a", default="fork")
    ap.add_argument("--b", default="official")
    args = ap.parse_args()
    rows = list(csv.DictReader(open(os.path.join(args.dir, "cases.csv"))))
    cases = load_cases(args.dir)
    verdicts = {}
    for r in rows:
        v = classify(r, cases.get(r["test"]))
        r["verdict"] = v
        verdicts[v] = verdicts.get(v, 0) + 1
    lines = [f"# {args.a} vs {args.b}: case results", "",
             f"cases: {len(rows)}", ""]
    for k in sorted(verdicts):
        lines.append(f"- {k}: {verdicts[k]}")
    lines += ["", "## Cases whose numeric output differs", "",
              "| case | diffs | max abs | max rel | kinds | fork status | official status |",
              "|---|---|---|---|---|---|---|"]
    for r in rows:
        if r["verdict"] != "data-differs":
            continue
        lines.append("| {test} | {files_differing} | {max_abs_diff} | {max_rel_diff} | {kinds} | {sa} | {sb} |".format(
            test=r["test"], files_differing=r["files_differing"], max_abs_diff=r["max_abs_diff"],
            max_rel_diff=r["max_rel_diff"], kinds=r["kinds"],
            sa=r.get(f"status_{args.a}", ""), sb=r.get(f"status_{args.b}", "")))
    lines += ["", "## Cases with different composed inputs (warpx_used_inputs) but identical data", "",
              "| case | used-inputs diff lines |", "|---|---|"]
    n_meta = 0
    for r in rows:
        if r["verdict"] != "metadata-only":
            continue
        rec = cases.get(r["test"], {})
        used = next((e for e in rec.get("details", []) if e.get("kind") == "used_inputs_diff"), {})
        if used:
            n_meta += 1
            lines.append(f"| {r['test']} | {used.get('n_diff_lines', '')} |")
        else:
            pass
    if n_meta == 0:
        lines.append("| (none) | |")
    lines += ["", "## Test failures", "",
              "| case | fork | official | note |", "|---|---|---|---|"]
    for r in rows:
        sa = r.get(f"status_{args.a}", "")
        sb = r.get(f"status_{args.b}", "")
        if "failed" not in (sa, sb) and sa and sb:
            continue
        if not sa and not sb:
            continue
        lines.append(f"| {r['test']} | {sa} | {sb} | {r.get('note','')} |")
    open(args.out, "w").write("\n".join(lines) + "\n")
    print(f"wrote {args.out}: {verdicts}")


if __name__ == "__main__":
    main()
