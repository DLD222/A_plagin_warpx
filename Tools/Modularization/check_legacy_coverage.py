#!/usr/bin/env python3
"""Verify every legacy WarpX source file still exists in the reorganized tree."""
from pathlib import Path
import sys

root = Path(__file__).resolve().parents[2]
manifest = root / "Docs/research_notes/legacy_source_manifest.txt"
current = {str(p.relative_to(root / "Source")) for p in (root / "Source").rglob("*") if p.is_file()}
renames = {
    "FieldSolver/FiniteDifferenceSolver/": "FieldSolver/FiniteDifference/",
    "FieldSolver/ElectrostaticSolvers/": "FieldSolver/Electrostatic/",
    "FieldSolver/ImplicitSolvers/": "FieldSolver/Implicit/",
    "FieldSolver/MagnetostaticSolver/": "FieldSolver/Magnetostatic/",
    "FieldSolver/SpectralSolver/": "FieldSolver/PSATD/",
}

missing = []
for line in manifest.read_text().splitlines():
    rel = line.strip().removeprefix("Source/")
    if not rel:
        continue
    for a, b in renames.items():
        if rel.startswith(a):
            rel = b + rel[len(a):]
            break
    candidates = [rel, f"Core/{rel}", f"Plugins/PicEm/{rel}", f"Plugins/PicEm/Modules/{rel}"]
    if not any(c in current for c in candidates):
        missing.append(rel)

print(f"legacy files: {len(manifest.read_text().splitlines())}")
print(f"missing: {len(missing)}")
for m in missing:
    print(m)
sys.exit(1 if missing else 0)
