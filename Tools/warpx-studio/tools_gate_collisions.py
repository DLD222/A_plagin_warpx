"""按源码重建 <collision_name>.type -> 参数集合的门控。

源码依据：
  - CollisionHandler.cpp:60-120  (11 种类型的分派，默认 pairwisecoulomb)
  - CollisionBase.cpp:22-98      (公共参数；已删除/禁止的旧键)
  - BinaryCollision.H / BinaryCollisionUtils.cpp / 各模型 .cpp
    (逐类型参数与背景/散射过程子开关)

旧 catalog 只有 7 种类型选项，且没有任何 type 门控；这里同时：
  1. 补齐 11 种类型选项；
  2. 补进 source-only 但实际会读的关键参数；
  3. 删掉引用 <collision_name>.type 的旧规则，按源码写精确集合。
"""
from __future__ import annotations

import json
import pathlib

DATA = pathlib.Path(__file__).parent / "warpx_studio/catalog/data"
C = "<collision_name>."
TYPES = ["pairwisecoulomb", "dsmc", "background_mcc", "background_stopping",
         "nuclearfusion", "bremsstrahlung", "inverse_bremsstrahlung",
         "linear_breit_wheeler", "linear_compton", "pulsed_decay",
         "hybrid_resistive_drag"]
BINARY = ["pairwisecoulomb", "dsmc", "nuclearfusion", "bremsstrahlung",
          "inverse_bremsstrahlung", "linear_breit_wheeler", "linear_compton"]
SHUFFLING_TYPES = [t for t in BINARY]
THREE = ["nuclearfusion", "linear_breit_wheeler", "linear_compton"]

# 源码里缺失/未收录、但确实会读的参数（类型、默认值、说明）
EXTRA = {
    C + "shuffling_method": ("string", "default",
        "洗牌方式 none/fisheryates/modulus/standard/default；per-collision 覆盖 collisions.shuffling_method"),
    C + "modulus_rounds": ("int", "5",
        "shuffling_method=modulus 时的轮数；per-collision 覆盖 collisions.modulus_rounds"),
    C + "correct_energy_momentum": ("bool", "0",
        "成对库仑：能量/动量修正"),
    C + "energy_correction_sort_by_weight": ("bool", "0",
        "成对库仑：按权重排序做能量修正"),
    C + "np_warning_threshold": ("int", "20",
        "成对库仑：每格粒子数告警阈值"),
    C + "beta_weight_exponent": ("float", "1.0",
        "成对库仑：权重 beta 指数"),
    C + "energy_fraction": ("float", "0.05",
        "成对库仑/逆轫致：能量修正比例"),
    C + "max_background_density": ("float", None,
        "background_mcc：背景密度上限"),
    C + "background_density(x,y,z,t)": ("string", None,
        "背景密度表达式（未给常数 background_density 时使用）"),
    C + "background_temperature(x,y,z,t)": ("string", None,
        "背景温度表达式（未给常数 background_temperature 时使用）"),
}


def cond(param: str, op: str, value=None) -> dict:
    return {"param": param, "op": op, "value": value, "raw": "collision type gate"}


def eq(param: str, value: str) -> dict:
    return cond(param, "equals", value)


def empty(param: str) -> dict:
    return cond(param, "empty")


def in_(param: str, values) -> dict:
    return cond(param, "in", " ".join(values))


def t_in(values) -> dict:
    return in_(C + "type", values)


RULES: dict[str, list] = {}


def add(names, *rules):
    for name in names:
        RULES.setdefault(name, []).extend(rules)


# 公共：任何类型都读
for name in ("type", "species", "ndt_supercycle", "ndt_subcycle", "start_step"):
    RULES[C + name] = []

# pairwisecoulomb
add([C + "CoulombLog", C + "cumulative_scattering_angle_model",
     C + "use_global_debye_length", C + "correct_energy_momentum",
     C + "energy_correction_sort_by_weight", C + "np_warning_threshold",
     C + "beta_weight_exponent"],
    t_in(["pairwisecoulomb"]))
add([C + "energy_fraction"], t_in(["pairwisecoulomb", "inverse_bremsstrahlung"]))
add([C + "shuffling_method"], t_in(SHUFFLING_TYPES))
add([C + "modulus_rounds"],
    [t_in(SHUFFLING_TYPES), eq(C + "shuffling_method", "modulus")])

# dsmc / background_mcc 的散射过程
add([C + "scattering_processes",
     C + "<scattering_process>_cross_section",
     C + "<scattering_process>_energy",
     C + "<scattering_process>_scattering_angle_model"],
    t_in(["dsmc", "background_mcc"]))
add([C + "ionization_target_species"], t_in(["dsmc"]))
add([C + "ionization_species"], t_in(["background_mcc"]))

# background 模型
add([C + "background_density", C + "background_temperature",
     C + "background_mass"],
    t_in(["background_mcc", "background_stopping"]))
add([C + "background_density(x,y,z,t)"],
    [t_in(["background_mcc", "background_stopping"]), empty(C + "background_density")])
add([C + "background_temperature(x,y,z,t)"],
    [t_in(["background_mcc", "background_stopping"]), empty(C + "background_temperature")])
