#!/usr/bin/env python3
"""Check build-level plugin boundaries (A7 B1-B7).

Core must not compile plugin sources.  The C++ include checker
(check_plugin_boundaries.py) only proves that Core includes no plugin header;
this script proves that no CMake registration under Source/Core adds a file
that lives under Source/Plugins, and that the expected per-layer targets exist.

The plugin list is derived from the filesystem instead of being hard-coded: the
previous table named ``Source/Plugins/IO`` (which does not exist) and missed
``FluidMoments`` and ``NewtonGravity``, so two plugins were never checked and a
new directory would have crashed the script with a KeyError (Tools_Tests/F16).

Exit code 0 = OK, 1 = violation, 2 = the tree it is supposed to check is missing.
"""
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "Source"
PLUGINS = SRC / "Plugins"

errors = []


def target_sources_blocks(text):
    """Yield the body of every target_sources(...) call, counting parentheses.

    ``re.finditer(r"target_sources\\((.*?)\\)")`` stops at the first ')' and therefore
    silently truncates a call that contains a generator expression or a nested ')'.
    """
    for match in re.finditer(r"target_sources\s*\(", text):
        depth = 1
        index = match.end()
        start = index
        while index < len(text) and depth:
            if text[index] == "(":
                depth += 1
            elif text[index] == ")":
                depth -= 1
            index += 1
        yield text[start:index - 1]


if not SRC.is_dir() or not PLUGINS.is_dir():
    print(f"missing {SRC} or {PLUGINS}", file=sys.stderr)
    sys.exit(2)

# layer name (lower-case directory name) -> expected object-library prefix
PLUGIN_DIRS = {p.name.lower(): p for p in sorted(PLUGINS.iterdir()) if p.is_dir()}
LAYERS = {layer: f"warpx_{layer}_" for layer in PLUGIN_DIRS}

# 1. Nothing under Source/Core (or the top-level CMakeLists) may attach plugin sources
#    to a core target.
core_cmake = list((SRC / "Core").rglob("CMakeLists.txt")) + [ROOT / "CMakeLists.txt"]
for cm in core_cmake:
    text = cm.read_text()
    for body in target_sources_blocks(text):
        tokens = re.findall(r"[A-Za-z0-9_./+-]+\.(?:cpp|cc|cxx|H|h)", body)
        target = body.split()[0] if body.split() else ""
        for tok in tokens:
            if "Plugins/" not in tok:
                continue
            if cm == ROOT / "CMakeLists.txt" and not target.startswith(("warpx_core_", "lib_")):
                continue
            errors.append(f"Core CMake compiles plugin source: "
                          f"{cm.relative_to(ROOT)} -> {tok}")

# 2. Plugin CMakeLists.txt must not attach files outside their own tree.
for layer, base in PLUGIN_DIRS.items():
    expected = LAYERS[layer]
    for cm in base.rglob("CMakeLists.txt"):
        text = cm.read_text()
        for body in target_sources_blocks(text):
            target = body.split()[0] if body.split() else ""
            # Python bindings live in their own pybind11 module by design.
            if target.startswith("pyWarpX_"):
                continue
            if not target.startswith(expected):
                errors.append(f"{cm.relative_to(ROOT)}: sources go to '{target}', "
                              f"expected '{expected}<dim>'")
            for tok in re.findall(r"[A-Za-z0-9_./+-]+\.(?:cpp|cc|cxx)", body):
                resolved = (cm.parent / tok).resolve()
                try:
                    resolved.relative_to(base.resolve())
                except ValueError:
                    errors.append(f"{cm.relative_to(ROOT)}: source outside plugin "
                                  f"tree: {tok}")

# 3. Every layer must be declared in WARPX_MODULE_LAYERS and collected per active layer.
root_text = (ROOT / "CMakeLists.txt").read_text()
layers_match = re.search(r"set\(WARPX_MODULE_LAYERS([^)]*)\)", root_text)
declared = set(layers_match.group(1).split()) if layers_match else set()
declared |= set(re.findall(r"list\(APPEND WARPX_MODULE_LAYERS\s+(\w+)\)", root_text))
for layer in LAYERS:
    if layer not in declared:
        errors.append(f"CMakeLists.txt: layer '{layer}' (Source/Plugins/{PLUGIN_DIRS[layer].name}) "
                      f"missing from WARPX_MODULE_LAYERS")
for layer in declared:
    if layer != "core" and layer not in LAYERS:
        errors.append(f"CMakeLists.txt: layer '{layer}' in WARPX_MODULE_LAYERS has no "
                      f"Source/Plugins/{layer} directory")
if not re.search(r"add_library\(\s*warpx_\$\{_layer\}_\$\{SD\}", root_text):
    errors.append("CMakeLists.txt: missing the add_library(warpx_${_layer}_${SD}) loop")
# lib_<dim> must aggregate every active layer through the object list
if not re.search(r"list\(APPEND _WARPX_LAYER_OBJECTS\s+\$<TARGET_OBJECTS:warpx_\$\{_layer\}_\$\{SD\}>\)",
                 root_text):
    errors.append("CMakeLists.txt: module objects are not collected per active layer")
if not re.search(r"target_sources\(lib_\$\{SD\} PRIVATE \$\{_WARPX_LAYER_OBJECTS\}\)", root_text):
    errors.append("CMakeLists.txt: lib_<dim> does not aggregate the layer objects")

if errors:
    print("\n".join(errors))
    sys.exit(1)

print(f"build boundaries OK ({len(PLUGIN_DIRS)} plugin layers: "
      f"{', '.join(sorted(PLUGIN_DIRS))})")
