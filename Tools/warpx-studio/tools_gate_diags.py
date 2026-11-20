"""按源码重建普通诊断的 <diag_name>.diag_type / .format -> 参数集合门控。

源码依据：
  - MultiDiagnostics.cpp:83-86       diag_type 只能是 Full/TimeAveraged/BackTransformed/BoundaryScraping
  - Diagnostics.cpp:101-428          BaseReadParameters（与类型/格式无关的公共参数）
  - FullDiagnostics.cpp:118-247      Full 与 TimeAveraged（共用一个类；TimeAveraged 多 4 个参数）
  - BTDiagnostics.cpp:215-302        BackTransformed 专属参数；format 只能是 plotfile/openpmd
  - BoundaryScrapingDiagnostics.cpp:56-66  format 只能是 openpmd
  - FlushFormatOpenPMD.cpp:26-129    openpmd 专属；tspf 只在没有 encoding 时读
  - FlushFormatSensei.cpp:17-21      sensei 专属
  - FlushFormatCatalyst.cpp:78-85    catalyst 专属（全局 catalyst.*，不是 <diag_name>.*）

旧 catalog 的 options 缺 TimeAveraged/BoundaryScraping（diag_type）和 catalyst（format），
并且没有任何 diag_type/format 门控；这里一起补齐。
"""
from __future__ import annotations

import json
import pathlib

DATA = pathlib.Path(__file__).parent / "warpx_studio/catalog/data"
D = "<diag_name>."
TYPES = ["Full", "TimeAveraged", "BackTransformed", "BoundaryScraping"]
FORMATS = ["plotfile", "openpmd", "checkpoint", "ascent", "sensei", "catalyst"]
FULL_TA = ["Full", "TimeAveraged"]


def cond(param: str, op: str, value=None) -> dict:
    return {"param": param, "op": op, "value": value, "raw": "diag type/format gate"}


def eq(param: str, value: str) -> dict:
    return cond(param, "equals", value)


def empty(param: str) -> dict:
    return cond(param, "empty")


def in_(param: str, values) -> dict:
    return cond(param, "in", " ".join(values))


def t_in(values) -> dict:
    return in_(D + "diag_type", values)


def f_in(values) -> dict:
    return in_(D + "format", values)


RULES: dict[str, list] = {}


def add(names, *rules):
    for name in names:
        RULES.setdefault(name, []).extend(rules)


# ---- 公共参数：显式清空规则（旧扫描规则按"或"拼，必须去掉） ----
COMMON = [
    "diag_type", "format", "file_prefix", "file_min_digits", "dump_last_timestep",
    "verbose", "fields_to_plot", "additional_fields_to_plot",
    "particle_fields_to_plot", "particle_fields.<field_name>(x,y,z,ux,uy,uz)",
    "particle_fields.<field_name>.do_average",
    "particle_fields.<field_name>.filter(x,y,z,ux,uy,uz)",
    "particle_fields_species", "diag_lo", "diag_hi", "coarsening_ratio",
    "species", "write_species", "intervals",
    "<species_name>.variables", "<species_name>.additional_variables",
    "<species_name>.random_fraction", "<species_name>.uniform_stride",
    "<species_name>.plot_filter_function(t,x,y,z,ux,uy,uz)",
]
for name in COMMON:
    RULES[D + name] = []

# ---- Full / TimeAveraged：同一个 FullDiagnostics 类 ----
add([D + "time_average_mode", D + "average_period_steps", D + "average_period_time"],
    t_in(["TimeAveraged"]))
# average_start_step 只在 time_average_mode=fixed_start 的静态平均分支读
add([D + "average_start_step"],
    [t_in(["TimeAveraged"]), eq(D + "time_average_mode", "fixed_start")])

# ---- BackTransformed ----
add([D + "do_back_transformed_fields", D + "do_back_transformed_particles",
     D + "num_snapshots_lab", D + "dt_snapshots_lab", D + "dz_snapshots_lab",
     D + "buffer_size"], t_in(["BackTransformed"]))
# BTD 不支持粒子场诊断（源码 assert）；只在 Full/TimeAveraged 下给这一行
add([D + "particle_fields_to_plot"], t_in(FULL_TA))

# ---- 与维度/格式交叉 ----
# dump_rz_modes 只在 RZ 构建、Full/TimeAveraged 下读
add([D + "dump_rz_modes"],
    [t_in(FULL_TA), eq("geometry.dims", "RZ")])
# openpmd 专属；tspf 只在没有显式 encoding 时读
add([D + "openpmd_backend", D + "openpmd_encoding",
     D + "adios2_operator.type", D + "adios2_operator.parameters.*",
     D + "adios2_engine.type", D + "adios2_engine.parameters.*"],
    eq(D + "format", "openpmd"))
