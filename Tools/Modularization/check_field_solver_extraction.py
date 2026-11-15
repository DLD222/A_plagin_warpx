#!/usr/bin/env python3
"""Ratchet for extracting the field solvers out of PicEm.

Scans ``Source/Plugins`` (excluding the ``FieldSolver`` plugin itself) and counts

* includes that still reach into PicEm, i.e. ``#include "FieldSolver/..."``;
* uses of the concrete solver types.

The counts may only go **down**: the checked-in baseline
(``field_solver_extraction_baseline.json`` next to this file) records the current
values and the gate fails when a count grows or a new file shows up.  Refresh the
baseline with ``--update-baseline`` when a move really changed the numbers, and
commit the new file together with the move.

``--report`` prints the numbers without comparing them (useful while refactoring).
"""
from __future__ import annotations

import json
import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
PLUGINS = ROOT / "Source/Plugins"
EXTRACTED = PLUGINS / "FieldSolver"
BASELINE = HERE / "field_solver_extraction_baseline.json"

INCLUDE = re.compile(r'#include\s+"(FieldSolver/[^"]+)"')
SYMBOLS = (
    "FiniteDifferenceSolver",
    "ElectrostaticSolver",
    "ImplicitSolver",
    "HybridPICModel",
    "MacroscopicProperties",
    "MagnetostaticSolver",
)


def scan() -> dict:
    """Return the current counts (everything outside Source/Plugins/FieldSolver)."""
    includes: dict[str, int] = {}
    symbols = {name: 0 for name in SYMBOLS}
    for path in sorted(PLUGINS.rglob("*")):
        if path.suffix not in (".cpp", ".H", ".hpp", ".h"):
            continue
        if EXTRACTED in path.parents:
            continue
        text = path.read_text(errors="ignore")
        hits = INCLUDE.findall(text)
        if hits:
            includes[str(path.relative_to(ROOT))] = len(hits)
        for name in SYMBOLS:
            symbols[name] += len(re.findall(r"\b" + name + r"\b", text))
    return {"includes": includes,
            "include_total": sum(includes.values()),
            "symbols": symbols}


def main() -> int:
    if not PLUGINS.is_dir():
        print(f"missing {PLUGINS}", file=sys.stderr)
        return 2
    current = scan()
    update = "--update-baseline" in sys.argv[1:]
    report_only = "--report" in sys.argv[1:]

    print(f"FieldSolver includes outside the plugin: {current['include_total']}")
    for name, count in current["includes"].items():
        print(f"  {name}: {count}")
    for name, count in current["symbols"].items():
        print(f"{name}: {count}")

    if report_only:
        return 0
    if not (update or not BASELINE.exists()):
        base = json.loads(BASELINE.read_text())
        problems = []
        for name, count in current["includes"].items():
            was = base.get("includes", {}).get(name, 0)
            if count > was:
                problems.append(f"{name}: FieldSolver includes {was} -> {count}")
        if current["include_total"] > base.get("include_total", 0):
            problems.append(
                f"total FieldSolver includes {base.get('include_total', 0)} -> "
                f"{current['include_total']}")
        for name, count in current["symbols"].items():
            was = base.get("symbols", {}).get(name, 0)
            if count > was:
                problems.append(f"{name} uses {was} -> {count}")
        if problems:
            print("\nfield solver extraction regressed (the counts may only go down):",
                  file=sys.stderr)
            for line in problems:
                print("  " + line, file=sys.stderr)
            print("if the change is a genuine improvement, rerun with "
                  "--update-baseline and commit the new baseline", file=sys.stderr)
            return 1
        print(f"no regression against {BASELINE.name}")
    if update or not BASELINE.exists():
        BASELINE.write_text(json.dumps(current, indent=1, sort_keys=True) + "\n")
        print(f"baseline written to {BASELINE.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
