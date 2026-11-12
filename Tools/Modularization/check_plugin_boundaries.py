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

# Resolve an include the way the compiler does, so a directory name that exists
# in two plugins (or in the core) is attributed to the file actually used.
INCLUDE_ROOTS = [
    SRC / "Core",
    SRC / "Plugins" / "PicEm",
    SRC / "Plugins" / "PicEm" / "Modules",
    SRC / "Plugins" / "PicEm" / "Modules" / "IO",
    SRC / "Plugins" / "PicEm" / "RuntimeBridge",
    SRC / "Plugins" / "PicEm" / "Modules" / "MomentsSolver",
    SRC / "Plugins",
    SRC / "Plugins" / "IO",
    SRC,
]


def owner(path: Path) -> str:
    """Return 'core', 'plugin:<name>' or 'other' for a source file."""
    try:
        rel = path.relative_to(SRC)
    except ValueError:
        return "other"
    parts = rel.parts
    if parts and parts[0] == "Plugins" and len(parts) > 2:
        return "plugin:" + parts[1]
    if parts and parts[0] == "Core":
        return "core"
    return "other"


def resolve(inc: str):
    for root in INCLUDE_ROOTS:
        candidate = root / inc
        if candidate.is_file():
            return candidate
    return None


allow_file = ROOT / "Tools" / "Modularization" / "plugin_boundary_allowlist.txt"
allowed = set()
if allow_file.exists():
    for line in allow_file.read_text().splitlines():
        line = line.strip()
        if line and not line.startswith("#"):
            allowed.add(line)

# Declared, one-way plugin dependencies: "PluginA -> PluginB" means A may include B's
# headers.  The graph must be acyclic; Core still never includes a plugin.
declared_edges = set()
edge_file = ROOT / "Tools" / "Modularization" / "plugin_dependencies.txt"
if edge_file.exists():
    for line in edge_file.read_text().splitlines():
        line = line.split("#")[0].strip()
        if "->" not in line:
            continue
        a, b = (part.strip() for part in line.split("->", 1))
        declared_edges.add(("plugin:" + a, "plugin:" + b))

# Reject cycles in the declared graph: dependencies must form a DAG.
def _has_cycle(edges):
    graph = {}
    for a, b in edges:
        graph.setdefault(a, set()).add(b)
    state = {}

    def visit(node, stack):
        if state.get(node) == 1:
            return True
        if state.get(node) == 2:
            return False
        state[node] = 1
        for nxt in graph.get(node, ()):
            if visit(nxt, stack + [node]):
                return True
        state[node] = 2
        return False

    return any(visit(node, []) for node in list(graph))

errors = []
violations = set()
files = [p for p in SRC.rglob("*") if p.suffix in (".h", ".H", ".hpp", ".cpp")]
for path in files:
    here = owner(path)
    if here == "other":
        continue
    for inc in inc_re.findall(path.read_text(errors="ignore")):
        # A forward-declaration header carries no definition and no code
        # dependency: it names a type, it does not use it.
        if inc.endswith("_fwd.H"):
            continue
        target = resolve(inc)
        if target is None:
            continue
        there = owner(target)
        if there == "other" or there == here:
            continue
        if here == "core":
            violations.add(f"Core->Plugin: {path.relative_to(ROOT)} includes {inc}")
        elif there == "core":
            continue  # plugins may use the core
        else:
            # A plugin may depend on another plugin when that edge is declared in
            # plugin_dependencies.txt (the kernel host owns the implementation it links).
            # The declared graph must stay acyclic; a cycle is reported below.
            if (here, there) in declared_edges:
                continue
            violations.add(f"Plugin->Plugin: {path.relative_to(ROOT)} includes {inc}")

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

if "--dump" in sys.argv:
    for v in sorted(violations):
        print(v)
    sys.exit(0)

if _has_cycle(declared_edges):
    errors.append("plugin_dependencies.txt declares a dependency cycle")

errors = sorted(v for v in violations if v not in allowed)
resolved = sorted(allowed - violations)
if resolved:
    print("note: allowlist entries no longer violate (remove them):")
    for entry in resolved:
        print("  " + entry)

if errors:
    print("\n".join(errors))
    sys.exit(1)
print(f"plugin boundaries OK ({len(violations)} known, 0 new)")
