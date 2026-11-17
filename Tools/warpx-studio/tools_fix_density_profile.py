"""密度剖面只属于"按体积铺粒子"的注入方式（NUniformPerCell / NRandomPerCell）。

源码依据（PlasmaInjector.cpp）：
  setupNUniformPerCell / setupNRandomPerCell 里调用了 SpeciesUtils::parseDensity(...)
  setupNFluxPerCell 没有调用 —— 面注入用 flux/flux_profile，不用 profile/density。

所以：
  profile                    -> 仅 NUniformPerCell / NRandomPerCell
  density, density_min/max   -> 上述两种方式 且 profile = constant（或未填）
  density_function(x,y,z)    -> 上述两种方式 且 profile = parse_density_function
  read_density_from_path 等  -> 上述两种方式 且 profile = read_from_file
"""
from __future__ import annotations

import json
import pathlib

DATA = pathlib.Path(__file__).parent / "warpx_studio/catalog/data"
S = "<species_name>."
STYLE = S + "injection_style"
PROFILE = S + "profile"
VOLUME = ("NUniformPerCell", "NRandomPerCell")


def style_rule(v):
    return {"param": STYLE, "op": "equals", "value": v, "raw": "density"}


def prof(v):
    return {"param": PROFILE, "op": "equals", "value": v, "raw": "density"}


def prof_empty():
    return {"param": PROFILE, "op": "empty", "value": None, "raw": "density"}


def main() -> int:
    path = DATA / "catalog.json"
    cat = json.loads(path.read_text(encoding="utf-8"))
    names = {p["name"] for p in cat["params"]}
    rules = cat.setdefault("gating_rules", {})

    targets = [PROFILE, S + "density", S + "density_max", S + "density_min",
               S + "density_function(x,y,z)", S + "read_density_from_path",
               S + "density_mesh_name", S + "read_density_distributed"]
    for t in targets:
        rules.pop(t, None)

    def add(name, groups):
        if name in names:
            rules[name] = groups

    # 每种注入方式各成一组（OR），组内是"与"
    add(PROFILE, [[style_rule(v)] for v in VOLUME])
    for n in (S + "density", S + "density_max", S + "density_min"):
        add(n, [[style_rule(v), prof("constant")] for v in VOLUME]
              + [[style_rule(v), prof_empty()] for v in VOLUME])
    add(S + "density_function(x,y,z)",
        [[style_rule(v), prof("parse_density_function")] for v in VOLUME])
    for n in (S + "read_density_from_path", S + "density_mesh_name", S + "read_density_distributed"):
        add(n, [[style_rule(v), prof("read_from_file")] for v in VOLUME])

    path.write_text(json.dumps(cat, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")
    print(f"密度剖面门控重建：{len(targets)} 个参数")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
