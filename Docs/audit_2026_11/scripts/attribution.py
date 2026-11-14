#!/usr/bin/env python3
"""Attribution: which numeric differences vs official 26.10 survive when the official
source is built against the fork's own AMReX commit?

Usage: attribution.py --a DIR --b DIR --out FILE.md
  --a  compare_all output: fork vs official 26.10
  --b  compare_all output: fork vs official_amrex (same AMReX as fork)
"""
import argparse
import glob
import json
import os

META = {"build_metadata_diff", "used_inputs_diff"}


def numeric(rec):
    out = []
    for p in rec.get("plotfiles", []):
        for f in p.get("fields", []):
            if f.get("n_differ") or f.get("shape_mismatch"):
                out.append((p.get("path", ""), f.get("field"), f.get("n_differ"),
                            f.get("max_abs_diff", 0.0), f.get("max_rel_diff", 0.0)))
    for e in rec.get("details", []):
        if e.get("kind") == "hdf5_diff":
            for ds in e.get("datasets", []):
                if ds.get("n_differ") or ds.get("shape_mismatch"):
                    out.append((e.get("path", ""), ds.get("dataset"), ds.get("n_differ"),
                                ds.get("max_abs_diff", 0.0), ds.get("max_rel_diff", 0.0)))
    return out


def load(d):
    out = {}
    for f in glob.glob(os.path.join(d, "cases", "*.json")):
        rec = json.load(open(f))
        out[rec["name"]] = rec
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--a", required=True)
    ap.add_argument("--b", required=True)
    ap.add_argument("--out", required=True)
    args = ap.parse_args()
    A, B = load(args.a), load(args.b)
    rows = []
    for name, rec in A.items():
        na = numeric(rec)
        if not na:
            continue
        nb = numeric(B[name]) if name in B else []
        worst = max(na, key=lambda x: x[3] or 0.0)
        worst_b = max(nb, key=lambda x: x[3] or 0.0) if nb else None
        if name not in B:
            verdict = "not-attributed (official_amrex 未运行)"
        elif not nb:
            verdict = "AMReX/上游演进（同 AMReX 下无差异）"
        else:
            # survives the same-AMReX build -> fork code
            ratio = (worst_b[3] / worst[3]) if worst[3] else 1.0
            verdict = f"fork 代码差异（同 AMReX 仍差，比值 {ratio:.1e}）"
        rows.append((name, worst[1], worst[3], verdict))
    rows.sort(key=lambda r: -(r[2] or 0.0))
    n_amrex = sum(1 for r in rows if r[3].startswith("AMReX"))
    n_fork = sum(1 for r in rows if r[3].startswith("fork"))
    n_na = len(rows) - n_amrex - n_fork
    out = ["# 差异归因（fork vs official 26.10，隔离对照：official + fork 同款 AMReX）", "",
           f"- 参与归因的数值差异用例：{len(rows)}",
           f"- 归因到 AMReX/上游演进（同 AMReX 构建下数值一致）：{n_amrex}",
           f"- 归因到 fork 代码（同 AMReX 构建下仍差）：{n_fork}",
           f"- 未归因（official_amrex 未运行该用例，多为 >10 分钟的重算例）：{n_na}",
           "", "| case | 最差字段 | max abs (vs 官方) | 归因 |", "|---|---|---|---|"]
    for r in rows:
        out.append(f"| {r[0]} | {r[1]} | {r[2]:.3e} | {r[3]} |")
    open(args.out, "w").write("\n".join(out) + "\n")
    print(f"attributed {len(rows)}: amrex={n_amrex} fork={n_fork} not={n_na} -> {args.out}")


if __name__ == "__main__":
    main()
