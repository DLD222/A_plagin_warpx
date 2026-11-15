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
import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
DOCS = HERE / "data/params_docs.json"
SITES = HERE / "data/param_sites.json"
SITES_AST = HERE / "data/param_sites_ast.json"
OUT = HERE / "data/catalog.json"

TEMPLATES = ("<species_name>", "<collision_name>", "<rd_name>", "<m_diag_name>",
             "<diag_name>", "<a_name>", "<a_prefix>", "<a_pp_prefix>")


RULE = re.compile(r"(\w+)\s*==\s*(?:[A-Za-z_]\w*::)?([A-Za-z_]\w*)")
BOOL = re.compile(r"^if \(\s*(!?)\s*([A-Za-z_]\w*)\s*\)$")


def gating_rules(sites: list[dict], var_param: dict[tuple[str, str], str]) -> list[dict]:
    """Turn "if (collision_type == CollisionType::PairwiseCoulomb)" into a form rule.

    The variable -> parameter map comes from the same file: a variable that some read in
    that file assigns to is a parameter.  Only simple comparisons are turned into rules;
    anything else keeps the raw condition as a tooltip and the field stays visible, because
    hiding a field on a mis-parsed condition is worse than showing it.
    """
    rules: list[dict] = []
    for site in sites:
        for condition in site.get("conditions", []):
            for var, value in RULE.findall(condition):
                param = var_param.get((site["file"], var))
                if param:
                    rules.append({"param": param, "op": "==", "value": value.lower(),
                                  "raw": condition})
            match = BOOL.match(condition)
            if match:
                negated, var = match.group(1) == "!", match.group(2)
                param = var_param.get((site["file"], var))
                if param:
                    rules.append({"param": param, "op": "falsy" if negated else "truthy",
                                  "value": None, "raw": condition})
    unique, seen = [], set()
    for rule in rules:
        key = (rule["param"], rule["op"], rule["value"])
        if key not in seen:
            seen.add(key)
            unique.append(rule)
    return unique


def group_of(name: str) -> str:
    """Top-level group a key belongs to: its first segment, or the template name."""
    for template in TEMPLATES:
        if name.startswith(template):
            return template
    return name.split(".", 1)[0] if "." in name else "(root)"


def main() -> int:
    docs = json.loads(DOCS.read_text())["params"]
    regex_sites = json.loads(SITES.read_text())["sites"]
    ast_sites = []
    if SITES_AST.exists():
        ast_sites = json.loads(SITES_AST.read_text())["sites"]

    # Merge the two scanners.  Clang contributes what text cannot express (switch/case,
    # "is this read unconditional", the real condition expression) and the regex scan
    # contributes the reads whose translation unit clang only parsed partially.
    merged: dict[tuple[str, str, int], dict] = {}
    for site in regex_sites:
        merged[(site["param"], site["file"], site["line"])] = {**site, "scanner": "regex"}
    for site in ast_sites:
        key = (site["param"], site["file"], site["line"])
        previous = merged.get(key)
        record = {**site}
        if previous is not None:
            # keep the regex-derived switch keys while taking clang's conditions
            record["switch_cases"] = site.get("switch_cases") or previous.get("switch_cases", [])
        merged[key] = record
    sites = list(merged.values())

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
                               "conditions": site["conditions"],
                               "required": site.get("required", False),
                               "options": site.get("options", [])})


    # variable -> parameter, globally: a value read once can gate reads in another parameter
    var_param: dict[tuple[str, str], str] = {}
    for site in sites:
        if site.get("variable"):
            var_param.setdefault((site["file"], site["variable"]), site["param"])

    params = sorted(by_name.values(), key=lambda p: p["name"])
    gating, guards, groups = {}, {}, {}
    gating_rules_out: dict[str, list[dict]] = {}
    for param in params:
        # `get` in the source means the simulation aborts without this key; `query` means
        # it is optional.  A parameter is required if any read of it is mandatory.
        # Conservative: only a parameter with a single, unconditional `get` is treated as
        # required.  Anything read inside a branch may legitimately be absent, and a false
        # "required" would block a deck that actually runs.
        param["required"] = (len(param["sites"]) == 1
                             and param["sites"][0].get("required")
                             and not param["sites"][0]["conditions"])
        options = sorted({o for s in param["sites"] for o in s.get("options", [])})
        # keep options only when they look like an enumeration of keywords
        param["options"] = options if 1 < len(options) <= 25 else []
        conditions = sorted({c for s in param["sites"] for c in s["conditions"]})
        runtime = [c for c in conditions if not c.startswith("#")]
        builds = [c for c in conditions if c.startswith("#")]
        if runtime:
            gating[param["name"]] = runtime
            rules = gating_rules(param["sites"], var_param)
            if rules:
                gating_rules_out[param["name"]] = rules
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
            "with_gating_rules": len(gating_rules_out),
            "sites": len(sites),
            "sites_by_scanner": {
                "clang": sum(1 for s in sites if s.get("scanner") == "clang"),
                "regex": sum(1 for s in sites if s.get("scanner") == "regex"),
            },
            "switch_gated": sum(1 for s in sites if s.get("switch_cases")),
            "required": sum(1 for p in params if p.get("required")),
            "with_options": sum(1 for p in params if p.get("options")),
            "with_build_guard": len(guards),
            "templates": sum(1 for g in groups if g in TEMPLATES),
        },
        "groups": {g: sorted(names) for g, names in sorted(groups.items())},
        "params": params,
        "gating": gating,
        "gating_rules": gating_rules_out,
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
