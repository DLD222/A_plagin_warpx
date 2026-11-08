#!/usr/bin/env python3
"""Kernel-coupling ratchet for modules being turned into plugins (S6).

A module that should become its own plugin may not keep calling the kernel
singleton.  Until it is fully extracted, every remaining reference must be
listed in kernel_coupling_allowlist.txt; a new one fails the check, so the
number can only go down.

Scope is the list of trees below (one per extraction batch).  Extend it when a
new module starts its extraction.
"""
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[2]
ALLOWLIST = ROOT / "Tools" / "Modularization" / "kernel_coupling_allowlist.txt"

# Trees scheduled for extraction.  Add one per S6 batch.
TREES = [
    "Source/Plugins/PicEm/Modules/FieldSolver/FiniteDifference",
    "Source/Plugins/PicEm/Modules/FieldSolver/Electrostatic",
    "Source/Plugins/PicEm/Modules/FieldSolver/Implicit",
    "Source/Plugins/PicEm/Modules/FieldSolver/PSATD",
    "Source/Plugins/PicEm/Modules/FieldSolver/Magnetostatic",
]

TOKENS = ["WarpX::", '#include "WarpX.H"']

allowed = set()
if ALLOWLIST.exists():
    for line in ALLOWLIST.read_text().splitlines():
        line = line.strip()
        if line and not line.startswith("#"):
            allowed.add(line)

errors = []
found = set()
for tree in TREES:
    base = ROOT / tree
    if not base.is_dir():
        continue
    for path in sorted(base.rglob("*")):
        if path.suffix not in (".H", ".h", ".cpp", ".cxx"):
            continue
        rel = str(path.relative_to(ROOT))
        text = path.read_text(errors="ignore")
        for token in TOKENS:
            # comments do not count for the singleton reference
            occurrences = 0
            for line in text.splitlines():
                stripped = line.lstrip()
                if token == "WarpX::" and stripped.startswith(("//", "*", "/*")):
                    continue
                if token in line:
                    occurrences += 1
            if occurrences:
                entry = f"{rel}|{token}"
                found.add(entry)
                if entry not in allowed:
                    errors.append(f"{entry} ({occurrences} lines)")

stale = sorted(allowed - found)
if stale:
    print("note: allowlist entries no longer present (remove them):")
    for entry in stale:
        print("  " + entry)

if errors:
    print("kernel coupling without an allowlist entry:")
    for err in errors:
        print("  " + err)
    sys.exit(1)

print(f"kernel coupling OK ({len(found)} known entries, {len(errors)} new)")