add([C + "max_background_density"], t_in(["background_mcc"]))
add([C + "background_type"], t_in(["background_stopping"]))
add([C + "background_charge_state"],
    [t_in(["background_stopping"]), eq(C + "background_type", "ions")])

# 产物/概率模型
add([C + "product_species"],
    t_in(["dsmc", "nuclearfusion", "bremsstrahlung", "linear_breit_wheeler",
          "linear_compton", "pulsed_decay"]))
for name in ("event_multiplier", "probability_threshold", "probability_target_value",
             "legendre_angular_distribution_coefficients",
             "legendre_angular_distribution_coefficients_format"):
    add([C + name], t_in(THREE))
add([C + "create_products", C + "save_particle_production", C + "scattering_angle_model"],
    t_in(["nuclearfusion"]))
add([C + "Z", C + "multiplier", C + "koT1_cut", C + "create_photons"],
    t_in(["bremsstrahlung"]))
add([C + "weight_ratio", C + "productA_temperature_eV", C + "productB_temperature_eV",
     C + "decay_rate(x,y,z,t)"],
    t_in(["pulsed_decay"]))

# hybrid_resistive_drag 没有任何专属输入；只保留公共参数


def ensure_param(catalog: dict, name: str, kind: str, default, doc: str,
                 options=None) -> bool:
    if any(p["name"] == name for p in catalog["params"]):
        return False
    catalog["params"].append({
        "name": name, "type": kind, "default": default, "optional": True,
        "doc": doc, "doc_section": None, "core": False,
        "options": options, "sites": [], "source_only": True,
    })
    catalog["params"].sort(key=lambda p: p["name"])
    catalog["groups"].setdefault(C.rstrip("."), []).append(name)
    catalog["groups"][C.rstrip(".")] = sorted(catalog["groups"][C.rstrip(".")])
    return True


def main() -> int:
    catalog_path = DATA / "catalog.json"
    catalog = json.loads(catalog_path.read_text(encoding="utf-8"))
    names = {p["name"] for p in catalog["params"]}
    rules = catalog.setdefault("gating_rules", {})

    # 1) 补选项 / 参数
    added = []
    for name, (kind, default, doc) in EXTRA.items():
        if ensure_param(catalog, name, kind, default, doc):
            added.append(name)
    type_param = next(p for p in catalog["params"] if p["name"] == C + "type")
    type_param["options"] = list(TYPES)
    added.append(C + "type (options)")
    for pname, opts in ((C + "shuffling_method",
                          ["none", "fisheryates", "modulus", "standard", "default"]),
                         (C + "background_type", ["electrons", "ions"]),
                         (C + "cumulative_scattering_angle_model", ["BOBYLEV", "NANBU"])):
        p = next((p for p in catalog["params"] if p["name"] == pname), None)
        if p is not None:
            p["options"] = opts
    # 全局 collisions.shuffling_method 是枚举，不是 bool
    for p in catalog["params"]:
        if p["name"] == "collisions.shuffling_method":
            p["options"] = ["none", "fisheryates", "modulus", "standard", "default"]

    # 2) 删掉所有引用 <collision_name>.type 的旧规则
    removed = 0
    for name in list(rules):
        kept = []
        for rule in rules[name]:
            items = rule if isinstance(rule, list) else [rule]
            if any(isinstance(x, dict) and x.get("param") == C + "type" for x in items):
                removed += 1
                continue
            kept.append(rule)
        if kept:
            rules[name] = kept
        else:
            rules.pop(name, None)

    # 3) 写精确规则
    written = 0
    for name, rs in RULES.items():
        if name not in {p["name"] for p in catalog["params"]}:
            continue
        if rs:
            rules[name] = rs
            written += 1
        else:
            rules.pop(name, None)

    catalog["counts"]["total"] = len(catalog["params"])
    catalog_path.write_text(json.dumps(catalog, ensure_ascii=False, indent=1) + "\n",
                            encoding="utf-8")

    # 4) 同步 enums.json / categories.json
    enums_path = DATA / "enums.json"
    enums = json.loads(enums_path.read_text(encoding="utf-8"))
    enums["enums"][C + "type"] = list(TYPES)
    enums["enums"]["collisions.shuffling_method"] = [
        "none", "fisheryates", "modulus", "standard", "default"]
    enums_path.write_text(json.dumps(enums, ensure_ascii=False, indent=1) + "\n",
                          encoding="utf-8")
    cats_path = DATA / "categories.json"
    cats = json.loads(cats_path.read_text(encoding="utf-8"))
    for cat in cats["categories"]:
        if cat["id"] == "collision":
            for name in [n for n in RULES if n in {p["name"] for p in catalog["params"]}]:
                if name not in cat["params"]:
                    cat["params"].append(name)
            cat["params"].sort()
    cats_path.write_text(json.dumps(cats, ensure_ascii=False, indent=1) + "\n",
                         encoding="utf-8")

    print(f"碰撞门控: 删除 {removed} 条旧规则，写入 {written} 个参数，"
          f"补参数 {len(added)} 项，gating_rules {len(rules)} 条")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
