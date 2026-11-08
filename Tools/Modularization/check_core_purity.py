#!/usr/bin/env python3
"""Keep concrete WarpX particle types out of the core (D1-a ratchet).

The core must stay physics-agnostic: no concrete particle container, no
concrete tile iterator, no WarpX singleton.  The remaining references are
listed in core_concrete_allowlist.txt with the reason; this checker fails when
a new one appears, so the number can only go down.

A reference inside a comment does not count.
"""
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[2]
CORE = ROOT / "Source" / "Core"
ALLOWLIST = ROOT / "Tools" / "Modularization" / "core_concrete_allowlist.txt"

TOKENS = [
    "WarpXParticleContainer",
    "MultiParticleContainer",
    "PhysicalParticleContainer",
    "LaserParticleContainer",
    "WarpXParIter",
    "WarpX::",
]

allowed = set()
if ALLOWLIST.exists():
    for line in ALLOWLIST.read_text().splitlines():
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        allowed.add(line)

errors = []
stale = []
found = set()
for path in sorted(CORE.rglob("*")):
    if path.suffix not in (".H", ".h", ".cpp", ".cxx"):
        continue
    rel = str(path.relative_to(ROOT))
    for lineno, line in enumerate(path.read_text(errors="ignore").splitlines(), 1):
        stripped = line.lstrip()
        if stripped.startswith(("//", "*", "/*")):
            continue
        for token in TOKENS:
            if re.search(r"\b" + re.escape(token) + r"\b", line):
                entry = f"{rel}|{token}"
                found.add(entry)
                if entry not in allowed:
                    errors.append(f"{entry} (line {lineno})")

for entry in sorted(allowed - found):
    stale.append(entry)

if stale:
    print("note: allowlist entries no longer present (remove them):")
    for entry in stale:
        print("  " + entry)

if errors:
    print("concrete particle references in Core without an allowlist entry:")
    for err in errors:
        print("  " + err)
    sys.exit(1)

print(f"core purity OK ({len(found)} known references, {len(errors)} new)")