add([D + "openpmd_tspf"],
    [eq(D + "format", "openpmd"), empty(D + "openpmd_encoding")])
add([D + "buffer_flush_limit_btd"],
    [t_in(["BackTransformed"]), eq(D + "format", "openpmd")])
# sensei 专属
add([D + "sensei_config", D + "sensei_pin_mesh"], eq(D + "format", "sensei"))

# ---- 禁止组合：checkpoint 不接受 fields/particle_fields/diag 范围等 ----
# Diagnostics.cpp:417-425 + FullDiagnostics.cpp:225-231
# checkpoint 下这些键必须不出现（源码 assert）；用"允许的 format 列表"表达
NON_CHECKPOINT = ["plotfile", "openpmd", "ascent", "sensei", "catalyst"]
CHECKPOINT_BAD = [D + "fields_to_plot", D + "particle_fields_species",
                  D + "diag_lo", D + "diag_hi", D + "coarsening_ratio",
                  D + "species"]
for name in CHECKPOINT_BAD:
    add([name], [t_in(FULL_TA), f_in(NON_CHECKPOINT)],
               t_in(["BackTransformed", "BoundaryScraping"]))
# plot_raw_fields 本来就只属于 Full/TimeAveraged，checkpoint 下隐藏
for name in (D + "plot_raw_fields", D + "plot_raw_fields_guards",
             D + "particle_fields_to_plot"):
    # 直接覆盖而不是 append：前面可能已经加过"只看 diag_type"的规则
    RULES[name] = [[t_in(FULL_TA), f_in(NON_CHECKPOINT)]]

# 只允许某些 diag_type 的 format：可见性只能表达"这个格式的参数"，不能改下拉
# 选项；非法组合交给 validate（见 warpx_studio/deck/validate.py）。
FORMAT_BY_TYPE = {
    "Full": FORMATS,
    "TimeAveraged": FORMATS,
    "BackTransformed": ["plotfile", "openpmd"],
    "BoundaryScraping": ["openpmd"],
}

OWNED_SELECTORS = {D + "diag_type", D + "format", D + "time_average_mode",
                   D + "openpmd_encoding", D + "particle_fields_to_plot"}


def ensure_openpmd_tspf(catalog: dict) -> bool:
    name = D + "openpmd_tspf"
    if any(p["name"] == name for p in catalog["params"]):
        return False
    catalog["params"].append({
        "name": name, "type": "bool", "default": None, "optional": True,
        "doc": "openPMD 每条记录一个文件（tspf）；未显式设置 openpmd_encoding 时读取",
        "doc_section": None, "core": False, "options": None,
        "sites": [], "source_only": True,
    })
    catalog["params"].sort(key=lambda p: p["name"])
    catalog["groups"].setdefault("<diag_name>", []).append(name)
    catalog["groups"]["<diag_name>"] = sorted(catalog["groups"]["<diag_name>"])
    return True


def main() -> int:
    path = DATA / "catalog.json"
    catalog = json.loads(path.read_text(encoding="utf-8"))
    rules = catalog.setdefault("gating_rules", {})
    names = {p["name"] for p in catalog["params"]}

    added = ensure_openpmd_tspf(catalog)
    for p in catalog["params"]:
        if p["name"] == D + "diag_type":
            p["options"] = list(TYPES)
        elif p["name"] == D + "format":
            p["options"] = list(FORMATS)

    removed = 0
    for name in list(rules):
        kept = []
        for rule in rules[name]:
            items = rule if isinstance(rule, list) else [rule]
            if any(isinstance(x, dict) and x.get("param") in OWNED_SELECTORS for x in items):
                removed += 1
                continue
            kept.append(rule)
        if kept:
            rules[name] = kept
        else:
            rules.pop(name, None)

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
    path.write_text(json.dumps(catalog, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")

    enums_path = DATA / "enums.json"
    enums = json.loads(enums_path.read_text(encoding="utf-8"))
    enums["enums"][D + "diag_type"] = list(TYPES)
    enums["enums"][D + "format"] = list(FORMATS)
    enums_path.write_text(json.dumps(enums, ensure_ascii=False, indent=1) + "\n",
                          encoding="utf-8")

    cats_path = DATA / "categories.json"
    cats = json.loads(cats_path.read_text(encoding="utf-8"))
    for cat in cats["categories"]:
        if cat["id"] == "diag":
            for name in RULES:
                if name in {p["name"] for p in catalog["params"]} and name not in cat["params"]:
                    cat["params"].append(name)
            cat["params"].sort()
    cats_path.write_text(json.dumps(cats, ensure_ascii=False, indent=1) + "\n",
                         encoding="utf-8")
    print(f"诊断门控: 删除 {removed} 条旧规则，写入 {written} 个参数，"
          f"补 openpmd_tspf={added}，gating_rules {len(rules)} 条")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
