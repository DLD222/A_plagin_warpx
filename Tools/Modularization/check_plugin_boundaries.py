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
# A header that exists under both Core and a plugin with the same relative
# path is resolved by include order, so the same #include can compile against
# two different files (ODR hazard).  Fail on duplicates.
def relative_files(base: Path, prefix: str):
    root = base / prefix
    if not root.is_dir():
        return set()
    return {str(p.relative_to(base)) for p in root.rglob("*") if p.suffix in (".H", ".h", ".hpp")}

core_headers = relative_files(SRC / "Core", "Modules")
for plugin_dir in PLUGINS.iterdir():
    if not plugin_dir.is_dir():
        continue
    plugin_headers = relative_files(plugin_dir, "Modules")
    for rel in sorted(core_headers & plugin_headers):
        # A one-line forwarder ("#include Modules/<rel>") is an explicit
        # redirect, not a second definition.
        forwarder = plugin_dir / rel
        text = forwarder.read_text(errors="ignore")
        if f'#include "{rel}"' in text:
            continue
        errors.append(f"Duplicate header in Core and {plugin_dir.name}: {rel}")

if errors:
    print("\n".join(errors))
    sys.exit(1)
print("plugin boundaries OK")
