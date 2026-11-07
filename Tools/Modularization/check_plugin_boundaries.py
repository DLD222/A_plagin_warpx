#!/usr/bin/env python3
"""Check plugin include boundaries.

Core may not include any plugin-owned header.
A plugin may not include another plugin's headers.
"""
from pathlib import Path
import re, sys
ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "Source"
PLUGINS = SRC / "Plugins"
plugin_names = {p.name for p in PLUGINS.iterdir() if p.is_dir()}
# Headers historically owned by the single PicEm plugin.
picem_prefixes = ("Particles/", "Diagnostics/", "FieldSolver/", "Evolve/",
                  "Initialization/", "AcceleratorLattice/", "BoundaryConditions/",
                  "EmbeddedBoundary/", "Fluids/", "Laser/", "MomentsSolver/",
                  "NonlinearSolvers/", "Parallelization/", "Python/")
inc_re = re.compile(r'#include\s+"([^"]+)"')
errors = []
for path in (SRC / "Core").rglob("*"):
    if path.suffix not in (".h", ".H", ".hpp", ".cpp"):
        continue
    for inc in inc_re.findall(path.read_text(errors="ignore")):
        if inc.startswith(picem_prefixes) or any(inc.startswith(n + "/") for n in plugin_names):
            errors.append(f"Core->Plugin: {path.relative_to(ROOT)} includes {inc}")
for p in PLUGINS.iterdir():
    if not p.is_dir():
        continue
    own = p.name
    for path in p.rglob("*"):
        if path.suffix not in (".h", ".H", ".hpp", ".cpp"):
            continue
        for inc in inc_re.findall(path.read_text(errors="ignore")):
            for other in plugin_names:
                if other != own and inc.startswith(other + "/"):
                    errors.append(f"Plugin->Plugin: {path.relative_to(ROOT)} includes {inc}")
if errors:
    print("\n".join(errors))
    sys.exit(1)
print("plugin boundaries OK")
