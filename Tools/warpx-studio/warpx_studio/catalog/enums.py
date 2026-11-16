#!/usr/bin/env python3
"""Dropdown values for parameters that only accept a fixed set of words.

Three sources, in order of authority:

1. the **source scan** (``catalog.json`` → ``options``): values the C++ compares against,
   e.g. ``<collision_name>.type``;
2. a **curated table** below, for switches whose values are compared through helper
   functions or defined in the documentation bullet list.  Each entry names where it came
   from so it can be re-checked instead of trusted blindly;
3. the **type**: ``bool`` / "0 or 1" becomes 0/1, and small integer ranges (particle shape)
   become their own enumeration.

The result is ``data/enums.json``: ``{param: [value, ...]}``.  The UI turns every parameter
with an entry into a dropdown, so nobody has to type "TimeAveraged" from memory.
"""
from __future__ import annotations

import json
import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
DATA = HERE / "data"
CATALOG = DATA / "catalog.json"
OUT = DATA / "enums.json"

# name -> (values, where the values come from)
CURATED: dict[str, tuple[list[str], str]] = {
    "algo.maxwell_solver": (["yee", "ckc", "psatd", "ect"], "Source: ElectromagneticSolverAlgo"),
    "algo.particle_pusher": (["boris", "vay", "hybrid", "kinetic_energy", "adaptive_boris"],
                             "docs: Numerics and algorithms"),
    "algo.current_deposition": (["esirkepov", "direct", "vay"], "docs: current deposition"),
    "algo.field_gathering": (["energy-conserving", "momentum-conserving", "direct"],
                             "docs: field gathering"),
    "algo.em_solver_medium": (["vacuum", "macroscopic"], "docs: EM solver medium"),
    "algo.particle_shape": (["1", "2", "3", "4"], "docs: 1 linear … 4 quartic"),
    "<diag_name>.diag_type": (["Full", "TimeAveraged", "BackTransformed"],
                              "Source: Diagnostics (string compare)"),
    "<diag_name>.format": (["plotfile", "checkpoint", "openpmd", "ascent", "sensei"],
                           "Source: Diagnostics/FlushFormats"),
    "<diag_name>.openpmd_backend": (["h5", "bp", "json"], "docs: openPMD backend"),
    "<species_name>.injection_style": (
        ["NUniformPerCell", "NRandomPerCell", "NFluxPerCell", "gaussian_beam",
         "external_file", "singleparticle", "multipleparticles", "none"],
        "Source: PlasmaInjector.cpp string compares"),
    "<species_name>.profile": (["constant", "parse_density_function", "read_from_file"],
                               "Source: SpeciesUtils.cpp parseDensity"),
    "<species_name>.momentum_distribution_type": (
        ["constant", "maxwellian", "maxwell_juttner", "parse_momentum_function",
         "read_from_file"], "Source: SpeciesUtils.cpp"),
    "<species_name>.position_initialization": (["standard", "random", "parse_position_function"],
                                                "docs: particle initialization"),
    "boundary.field_lo": (["periodic", "pml", "absorbing_silver_mueller", "damped", "pec",
                           "pmc", "pec_insulator", "neumann", "open", "none"],
                          "docs: boundary.field_lo/hi bullets"),
    "boundary.field_hi": (["periodic", "pml", "absorbing_silver_mueller", "damped", "pec",
                           "pmc", "pec_insulator", "neumann", "open", "none"],
                          "docs: boundary.field_lo/hi bullets"),
    "boundary.particle_lo": (["periodic", "absorbing", "reflecting", "thermal", "none"],
                             "docs: boundary.particle_lo/hi bullets"),
    "boundary.particle_hi": (["periodic", "absorbing", "reflecting", "thermal", "none"],
                             "docs: boundary.particle_lo/hi bullets"),
    "geometry.dims": (["1", "2", "3", "RZ"], "docs: dimensionality"),
    "warpx.E_ext_grid_init_style": (["none", "constant", "parse_e_ext_grid_function",
                                     "read_from_file"], "Source: Initialization"),
    "warpx.B_ext_grid_init_style": (["none", "constant", "parse_b_ext_grid_function",
                                     "read_from_file"], "Source: Initialization"),
}

BOOL_LIKE = re.compile(r"\b(bool|boolean|0 or 1|1 or 0)\b", re.I)


def main() -> int:
    catalog = json.loads(CATALOG.read_text(encoding="utf-8"))
    enums: dict[str, list[str]] = {}
    origin: dict[str, str] = {}

    for name, (values, where) in CURATED.items():
        enums[name] = values
        origin[name] = where

    for param in catalog["params"]:
        name = param["name"]
        if param.get("options") and name not in enums:
            enums[name] = param["options"]
            origin[name] = "source scan (string compares)"
        elif BOOL_LIKE.search(str(param.get("type") or "")) and name not in enums:
            enums[name] = ["0", "1"]
            origin[name] = "type bool"

    # particle-level external fields mirror the grid ones
    for field in ("E", "B"):
        for name in (f"<species_name>.{field}_ext_particle_init_style",
                     f"<species_name>.{field}_ext_init_style"):
            if name in {p["name"] for p in catalog["params"]}:
                enums[name] = ["none", "constant", f"parse_{field.lower()}_ext_particle_function",
                               "read_from_file"]
                origin[name] = "Source: Initialization"

    OUT.write_text(json.dumps({"_origin": origin, "enums": enums},
                              ensure_ascii=False, indent=1) + "\n", encoding="utf-8")
    known = {p["name"] for p in catalog["params"]}
    # the documentation folds the lo/hi boundary pair into one entry
    for alias in ("boundary.field_lo/hi", "boundary.particle_lo/hi"):
        base = alias.replace("_lo/hi", "_lo")
        if base in enums and alias in known:
            enums[alias] = enums[base]
            origin[alias] = origin.get(base, "")
    dropped = [n for n in enums if n not in known]
    for name in dropped:
        enums.pop(name, None)
        origin.pop(name, None)
    print(f"enums: {len(enums)} 个参数有下拉（{len([n for n in enums if n not in CURATED])} 个来自源码扫描）")
    for name in list(enums)[:6]:
        print(f"  {name}: {enums[name][:5]}")
    if dropped:
        print("  目录里不存在的键（已跳过）:", dropped[:4])
    return 0


if __name__ == "__main__":
    sys.exit(main())
