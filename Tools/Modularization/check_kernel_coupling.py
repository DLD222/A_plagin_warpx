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
#
# Excluded for now (documented in PLUGIN_REQUIREMENTS_AND_ROADMAP.md):
#   FieldSolver/Implicit - its Define(WarpX*) drives the kernel field advance,
#   particle save/restore and diagnostics (198 member uses over ~11 operations),
#   so it needs a kernel-service layer before it can leave the kernel plugin.
# The solver layers moved out of PicEm/Modules/FieldSolver when they became plugins; the old
# paths were silently skipped ("if not base.is_dir(): continue"), so this check reported
# "kernel coupling OK (0 known entries, 0 new)" while only really looking at two trees.
TREES = [
    "Source/Plugins/FieldSolver/FiniteDifference",
    "Source/Plugins/FieldSolver/Electrostatic",
    "Source/Plugins/FieldSolver/PSATD",
    "Source/Plugins/FieldSolver/Magnetostatic",
    "Source/Plugins/PicEm/Modules/FieldSolver/Magnetostatic",
    # Extracted particle layer: it must stay free of the kernel singleton, so
    # any new reference here fails outright (no allowlist entries).
    "Source/Plugins/ParticleCore",
]

# A tree that does not exist is a broken check, not a pass.
MISSING_TREES = [tree for tree in TREES if not (ROOT / tree).is_dir()]

# * any qualified use of the singleton (WarpX::, and namespace aliases are caught below)
# * the header, in either quote or angle-bracket form (the angle form was missing here while
#   check_plugin_boundaries.py already handled it)
TOKENS = ["WarpX::", "WarpX::GetInstance"]
HEADER_PATTERNS = [r'#\s*include\s*[<"]WarpX\.H[>"]', r'namespace\s+\w+\s*=\s*WarpX\s*;']

allowed = set()
if ALLOWLIST.exists():
    for line in ALLOWLIST.read_text().splitlines():
        line = line.strip()
        if line and not line.startswith("#"):
            allowed.add(line)

errors = []
found = set()
for tree in MISSING_TREES:
    errors.append(f"monitored tree does not exist: {tree}")
for tree in TREES:
    base = ROOT / tree
    if not base.is_dir():
        continue
    for path in sorted(base.rglob("*")):
        if path.suffix not in (".H", ".h", ".cpp", ".cxx"):
            continue
        rel = str(path.relative_to(ROOT))
        text = path.read_text(errors="ignore")
        for pattern in HEADER_PATTERNS:
            if re.search(pattern, text):
                entry = f"{rel}|include-or-alias"
                found.add(entry)
                if entry not in allowed:
                    errors.append(f"{entry} (matches /{pattern}/)")
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
