"""激光 profile 与外场初始化方式的门控，并清掉源码里不存在的 GUI 死键。

源码依据：
  - LaserParticleContainer.cpp:91-165,221-232,266-279
  - LaserProfiles.H:391-404 (gaussian / parse_field_function / from_file)
  - LaserProfileGaussian.cpp:40-51, LaserProfileFieldFunction.cpp:32-37,
    LaserProfileFromFile.cpp:64-101
  - ExternalField.cpp:31-66,89-123,134-201,205-211
  - WarpX.cpp:939-947,2513-2519
"""
from __future__ import annotations

import json
import pathlib

DATA = pathlib.Path(__file__).parent / "warpx_studio/catalog/data"
L = "<laser_name>."
B = "warpx.B_ext_grid_init_style"
E = "warpx.E_ext_grid_init_style"
LASER_PROFILES = ["gaussian", "parse_field_function", "from_file"]


def cond(param, op, value=None):
    return {"param": param, "op": op, "value": value, "raw": "profile/style gate"}


def eq(param, value):
    return cond(param, "equals", value)


def in_(param, values):
    return cond(param, "in", " ".join(values))


def add_param(catalog, name, kind, doc, options=None, group=None, category=None):
    if any(p["name"] == name for p in catalog["params"]):
        return False
    catalog["params"].append({
        "name": name, "type": kind, "default": None, "optional": True,
        "doc": doc, "doc_section": None, "core": False,
        "options": options, "sites": [], "source_only": True,
    })
    catalog["params"].sort(key=lambda p: p["name"])
    if group:
        catalog["groups"].setdefault(group, []).append(name)
        catalog["groups"][group] = sorted(catalog["groups"][group])
    return True


LASER_EXTRA = {
    "prob_lo": ("string", "激光注入盒下界（AMREX_SPACEDIM 个值）"),
    "prob_hi": ("string", "激光注入盒上界（AMREX_SPACEDIM 个值）"),
    "field_function(X,Y,t)": ("string", "parse_field_function 的场表达式，变量是 X,Y,t"),
    "binary_file_name": ("string", "from_file：二进制场文件"),
    "lasy_file_name": ("string", "from_file：LASY openPMD 文件"),
    "time_chunk_size": ("int", "from_file：时间块大小（>=2）"),
    "delay": ("float", "from_file：时间延迟"),
}
EXTERN_EXTRA = {
    "warpx.Bx_external_grid_function(x,y,z)": ("string", "Bx 解析式外场"),
    "warpx.By_external_grid_function(x,y,z)": ("string", "By 解析式外场"),
    "warpx.Bz_external_grid_function(x,y,z)": ("string", "Bz 解析式外场"),
    "warpx.Ex_external_grid_function(x,y,z)": ("string", "Ex 解析式外场"),
    "warpx.Ey_external_grid_function(x,y,z)": ("string", "Ey 解析式外场"),
    "warpx.Ez_external_grid_function(x,y,z)": ("string", "Ez 解析式外场"),
    "warpx.read_fields_from_path": ("string", "从 openPMD 文件读 E/B 外场（两者共用一个路径）"),
    "warpx.maxlevel_extEMfield_init": ("int", "外场初始化到哪一层；默认 maxLevel"),
}


