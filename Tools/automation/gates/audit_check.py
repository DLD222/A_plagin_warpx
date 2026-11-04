#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
"""A0 audit gate: headers, class catalog, and lifecycle docs exist and are consistent."""
from __future__ import annotations
import csv
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
EXPECTED = {
    "min_headers": 372,
    "min_classes": 300,
    "min_methods": 2000,
}
REQUIRED_FILES = [
    "Docs/research_notes/A0/HEADER_CATALOG.md",
    "Docs/research_notes/A0/COVERAGE.md",
    "Docs/research_notes/A0/LIFECYCLE_MATRIX.md",
    "Docs/research_notes/A0/VERIFICATION.md",
    "Docs/research_notes/A0/STATIC_INVENTORY.md",
    "Docs/research_notes/A0/classes.csv",
]


def main() -> int:
    errors = []
    headers = list((ROOT / "Source").rglob("*.H"))
    if len(headers) < EXPECTED["min_headers"]:
        errors.append(f"headers: expected >= {EXPECTED['min_headers']}, found {len(headers)}")

    csv_path = ROOT / "Docs/research_notes/A0/classes.csv"
    rows = list(csv.DictReader(csv_path.open())) if csv_path.exists() else []
    methods = sum(int(r["n_public_methods"]) for r in rows) if rows else 0
    if len(rows) < EXPECTED["min_classes"]:
        errors.append(f"classes: expected >= {EXPECTED['min_classes']}, found {len(rows)}")
    if methods < EXPECTED["min_methods"]:
        errors.append(f"public methods: expected >= {EXPECTED['min_methods']}, found {methods}")

    for f in REQUIRED_FILES:
        if not (ROOT / f).exists():
            errors.append(f"missing {f}")

    print(f"headers={len(headers)} classes={len(rows)} public_methods={methods}")
    if errors:
        print("FAIL:")
        for e in errors:
            print("  -", e)
        return 1
    print("PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main())
