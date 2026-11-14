#!/usr/bin/env python3
"""List the cases of a compare_all.py run that differ beyond build metadata.

Usage: select_diff_cases.py --dir DIR [--out-prefix PATH]
Prints one test name per line for cases whose numeric output differs, and writes
<prefix>.data.txt / <prefix>.meta.txt when --out-prefix is given.
"""
import argparse
import glob
import json
import os

META_KINDS = {"build_metadata_diff", "used_inputs_diff"}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--dir", required=True)
    ap.add_argument("--out-prefix", default="")
    args = ap.parse_args()
    data, meta = [], []
    for f in sorted(glob.glob(os.path.join(args.dir, "cases", "*.json"))):
        rec = json.load(open(f))
        kinds = {e.get("kind") for e in rec.get("details", [])} - META_KINDS
        names = {e.get("kind") for p in rec.get("plotfiles", []) for e in p.get("fields", [])
                 if e.get("n_differ") or e.get("shape_mismatch")}
        if kinds or names:
            data.append(rec["name"])
        else:
            meta.append(rec["name"])
    if args.out_prefix:
        open(args.out_prefix + ".data.txt", "w").write("\n".join(data) + "\n")
        open(args.out_prefix + ".meta.txt", "w").write("\n".join(meta) + "\n")
    print("\n".join(data))
    print(f"# {len(data)} data-differing, {len(meta)} metadata-only", file=__import__("sys").stderr)


if __name__ == "__main__":
    main()
