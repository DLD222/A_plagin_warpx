#!/usr/bin/env python3
"""Check build-level plugin boundaries (A7 B1-B7).

Core must not compile plugin sources.  The C++ include checker
(check_plugin_boundaries.py) only proves that Core includes no plugin header;
this script proves that no CMake registration under Source/Core adds a file
that lives under Source/Plugins, and that the expected per-layer targets exist.

Exit code 0 = OK, 1 = violation.
"""
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "Source"
CMAKE_SOURCES = list(SRC.rglob("CMakeLists.txt")) + [ROOT / "CMakeLists.txt"]

# layer -> expected object-library prefix
LAYERS = {
    "core": "warpx_core_",
    "picem": "warpx_picem_",
    "diagnostics": "warpx_diagnostics_",
    "collision": "warpx_collision_",
    "fieldsolver": "warpx_fieldsolver_",
    "particlecore": "warpx_particlecore_",
}

errors = []

# 1. Source/Core/**/CMakeLists.txt must not attach anything from Source/Plugins.
for cm in (SRC / "Core").rglob("CMakeLists.txt"):
    text = cm.read_text()
    for block in re.finditer(r"target_sources\((.*?)\)", text, re.S):
        body = block.group(1)
        for tok in re.findall(r"[A-Za-z0-9_./+-]+\.(?:cpp|cc|cxx|H|h)", body):
            if "Plugins/" in tok or "/Plugins" in tok:
                errors.append(f"Core CMake compiles plugin source: "
                              f"{cm.relative_to(ROOT)} -> {tok}")

# 2. Plugin CMakeLists.txt must not attach files outside their own tree.
PLUGIN_DIRS = {
    "picem": SRC / "Plugins/PicEm",
    "diagnostics": SRC / "Plugins/Diagnostics",
    "io": SRC / "Plugins/IO",
    "particlecore": SRC / "Plugins/ParticleCore",
}
for layer, base in PLUGIN_DIRS.items():
    for cm in base.rglob("CMakeLists.txt"):
        text = cm.read_text()
        for block in re.finditer(r"target_sources\((.*?)\)", text, re.S):
            body = block.group(1)
            target = body.split()[0] if body.split() else ""
            # Python bindings live in their own pybind11 module by design.
            if target.startswith("pyWarpX_"):
                continue
            if not target.startswith(LAYERS[layer]):
                errors.append(f"{cm.relative_to(ROOT)}: sources go to '{target}', "
                              f"expected '{LAYERS[layer]}<dim>'")
            for tok in re.findall(r"[A-Za-z0-9_./+-]+\.(?:cpp|cc|cxx)", body):
                resolved = (cm.parent / tok).resolve()
                try:
                    resolved.relative_to(base.resolve())
                except ValueError:
                    errors.append(f"{cm.relative_to(ROOT)}: source outside plugin "
                                  f"tree: {tok}")

# 3. The four object libraries must be declared in the top-level CMakeLists.
root_text = (ROOT / "CMakeLists.txt").read_text()
layers_match = re.search(r"set\(WARPX_MODULE_LAYERS([^)]*)\)", root_text)
declared = set(layers_match.group(1).split()) if layers_match else set()
declared |= set(re.findall(r"list\(APPEND WARPX_MODULE_LAYERS\s+(\w+)\)", root_text))
for layer in LAYERS:
    if layer not in declared:
        errors.append(f"CMakeLists.txt: layer '{layer}' missing from WARPX_MODULE_LAYERS")
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

print("build boundaries OK")