def main() -> int:
    path = DATA / "catalog.json"
    catalog = json.loads(path.read_text(encoding="utf-8"))
    names = {p["name"] for p in catalog["params"]}
    rules = catalog.setdefault("gating_rules", {})

    added = []
    for suffix, (kind, doc) in LASER_EXTRA.items():
        if add_param(catalog, L + suffix, kind, doc, group="<laser_name>"):
            added.append(L + suffix)
    for name, (kind, doc) in EXTERN_EXTRA.items():
        if add_param(catalog, name, kind, doc):
            added.append(name)

    for p in catalog["params"]:
        if p["name"] == L + "profile":
            p["options"] = list(LASER_PROFILES)
        elif p["name"] == B:
            p["options"] = ["default", "constant", "parse_b_ext_grid_function",
                            "read_from_file", "load_from_python", "none"]
        elif p["name"] == E:
            p["options"] = ["default", "constant", "parse_e_ext_grid_function",
                            "read_from_file", "load_from_python"]

    OWNED = {L + "profile", B, E}
    removed = 0
    for name in list(rules):
        kept = []
        for rule in rules[name]:
            items = rule if isinstance(rule, list) else [rule]
            if any(isinstance(x, dict) and x.get("param") in OWNED for x in items):
                removed += 1
                continue
            kept.append(rule)
        if kept:
            rules[name] = kept
        else:
            rules.pop(name, None)

    def set_rules(name, rs):
        if name in {p["name"] for p in catalog["params"]}:
            rules[name] = rs

    # 激光：profile 是唯一选择器
    for suffix in ("profile_waist", "profile_duration", "profile_t_peak",
                   "profile_focal_distance", "zeta", "beta", "phi2", "phi0",
                   "stc_direction"):
        set_rules(L + suffix, [eq(L + "profile", "gaussian")])
    set_rules(L + "field_function(X,Y,t)", [eq(L + "profile", "parse_field_function")])
    for suffix in ("binary_file_name", "lasy_file_name", "time_chunk_size", "delay"):
        set_rules(L + suffix, [eq(L + "profile", "from_file")])

    # 外场
    set_rules("warpx.B_external_grid", [eq(B, "constant")])
    set_rules("warpx.E_external_grid", [eq(E, "constant")])
    for name in ("warpx.Bx_external_grid_function(x,y,z)",
                 "warpx.By_external_grid_function(x,y,z)"):
        set_rules(name, [eq(B, "parse_b_ext_grid_function"),
                         in_("geometry.dims", ["1", "2", "3"])])
    set_rules("warpx.Bz_external_grid_function(x,y,z)",
              [eq(B, "parse_b_ext_grid_function")])
    for name in ("warpx.Ex_external_grid_function(x,y,z)",
                 "warpx.Ey_external_grid_function(x,y,z)",
                 "warpx.Ez_external_grid_function(x,y,z)"):
        set_rules(name, [eq(E, "parse_e_ext_grid_function")])
    set_rules("warpx.read_fields_from_path",
              [eq(B, "read_from_file"), eq(E, "read_from_file")])

    # 源码里不存在的 GUI 死键：从 enums 里删掉，避免再被写回输入
    enums_path = DATA / "enums.json"
    enums = json.loads(enums_path.read_text(encoding="utf-8"))
    for dead in ("<species_name>.position_initialization",
                 "<species_name>.momentum_initialization",
                 "<laser_name>.injection_type"):
        enums["enums"].pop(dead, None)
    enums["enums"][L + "profile"] = list(LASER_PROFILES)
    enums_path.write_text(json.dumps(enums, ensure_ascii=False, indent=1) + "\n",
                          encoding="utf-8")

    # 分类
    cats_path = DATA / "categories.json"
    cats = json.loads(cats_path.read_text(encoding="utf-8"))
    for cat in cats["categories"]:
        if cat["id"] == "laser":
            for suffix in LASER_EXTRA:
                n = L + suffix
                if n in {p["name"] for p in catalog["params"]} and n not in cat["params"]:
                    cat["params"].append(n)
            cat["params"].sort()
        if cat["id"] == "extern":
            for n in EXTERN_EXTRA:
                if n in {p["name"] for p in catalog["params"]} and n not in cat["params"]:
                    cat["params"].append(n)
            cat["params"].sort()
    cats_path.write_text(json.dumps(cats, ensure_ascii=False, indent=1) + "\n",
                         encoding="utf-8")

    catalog["counts"]["total"] = len(catalog["params"])
    path.write_text(json.dumps(catalog, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")
    print(f"激光/外场: 补 {len(added)} 个参数，删 {removed} 条旧规则，"
          f"gating_rules {len(rules)} 条")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
