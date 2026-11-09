#!/usr/bin/env python3
"""Parse and compare the add_warpx_test() definitions of two WarpX trees.

The report Docs/research_notes/UPSTREAM_PARITY.md claims that every upstream
test definition exists locally with byte-identical arguments.  This script is
what makes that claim checkable: it walks both trees, parses each
add_warpx_test(...) call into its arguments (comments stripped first), and
reports

  * the number of definitions,
  * upstream-only and local-only test names,
  * how many shared tests have a different argument list,
  * the per-dimensionality distribution.

Usage:
    python3 Tools/verify/parse_tests.py --compare /tmp/warpx_up2610 .
    python3 Tools/verify/parse_tests.py --list .            # local names only
"""

import argparse
import collections
import json
import os
import re
import sys

# Directories that never contain test definitions (or must not be searched).
SKIP_DIRS = {
    ".git", "build", "build_takeover", "build_multi2", "build_2d_full",
    "build_multi_hdf5_fft", "_deps", "node_modules",
}


def strip_cmake_comments(text):
    """Remove '#' comments, honouring double-quoted strings."""
    out = []
    for line in text.splitlines():
        in_quote = False
        cut = len(line)
        for i, ch in enumerate(line):
            if ch == '"':
                in_quote = not in_quote
            elif ch == "#" and not in_quote:
                cut = i
                break
        out.append(line[:cut])
    return "\n".join(out)


def parse_tree(root):
    """Return [{name, dims, nprocs, args, file}, ...] for one tree."""
    tests = []
    for dirpath, dirnames, filenames in os.walk(root):
        dirnames[:] = [d for d in dirnames if d not in SKIP_DIRS]
        if "CMakeLists.txt" not in filenames:
            continue
        path = os.path.join(dirpath, "CMakeLists.txt")
        text = strip_cmake_comments(
            open(path, encoding="utf-8", errors="replace").read())
        for match in re.finditer(r"(?m)^\s*add_warpx_test\s*\(", text):
            # walk to the matching close paren, skipping quoted strings
            i = match.end()
            depth, j = 1, i
            while j < len(text) and depth:
                if text[j] == "(":
                    depth += 1
                elif text[j] == ")":
                    depth -= 1
                elif text[j] == '"':
                    j += 1
                    while j < len(text) and text[j] != '"':
                        if text[j] == "\\":
                            j += 1
                        j += 1
                j += 1
            body = text[i:j - 1]
            toks = [t[1:-1] if t.startswith('"') else t
                    for t in re.findall(r'"(?:[^"\\]|\\.)*"|\S+', body)]
            if len(toks) < 2:
                continue
            tests.append({
                "name": toks[0],
                "dims": toks[1],
                "nprocs": toks[2] if len(toks) > 2 else "",
                "args": toks,
                "file": os.path.relpath(path, root),
            })
    return tests


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("root", nargs="?", default=".",
                    help="tree to parse (default: current directory)")
    ap.add_argument("--compare", metavar="REFERENCE_TREE", nargs="?",
                    const="", default=None,
                    help="compare against another tree; pass --compare REF ROOT "
                         "or --compare REF (ROOT defaults to '.')")
    ap.add_argument("--list", action="store_true", help="print test names")
    ap.add_argument("--json", metavar="PATH", help="dump the parsed list as JSON")
    args = ap.parse_args()

    root, reference = args.root, args.compare
    if reference == "":
        reference = root
        root = "."

    local = parse_tree(root)
    if args.json:
        json.dump(local, open(args.json, "w"), indent=1)
    if args.list:
        for t in local:
            print(t["name"])
        return 0

    if reference is None:
        print(f"{root}: {len(local)} add_warpx_test definitions")
        print(collections.Counter(t["dims"] for t in local))
        return 0

    upstream = parse_tree(reference)
    print(f"reference {reference}: {len(upstream)} definitions")
    print(f"local     {root}: {len(local)} definitions")

    up = {t["name"]: t for t in upstream}
    lo = {t["name"]: t for t in local}
    common = sorted(set(up) & set(lo))

    print(f"\nshared names: {len(common)}")
    print(f"only in reference: {sorted(set(up) - set(lo))}")
    print(f"only in local    : {sorted(set(lo) - set(up))}")

    differing = [n for n in common if up[n]["args"] != lo[n]["args"]]
    print(f"\ntests whose full argument list differs: {len(differing)}")
    for n in differing:
        print(f"  {n}")
        print(f"    reference: {up[n]['args']}")
        print(f"    local    : {lo[n]['args']}")
        print(f"    files    : {up[n]['file']} | {lo[n]['file']}")

    print("\nper-dimension distribution (reference -> local):")
    dims = collections.Counter(t["dims"] for t in upstream)
    dims_local = collections.Counter(t["dims"] for t in local)
    for d in sorted(set(dims) | set(dims_local)):
        print(f"  {d:10s} {dims.get(d, 0):4d} -> {dims_local.get(d, 0):4d}")

    return 1 if (set(up) - set(lo)) or differing else 0


if __name__ == "__main__":
    sys.exit(main())
