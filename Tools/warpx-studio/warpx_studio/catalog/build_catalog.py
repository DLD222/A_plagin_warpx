#!/usr/bin/env python3
"""Merge the documented parameters with the source scan into ``catalog.json``.

Sources of truth, in order:

1. ``data/params_docs.json`` - the 387 documented inputs (type, default, prose).
2. ``data/param_sites.json`` - every ParmParse read in this fork's ``Source/``:
   resolves section prefixes (``pp_psatd.query("nx")`` -> ``psatd.nx``), repeatable
   groups (``ParmParse pp_species_name(species_name)`` -> ``<species_name>.charge``)
   and the ``if``/``#if`` conditions around each read.

A parameter is "documented", "source-only" (this fork adds inputs that its own
docs do not mention) or both.  ``build_guards`` keeps the ``#if`` feature guards
apart: they are shown as notes, they do not hide the field, because the UI is
built for one binary at a time and the user explicitly asked not to gate on the
build configuration.
"""
from __future__ import annotations

import json
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
DOCS = HERE / "data/params_docs.json"
SITES = HERE / "data/param_sites.json"
OUT = HERE / "data/catalog.json"

TEMPLATES = ("<species_name>", "<collision_name>", "<rd_name>", "<m_diag_name>",
             "<diag_name>", "<a_name>", "<a_prefix>", "<a_pp_prefix>")


def group_of(name: str) -> str:
    """Top-level group a key belongs to: its first segment, or the template name."""
    for template in TEMPLATES:
        if name.startswith(template):
            return template
    return name.split(".", 1)[0] if "." in name else "(root)"


def main() -> int:
    docs = json.loads(DOCS.read_text())["params"]
    sites = json.loads(SITES.read_text())["sites"]

    by_name: dict[str, dict] = {}
    for entry in docs:
        by_name[entry["name"]] = {
            "name": entry["name"],
            "type": entry["type"],
            "default": entry["default"],
            "optional": entry["optional"],
            "doc": entry["doc"],
            "doc_section": entry["section"],
            "sites": [],
            "source_only": False,
        }

    for site in sites:
        entry = by_name.get(site["param"])
        if entry is None:
            entry = by_name[site["param"]] = {
                "name": site["param"], "type": None, "default": None, "optional": False,
                "doc": "", "doc_section": None, "sites": [], "source_only": True,
            }
        entry["sites"].append({"file": site["file"], "line": site["line"],
                               "variable": site["variable"],
                               "conditions": site["conditions"]})


    params = sorted(by_name.values(), key=lambda p: p["name"])
    gating, guards, groups = {}, {}, {}
    for param in params:
        conditions = sorted({c for s in param["sites"] for c in s["conditions"]})
        runtime = [c for c in conditions if not c.startswith("#")]
        builds = [c for c in conditions if c.startswith("#")]
        if runtime:
            gating[param["name"]] = runtime
        if builds:
            guards[param["name"]] = builds
        group = group_of(param["name"])
        groups.setdefault(group, []).append(param["name"])

    documented = [p for p in params if not p["source_only"]]
    source_only = [p for p in params if p["source_only"]]
    catalog = {
        "generated_by": "Tools/warpx-studio/warpx_studio/catalog/build_catalog.py",
        "sources": {"docs": str(DOCS.name), "source_scan": str(SITES.name)},
        "counts": {
            "total": len(params),
            "documented": len(documented),
            "source_only": len(source_only),
            "with_runtime_gating": len(gating),
            "with_build_guard": len(guards),
            "templates": sum(1 for g in groups if g in TEMPLATES),
        },
        "groups": {g: sorted(names) for g, names in sorted(groups.items())},
        "params": params,
        "gating": gating,
        "build_guards": guards,
    }
    OUT.write_text(json.dumps(catalog, indent=1, ensure_ascii=False) + "\n", encoding="utf-8")

    print(f"catalog: {len(params)} parameters -> {OUT.relative_to(HERE.parents[2])}")
    print(f"  documented {len(documented)}, source-only {len(source_only)}, "
          f"runtime gating {len(gating)}, build guards {len(guards)}")
    print("  groups:")
    for group, names in sorted(groups.items(), key=lambda kv: -len(kv[1]))[:12]:
        flag = " <-- repeatable" if group.startswith("<") else ""
        print(f"    {len(names):4d}  {group}{flag}")
    if source_only:
        print("  source-only sample:", ", ".join(p["name"] for p in source_only[:12]))
    return 0


if __name__ == "__main__":
    sys.exit(main())
