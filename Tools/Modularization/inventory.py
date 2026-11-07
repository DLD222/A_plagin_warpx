#!/usr/bin/env python3
"""Inventory WarpX sources that must become project modules."""
from pathlib import Path
import sys

root = Path(__file__).resolve().parents[2]
modules = root / "Source/Plugins/PicEm/Modules"
rows = []
for cpp in sorted(modules.rglob("*.cpp")):
    rel = cpp.relative_to(modules)
    domain = rel.parts[0]
    unit = cpp.stem
    rows.append((domain, str(rel), unit))

out = Path(sys.argv[1]) if len(sys.argv) > 1 else None
lines = ["# Module inventory", "", f"Total `.cpp`: **{len(rows)}**", "",
         "| domain | current file | proposed unit |", "|---|---|---|"]
for domain, rel, unit in rows:
    lines.append(f"| `{domain}` | `{rel}` | `{unit}` |")
text = "\n".join(lines) + "\n"
if out:
    out.write_text(text)
else:
    print(text)
