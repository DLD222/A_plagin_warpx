#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
"""A0 audit gate: headers, class catalog, and lifecycle docs exist and are consistent."""
from __future__ import annotations
import csv
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
EXPECTED = {
    "min_headers": 372,
    "min_classes": 300,
    "min_methods": 2000,
}
# classes.csv is a snapshot of Source/.  A pure threshold check cannot tell a fresh catalog from
# a stale one (Tools_Tests/F31), so the drift against the current tree is measured and ratcheted:
# the counts may only go down.  Refresh the catalog (Tools/A0/parse_headers.py +
# build_catalog.py) and lower these numbers in the same commit.
DRIFT_BASELINE = {"csv_missing_from_source": 35, "source_missing_from_csv": 103}

DECL = re.compile(r"^\s*(?:template\s*<[^>]*>\s*)?(class|struct)\s+([A-Za-z_]\w*)\s*(?::|\{|$)",
                  re.M)
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

    declared = set()
    for header in headers:
        try:
            text = header.read_text(errors="ignore")
        except OSError:
            continue
        declared.update((m.group(1), m.group(2)) for m in DECL.finditer(text))
    catalogued = {(r["kind"], r["class"]) for r in rows}
    drift = {"csv_missing_from_source": len(catalogued - declared),
             "source_missing_from_csv": len(declared - catalogued)}
    for key, value in drift.items():
        if value > DRIFT_BASELINE[key]:
            errors.append(f"classes.csv drift grew: {key} {DRIFT_BASELINE[key]} -> {value} "
                          f"(regenerate the A0 catalog or justify the new baseline)")

    for f in REQUIRED_FILES:
        if not (ROOT / f).exists():
            errors.append(f"missing {f}")

    print(f"headers={len(headers)} classes={len(rows)} public_methods={methods}")
    print(f"catalog drift: {drift['csv_missing_from_source']} catalogued entries no longer in "
          f"Source, {drift['source_missing_from_csv']} Source classes not catalogued "
          f"(baseline {DRIFT_BASELINE['csv_missing_from_source']}/"
          f"{DRIFT_BASELINE['source_missing_from_csv']})")
    if errors:
        print("FAIL:")
        for e in errors:
            print("  -", e)
        return 1
    print("PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main())
