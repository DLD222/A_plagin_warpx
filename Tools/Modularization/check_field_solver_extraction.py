#!/usr/bin/env python3
"""Report blockers for moving FieldSolver out of PicEm."""
from pathlib import Path
import re, sys
ROOT = Path(__file__).resolve().parents[2]
PICEM = ROOT / "Source/Plugins/PicEm"
inc = re.compile(r'#include\s+"(FieldSolver/[^"]+)"')
rows = []
for p in PICEM.rglob("*"):
    if p.suffix not in (".cpp", ".H", ".hpp"):
        continue
    if "Modules/FieldSolver" in p.as_posix():
        continue
    text = p.read_text(errors="ignore")
    for m in inc.findall(text):
        rows.append((str(p.relative_to(ROOT)), m))
print(f"external FieldSolver includes: {len(rows)}")
for f,i in sorted(rows):
    print(f"  {f}: {i}")
# direct concrete symbols outside FieldSolver
symbols = ["FiniteDifferenceSolver","ElectrostaticSolver","ImplicitSolver",
           "HybridPICModel","MacroscopicProperties","MagnetostaticSolver"]
for s in symbols:
    n = 0
    for p in PICEM.rglob("*"):
        if p.suffix not in (".cpp",".H",".hpp"): continue
        if "Modules/FieldSolver" in p.as_posix(): continue
        n += len(re.findall(r'\b'+s+r'\b', p.read_text(errors="ignore")))
    print(f"{s}: {n}")
sys.exit(0)
